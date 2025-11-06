# Test Structure Reorganization

**日付**: 2025年11月5日  
**変更**: テスト構造を`stdlib/`の階層に合わせて再編成

---

## 変更内容

### 以前の構造

```
tests/cases/stdlib/
├── test_map_stress.cb
└── collections/
    ├── test_map.cb
    ├── test_vector_*.cb (9ファイル)
    ├── test_queue_*.cb (4ファイル)
    └── (その他20ファイル)
```

**問題点**:
- フラットな構造で、関連するテストの整理が困難
- `stdlib/collections/`の階層構造と不一致
- 新しいテストの追加時にどこに配置すべきか不明確

### 新しい構造

```
tests/cases/stdlib/collections/
├── README.md                    # 全体構造の説明
├── map/                         # Map<K, V>のテスト
│   ├── README.md
│   ├── test_basic.cb           # (旧: test_map.cb)
│   └── test_stress.cb          # (旧: ../test_map_stress.cb)
├── vector/                      # Vector<T>のテスト
│   ├── README.md
│   ├── test_vector_comprehensive.cb
│   ├── test_vector_int_complete.cb
│   ├── test_vector_string.cb
│   ├── test_vector_double.cb
│   ├── test_vector_struct.cb
│   └── ...                      # 他6ファイル
├── queue/                       # Queue<T>のテスト
│   ├── README.md
│   ├── test_queue_comprehensive.cb
│   ├── test_queue_string.cb
│   ├── test_queue_single_enqueue.cb
│   └── test_queue_import.cb
└── test_*.cb                    # 共通テスト（20ファイル）
    ├── test_malloc.cb
    ├── test_generic_containers.cb
    ├── test_nested_generics.cb
    └── ...
```

**改善点**:
- ✅ `stdlib/collections/`の構造と一致
- ✅ 各コレクション型ごとに専用フォルダ
- ✅ 各フォルダに説明用README.md
- ✅ テストの追加場所が明確
- ✅ 関連テストのグループ化

---

## 移動したファイル

### Map関連
- `tests/cases/stdlib/test_map_stress.cb` → `collections/map/test_stress.cb`
- `collections/test_map.cb` → `collections/map/test_basic.cb`

### Vector関連（9ファイル）
- `collections/test_vector_*.cb` → `collections/vector/test_vector_*.cb`

### Queue関連（4ファイル）
- `collections/test_queue_*.cb` → `collections/queue/test_queue_*.cb`

### 共通テスト（20ファイル）
- `collections/`直下に維持（メモリ管理、インポート、ジェネリックなど）

---

## 作成したドキュメント

1. **tests/cases/stdlib/collections/README.md**
   - 全体構造の説明
   - 各コレクションの概要
   - テスト実行方法
   - 新しいテストの追加方法

2. **tests/cases/stdlib/collections/map/README.md**
   - Map<K, V>の実装詳細（AVLツリー）
   - テストファイルの説明
   - パフォーマンス特性（O(log n)）
   - 実行方法

3. **tests/cases/stdlib/collections/vector/README.md**
   - Vector<T>の実装詳細
   - 10個のテストファイルの説明
   - パフォーマンス特性
   - 実行方法

4. **tests/cases/stdlib/collections/queue/README.md**
   - Queue<T>の実装詳細（FIFO）
   - 4個のテストファイルの説明
   - パフォーマンス特性（O(1)）
   - 実行方法

---

## テスト実行の変更

### 以前

```bash
./main tests/cases/stdlib/test_map_stress.cb
./main tests/cases/stdlib/collections/test_map.cb
./main tests/cases/stdlib/collections/test_vector_comprehensive.cb
```

### 現在

```bash
# より明確なパス
./main tests/cases/stdlib/collections/map/test_stress.cb
./main tests/cases/stdlib/collections/map/test_basic.cb
./main tests/cases/stdlib/collections/vector/test_vector_comprehensive.cb

# 一括実行も簡単
for test in tests/cases/stdlib/collections/map/*.cb; do
    ./main "$test"
done
```

---

## 今後の拡張

新しいコレクション型（例: Stack<T>, LinkedList<T>）を追加する際は：

1. **stdlib実装**: `stdlib/collections/stack.cb`
2. **テストフォルダ作成**: `mkdir tests/cases/stdlib/collections/stack/`
3. **README作成**: `tests/cases/stdlib/collections/stack/README.md`
4. **テスト追加**: `tests/cases/stdlib/collections/stack/test_*.cb`
5. **メインREADME更新**: `tests/cases/stdlib/collections/README.md`

---

## 検証結果

すべてのテストが新しいパスで正常に実行されることを確認：

```bash
✅ ./main tests/cases/stdlib/collections/map/test_stress.cb
   → Map<K, V> Stress Test Suite - All tests passed
   → Tree height: 10 for 1000 elements (perfect AVL balance)

✅ ./main tests/cases/stdlib/collections/map/test_basic.cb
   → Basic Map operations working correctly

✅ ./main tests/cases/stdlib/collections/vector/*.cb
   → All vector tests accessible

✅ ./main tests/cases/stdlib/collections/queue/*.cb
   → All queue tests accessible
```

---

## 関連ドキュメント

- **Feature**: `docs/features/vector_queue_generic_complete.md`
- **Testing**: `docs/testing/stdlib_test_structure.md`
- **Architecture**: `docs/architecture.md`

---

**完了**: テスト構造の再編成が完了しました。新しい構造により、テストの整理と拡張が容易になりました。
