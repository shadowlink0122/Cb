# インタプリタ実装不足: 構造体配列への代入

**日付**: 2025年10月27日  
**重要度**: 🔴 High  
**影響範囲**: 構造体配列を使用する全てのコード

---

## 問題の説明

Cbインタプリタは現在、**構造体全体を配列要素に代入する操作**をサポートしていません。

### 期待される動作

```cb
struct Task {
    int task_id;
    int priority;
};

void main() {
    Task[10] tasks;
    Task t = {42, 5};
    
    tasks[0] = t;  // ✅ これが動作すべき
    
    println(tasks[0].task_id);  // 期待値: 42
}
```

### 実際の動作

```cb
tasks[0] = t;  // 代入は成功するように見えるが...
println(tasks[0].task_id);  // 実際: 0 (初期値のまま)
```

**結果**: 代入が無視され、配列要素はゼロ初期化されたまま

---

## 再現手順

1. テストファイルを実行:
```bash
./main tests/cases/async/test_struct_array_assign.cb
```

2. 出力結果:
```
❌ Struct array assignment failed!
Expected: 42, Got: 0
```

---

## 根本原因の推定

インタプリタの`array_element_assignment`処理において、構造体のバイトコピーが正しく実装されていない可能性があります。

### 考えられる原因

1. **配列要素のアドレス解決**: `tasks[0]`が正しいメモリアドレスを指していない
2. **構造体コピー**: 構造体全体のmemcpyが実行されていない
3. **型情報の欠落**: 配列要素の型が`struct`であることを認識していない

---

## 現在の回避策

構造体配列の代わりに**並列配列パターン**を使用:

```cb
// ❌ 理想的だが動作しない
struct TaskQueue {
    Task[100] tasks;
    int length;
};

// ✅ 現在の回避策
struct TaskQueue {
    int[100] task_ids;
    int[100] priorities;
    int[100] callback_types;
    void*[100] data_ptrs;
    int length;
};
```

### 回避策の問題点

- コードが冗長になる
- フィールド追加時に複数箇所を修正
- 構造体の意味が失われる
- 保守性が低下

---

## 修正すべきファイル

インタプリタのコード（推定）:

```
src/backend/interpreter/
├── variable_manager.cpp  # 変数代入処理
├── array_operations.cpp  # 配列操作
└── memory_manager.cpp    # メモリコピー処理
```

### 必要な実装

```cpp
// 配列要素への構造体代入を処理
void assign_to_array_element(ArrayInfo& array, int index, Value& value) {
    if (value.type == ValueType::STRUCT) {
        // 構造体全体をコピー
        void* dest = get_array_element_address(array, index);
        void* src = value.struct_data;
        size_t struct_size = get_struct_size(value.struct_type);
        memcpy(dest, src, struct_size);
    } else {
        // プリミティブ型の代入
        // ... 既存の処理
    }
}
```

---

## テストケース

修正後に以下のテストが全て通過すべき:

### Test 1: 基本的な代入
```cb
Task[5] tasks;
Task t = {42, 5, 1, nullptr};
tasks[0] = t;
assert(tasks[0].task_id == 42);
```

### Test 2: ループ内での代入
```cb
Task[10] tasks;
for (int i = 0; i < 10; i++) {
    Task t = {i, i * 2, 0, nullptr};
    tasks[i] = t;
}
assert(tasks[5].task_id == 5);
```

### Test 3: 配列要素間のコピー
```cb
Task[5] tasks;
tasks[0] = {1, 10, 0, nullptr};
tasks[1] = tasks[0];
assert(tasks[1].task_id == 1);
```

### Test 4: 関数引数として渡す
```cb
void set_task(Task[10]& tasks, int idx, Task& t) {
    tasks[idx] = t;
}

void main() {
    Task[10] tasks;
    Task t = {99, 1, 0, nullptr};
    set_task(tasks, 0, t);
    assert(tasks[0].task_id == 99);
}
```

---

## 影響を受けるコード

### 現在のコードベース

- `stdlib/async/task_queue.cb` - 並列配列を使用（本来は`Task[100]`を使うべき）
- その他の構造体配列を使用するコード

### 将来的な実装

- Week 3 EventLoop実装
- データ構造（Stack, Queue, etc.）
- ゲーム開発のエンティティ配列
- 物理演算のパーティクルシステム

---

## 優先度

**🔴 High Priority**

理由:
1. 基本的な言語機能として必須
2. 並列配列パターンは保守性が低い
3. 多くのデータ構造実装で必要
4. ユーザーコードの品質に直結

---

## 修正後の理想的なコード

```cb
// 理想的なTaskQueue実装（インタプリタ修正後）
struct TaskQueue {
    Task[100] tasks;  // ✅ シンプルで明確
    int length;
    int capacity;
};

void push(TaskQueue& queue, Task& task) {
    if (queue.length >= queue.capacity) {
        return;
    }
    
    // ✅ 直接代入できる
    queue.tasks[queue.length] = task;
    queue.length = queue.length + 1;
    
    // ソート処理も簡潔に
    int i = queue.length - 1;
    while (i > 0 && queue.tasks[i].priority < queue.tasks[i-1].priority) {
        Task temp = queue.tasks[i];
        queue.tasks[i] = queue.tasks[i - 1];
        queue.tasks[i - 1] = temp;
        i = i - 1;
    }
}

Task pop(TaskQueue& queue) {
    if (queue.length <= 0) {
        return {-1, 999, -1, nullptr};
    }
    
    // ✅ 直接読み取れる
    Task result = queue.tasks[0];
    
    // シフト処理も簡潔
    int i = 0;
    while (i < queue.length - 1) {
        queue.tasks[i] = queue.tasks[i + 1];
        i = i + 1;
    }
    
    queue.length = queue.length - 1;
    return result;
}
```

**コード量比較**:
- 並列配列版: ~150行
- 構造体配列版: ~80行（約50%削減）

---

## 次のアクション

1. ✅ 問題を文書化（このファイル）
2. ⬜ インタプリタのソースコード調査
3. ⬜ 配列要素代入処理の修正実装
4. ⬜ テストケースの追加
5. ⬜ 修正後にTaskQueueを理想的な実装に書き換え

---

## 関連ドキュメント

- [Week 3 Day 1 Report](week3_day1_taskqueue_report.md)
- [Coding Guidelines](../tutorial/common_mistakes.md)
- テストファイル: `tests/cases/async/test_struct_array_assign.cb`

---

## まとめ

構造体配列への代入は**Cb言語として当然サポートすべき機能**です。現在の並列配列パターンは一時的な回避策に過ぎず、インタプリタの修正が必要です。

修正されれば、コードは大幅に簡潔になり、保守性も向上します。
