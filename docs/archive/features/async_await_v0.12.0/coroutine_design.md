# Cb言語 Coroutine 設計ドキュメント

**バージョン**: v0.14.0（予定）  
**作成日**: 2025年11月7日  
**ステータス**: 設計フェーズ

---

## 概要

Cb言語の**Coroutine**システムは、Go言語のgoroutineを参考にした**OSスレッドベースの並列実行**機構です。

async/awaitとは**完全に独立したシステム**であり、異なる用途に使用します：

- **async/await**: 協調的マルチタスク（単一スレッド、I/O待ち）
- **Coroutine**: プリエンプティブマルチタスク（マルチスレッド、CPU集約的処理）

### 参考実装

#### Go (goroutine + channel)
```go
func worker(id int, ch chan int) {
    for i := 0; i < 5; i++ {
        ch <- id*10 + i  // チャネルに送信
        time.Sleep(100 * time.Millisecond)
    }
}

func main() {
    ch := make(chan int, 10)  // バッファ付きチャネル
    
    go worker(1, ch)  // goroutineを起動
    go worker(2, ch)
    go worker(3, ch)
    
    for i := 0; i < 15; i++ {
        value := <-ch  // チャネルから受信
        fmt.Println("Received:", value)
    }
}
```

---

## 設計方針

### 実行モデル

- **スレッド**: OSスレッド（`std::thread`）による真の並列実行
- **スケジューリング**: プリエンプティブ（OSカーネルが管理）
- **同期**: Channel、Mutex、Semaphoreなどの同期プリミティブ
- **デバッグ**: `--debug`フラグでCoroutine起動・終了・同期をトレース

### データ構造

```cpp
// Coroutine: OSスレッドで実行される関数
struct Coroutine {
    int coroutine_id;
    std::thread thread;
    FunctionDef* function;
    std::vector<Variable> arguments;
    
    std::atomic<bool> is_running{true};
    std::atomic<bool> is_completed{false};
};

// Channel: スレッド間通信
template<typename T>
class Channel {
    std::queue<T> buffer;
    std::mutex mtx;
    std::condition_variable cv_send;
    std::condition_variable cv_recv;
    size_t capacity;
    bool closed = false;
    
public:
    void send(const T& value);
    T recv();
    void close();
    bool is_closed() const;
};

// Mutex: 排他制御
class Mutex {
    std::mutex mtx;
    std::thread::id owner;
    
public:
    void lock();
    void unlock();
    bool try_lock();
};
```

---

## 使用例

### 基本的なCoroutine起動

```cb
void worker(int id) {
    for (int i = 0; i < 5; i = i + 1) {
        println("Worker {id}: step {i}");
        sleep(100);  // 100ms待機
    }
    println("Worker {id} done");
}

void main() {
    // Coroutineを起動（goroutine風）
    go worker(1);
    go worker(2);
    go worker(3);
    
    // メインスレッドは待機
    sleep(1000);
    println("Main done");
}
```

### Channelによる通信

```cb
void producer(Channel<int> ch, int id) {
    for (int i = 0; i < 5; i = i + 1) {
        int value = id * 10 + i;
        ch.send(value);
        println("Producer {id} sent: {value}");
    }
    ch.close();
}

void consumer(Channel<int> ch) {
    while (true) {
        int value = ch.recv();  // ブロッキング受信
        if (ch.is_closed() && ch.is_empty()) {
            break;
        }
        println("Consumer received: {value}");
    }
}

void main() {
    Channel<int> ch = Channel<int>(10);  // バッファサイズ10
    
    go producer(ch, 1);
    go producer(ch, 2);
    go consumer(ch);
    
    sleep(2000);  // 全Coroutineの完了を待つ
}
```

### Mutexによる排他制御

```cb
struct Counter {
    int value;
    Mutex mtx;
};

void increment(Counter* counter, int times) {
    for (int i = 0; i < times; i = i + 1) {
        counter->mtx.lock();
        counter->value = counter->value + 1;
        counter->mtx.unlock();
    }
}

void main() {
    Counter counter;
    counter.value = 0;
    counter.mtx = Mutex();
    
    // 3つのCoroutineが同時にカウンタをインクリメント
    go increment(&counter, 1000);
    go increment(&counter, 1000);
    go increment(&counter, 1000);
    
    sleep(1000);  // 完了を待つ
    
    println("Final counter value: {counter.value}");  // 3000になるはず
}
```

---

## 実行出力例

### 基本的なCoroutine起動

```
Worker 1: step 0
Worker 2: step 0
Worker 3: step 0
Worker 1: step 1
Worker 3: step 1
Worker 2: step 1
Worker 2: step 2
Worker 1: step 2
Worker 3: step 2
...
Worker 1 done
Worker 2 done
Worker 3 done
Main done
```

### デバッグ出力 (`./main --debug test.cb`)

```
[COROUTINE] Launching coroutine #1: worker(id=1)
[COROUTINE] Launching coroutine #2: worker(id=2)
[COROUTINE] Launching coroutine #3: worker(id=3)
[COROUTINE] Thread #1 started
[COROUTINE] Thread #2 started
[COROUTINE] Thread #3 started
Worker 1: step 0
Worker 2: step 0
Worker 3: step 0
...
[COROUTINE] Thread #1 completed
[COROUTINE] Thread #2 completed
[COROUTINE] Thread #3 completed
Main done
```

### Channelデバッグ出力

```
[CHANNEL] Creating channel with capacity=10
[COROUTINE] Launching coroutine #1: producer(ch, id=1)
[COROUTINE] Launching coroutine #2: producer(ch, id=2)
[COROUTINE] Launching coroutine #3: consumer(ch)
[CHANNEL] Send: value=10 (buffer: 1/10)
Producer 1 sent: 10
[CHANNEL] Send: value=20 (buffer: 2/10)
Producer 2 sent: 20
[CHANNEL] Recv: value=10 (buffer: 1/10)
Consumer received: 10
[CHANNEL] Recv: value=20 (buffer: 0/10)
Consumer received: 20
...
[CHANNEL] Channel closed
[COROUTINE] Thread #3 completed
```

---

## 実装詳細

### Coroutineの起動

```cpp
// go文の処理
int Interpreter::launch_coroutine(
    FunctionDef* func,
    const std::vector<Variable>& args
) {
    int id = next_coroutine_id++;
    
    if (debug_mode) {
        std::cerr << "[COROUTINE] Launching coroutine #" << id 
                  << ": " << func->name << "(";
        for (size_t i = 0; i < args.size(); i++) {
            if (i > 0) std::cerr << ", ";
            std::cerr << func->parameters[i]->name << "=" << args[i].value;
        }
        std::cerr << ")" << std::endl;
    }
    
    Coroutine& coro = coroutines[id];
    coro.coroutine_id = id;
    coro.function = func;
    coro.arguments = args;
    
    // 新しいスレッドで実行
    coro.thread = std::thread([this, id, func, args]() {
        if (debug_mode) {
            std::cerr << "[COROUTINE] Thread #" << id << " started" << std::endl;
        }
        
        // 新しいインタプリタコンテキストを作成
        Scope coroutine_scope;
        push_scope(&coroutine_scope);
        
        // 引数をセットアップ
        for (size_t i = 0; i < args.size(); i++) {
            coroutine_scope.variables[func->parameters[i]->name] = args[i];
        }
        
        try {
            // 関数本体を実行
            execute_statement(func->body.get());
        } catch (const ReturnException& e) {
            // 正常終了
        } catch (const std::exception& e) {
            std::cerr << "[COROUTINE] Thread #" << id << " error: " 
                      << e.what() << std::endl;
        }
        
        pop_scope();
        coroutines[id].is_completed = true;
        
        if (debug_mode) {
            std::cerr << "[COROUTINE] Thread #" << id << " completed" << std::endl;
        }
    });
    
    // スレッドをデタッチ（自動的にクリーンアップ）
    coro.thread.detach();
    
    return id;
}
```

### Channelの実装

```cpp
template<typename T>
void Channel<T>::send(const T& value) {
    std::unique_lock<std::mutex> lock(mtx);
    
    // バッファが空くまで待機
    cv_send.wait(lock, [this] { 
        return buffer.size() < capacity || closed; 
    });
    
    if (closed) {
        throw std::runtime_error("Cannot send to closed channel");
    }
    
    buffer.push(value);
    
    if (debug_mode) {
        std::cerr << "[CHANNEL] Send: value=" << value 
                  << " (buffer: " << buffer.size() << "/" << capacity << ")" 
                  << std::endl;
    }
    
    // 受信側に通知
    cv_recv.notify_one();
}

template<typename T>
T Channel<T>::recv() {
    std::unique_lock<std::mutex> lock(mtx);
    
    // データが来るまで待機
    cv_recv.wait(lock, [this] { 
        return !buffer.empty() || closed; 
    });
    
    if (buffer.empty() && closed) {
        throw std::runtime_error("Channel is closed and empty");
    }
    
    T value = buffer.front();
    buffer.pop();
    
    if (debug_mode) {
        std::cerr << "[CHANNEL] Recv: value=" << value 
                  << " (buffer: " << buffer.size() << "/" << capacity << ")" 
                  << std::endl;
    }
    
    // 送信側に通知
    cv_send.notify_one();
    
    return value;
}

template<typename T>
void Channel<T>::close() {
    std::unique_lock<std::mutex> lock(mtx);
    closed = true;
    
    if (debug_mode) {
        std::cerr << "[CHANNEL] Channel closed" << std::endl;
    }
    
    // 待機中の全スレッドに通知
    cv_send.notify_all();
    cv_recv.notify_all();
}
```

### Mutexの実装

```cpp
class Mutex {
    std::mutex mtx;
    std::thread::id owner;
    bool locked = false;
    
public:
    void lock() {
        if (debug_mode) {
            std::cerr << "[MUTEX] Thread " << std::this_thread::get_id() 
                      << " attempting to lock" << std::endl;
        }
        
        mtx.lock();
        owner = std::this_thread::get_id();
        locked = true;
        
        if (debug_mode) {
            std::cerr << "[MUTEX] Thread " << owner << " acquired lock" << std::endl;
        }
    }
    
    void unlock() {
        if (!locked) {
            throw std::runtime_error("Mutex is not locked");
        }
        
        if (owner != std::this_thread::get_id()) {
            throw std::runtime_error("Mutex can only be unlocked by the owner thread");
        }
        
        if (debug_mode) {
            std::cerr << "[MUTEX] Thread " << owner << " releasing lock" << std::endl;
        }
        
        locked = false;
        mtx.unlock();
    }
    
    bool try_lock() {
        bool acquired = mtx.try_lock();
        if (acquired) {
            owner = std::this_thread::get_id();
            locked = true;
        }
        
        if (debug_mode) {
            std::cerr << "[MUTEX] Thread " << std::this_thread::get_id() 
                      << " try_lock: " << (acquired ? "success" : "failed") << std::endl;
        }
        
        return acquired;
    }
};
```

---

## デバッグガイド

### デバッグフラグの使用

```bash
# Coroutineのデバッグ
./main --debug coroutine_test.cb
```

### デバッグ出力の種類

| プレフィックス | 意味 | 内容 |
|--------------|------|------|
| `[COROUTINE]` | Coroutineの管理 | 起動、開始、完了 |
| `[CHANNEL]` | Channel操作 | 送信、受信、クローズ、バッファ状態 |
| `[MUTEX]` | Mutex操作 | ロック取得、解放、競合 |
| `[DEADLOCK]` | デッドロック検出 | 循環待機の検出（将来実装） |

---

## async/await との比較

| 特徴 | async/await | Coroutine |
|------|-------------|-----------|
| **実行モデル** | 協調的（明示的yield） | プリエンプティブ（OS管理） |
| **スレッド** | シングルスレッド | マルチスレッド |
| **並列性** | 並行（concurrent） | 並列（parallel） |
| **オーバーヘッド** | 低い | 高い（スレッド作成コスト） |
| **用途** | I/O待ち、UI応答性 | CPU集約的処理 |
| **同期** | Future/Promise | Channel/Mutex |
| **デバッグ** | 容易（単一スレッド） | 困難（レースコンディション） |
| **GIL** | 不要 | 不要（真の並列実行） |

---

## 実装ロードマップ

### Phase 1 (v0.14.0)
- [ ] `go`キーワードまたは`spawn()`組み込み関数
- [ ] 基本的なCoroutine起動・実行
- [ ] スレッド管理（作成・デタッチ）
- [ ] デバッグトレース

### Phase 2 (v0.14.1)
- [ ] `Channel<T>`型の実装
- [ ] send/recv操作
- [ ] バッファ管理
- [ ] close操作

### Phase 3 (v0.14.2)
- [ ] `Mutex`型の実装
- [ ] lock/unlock操作
- [ ] try_lock操作
- [ ] デッドロック検出（オプション）

### Phase 4 (v0.15.0)
- [ ] `select`文（複数チャネルの待機）
- [ ] タイムアウト機能
- [ ] Coroutineのキャンセル
- [ ] スレッドプール（オプション）

---

## まとめ

Cb言語のCoroutineシステムは、Go言語のgoroutineを参考にした**真の並列実行**機構です。

async/awaitとは完全に独立しており、適切に使い分けることで効率的な並行・並列プログラミングが可能になります。

**使い分けの指針**:
- **I/O待ちが多い**: async/await（協調的マルチタスク）
- **CPU集約的処理**: Coroutine（OSスレッドベース並列実行）
