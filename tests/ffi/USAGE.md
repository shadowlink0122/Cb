# FFI Test Environment - 完全ガイド

## 🎯 目的

このDockerベースのテスト環境は、Cb言語のFFI機能を複数の言語（C、C++、Rust、Go、Zig）で検証するために作成されました。

## ✨ 特徴

### 1. 完全に独立した環境
- `make test`とは完全に分離
- ホスト環境を汚さない
- Docker内で完結

### 2. 多言語対応
- ✅ C
- ✅ C++ (STL使用)
- ✅ Rust (標準ライブラリ使用)
- ✅ Go (math パッケージ使用)
- ✅ Zig (std.math 使用)

### 3. 標準ライブラリのラッパー検証
各言語の標準ライブラリをFFI経由で呼び出せることを確認：
- C: `math.h` (sin, cos, sqrt, pow, ceil, floor など)
- C++: STL (algorithm, vector など)
- Rust: std (イテレータ、数学関数)
- Go: math パッケージ
- Zig: std.math

## 📁 ファイル構成

```
tests/ffi/
├── Dockerfile                  # Ubuntu 22.04 + Rust + Go + Zig
├── Makefile                    # テスト実行コマンド
├── run_tests.sh               # テスト実行スクリプト
├── README.md                   # 詳細ドキュメント
├── QUICKSTART.md              # クイックスタート
├── USAGE.md                   # このファイル
│
├── libs/                      # 各言語のライブラリソース
│   ├── c/
│   │   ├── clib.c            # C実装
│   │   └── Makefile
│   ├── cpp/
│   │   ├── cpplib.cpp        # C++実装 (extern "C")
│   │   └── Makefile
│   ├── rust/
│   │   ├── Cargo.toml        # cdylib設定
│   │   └── src/lib.rs        # Rust実装 (#[no_mangle])
│   ├── go/
│   │   ├── golib.go          # Go実装 (//export)
│   │   └── Makefile
│   └── zig/
│       ├── ziglib.zig        # Zig実装 (export)
│       └── Makefile
│
└── tests/                     # Cbテストファイル
    ├── c/                     # 3つのテスト
    ├── cpp/                   # 2つのテスト
    ├── rust/                  # 2つのテスト
    ├── go/                    # 2つのテスト (標準ライブラリ含む)
    └── zig/                   # 2つのテスト (std.math含む)
```

## 🚀 使い方

### 初回セットアップ

```bash
cd tests/ffi
make build
```

これにより、以下がインストールされたDockerイメージが作成されます：
- Ubuntu 22.04
- GCC/Clang
- Rust (rustc + cargo)
- Go 1.21.5
- Zig 0.11.0

**所要時間**: 10-15分（初回のみ）

### すべてのテストを実行

```bash
make test
```

**実行内容**:
1. Cbインタプリタをビルド
2. 各言語のライブラリをコンパイル
3. ライブラリを`stdlib/foreign/`にコピー
4. 各言語のテストを実行

**所要時間**: 5-10分

### 言語別テスト

```bash
# 個別にテスト
make test-c      # C: 3つのテスト
make test-cpp    # C++: 2つのテスト
make test-rust   # Rust: 2つのテスト
make test-go     # Go: 2つのテスト (標準ライブラリ含む)
make test-zig    # Zig: 2つのテスト (std.math含む)
```

### デバッグ・手動テスト

```bash
# コンテナのシェルに入る
make shell
```

コンテナ内で：

```bash
# Cbをビルド
cd /cb
make

# Cライブラリをビルド
cd /cb/tests/ffi/libs/c
make
cp libclib.so /cb/stdlib/foreign/

# テストを実行
cd /cb/tests/ffi
/cb/main tests/c/basic_test.cb

# ライブラリのシンボルを確認
nm -D /cb/stdlib/foreign/libclib.so | grep add
```

### クリーンアップ

```bash
# Dockerリソースを削除
make clean
```

## 📊 テスト内容

### C ライブラリ (libclib.so)

#### basic_test.cb
- `add(10, 5)` → 15
- `subtract(10, 5)` → 5
- `multiply(10, 5)` → 50
- `divide(10, 5)` → 2

#### math_test.cb
- `factorial(5)` → 120
- `factorial(10)` → 3628800
- `is_prime(17)` → true
- `is_prime(20)` → false
- `power(2.0, 10.0)` → 1024.0
- `square_root(16.0)` → 4.0

#### stdlib_test.cb (C標準ライブラリのラッパー)
- `sine(0.0)` → 0.0 (math.h の sin)
- `cosine(0.0)` → 1.0 (math.h の cos)
- `absolute(-5.5)` → 5.5 (math.h の fabs)
- `ceiling(3.2)` → 4.0 (math.h の ceil)
- `floor_value(3.8)` → 3.0 (math.h の floor)

### C++ ライブラリ (libcpplib.so)

#### basic_test.cb
- `cpp_add(20, 30)` → 50
- `cpp_multiply(6, 7)` → 42
- `circle_area(5.0)` → 78.54 (M_PI使用)
- `triangle_area(10.0, 6.0)` → 30.0

#### std_test.cb (C++ STLのラッパー)
- `euclidean_distance((0,0), (3,4))` → 5.0 (std::sqrt使用)

### Rust ライブラリ (librustlib.so)

#### basic_test.cb
- `rust_add(15, 25)` → 40
- `rust_multiply(8, 9)` → 72
- `rust_factorial(6)` → 720 (イテレータ使用)
- `rust_fibonacci(10)` → 55
- `rust_fibonacci(20)` → 6765

#### advanced_test.cb
- `rust_is_prime(29)` → true
- `rust_gcd(48, 18)` → 6
- `rust_lcm(12, 18)` → 36
- `rust_circle_area(10.0)` → 314.16 (std::f64::consts::PI使用)
- `rust_sqrt(144.0)` → 12.0

### Go ライブラリ (libgolib.so)

#### basic_test.cb
- `go_add(100, 200)` → 300
- `go_subtract(100, 30)` → 70
- `go_multiply(12, 13)` → 156
- `go_power(2.0, 8.0)` → 256.0 (math.Pow使用)
- `go_sqrt(81.0)` → 9.0 (math.Sqrt使用)

#### concurrent_test.cb (Go標準ライブラリのラッパー)
- `go_fibonacci(15)` → 610
- `go_factorial(7)` → 5040
- `go_is_prime(97)` → true
- `go_sin(0.0)` → 0.0 (**math.Sin** をラップ)
- `go_cos(0.0)` → 1.0 (**math.Cos** をラップ)

### Zig ライブラリ (libziglib.so)

#### basic_test.cb
- `zig_add(50, 75)` → 125
- `zig_max(42, 17)` → 42
- `zig_min(42, 17)` → 17
- `zig_abs(-10)` → 10

#### math_test.cb (Zig標準ライブラリのラッパー)
- `zig_factorial(8)` → 40320
- `zig_fibonacci(12)` → 144
- `zig_is_prime(37)` → true
- `zig_sqrt(100.0)` → 10.0 (**@sqrt** 組み込み関数)
- `zig_power(3.0, 4.0)` → 81.0 (**std.math.pow** をラップ)
- `zig_circle_area(7.0)` → 153.94 (**std.math.pi** 使用)
- `zig_gcd(56, 98)` → 14

## ✅ 検証項目

このテスト環境で以下が確認できます：

1. **基本的なFFI動作**
   - 整数型 (int, long)
   - 浮動小数点型 (double)
   - ブール型 (bool)
   - 関数呼び出し

2. **各言語のビルド**
   - C: gcc/clang
   - C++: g++/clang++ (extern "C")
   - Rust: cargo build (cdylib)
   - Go: go build -buildmode=c-shared
   - Zig: zig build-lib -dynamic

3. **標準ライブラリのラッパー** ⭐️
   - C: math.h の関数群
   - Go: math パッケージ
   - Zig: std.math

4. **複雑な演算**
   - 再帰 (factorial, fibonacci)
   - ループ
   - 条件分岐
   - 浮動小数点演算

## 🔧 トラブルシューティング

### Docker が起動しない

```bash
# Docker の状態を確認
docker info

# Docker デーモンを再起動
sudo systemctl restart docker  # Linux
# または Docker Desktop を再起動 (macOS/Windows)
```

### ビルドに失敗する

```bash
# キャッシュをクリアして再ビルド
docker build --no-cache -t cb-ffi-test .
```

### テストが失敗する

```bash
# シェルに入って手動確認
make shell

# Cbインタプリタの動作確認
/cb/main --version

# ライブラリの存在確認
ls -la /cb/stdlib/foreign/

# シンボルの確認
nm -D /cb/stdlib/foreign/libclib.so
```

### ライブラリが見つからない

```bash
# コンテナ内で手動コピー
cd /cb/tests/ffi/libs/c
make
cp libclib.so /cb/stdlib/foreign/
ls -la /cb/stdlib/foreign/
```

## 📝 追加の言語をテストする場合

### 例: D言語を追加

1. **Dockerfileに追加**
```dockerfile
RUN apt-get install -y dmd
```

2. **ライブラリを作成**
```d
// libs/d/dlib.d
extern (C):
int d_add(int a, int b) { return a + b; }
```

3. **Makefileを作成**
```makefile
# libs/d/Makefile
libdlib.so: dlib.d
	dmd -shared -fPIC dlib.d -of=libdlib.so
```

4. **テストを作成**
```cb
// tests/d/basic_test.cb
use foreign.dlib {
    int d_add(int a, int b);
}

void main() {
    println("D FFI:", dlib.d_add(10, 20));
}
```

5. **run_tests.shに追加**
```bash
test_d() {
    cd /cb/tests/ffi/libs/d
    make
    cp *.so /cb/stdlib/foreign/
    cd /cb/tests/ffi
    run_test "d" "basic_test"
}
```

## 🎉 まとめ

この FFI テスト環境により：

✅ **5つの言語** でFFIが動作することを確認
✅ **標準ライブラリ** もFFI経由で利用可能
✅ **独立した環境** で安全にテスト
✅ **再現可能** なDockerベース

Cb の FFI は、C ABI を公開できる任意の言語と相互運用可能です！

## 📚 関連ドキュメント

- `README.md` - 詳細な説明
- `QUICKSTART.md` - すぐに始める方法
- `../../docs/FFI_GUIDE.md` - FFI 使用ガイド
- `../../docs/FFI_IMPLEMENTATION_GUIDE.md` - 各言語での実装ガイド
