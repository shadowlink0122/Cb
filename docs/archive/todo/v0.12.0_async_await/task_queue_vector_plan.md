# TaskQueue Vector実装計画

**作成予定**: Week 3 Day 2以降  
**前提条件**: 
1. 構造体配列への代入修正完了
2. VectorのAPI確認・整備

---

## 概要

固定サイズ配列`Task[100]`を`Vector<Task, A: Allocator>`に置き換え、容量制限を撤廃します。

---

## 実装仕様

### 構造体定義

```cb
struct TaskQueue<A: Allocator> {
    Vector<Task, A> tasks;
    A allocator;
};
```

### API設計

#### 初期化

```cb
void init(TaskQueue<A>& queue, A& alloc) {
    queue.allocator = alloc;
    vector_init(queue.tasks, alloc);
}

void destroy(TaskQueue<A>& queue) {
    vector_destroy(queue.tasks);
}
```

#### Push（優先度順に挿入）

```cb
void push(TaskQueue<A>& queue, Task& task) {
    // 末尾に追加
    vector_push(queue.tasks, task);
    
    // 挿入ソートで優先度順を維持
    int i = vector_size(queue.tasks) - 1;
    while (i > 0) {
        Task& curr = vector_at(queue.tasks, i);
        Task& prev = vector_at(queue.tasks, i - 1);
        
        if (curr.priority < prev.priority) {
            // スワップ
            Task temp = curr;
            vector_set(queue.tasks, i, prev);
            vector_set(queue.tasks, i - 1, temp);
            i = i - 1;
        } else {
            break;
        }
    }
}
```

#### Pop（先頭要素を取得）

```cb
Task pop(TaskQueue<A>& queue) {
    if (vector_is_empty(queue.tasks)) {
        return {-1, 999, -1, nullptr};  // Invalid task
    }
    
    // 先頭要素を取得
    Task result = vector_at(queue.tasks, 0);
    
    // 先頭を削除（要素をシフト）
    vector_erase(queue.tasks, 0);
    
    return result;
}
```

#### ユーティリティ

```cb
bool is_empty(TaskQueue<A>& queue) {
    return vector_is_empty(queue.tasks);
}

int size(TaskQueue<A>& queue) {
    return vector_size(queue.tasks);
}

Task& peek(TaskQueue<A>& queue) {
    return vector_at(queue.tasks, 0);
}

void clear(TaskQueue<A>& queue) {
    vector_clear(queue.tasks);
}
```

---

## 必要なVector API

以下のAPIがVectorに実装されている必要があります：

```cb
// 基本操作
void vector_init<T, A>(Vector<T, A>& vec, A& alloc);
void vector_destroy<T, A>(Vector<T, A>& vec);

// 追加・削除
void vector_push<T, A>(Vector<T, A>& vec, T& item);
void vector_erase<T, A>(Vector<T, A>& vec, int index);
void vector_clear<T, A>(Vector<T, A>& vec);

// アクセス
T& vector_at<T, A>(Vector<T, A>& vec, int index);
void vector_set<T, A>(Vector<T, A>& vec, int index, T& value);

// 状態確認
bool vector_is_empty<T, A>(Vector<T, A>& vec);
int vector_size<T, A>(Vector<T, A>& vec);
int vector_capacity<T, A>(Vector<T, A>& vec);
```

---

## 使用例

```cb
void main() {
    // SystemAllocatorを使用
    SystemAllocator alloc;
    TaskQueue<SystemAllocator> queue;
    init(queue, alloc);
    
    // タスクを追加（容量制限なし）
    for (int i = 0; i < 1000; i++) {
        Task t = {i, i % 10, 0, nullptr};
        push(queue, t);
    }
    
    println("Queue size: ", size(queue));  // 1000
    
    // 優先度順に処理
    while (!is_empty(queue)) {
        Task t = pop(queue);
        println("Processing task ", t.task_id, " priority ", t.priority);
    }
    
    destroy(queue);
}
```

---

## パフォーマンス

### 現在（挿入ソート版）

- **Push**: O(n) - 挿入位置まで要素をシフト
- **Pop**: O(n) - 先頭削除後、要素をシフト
- **メモリ**: 動的に伸縮

### 将来の最適化（Phase 3: ヒープ化）

```cb
// Min-Heapを使用
struct TaskQueue<A: Allocator> {
    Vector<Task, A> heap;  // バイナリヒープ
    A allocator;
};

// Push: O(log n)
void push(TaskQueue<A>& queue, Task& task) {
    vector_push(queue.heap, task);
    heap_up(queue.heap, vector_size(queue.heap) - 1);
}

// Pop: O(log n)
Task pop(TaskQueue<A>& queue) {
    Task result = vector_at(queue.heap, 0);
    Task last = vector_pop(queue.heap);
    if (!vector_is_empty(queue.heap)) {
        vector_set(queue.heap, 0, last);
        heap_down(queue.heap, 0);
    }
    return result;
}
```

---

## テストケース

```cb
void test_vector_based_queue() {
    SystemAllocator alloc;
    TaskQueue<SystemAllocator> queue;
    init(queue, alloc);
    
    // Test 1: 大量のタスク追加
    for (int i = 0; i < 10000; i++) {
        Task t = {i, random(100), 0, nullptr};
        push(queue, t);
    }
    assert(size(queue) == 10000);
    
    // Test 2: 優先度順に取得
    int prev_priority = -1;
    while (!is_empty(queue)) {
        Task t = pop(queue);
        assert(t.priority >= prev_priority);
        prev_priority = t.priority;
    }
    
    destroy(queue);
    println("✅ Vector-based queue test passed");
}
```

---

## 移行計画

### Step 1: Vector API確認
- Week 2で実装したVectorのAPIを確認
- 不足している機能を実装（特にeraseとset）

### Step 2: TaskQueue実装
- `stdlib/async/task_queue_vector.cb`を作成
- Vectorベースの実装を完成

### Step 3: テスト
- 大量データでのテスト
- メモリリーク確認
- パフォーマンス測定

### Step 4: 統合
- EventLoopで使用
- 本番環境でのテスト

### Step 5: 最適化（オプション）
- ヒープ構造に変更
- O(log n)のpush/pop実現

---

## 依存関係

```
Week 2: Allocator + Vector
    ↓
Week 3 Day 1: Task + TaskQueue (fixed array)
    ↓
インタプリタ修正: 構造体配列代入サポート
    ↓
Week 3 Day 2: TaskQueue (Vector-based) ← これ
    ↓
Week 3 Day 3-4: EventLoop実装
    ↓
Week 3 Day 5: 最適化（Heap化）
```

---

## ファイル構成

```
stdlib/async/
├── task.cb                      # Task構造体
├── task_queue_final.cb          # 並列配列版（Phase 0）
├── task_queue_ideal.cb          # 固定配列版（Phase 1）
└── task_queue_vector.cb         # Vector版（Phase 2）← 作成予定

tests/cases/async/
├── test_task_queue_comprehensive.cb  # 現行テスト
└── test_task_queue_vector.cb         # Vector版テスト（作成予定）
```

---

## まとめ

Vector版TaskQueueは**本来あるべき実装**です。

**メリット**:
- ✅ 容量無制限
- ✅ メモリ効率的
- ✅ Week 2の成果を活用
- ✅ Allocatorで柔軟な管理

**前提条件**:
1. 構造体配列への代入修正
2. VectorのAPI整備（erase, set等）

**実装タイミング**: Week 3 Day 2以降
