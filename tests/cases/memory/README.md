# Memory Management Tests

このディレクトリには、Cb言語のメモリ管理機能に関するテストケースが含まれています。

## テスト対象機能

### v0.11.0 Phase 1a: メモリ管理演算子

1. **new演算子**
   - 単一オブジェクトの割り当て: `new int`
   - 配列の割り当て: `new int[10]`
   - 構造体の割り当て: `new Point`
   - ジェネリクス構造体の割り当て: `new Box<int>`

2. **delete演算子**
   - 単一オブジェクトの解放: `delete ptr`
   - 配列の解放: `delete arr` (delete[]構文は廃止)
   - nullptr削除の安全性

3. **sizeof演算子**
   - プリミティブ型: `sizeof(int)`, `sizeof(long)`, etc.
   - 構造体: `sizeof(Point)`
   - ネストした構造体: `sizeof(Rectangle)`
   - typedef型: `sizeof(Integer)`
   - ジェネリクス構造体: `sizeof(Box<int>)`

## テストファイル

### test_new_delete_sizeof.cb
基本的なnew/delete/sizeof演算子のテスト
- プリミティブ型のsizeof
- 構造体のsizeof
- 単一オブジェクトのnew/delete
- 配列のnew/delete
- 構造体のnew/delete

### test_sizeof_advanced.cb
高度なsizeof機能のテスト
- typedef型のsizeof
- ネストした構造体のsizeof
- ジェネリクス構造体のsizeof (部分対応)
- malloc/sizeofの統合

### test_memory_edge_cases.cb
エッジケースとストレステスト
- ポインタ型のサイズ（全て8バイト）
- 自己参照構造体（Node with next pointer）
- 大きな配列の割り当て（int[1000]）
- 構造体配列の割り当て
- ジェネリクス構造体の割り当て
- 式に対するsizeof
- 複数の同時割り当て・解放
- ネストされた構造体配列

### test_memory_errors.cb
エラーケースとベストプラクティス
- nullポインタの削除（安全）
- ゼロサイズ配列の割り当て
- 適切なメモリ管理パターン
- 大量割り当て・解放テスト
- 異なる型の混合割り当て
- 二重削除の警告（ドキュメント目的）
- ダングリングポインタの警告（ドキュメント目的）
- メモリリークの例（ドキュメント目的）

## 型サイズ定義

Cb言語の型サイズ:
- `tiny`: 1 byte (8bit)
- `short`: 2 bytes (16bit)
- `int`: 4 bytes (32bit)
- `long`: 8 bytes (64bit)
- `float`: 4 bytes (32bit)
- `double`: 8 bytes (64bit)
- `char`: 1 byte
- `bool`: 1 byte
- `string`: 8 bytes (ポインタ)
- `void*`: 8 bytes (64bit環境)

## 実行方法

```bash
# 個別テスト実行
./main tests/cases/memory/test_new_delete_sizeof.cb
./main tests/cases/memory/test_sizeof_advanced.cb

# Integration test実行
make integration-test

# 全テスト実行
make test
```

## 注意事項

1. **delete[]構文の廃止**
   - ❌ 非推奨: `delete[] arr;`
   - ✅ 推奨: `delete arr;`
   - 理由: ポインタが配列を指すかどうかは実装の詳細

2. **ポインタ表示**
   - ❌ 非推奨: `println("ptr = {ptr}");` (10進数)
   - ✅ 推奨: `println("ptr = {hex(ptr)}");` (16進数、0xプリフィックス付き)

3. **構造体サイズ**
   - 現在はメンバーのサイズの単純な合計
   - パディングとアライメントは未実装

4. **ジェネリクス型パラメータ**
   - `sizeof(Box<T>)` の構文は解析可能
   - 型パラメータTの完全解決は将来の実装課題

## 将来の拡張

- [ ] memcpy/memset関数の実装
- [ ] メモリアライメントとパディング
- [ ] ジェネリクス型パラメータの完全対応
- [ ] カスタムアロケータのサポート
- [ ] メモリプールの実装
