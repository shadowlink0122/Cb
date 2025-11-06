# Collections Tests

**カテゴリ**: コレクション（データ構造）  
**対象**: `stdlib/collections/`

---

## 📖 概要

このディレクトリには、Cbの標準ライブラリのコレクション（データ構造）に関するテストが含まれています。

**2025年11月更新**: テスト構造を`stdlib/collections/`の階層に合わせて再編成しました。

---

## 📂 ディレクトリ構造

```
tests/cases/stdlib/collections/
├── map/                    # Map<K, V>のテスト
│   ├── README.md
│   ├── test_basic.cb
│   └── test_stress.cb
├── vector/                 # Vector<T>のテスト  
│   ├── README.md
│   ├── test_vector_comprehensive.cb
│   ├── test_vector_int_complete.cb
│   ├── test_vector_string.cb
│   ├── test_vector_double.cb
│   ├── test_vector_struct.cb
│   └── ...
├── queue/                  # Queue<T>のテスト
│   ├── README.md
│   ├── test_queue_comprehensive.cb
│   ├── test_queue_string.cb
│   └── ...
└── (その他のテストファイル)
```

---

## 🧪 実装済みコレクション

### Map<K, V> - AVLツリー実装 ✅

- **データ構造**: 自己平衡二分探索木（AVLツリー）
- **パフォーマンス**: すべての操作がO(log n)
- **テストファイル**: `map/test_basic.cb`, `map/test_stress.cb`
- **検証済み**: 1000要素で高さ10（理論的最適値）

**主な機能**:
- `insert(K key, V value)` - O(log n)
- `get(K key)` - O(log n)
- `remove(K key)` - O(log n)
- `contains(K key)` - O(log n)
- 自動バランシング（4つの回転ケース: LL, LR, RR, RL）

### Vector<T> - 動的配列 ✅

- **データ構造**: 動的配列（可変長配列）
- **パフォーマンス**: push_back()は償却O(1)、アクセスはO(1)
- **テストファイル**: 10個のテストファイル（包括的、文字列、構造体など）
- **ジェネリック型**: Vector<int>, Vector<String>, Vector<StructType>など

**主な機能**:
- `push_back(T value)` - 償却O(1)
- `get(int index)` / `set(int index, T value)` - O(1)
- `size()` - O(1)
- `capacity()` - O(1)

### Queue<T> - FIFOキュー ✅

- **データ構造**: FIFO（First In First Out）キュー
- **パフォーマンス**: すべての操作がO(1)
- **テストファイル**: 4個のテストファイル（包括的、文字列、基本操作など）

**主な機能**:
- `enqueue(T value)` - O(1)
- `dequeue()` - O(1)
- `front()` - O(1)
- `is_empty()` - O(1)

---

## 🚀 実行方法

### 各コレクションのテスト実行

```bash
# Mapのテスト
./main tests/cases/stdlib/collections/map/test_basic.cb
./main tests/cases/stdlib/collections/map/test_stress.cb

# Vectorのテスト
./main tests/cases/stdlib/collections/vector/test_vector_comprehensive.cb
./main tests/cases/stdlib/collections/vector/test_vector_int_complete.cb

# Queueのテスト
./main tests/cases/stdlib/collections/queue/test_queue_comprehensive.cb
./main tests/cases/stdlib/collections/queue/test_queue_string.cb
```

### 全コレクションテストの一括実行

```bash
# Mapテストをすべて実行
for test in tests/cases/stdlib/collections/map/*.cb; do
    echo "Running $test..."
    ./main "$test"
done

# Vectorテストをすべて実行
for test in tests/cases/stdlib/collections/vector/*.cb; do
    echo "Running $test..."
    ./main "$test"
done

# Queueテストをすべて実行
for test in tests/cases/stdlib/collections/queue/*.cb; do
    echo "Running $test..."
    ./main "$test"
done
```

### make経由（既存のタスク）

```bash
make test  # 統合テスト実行
```

---

## 📝 新しいテストの追加

### 既存のコレクションにテストを追加

1. **適切なフォルダを選択**
   ```
   map/     ← Map<K, V>のテスト
   vector/  ← Vector<T>のテスト
   queue/   ← Queue<T>のテスト
   ```

2. **テストファイルを作成**
   ```cb
   // tests/cases/stdlib/collections/map/test_custom_keys.cb
   import "stdlib/collections/map.cb"
   
   void test_custom_key_type() {
       println("=== Testing Map with custom key type ===");
       Map<CustomKey, int> m;
       m.init();
       // テストロジック...
       println("✅ Test passed");
   }
   
   void main() {
       test_custom_key_type();
   }
   ```

3. **対応するREADME.mdを更新**
   - `map/README.md`、`vector/README.md`、`queue/README.md`のいずれか
   - テストファイルの説明を追加

### 新しいコレクションのテスト追加

1. **stdlibに実装を追加**: `stdlib/collections/newtype.cb`
2. **テストフォルダを作成**: `mkdir tests/cases/stdlib/collections/newtype/`
3. **README.mdを作成**: `tests/cases/stdlib/collections/newtype/README.md`
4. **テストファイルを追加**: `tests/cases/stdlib/collections/newtype/test_*.cb`
5. **このREADMEを更新**: 新しいコレクションの説明を追加

---

## 🎯 実装状況

| コレクション | 実装状態 | テスト数 | パフォーマンス | 最終更新 |
|------------|---------|---------|--------------|---------|
| Map<K, V> | ✅ 完了 | 2 | O(log n) | 2025-11-05 |
| Vector<T> | ✅ 完了 | 10 | O(1) access | 2025-10-28 |
| Queue<T> | ✅ 完了 | 4 | O(1) ops | 2025-10-28 |
| Stack<T> | ⏳ 計画中 | 0 | - | - |
| LinkedList<T> | ⏳ 計画中 | 0 | - | - |

---

## 🔗 関連ドキュメント

- **stdlib実装**: `stdlib/collections/`
- **C++テスト**: `tests/stdlib/collections/`
- **Vector設計**: `docs/todo/phase1a_dynamic_array_design.md`
- **テスト構造**: `docs/testing/stdlib_test_structure.md`

---

**最終更新**: 2025年10月28日
