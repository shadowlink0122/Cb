# FFI (Foreign Function Interface) Test Cases

このディレクトリには、Cb言語のFFI機能のテストケースが含まれています。

## 概要

FFI (Foreign Function Interface)は、Cb言語から外部の共有ライブラリ（C, C++, Rust, Zigなど）の関数を呼び出すための機能です。

## 基本構文

```cb
use foreign.module_name {
    return_type function_name(param_type param_name, ...);
}

void main() {
    // 呼び出し
    result_type result = module_name.function_name(args);
}
```

## テストファイル

### 1. test_ffi_parse.cb
**目的**: FFI構文のパース確認

**テスト内容**:
- `use foreign.m` 構文のパース
- 複数の関数宣言
- 異なる型のパラメータと戻り値

**実行方法**:
```bash
./main tests/cases/ffi/test_ffi_parse.cb
```

**期待される出力**:
```
FFI parser test - declarations parsed successfully
```

### 2. math_functions.cb
**目的**: 数学ライブラリ関数の呼び出し

**テスト内容**:
- `sqrt()` - 平方根計算
- `pow()` - べき乗計算
- `sin()` - サイン計算
- double型の戻り値

**実行方法**:
```bash
./main tests/cases/ffi/math_functions.cb
```

**期待される出力**:
```
=== Math Functions FFI Test ===
sqrt(16.0) = 4
pow(2.0, 3.0) = 8
sin(0.0) = 0
=== All Tests Passed ===
```

### 3. double_return.cb
**目的**: double型戻り値の正確な伝播テスト

**テスト内容**:
- double型の戻り値が正しく返される
- 精度が保たれる
- 型変換が正しく行われる

**実行方法**:
```bash
./main tests/cases/ffi/double_return.cb
```

**期待される出力**:
```
=== Double Return Value Test ===
sqrt(2.0) = 1.41421 (approximately)
Test: Double return value - PASSED
=== All Tests Passed ===
```

### 4. int_functions.cb
**目的**: int型関数の呼び出し

**テスト内容**:
- int型のパラメータ
- int型の戻り値
- C言語のabs関数など

**実行方法**:
```bash
./main tests/cases/ffi/int_functions.cb
```

### 5. void_functions.cb
**目的**: void型関数の呼び出し

**テスト内容**:
- 戻り値なし関数の呼び出し
- 副作用のある関数

**実行方法**:
```bash
./main tests/cases/ffi/void_functions.cb
```

### 6. module_namespace.cb
**目的**: モジュール名前空間のテスト

**テスト内容**:
- `module.function()` 形式の呼び出し
- 名前空間の分離

**実行方法**:
```bash
./main tests/cases/ffi/module_namespace.cb
```

## 全テスト実行

```bash
for file in tests/cases/ffi/*.cb; do
    echo "Running $file..."
    ./main "$file"
    echo "---"
done
```

## サポートされているライブラリ

### macOS
- `libm.dylib` - 数学関数（sqrt, sin, cos, pow など）
- `libc.dylib` - 標準C関数（printf, malloc, free など）

### Linux
- `libm.so` - 数学関数
- `libc.so` - 標準C関数

## 制限事項

現在のバージョン（v0.13.0）では以下の制限があります：

- ❌ 構造体の受け渡し（v0.13.1で実装予定）
- ❌ ポインタ型の完全サポート（v0.13.1で実装予定）
- ❌ 可変長引数（v0.13.1で実装予定）
- ❌ コールバック関数（v0.13.1で実装予定）

## サポートされている型

### 基本型
- ✅ `int` - 整数型
- ✅ `double` - 倍精度浮動小数点型
- ✅ `float` - 単精度浮動小数点型
- ✅ `void` - 戻り値なし

### 今後サポート予定
- `char*` - 文字列
- `void*` - 汎用ポインタ
- `struct` - 構造体
- `...` - 可変長引数

## トラブルシューティング

### ライブラリが見つからない

**エラー例**:
```
Error: Failed to load library for module 'mylib': dlopen failed
```

**解決策**:
1. ライブラリが正しいパスにあるか確認
2. `DYLD_LIBRARY_PATH` (macOS) または `LD_LIBRARY_PATH` (Linux) を設定
3. システムライブラリの場合は標準パスに配置

### 関数が見つからない

**エラー例**:
```
Error: Failed to register function 'myfunc': dlsym failed
```

**解決策**:
1. 関数名が正しいか確認
2. C++関数の場合は `extern "C"` が必要
3. 関数がエクスポートされているか確認（`nm` コマンドで確認）

## 関連ドキュメント

- [FFI設計](../../../docs/todo/v0.13.0/modern_ffi_macro_design.md)
- [FFI実装進捗](../../../docs/todo/v0.13.0/ffi_implementation_progress.md)
- [Integration Test Guide](../../integration/framework/integration_test_framework.hpp)

## 実装ステータス

- [x] FFI構文のパース
- [x] ライブラリのロード
- [x] 基本型（int, double, void）のサポート
- [x] 関数の呼び出し
- [ ] 構造体のサポート（Phase 3）
- [ ] ポインタ型のサポート（Phase 3）
- [ ] 可変長引数のサポート（Phase 3）
- [ ] コールバック関数のサポート（Phase 3）

**最終更新**: 2025-11-14  
**バージョン**: v0.13.0
