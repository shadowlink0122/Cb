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

4. **memcpy組み込み関数**
   - メモリブロックのコピー: `memcpy(dest, src, size)`
   - 構造体のコピー（Variable*の深いコピー）
   - 生ポインタのバイト列コピー

5. **配列アクセス組み込み関数**
   - 配列要素の読み取り: `array_get_int(ptr, index)`
   - 配列要素の書き込み: `array_set_int(ptr, index, value)`
   - newで確保したポインタへの配列アクセスをサポート

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

### test_memcpy_verify.cb
memcpy基本動作の検証テスト
- 構造体の基本的なコピー
- コピー前後の値の検証
- メモリコピーの正確性確認

### test_memcpy_basic.cb
memcpy機能の包括的テスト
- 単純な構造体のコピー
- 配列アクセス関数との組み合わせ
- 複数の構造体のコピー
- コピー後のデータ独立性検証

### test_array_access.cb
配列アクセス組み込み関数のテスト
- array_get_int/array_set_intの基本動作
- newで確保した配列への読み書き
- 範囲内の正常なアクセス確認

### errors/ ディレクトリ
エラーケースの個別テスト（詳細はerrors/README.md参照）
- **double_delete.cb**: 二重削除エラー（abortが期待される）
- **use_after_delete.cb**: delete後の使用エラー（abortが期待される）
- **delete_uninitialized.cb**: 未初期化ポインタの削除（abortが期待される）
- **memory_leak_detection.cb**: メモリリーク検出（正常終了、リーク有り）
- **dangling_pointer_return.cb**: ダングリングポインタ返却（エラーが期待される）
- **invalid_pointer_arithmetic.cb**: 無効なポインタ演算（abortが期待される）

⚠️ **注意**: エラーテストは意図的にクラッシュを引き起こすため、
integration testからは除外され、手動でのみ実行されます。

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
# 個別テスト実行（正常系）
./main tests/cases/memory/test_new_delete_sizeof.cb
./main tests/cases/memory/test_sizeof_advanced.cb
./main tests/cases/memory/test_memory_edge_cases.cb

# エラーケーステスト（個別実行、クラッシュが期待される）
./main tests/cases/memory/errors/double_delete.cb          # Expected: abort
./main tests/cases/memory/errors/use_after_delete.cb        # Expected: abort
./main tests/cases/memory/errors/delete_uninitialized.cb    # Expected: abort
./main tests/cases/memory/errors/memory_leak_detection.cb   # Expected: success (with leak)
./main tests/cases/memory/errors/dangling_pointer_return.cb # Expected: error
./main tests/cases/memory/errors/invalid_pointer_arithmetic.cb # Expected: abort

# Integration test実行（エラーケースは除外）
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
