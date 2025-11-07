# Cb言語 並行処理設計ドキュメント v2.0

**バージョン**: v0.12.0  
**作成日**: 2025年11月7日  
**更新日**: 2025年11月7日  
**ステータス**: 設計見直し - 実装可能なアプローチへ変更

---

## 概要

Cb言語は2つの独立した並行処理モデルを提供します：

1. **協調的マルチタスク (Cooperative Multitasking)** - `async/await`
   - TypeScript風のAPI
   - Rust Tokioライクな内部実装
   - イベントループベースの実行
   - **重要**: `yield`は将来のバージョンで実装（Phase 2）

2. **OS スレッドベース (Preemptive Multitasking)** - `coroutine`
   - Go言語のgoroutine風のAPI
   - std::threadによる真の並列実行
   - Channel/Mutexによる同期

これらは**完全に別個のシステム**として実装され、異なる用途に対応します。

---

## 設計見直しの理由

### 当初の設計の問題点

1. **ステートメント単位実行の限界**:
   - トップレベルのyieldしかサポートできない
   - ループ内でyieldを使うと無限ループになる
   - 関数呼び出しをまたぐyieldができない

2. **実行位置追跡の複雑さ**:
   - ASTベースでは実行位置の保存・復元が困難
   - ループカウンタ、ネストした制御構造の状態管理が複雑

3. **await時の無限再帰**:
   - 全てのpendingタスクを再帰的に実行しようとして失敗
   - スタックオーバーフロー

### 新しいアプローチ

**Phase 1 (v0.12.0)**: イベントループベースの単純なasync/await
- `yield`は使わない（将来のバージョンで追加）
- タスクは完了まで実行される（中断なし）
- 複数のasync関数を「起動」して、awaitで完了を待つ
- 並行実行ではなく、順次実行（まず完全に動作させることを優先）

**Phase 2 (v0.13.0以降)**: yieldサポート
- バイトコードVMまたはCPS変換を導入
- 任意の位置での中断・再開をサポート

---

## 1. 協調的マルチタスク (async/await) - Phase 1

### 設計方針

- **API**: TypeScript風のシンプルさ
- **内部実装**: Rust Tokio風のFuture/Poll機構（簡易版）
- **スレッド**: 単一スレッド実行
- **実行モデル**: タスクは完了まで実行（Phase 1では中断なし）
- **デバッグ**: `--debug`フラグでタスクの開始・完了をトレース

### 使用例 (Phase 1)

```cb
// Future型の定義（標準ライブラリ）
struct Future<T> {
    T value;
    bool is_ready;
};

// async関数の定義
async Future<int> fetch_data(int id) {
    println("Fetching data {id}...");
    // Phase 1: ここで計算処理を完了まで実行
    int result = id * 100;
    println("Data {id} ready");
    return result;
}

async Future<void> process_data(int id) {
    println("Processing {id}...");
    int value = id + 10;
    println("Processed: {value}");
}

void main() {
    // 複数のasync関数を起動
    Future<int> f1 = fetch_data(1);
    Future<int> f2 = fetch_data(2);
    Future<void> f3 = process_data(3);
    
    // awaitで完了を待つ
    // Phase 1: 起動時に既に完了しているので、値を取得するだけ
    int result1 = await f1;  // 100
    int result2 = await f2;  // 200
    await f3;
    
    println("Results: {result1}, {result2}");
}
```

### Phase 1の実行フロー

```
main()
  → fetch_data(1) 呼び出し
    → "Fetching data 1..."
    → 計算実行
    → "Data 1 ready"
    → Future{value=100, is_ready=true} を返す
  → fetch_data(2) 呼び出し
    → "Fetching data 2..."
    → 計算実行
    → "Data 2 ready"
    → Future{value=200, is_ready=true} を返す
  → await f1
    → is_readyがtrueなので、すぐにvalueを返す (100)
  → await f2
    → is_readyがtrueなので、すぐにvalueを返す (200)
  → "Results: 100, 200"
```

**Phase 1の制限**:
- タスクは呼び出し時に完了まで実行される
- 真の並行実行はない（順次実行）
- yieldは使用できない

**Phase 1の利点**:
- 実装がシンプルで確実に動作する
- async/awaitの基本構文が使える
- 将来の拡張への基盤となる

### Phase 1 実装詳細

#### データ構造（簡易版）

```cpp
// Future構造体（Cbコードで定義）
struct Future<T> {
    T value;          // 結果値
    bool is_ready;    // 完了フラグ
};

// async関数は通常の関数として実行され、Futureを返す
// Phase 1では特別な実行管理は不要
```

#### 実装戦略

**Phase 1: 即座実行モデル**

1. **async関数の呼び出し**:
   ```cpp
   // async Future<int> fetch_data(int id) { ... }
   // の呼び出し時
   
   Future<int> f = fetch_data(1);
   
   // 内部処理:
   // 1. 関数本体を完全に実行
   // 2. return文の値をFuture.valueに格納
   // 3. Future.is_ready = true を設定
   // 4. Futureを返す
   ```

2. **awaitの評価**:
   ```cpp
   int result = await f;
   
   // 内部処理:
   // 1. Future.is_readyをチェック
   // 2. trueなら Future.value を返す
   // 3. falseなら例外（Phase 1では発生しない）
   ```

3. **デバッグトレース** (`--debug`フラグ):
   ```
   [ASYNC] Calling async function: fetch_data
   [ASYNC] Function fetch_data completed with return value
   [AWAIT] Awaiting Future (already ready)
   [AWAIT] Returning value: 100
async Future<void> task1() {
    println("Step 1");
    yield;  // ここで中断
    println("Step 2");  // 次回再開時にここから実行
    yield;
    println("Step 3");
}

// ❌ NG: ループ内のyield（Phase 1では未サポート）
async Future<void> task2() {
    for (int i = 0; i < 5; i = i + 1) {
        println("Step {i}");
        yield;  // ループカウンタが保存されない
    }
}
```

#### 実行アルゴリズム

```cpp
// execute_task_one_step(): 1タスクを1ステートメント実行
void Interpreter::execute_task_one_step(AsyncTask* task) {
    if (!task->is_started) {
        // 初回実行: スコープを初期化
        initialize_task_scope(task);
        task->is_started = true;
    }
    
    push_scope(task->task_scope);  // タスクのスコープに切り替え
    
    try {
        // ステートメントリストから1つずつ実行
        ASTNode* body = task->function->body;
        if (body->type == AST_STMT_LIST) {
            if (task->current_statement_index < body->children.size()) {
                ASTNode* stmt = body->children[task->current_statement_index];
                execute_statement(stmt);
                task->current_statement_index++;  // 次のステートメントへ
            } else {
                // 全ステートメント実行完了
                task->is_completed = true;
            }
        }
    } catch (const YieldException&) {
        // yieldで中断: 現在のインデックスはそのまま
        task->is_suspended = true;
    } catch (const ReturnException& e) {
        // return文で完了
        task->return_value = e.value;
        task->is_completed = true;
    }
    
    pop_scope();  // スコープを戻す
}

// execute_tasks_concurrently(): ラウンドロビンで複数タスクを実行
void Interpreter::execute_tasks_concurrently(
    std::vector<int>& task_ids,
    int statements_per_round
) {
    while (!task_ids.empty()) {
        std::vector<int> still_pending;
        
        for (int task_id : task_ids) {
            AsyncTask* task = &async_tasks[task_id];
            
            // 各タスクを指定ステートメント数だけ実行
            for (int i = 0; i < statements_per_round; i++) {
                if (task->is_completed) break;
                
                task->is_suspended = false;
                execute_task_one_step(task);
                
                if (task->is_suspended) break;  // yieldで中断
            }
            
            // まだ完了していなければキューに戻す
            if (!task->is_completed) {
                still_pending.push_back(task_id);
            }
        }
        
        task_ids = still_pending;  // 次のラウンドへ
    }
}
```

### Phase 1の制限事項

1. **ループ内のyield未サポート**: 
   - `for`、`while`ループ内で`yield`を使用すると、ループカウンタがリセットされる
   - 解決には、ループの実行状態（イテレーション回数）を保存する必要がある

2. **ネストした制御構造**:
   - `if`文のブロック内など、ネストした構造内でのyieldは動作するが、位置追跡が粗い

3. **例外処理との相互作用**:
   - `try-catch`ブロック内でのyieldは未検証

### Phase 2への拡張計画

ループ内でのyieldをサポートするには：

1. **ループ状態の保存**:
   ```cpp
   struct LoopState {
       std::string loop_var_name;
       int current_iteration;
       int max_iterations;
   };
   std::vector<LoopState> loop_stack;  // AsyncTaskに追加
   ```

2. **ループ実行の特殊処理**:
   - `for`ループをマニュアルで展開せず、イテレーション単位で実行
   - yieldが呼ばれたら、現在のイテレーション番号を保存
   - 再開時は保存されたイテレーションから続行

---

## 2. OS スレッドベース Coroutine (goroutine風)

### 設計方針

- **API**: Go言語のgoroutine風
- **内部実装**: `std::thread`によるOSレベルの並列実行
- **スレッド**: マルチスレッド（真の並列実行）
- **制御**: プリエンプティブ（OSスケジューラによる自動切り替え）

### 使用例

```cb
// Channel型（送受信用）
Channel<int> ch = Channel<int>(10);  // バッファサイズ10

// Coroutineの起動
void worker(int id, Channel<int> ch) {
    for (int i = 0; i < 5; i = i + 1) {
        ch.send(id * 10 + i);
        sleep(100);  // 100ms待機
    }
}

void main() {
    // goroutine風の起動
    go worker(1, ch);
    go worker(2, ch);
    go worker(3, ch);
    
    // チャネルから受信
    for (int i = 0; i < 15; i = i + 1) {
        int value = ch.recv();
        println("Received: {value}");
    }
}
```

### インタプリタ内部の実装戦略

#### データ構造

```cpp
// Coroutine: OSスレッドで実行される関数
struct Coroutine {
    int coroutine_id;
    std::thread thread;
    FunctionDef* function;
    std::vector<Value> arguments;
    
    // スレッド間同期
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
};

// Coroutine管理
std::map<int, Coroutine> coroutines;
int next_coroutine_id = 1;
```

#### 実行戦略

```cpp
// go関数: 新しいOSスレッドでCoroutineを起動
int Interpreter::launch_coroutine(
    FunctionDef* func,
    const std::vector<Value>& args
) {
    int id = next_coroutine_id++;
    
    Coroutine& coro = coroutines[id];
    coro.coroutine_id = id;
    coro.function = func;
    coro.arguments = args;
    
    // 新しいスレッドで実行
    coro.thread = std::thread([this, id, func, args]() {
        // 新しいインタプリタコンテキストを作成
        Scope coroutine_scope;
        push_scope(&coroutine_scope);
        
        // 引数をセットアップ
        setup_function_arguments(func, args);
        
        try {
            // 関数本体を実行（通常のexecute_statementを使用）
            execute_statement(func->body.get());
        } catch (const ReturnException& e) {
            // 正常終了
        } catch (const std::exception& e) {
            std::cerr << "Coroutine " << id << " error: " << e.what() << std::endl;
        }
        
        pop_scope();
        coroutines[id].is_completed = true;
    });
    
    return id;
}

// Channelの実装
template<typename T>
void Channel<T>::send(const T& value) {
    std::unique_lock<std::mutex> lock(mtx);
    cv_send.wait(lock, [this] { 
        return buffer.size() < capacity || closed; 
    });
    
    if (closed) {
        throw std::runtime_error("Cannot send to closed channel");
    }
    
    buffer.push(value);
    cv_recv.notify_one();
}

template<typename T>
T Channel<T>::recv() {
    std::unique_lock<std::mutex> lock(mtx);
    cv_recv.wait(lock, [this] { 
        return !buffer.empty() || closed; 
    });
    
    if (buffer.empty() && closed) {
        throw std::runtime_error("Channel is closed and empty");
    }
    
    T value = buffer.front();
    buffer.pop();
    cv_send.notify_one();
    return value;
}
```

### async/await との違い

| 特徴 | async/await + yield | coroutine (goroutine風) |
|------|---------------------|------------------------|
| **実行モデル** | 協調的（明示的yield） | プリエンプティブ（OS管理） |
| **スレッド** | シングルスレッド | マルチスレッド |
| **並列性** | 並行（concurrent） | 並列（parallel） |
| **オーバーヘッド** | 低い | 高い（スレッド作成コスト） |
| **用途** | I/O待ち、UI応答性 | CPU集約的処理 |
| **同期** | Future/Promise | Channel/Mutex |
| **デバッグ** | 容易（単一スレッド） | 困難（レースコンディション） |

---

## 3. 実装ロードマップ

### Phase 1: 協調的マルチタスク基礎 (v0.12.0)

**目標**: トップレベルのyieldで動作する基本的なasync/await

**タスク**:
- [x] `async`キーワードのパース
- [x] `await`式のパース
- [x] `yield`文のパース
- [x] `Future<T>`型の標準ライブラリ定義
- [x] `AsyncTask`データ構造
- [ ] `execute_task_one_step()` - ステートメント単位実行
- [ ] `execute_tasks_concurrently()` - ラウンドロビンスケジューリング
- [ ] トップレベルyieldの統合テスト

**制限事項**:
- ループ内のyieldは未サポート
- 関数呼び出しをまたぐyieldは未サポート

### Phase 2: ループ内yield対応 (v0.13.0)

**目標**: ループ内でyieldが正しく動作

**タスク**:
- [ ] ループ状態保存機構
- [ ] `for`/`while`ループの特殊処理
- [ ] ネストしたループのサポート
- [ ] 統合テスト（複雑な制御フロー）

### Phase 3: OS スレッドベース Coroutine (v0.14.0)

**目標**: goroutine風のマルチスレッド並行処理

**タスク**:
- [ ] `go`キーワードまたは`spawn()`組み込み関数
- [ ] `Channel<T>`型の実装
- [ ] `Mutex`型の実装
- [ ] `std::thread`ベースのCoroutine起動
- [ ] スレッドプールの実装（オプション）
- [ ] デッドロック検出機構（オプション）

### Phase 4: 高度な機能 (v0.15.0+)

- [ ] `select`文（複数チャネルの待機）
- [ ] タイムアウト機能
- [ ] Coroutineのキャンセル
- [ ] エラーハンドリングの統合

---

## 4. 参考実装

### TypeScript (async/await)
```typescript
async function fetchData(id: number): Promise<number> {
    console.log(`Fetching ${id}...`);
    await delay(1000);
    return id * 100;
}

async function main() {
    const f1 = fetchData(1);
    const f2 = fetchData(2);
    const [r1, r2] = await Promise.all([f1, f2]);
    console.log(r1, r2);
}
```

### Rust (async/await with tokio)
```rust
async fn fetch_data(id: i32) -> i32 {
    println!("Fetching {}...", id);
    tokio::time::sleep(Duration::from_secs(1)).await;
    id * 100
}

#[tokio::main]
async fn main() {
    let f1 = fetch_data(1);
    let f2 = fetch_data(2);
    let (r1, r2) = tokio::join!(f1, f2);
    println!("{} {}", r1, r2);
}
```

### Go (goroutine + channel)
```go
func worker(id int, ch chan int) {
    for i := 0; i < 5; i++ {
        ch <- id*10 + i
        time.Sleep(100 * time.Millisecond)
    }
}

func main() {
    ch := make(chan int, 10)
    go worker(1, ch)
    go worker(2, ch)
    go worker(3, ch)
    
    for i := 0; i < 15; i++ {
        value := <-ch
        fmt.Println("Received:", value)
    }
}
```

---

## 5. まとめ

Cb言語の並行処理は、以下の2つの独立したシステムで構成されます：

1. **async/await + yield**: 軽量な協調的マルチタスク（TypeScript風API、Rust風内部実装）
2. **coroutine**: 本格的な並列処理（goroutine風API、std::thread実装）

Phase 1では、まず協調的マルチタスクの基礎を実装し、トップレベルのyieldで動作する基本機能を提供します。その後、段階的にループ内yield、OSスレッドベースのCoroutineへと拡張していきます。
