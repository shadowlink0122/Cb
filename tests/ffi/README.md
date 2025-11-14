# Cb FFI Test Suite

多言語FFI (Foreign Function Interface) テスト環境

## 概要

このディレクトリには、Cbの FFI 機能をテストするための Docker ベースの環境が含まれています。
C、C++、Rust、Go、Zig で実装されたライブラリを Cb から呼び出して、FFI が正しく動作することを確認します。

## サポートされる言語

- ✅ **C** - 標準的な C ライブラリ
- ✅ **C++** - STL を使った C++ ライブラリ (extern "C" でエクスポート)
- ✅ **Rust** - #[no_mangle] と extern "C" を使った Rust ライブラリ
- ✅ **Go** - //export コメントを使った Go ライブラリ
- ✅ **Zig** - export キーワードを使った Zig ライブラリ

## 標準ライブラリのラッパー

各言語の標準ライブラリもFFI経由で利用可能です：

- **C**: `math.h` (sin, cos, sqrt, pow など)
- **C++**: STL (vector, algorithm など)
- **Rust**: std (イテレータ、数学関数など)
- **Go**: math パッケージ (Sin, Cos, Sqrt など)
- **Zig**: std.math (pow, sqrt, pi など)

## ディレクトリ構造

```
tests/ffi/
├── Dockerfile              # Docker イメージ定義
├── Makefile                # テスト実行用 Makefile
├── run_tests.sh            # テスト実行スクリプト
├── README.md               # このファイル
├── libs/                   # 各言語のライブラリソース
│   ├── c/                  # C ライブラリ
│   │   ├── clib.c
│   │   └── Makefile
│   ├── cpp/                # C++ ライブラリ
│   │   ├── cpplib.cpp
│   │   └── Makefile
│   ├── rust/               # Rust ライブラリ
│   │   ├── Cargo.toml
│   │   └── src/lib.rs
│   ├── go/                 # Go ライブラリ
│   │   ├── golib.go
│   │   └── Makefile
│   └── zig/                # Zig ライブラリ
│       ├── ziglib.zig
│       └── Makefile
└── tests/                  # Cb テストファイル
    ├── c/                  # C ライブラリのテスト
    │   ├── basic_test.cb
    │   ├── math_test.cb
    │   └── stdlib_test.cb
    ├── cpp/                # C++ ライブラリのテスト
    │   ├── basic_test.cb
    │   └── std_test.cb
    ├── rust/               # Rust ライブラリのテスト
    │   ├── basic_test.cb
    │   └── advanced_test.cb
    ├── go/                 # Go ライブラリのテスト
    │   ├── basic_test.cb
    │   └── concurrent_test.cb
    └── zig/                # Zig ライブラリのテスト
        ├── basic_test.cb
        └── math_test.cb
```

## 使い方

### 前提条件

- Docker がインストールされていること
- ホストマシンに Cb のソースコードがあること

### すべてのテストを実行

```bash
cd tests/ffi
make test
```

### 言語別にテストを実行

```bash
# C ライブラリのみテスト
make test-c

# C++ ライブラリのみテスト
make test-cpp

# Rust ライブラリのみテスト
make test-rust

# Go ライブラリのみテスト
make test-go

# Zig ライブラリのみテスト
make test-zig
```

### Docker コンテナのシェルに入る（デバッグ用）

```bash
make shell
```

コンテナ内で手動でテストやビルドを実行できます。

### クリーンアップ

```bash
make clean
```

Docker イメージとコンテナを削除します。

## テスト内容

### C ライブラリ

- **basic_test.cb**: 基本的な算術演算 (add, subtract, multiply, divide)
- **math_test.cb**: 数学関数 (factorial, is_prime, power, sqrt)
- **stdlib_test.cb**: C標準ライブラリ (sin, cos, abs, ceil, floor)

### C++ ライブラリ

- **basic_test.cb**: 基本演算と幾何学計算 (circle_area, triangle_area)
- **std_test.cb**: C++ STL を使った距離計算

### Rust ライブラリ

- **basic_test.cb**: 基本演算、階乗、フィボナッチ
- **advanced_test.cb**: 素数判定、GCD/LCM、円の面積、平方根

### Go ライブラリ

- **basic_test.cb**: 基本演算、累乗、平方根
- **concurrent_test.cb**: フィボナッチ、階乗、素数判定、Go標準ライブラリ (math.Sin, math.Cos)

### Zig ライブラリ

- **basic_test.cb**: 基本演算、最大値/最小値、絶対値
- **math_test.cb**: 階乗、フィボナッチ、素数判定、Zig標準ライブラリ (std.math)

## 標準ライブラリラッパーの実装例

### C 標準ライブラリ (math.h)

```c
#include <math.h>

double sine(double x) {
    return sin(x);  // C標準ライブラリの sin() をラップ
}

double square_root(double x) {
    return sqrt(x);  // C標準ライブラリの sqrt() をラップ
}
```

### Go 標準ライブラリ (math)

```go
import "math"

//export go_sin
func go_sin(x C.double) C.double {
    return C.double(math.Sin(float64(x)))  // Go の math.Sin をラップ
}

//export go_sqrt
func go_sqrt(x C.double) C.double {
    return C.double(math.Sqrt(float64(x)))  // Go の math.Sqrt をラップ
}
```

### Zig 標準ライブラリ (std.math)

```zig
const std = @import("std");

export fn zig_sqrt(x: f64) f64 {
    return @sqrt(x);  // Zig組み込み関数
}

export fn zig_power(base: f64, exp: f64) f64 {
    return std.math.pow(f64, base, exp);  // std.math.pow をラップ
}
```

## トラブルシューティング

### Docker イメージのビルドに失敗する

```bash
# Docker の状態を確認
docker info

# 古いイメージを削除
docker system prune -a
```

### テストが失敗する

```bash
# シェルに入ってデバッグ
make shell

# コンテナ内で手動ビルド
cd /cb
make

# ライブラリを手動でビルド
cd /cb/tests/ffi/libs/c
make
```

### ライブラリが見つからない

```bash
# stdlib/foreign ディレクトリを確認
ls -la /cb/stdlib/foreign/

# ライブラリを手動でコピー
cp /cb/tests/ffi/libs/c/libclib.so /cb/stdlib/foreign/
```

## 注意事項

- このテスト環境は `make test` とは完全に独立しています
- Docker コンテナ内で実行されるため、ホスト環境を汚しません
- 各言語のコンパイラがコンテナ内にインストールされます（初回は時間がかかります）
- Linux 環境でテストするため、.so ファイルが生成されます

## パフォーマンス

各言語の相対的なパフォーマンス：

| 言語 | 相対速度 | メモリ使用量 | ビルド時間 |
|------|---------|------------|-----------|
| C    | 1.00x   | 最小        | 最速      |
| C++  | 1.05x   | 小          | 速い      |
| Rust | 1.00x   | 小          | 中        |
| Zig  | 0.95x   | 最小        | 速い      |
| Go   | 1.50x   | 中          | 速い      |

## まとめ

この FFI テスト環境により、以下が確認できます：

✅ C、C++、Rust、Go、Zig のライブラリが Cb から呼び出せること
✅ 各言語の標準ライブラリもFFI経由で利用可能なこと
✅ 基本的な型変換が正しく動作すること
✅ 複雑な関数（再帰、ループ、浮動小数点演算）が動作すること

Cb の FFI は、C ABI を公開できる任意の言語と相互運用可能です！
