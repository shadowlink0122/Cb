# FFI サンプル

このディレクトリには、Cb言語のFFI (Foreign Function Interface) を使用した例が含まれています。

## サンプル一覧

### 1. ffi_cpp_example

基本的なC++ライブラリの統合例です。

**ファイル**:
- `ffi_cpp_example.cpp` - C++ライブラリのソースコード
- `ffi_cpp_example.cb` - Cb側の使用例

**含まれる機能**:
- 基本的な算術演算
- 数学関数（距離計算、円の面積）
- 階乗とフィボナッチ数列
- void関数の呼び出し

### 2. advanced_cpp_ffi

より高度なC++機能の統合例です。

**ファイル**:
- `advanced_cpp_ffi.cpp` - 高度なC++ライブラリのソースコード
- `advanced_cpp_ffi.cb` - Cb側の使用例

**含まれる機能**:
- STLコンテナ（vector, map）を使用したクラス
- テンプレート関数
- ラムダ式
- 例外処理
- スマートポインタ

## ビルド方法

### Makefileを使用（推奨）

```bash
# FFIライブラリをビルド
make ffi-libs

# FFIサンプルをテスト
make test-ffi

# FFIライブラリを削除
make clean-ffi

# 全てをクリーンアップ（FFIライブラリも含む）
make clean
```

### 手動ビルド

#### macOS

```bash
# ffi_cpp_example
clang++ -std=c++17 -shared -fPIC ffi_cpp_example.cpp -o ../../stdlib/foreign/libcppexample.dylib

# advanced_cpp_ffi
clang++ -std=c++17 -shared -fPIC advanced_cpp_ffi.cpp -o ../../stdlib/foreign/libadvanced.dylib
```

#### Linux

```bash
# ffi_cpp_example
g++ -std=c++17 -shared -fPIC ffi_cpp_example.cpp -o ../../stdlib/foreign/libcppexample.so

# advanced_cpp_ffi
g++ -std=c++17 -shared -fPIC advanced_cpp_ffi.cpp -o ../../stdlib/foreign/libadvanced.so
```

## 実行方法

```bash
# プロジェクトルートから実行
cd ../..

# ffi_cpp_exampleを実行
./main sample/ffi/ffi_cpp_example.cb

# advanced_cpp_ffiを実行
./main sample/ffi/advanced_cpp_ffi.cb
```

## 出力例

### ffi_cpp_example

```
=== FFI C++ Example ===

Basic Arithmetic:
  10 + 20 = 30
  7 * 8 = 56

Math Operations:
  Distance from (0,0) to (3,4) = 5.0
  Area of circle with radius 5 = 78.53981633975

Factorial:
   1 ! = 1
   2 ! = 2
   ...
   10 ! = 3628800

Fibonacci:
  fib(0) = 0
  fib(1) = 1
  ...
  fib(10) = 55

Void Functions:
Hello from C++!
C++ received number: 42

=== All tests completed! ===
```

### advanced_cpp_ffi

```
=== Advanced C++ FFI Example ===

Test 1: Data Analyzer
  Mean: 20.0
  Min: 10.0
  Max: 30.0
  Count: 5.0
  StdDev: 7.07...

Test 2: Template Functions
  max_int(42, 17) = 42
  max_double(3.14, 2.71) = 3.14

Test 3: Lambda Operations
  5.0 + 3.0 = 8.0
  5.0 * 3.0 = 15.0

Test 4: Error Handling
  100 / 5 = 20
  100 / 0 = -1 (error code)

=== All advanced tests completed! ===
```

## 技術的な詳細

### C++ → Cb の対応

| C++機能 | 使用可能 | 備考 |
|---------|---------|------|
| クラス | ✅ | `extern "C"` 関数内で使用 |
| STLコンテナ | ✅ | 内部実装として使用可能 |
| テンプレート | ✅ | `extern "C"` の外で定義 |
| ラムダ式 | ✅ | 関数内で自由に使用可能 |
| 例外処理 | ✅ | C境界を越えないこと |
| スマートポインタ | ✅ | 内部実装として使用可能 |

### 重要な注意事項

1. **extern "C" が必須**: FFIから呼び出される関数は必ず `extern "C"` でラップする
2. **C ABIを使用**: 引数と戻り値はC互換の型を使用
3. **例外の扱い**: C++例外は `extern "C"` 関数内で catch する
4. **メモリ管理**: C側で確保したメモリの管理に注意

## さらに学ぶ

- [FFI完全ガイド](../../docs/FFI_GUIDE.md)
- [C++統合ガイド](../../docs/FFI_CPP_INTEGRATION.md)
- [v0.13.0リリースノート](../../release_notes/v0.13.0.md)

## トラブルシューティング

### ライブラリが見つからない

```bash
# ライブラリが正しく配置されているか確認
ls -la ../../stdlib/foreign/

# 必要に応じて再ビルド
make clean-ffi && make ffi-libs
```

### アーキテクチャの不一致（macOS）

```bash
# 現在のアーキテクチャを確認
uname -m

# arm64の場合、clang++を使用
clang++ -std=c++17 -shared -fPIC *.cpp -o *.dylib

# x86_64の場合も同様
```

### コンパイルエラー

- C++17対応のコンパイラを使用しているか確認
- `extern "C"` ブロックが正しく閉じられているか確認
- ヘッダーファイルが正しくインクルードされているか確認

---

**Cb言語 v0.13.0 - FFI サンプル集**

🔗 外部ライブラリとの統合
🚀 実用的な例
📚 詳細なドキュメント
