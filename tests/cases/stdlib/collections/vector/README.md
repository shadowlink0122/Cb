# Vector Tests

このフォルダには、`stdlib/collections/vector.cb`の実装をテストするファイルが含まれています。

## テストファイル

### test_vector_comprehensive.cb
- **目的**: Vector<T>の全機能の包括的なテスト
- **内容**:
  - push_back() - 要素の末尾追加
  - get() / set() - インデックスアクセス
  - size() - サイズ取得
  - capacity() - 容量管理
  - 動的なメモリ再割り当て

### test_vector_int_complete.cb
- **目的**: Vector<int>の完全な動作検証
- **内容**: 整数型ベクターの基本操作

### test_vector_string.cb
- **目的**: Vector<String>の文字列操作テスト
- **内容**: 文字列型ベクターの基本操作

### test_vector_string_debug.cb / test_vector_string_double.cb
- **目的**: 文字列ベクターのデバッグと追加検証

### test_vector_double.cb / test_vector_double_simple.cb
- **目的**: Vector<double>の浮動小数点数操作テスト
- **内容**: double型ベクターの精度と基本操作

### test_vector_struct.cb
- **目的**: Vector<StructType>の構造体操作テスト
- **内容**: カスタム構造体をジェネリック型として使用

### test_vector_generic_types.cb
- **目的**: 複数のジェネリック型でのテスト
- **内容**: Vector<T>のジェネリック実装の検証

### test_vector_import.cb
- **目的**: インポート機構のテスト
- **内容**: `import "stdlib/collections/vector.cb"`の動作確認

## パフォーマンス特性

- **push_back()**: 償却O(1)（動的配列の再割り当てあり）
- **get() / set()**: O(1)
- **size()**: O(1)

## テストの実行

```bash
# 包括的テスト
./main tests/cases/stdlib/collections/vector/test_vector_comprehensive.cb

# 整数型テスト
./main tests/cases/stdlib/collections/vector/test_vector_int_complete.cb

# 文字列型テスト
./main tests/cases/stdlib/collections/vector/test_vector_string.cb
```

## 新しいテストの追加

このフォルダに新しいテストファイルを追加する際は：
1. ファイル名: `test_vector_<feature>.cb`
2. 先頭にコメントでテストの目的を記述
3. このREADMEに説明を追加
