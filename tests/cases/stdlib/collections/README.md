# Collections Tests

**カテゴリ**: コレクション（データ構造）  
**対象**: `stdlib/collections/`

---

## 📖 概要

このディレクトリには、Cbの標準ライブラリのコレクション（データ構造）に関するテストが含まれています。

---

## 📂 テストファイル

| ファイル | 対象 | テスト数 | 状態 |
|---------|------|---------|------|
| `test_vector.cb` | Vector | 3 | ✅ |
| `test_queue.cb` | Queue | - | ⏳ 未作成 |
| `test_stack.cb` | Stack | - | ⏳ 未作成 |
| `test_map.cb` | Map | - | ⏳ 未作成 |

---

## 🧪 テスト対象

### Vector

- 動的配列（可変長配列）
- ジェネリック型パラメータ `Vector<T, A: Allocator>`
- カスタムアロケータサポート

**テスト項目**:
- 初期化
- 各種操作（push/pop/resize）
- 複数アロケータとの組み合わせ

### Queue（今後追加予定）

- FIFO（First In First Out）キュー
- enqueue/dequeue操作

### Stack（今後追加予定）

- LIFO（Last In First Out）スタック
- push/pop操作

### Map（今後追加予定）

- キー・バリュー連想配列
- ハッシュマップ実装

---

## 🚀 実行方法

### 全てのコレクションテストを実行

```bash
./main tests/cases/stdlib/collections/test_vector.cb
# 今後追加
# ./main tests/cases/stdlib/collections/test_queue.cb
# ./main tests/cases/stdlib/collections/test_stack.cb
# ./main tests/cases/stdlib/collections/test_map.cb
```

### make経由

```bash
make stdlib-test-cb
```

---

## 📝 新しいコレクションテストの追加

1. **stdlibファイル作成**
   ```cb
   // stdlib/collections/queue.cb
   export struct Queue<T, A: Allocator> {
       int front;
       int rear;
       int size;
   };
   
   export void queue_enqueue(Queue& q, T value) { }
   export T queue_dequeue(Queue& q) { }
   ```

2. **テストファイル作成**
   ```cb
   // tests/cases/stdlib/collections/test_queue.cb
   import stdlib.collections.queue;
   
   void test_queue_basic() {
       println("Queue operations test");
       println("✅ Test passed");
   }
   
   void main() {
       test_queue_basic();
   }
   ```

3. **Makefileに追加**
   ```makefile
   @echo "[2/2] Testing Queue..."
   @./$(MAIN_TARGET) tests/cases/stdlib/collections/test_queue.cb
   ```

---

## 🎯 実装ロードマップ

| コレクション | 優先度 | 実装予定 | 状態 |
|------------|-------|---------|------|
| Vector | 🔴 High | Week 4 | ✅ 完了 |
| Queue | 🟡 Medium | Week 5 | ⏳ 計画中 |
| Stack | 🟡 Medium | Week 5 | ⏳ 計画中 |
| Map | 🟢 Low | Week 6+ | ⏳ 計画中 |
| LinkedList | 🟢 Low | Week 7+ | ⏳ 計画中 |

---

## 🔗 関連ドキュメント

- **stdlib実装**: `stdlib/collections/`
- **C++テスト**: `tests/stdlib/collections/`
- **Vector設計**: `docs/todo/phase1a_dynamic_array_design.md`
- **テスト構造**: `docs/testing/stdlib_test_structure.md`

---

**最終更新**: 2025年10月28日
