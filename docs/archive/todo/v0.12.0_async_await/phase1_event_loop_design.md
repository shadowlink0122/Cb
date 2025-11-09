# Phase 1: Event Loop + タイマー 設計書

**作成日**: 2025年10月27日  
**対象**: v0.11.0 Phase 1  
**期間**: 2025/10/28 - 2025/11/10 (2週間)  
**ステータス**: 🔵 実装開始

---

## 📋 概要

Phase 1では、非同期処理の基盤となるEvent LoopとタイマーAPI実装します。これはv0.11.0の非同期処理（async/await）の土台となる重要なコンポーネントです。

### 目標
1. ✅ シングルスレッドのEvent Loop実装
2. ✅ タスクキューの実装
3. ✅ スケジューラーの実装
4. ✅ タイマーAPI（`sleep_ms`, `set_timeout`, `set_interval`）
5. ✅ 25個のテストケース

---

## 🎯 設計方針

### Event Loopのアーキテクチャ

```
┌─────────────────────────────────────┐
│         Event Loop                  │
│  ┌──────────────────────────────┐  │
│  │   Task Queue (FIFO)           │  │
│  │  [Task1, Task2, Task3, ...]   │  │
│  └──────────────────────────────┘  │
│                                     │
│  ┌──────────────────────────────┐  │
│  │   Timer Queue (Priority)      │  │
│  │  [Timer1(100ms), Timer2(...)] │  │
│  └──────────────────────────────┘  │
│                                     │
│  ┌──────────────────────────────┐  │
│  │   Scheduler                   │  │
│  │  - Execute ready tasks        │  │
│  │  - Check timer expiration     │  │
│  │  - Process callbacks          │  │
│  └──────────────────────────────┘  │
└─────────────────────────────────────┘
```

### 主要コンポーネント

#### 1. Task構造体
```cb
struct Task {
    int task_id;           // タスクID
    void* callback;        // コールバック関数ポインタ
    void* context;         // コンテキストデータ
    int priority;          // 優先度（将来拡張用）
}
```

#### 2. Timer構造体
```cb
struct Timer {
    int timer_id;          // タイマーID
    int delay_ms;          // 遅延時間（ミリ秒）
    int start_time;        // 開始時刻
    void* callback;        // コールバック関数
    void* context;         // コンテキスト
    bool repeat;           // 繰り返しフラグ（interval用）
}
```

#### 3. Queue<T>構造体（汎用キュー）
```cb
struct Queue<T> {
    T[1000] items;         // 要素の配列
    int front;             // 先頭インデックス
    int rear;              // 末尾インデックス
    int count;             // 要素数
    int capacity;          // 容量
}

impl Queue<T> {
    self() {
        self.front = 0;
        self.rear = 0;
        self.count = 0;
        self.capacity = 1000;
    }
    
    bool enqueue(T item) {
        if (self.is_full()) {
            return false;
        }
        self.items[self.rear] = item;
        self.rear = (self.rear + 1) % self.capacity;
        self.count = self.count + 1;
        return true;
    }
    
    T dequeue() {
        if (self.is_empty()) {
            // エラー処理
            T default_value;
            return default_value;
        }
        T item = self.items[self.front];
        self.front = (self.front + 1) % self.capacity;
        self.count = self.count - 1;
        return item;
    }
    
    T peek() {
        if (self.is_empty()) {
            T default_value;
            return default_value;
        }
        return self.items[self.front];
    }
    
    bool is_empty() {
        return self.count == 0;
    }
    
    bool is_full() {
        return self.count >= self.capacity;
    }
    
    int size() {
        return self.count;
    }
}
```

#### 4. EventLoop構造体
```cb
struct EventLoop {
    Queue<Task> task_queue;       // タスクキュー（循環キュー）
    Queue<Timer> timer_queue;     // タイマーキュー（循環キュー）
    
    bool is_running;              // 実行中フラグ
    int current_time;             // 現在時刻（ミリ秒）
    int next_task_id;             // 次のタスクID
    int next_timer_id;            // 次のタイマーID
}
```

---

## 🔧 実装詳細

### 1. Event Loop Core

#### EventLoop初期化
```cb
export struct EventLoop;

export impl EventLoop {
    self() {
        self.task_queue = Queue<Task>();
        self.timer_queue = Queue<Timer>();
        self.is_running = false;
        self.current_time = 0;
        self.next_task_id = 0;
        self.next_timer_id = 0;
    }
    
    ~self() {
        // クリーンアップ処理
        self.stop();
    }
}
```

#### タスクの追加
```cb
export impl EventLoop {
    bool enqueue_task(void* callback, void* context) {
        Task task;
        task.task_id = self.next_task_id;
        task.callback = callback;
        task.context = context;
        task.priority = 0;
        
        if (!self.task_queue.enqueue(task)) {
            println("Error: Task queue is full");
            return false;
        }
        
        self.next_task_id = self.next_task_id + 1;
        return true;
    }
}
```

#### Event Loopの実行
```cb
export impl EventLoop {
    void run() {
        self.is_running = true;
        
        while (self.is_running) {
            // 1. タイマーの確認と期限切れタスクの実行
            self.process_timers();
            
            // 2. タスクキューからタスクを1つ実行
            if (!self.task_queue.is_empty()) {
                self.execute_next_task();
            }
            
            // 3. タスクが無ければ終了
            if (self.task_queue.is_empty() && self.timer_queue.is_empty()) {
                self.is_running = false;
            }
            
            // 4. 時間を進める（シミュレーション）
            self.current_time = self.current_time + 1;
        }
    }
    
    void stop() {
        self.is_running = false;
    }
}
```

#### タスクの実行
```cb
export impl EventLoop {
    void execute_next_task() {
        if (self.task_queue.is_empty()) {
            return;
        }
        
        // キューからタスクを取り出す
        Task task = self.task_queue.dequeue();
        
        // コールバックを実行
        // TODO: 関数ポインタの呼び出し実装が必要
        println("Executing task: {task.task_id}");
    }
}
```

---

### 2. Timer API

#### タイマーの追加
```cb
export impl EventLoop {
    int set_timeout(void* callback, void* context, int delay_ms) {
        Timer timer;
        timer.timer_id = self.next_timer_id;
        timer.delay_ms = delay_ms;
        timer.start_time = self.current_time;
        timer.callback = callback;
        timer.context = context;
        timer.repeat = false;
        
        if (!self.timer_queue.enqueue(timer)) {
            println("Error: Timer queue is full");
            return -1;
        }
        
        self.next_timer_id = self.next_timer_id + 1;
        return timer.timer_id;
    }
    
    int set_interval(void* callback, void* context, int interval_ms) {
        Timer timer;
        timer.timer_id = self.next_timer_id;
        timer.delay_ms = interval_ms;
        timer.start_time = self.current_time;
        timer.callback = callback;
        timer.context = context;
        timer.repeat = true;  // 繰り返しフラグ
        
        if (!self.timer_queue.enqueue(timer)) {
            println("Error: Timer queue is full");
            return -1;
        }
        
        self.next_timer_id = self.next_timer_id + 1;
        return timer.timer_id;
    }
    
    void clear_timer(int timer_id) {
        // 一時的なキューを使ってフィルタリング
        Queue<Timer> temp_queue;
        
        while (!self.timer_queue.is_empty()) {
            Timer timer = self.timer_queue.dequeue();
            if (timer.timer_id != timer_id) {
                temp_queue.enqueue(timer);
            }
        }
        
        // 元のキューに戻す
        while (!temp_queue.is_empty()) {
            self.timer_queue.enqueue(temp_queue.dequeue());
        }
    }
}
```

#### タイマー処理
```cb
export impl EventLoop {
    void process_timers() {
        if (self.timer_queue.is_empty()) {
            return;
        }
        
        // 一時キューで期限切れタイマーを処理
        Queue<Timer> temp_queue;
        int queue_size = self.timer_queue.size();
        
        for (int i = 0; i < queue_size; i = i + 1) {
            Timer timer = self.timer_queue.dequeue();
            int elapsed = self.current_time - timer.start_time;
            
            if (elapsed >= timer.delay_ms) {
                // コールバック実行
                // TODO: 関数ポインタの呼び出し
                println("Timer {timer.timer_id} expired");
                
                if (timer.repeat) {
                    // intervalの場合は再スケジュール
                    timer.start_time = self.current_time;
                    temp_queue.enqueue(timer);
                }
                // timeoutの場合は再登録しない（自動削除）
            } else {
                // まだ期限切れでないタイマーは再登録
                temp_queue.enqueue(timer);
            }
        }
        
        // 残ったタイマーを元のキューに戻す
        while (!temp_queue.is_empty()) {
            self.timer_queue.enqueue(temp_queue.dequeue());
        }
    }
}
```

---

### 3. ヘルパー関数

#### sleep_ms関数
```cb
void sleep_ms(EventLoop* loop, int ms) {
    int start = loop.current_time;
    while (loop.current_time - start < ms) {
        // ビジーウェイト（将来的にはyieldに変更）
    }
}
```

#### 現在時刻の取得
```cb
int get_current_time_ms() {
    // システム時刻を取得（C++の実装側で実装）
    // 仮実装: ミリ秒単位のカウンタ
    return 0;  // TODO: 実装
}
```

---

## 📁 ファイル構成

### 新規作成ファイル

1. **stdlib/queue.cb** - 汎用Queue構造体の実装
   ```
   - Queue<T>構造体（ジェネリック）
   - enqueue, dequeue, peek メソッド
   - is_empty, is_full, size メソッド
   - 循環バッファの実装
   ```

2. **stdlib/event_loop.cb** - Event Loopの実装
   ```
   - EventLoop構造体
   - Task構造体
   - Timer構造体
   - enqueue_task, run, stop メソッド
   - set_timeout, set_interval, clear_timer
   - process_timers, execute_next_task
   ```

3. **stdlib/timer.cb** - タイマー関連ユーティリティ
   ```
   - sleep_ms
   - get_current_time_ms
   - delay関数群
   ```

4. **src/backend/interpreter/stdlib/event_loop.cpp** - C++側の実装
   ```
   - システム時刻の取得
   - 高精度タイマー
   - コールバック実行機構
   ```

---

## 🧪 テスト計画

### Week 1: Event Loop実装（15テスト）

#### 基本機能テスト
1. **test_event_loop_init.cb** - 初期化テスト
2. **test_task_enqueue.cb** - タスク追加テスト
3. **test_task_execution.cb** - タスク実行テスト
4. **test_multiple_tasks.cb** - 複数タスク実行テスト
5. **test_task_order.cb** - タスク実行順序テスト

#### Event Loop実行テスト
6. **test_loop_run.cb** - ループ実行テスト
7. **test_loop_stop.cb** - ループ停止テスト
8. **test_empty_loop.cb** - 空のループテスト
9. **test_loop_auto_stop.cb** - 自動停止テスト

#### コールバックテスト
10. **test_callback_basic.cb** - コールバック基本テスト
11. **test_callback_context.cb** - コンテキスト渡しテスト
12. **test_callback_multiple.cb** - 複数コールバックテスト

#### エラーハンドリングテスト
13. **test_queue_full.cb** - キュー満杯テスト
14. **test_null_callback.cb** - Nullコールバックテスト
15. **test_task_limit.cb** - タスク数制限テスト

---

### Week 2: タイマー実装（10テスト）

#### set_timeoutテスト
1. **test_set_timeout_basic.cb** - 基本的なタイムアウト
2. **test_set_timeout_multiple.cb** - 複数のタイムアウト
3. **test_set_timeout_order.cb** - タイムアウト順序テスト
4. **test_clear_timeout.cb** - タイムアウトキャンセル

#### set_intervalテスト
5. **test_set_interval_basic.cb** - 基本的なインターバル
6. **test_set_interval_multiple.cb** - 複数のインターバル
7. **test_clear_interval.cb** - インターバルキャンセル

#### タイマー統合テスト
8. **test_timer_accuracy.cb** - タイマー精度テスト
9. **test_mixed_timers.cb** - timeout/interval混在テスト
10. **test_sleep_ms.cb** - sleep_ms関数テスト

---

## 📊 実装マイルストーン

### Week 1: Event Loop Core（2025/10/28 - 2025/11/03）

**Day 1-2: 基本構造**
- [ ] EventLoop構造体の定義
- [ ] Task構造体の定義
- [ ] 初期化メソッド実装

**Day 3-4: タスクキュー**
- [ ] enqueue_task実装
- [ ] execute_next_task実装
- [ ] タスクキューのシフト処理

**Day 5-7: Event Loop実行**
- [ ] run()メソッド実装
- [ ] stop()メソッド実装
- [ ] 15個のテスト作成・実行

---

### Week 2: タイマーAPI（2025/11/04 - 2025/11/10）

**Day 1-2: タイマー基本実装**
- [ ] Timer構造体の定義
- [ ] set_timeout実装
- [ ] process_timers実装

**Day 3-4: タイマー拡張**
- [ ] set_interval実装
- [ ] clear_timer実装
- [ ] 繰り返しタイマーの処理

**Day 5-7: 統合とテスト**
- [ ] sleep_ms実装
- [ ] 10個のテスト作成・実行
- [ ] パフォーマンステスト
- [ ] ドキュメント作成

---

## 🎯 完了基準

### 必須要件
1. ✅ EventLoop構造体が動作する
2. ✅ タスクキューが正しく動作する
3. ✅ set_timeout/set_intervalが動作する
4. ✅ タイマーの精度が±10ms以内
5. ✅ 25個のテストがすべてパス
6. ✅ メモリリークがない

### 望ましい要件
- 🎯 タスク処理のパフォーマンス最適化
- 🎯 エラーハンドリングの充実
- 🎯 デバッグ用のログ機能

---

## 🚨 既知の制約と課題

### 制約
1. **シングルスレッド実行**
   - 真の並行処理は行わない
   - タスクは順次実行

2. **タイマー精度**
   - システムクロックに依存
   - ビジーウェイトによる精度低下の可能性

3. **キューサイズ**
   - Task: 最大1000個（Queue<Task>のcapacity）
   - Timer: 最大1000個（Queue<Timer>のcapacity）
   - ジェネリックQueueを使用することで統一的に管理

### 課題
1. **関数ポインタの呼び出し**
   - Cbの関数ポインタ機構の確認が必要
   - C++側での実装サポートが必要かもしれない

2. **コンテキストデータ**
   - void*の代わりにジェネリクスを使うべきか検討
   - 型安全性の確保

3. **時間管理**
   - 実時間とシミュレーション時間の使い分け
   - テスト時の時間制御

---

## 📖 使用例

### 基本的なEvent Loop
```cb
import "stdlib/event_loop.cb";

void my_task() {
    println("Task executed!");
}

void main() {
    EventLoop loop;
    
    // タスクを追加
    loop.enqueue_task(my_task, nullptr);
    
    // Event Loopを実行
    loop.run();
}
```

### タイマーの使用
```cb
import "stdlib/event_loop.cb";

void timeout_handler() {
    println("Timeout after 1000ms");
}

void interval_handler() {
    println("Interval triggered");
}

void main() {
    EventLoop loop;
    
    // 1秒後に実行
    loop.set_timeout(timeout_handler, nullptr, 1000);
    
    // 500msごとに実行
    int interval_id = loop.set_interval(interval_handler, nullptr, 500);
    
    // Event Loopを実行
    loop.run();
}
```

---

## 🔜 次のフェーズへの準備

Phase 1が完了したら、Phase 2（Future<T>型）で以下を実装：

1. **Future<T>構造体**
   - Event Loopと統合
   - 非同期タスクの結果を保持

2. **Promiseパターン**
   - resolve/reject機能
   - then/catch メソッド

3. **タスクのチェーン**
   - 複数の非同期処理を連鎖
   - エラー伝播

---

**作成者**: GitHub Copilot  
**レビュアー**: shadowlink0122  
**最終更新**: 2025年10月27日  
**次回レビュー**: 2025年11月03日（Week 1完了時）
