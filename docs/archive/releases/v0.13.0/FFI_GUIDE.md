# FFI (Foreign Function Interface) ガイド

## 概要

CbのFFI (Foreign Function Interface)を使用すると、C/C++/Rust/Zig/Go などの言語で書かれた既存のライブラリを呼び出すことができます。

## 対応言語

FFIは **C ABI (Application Binary Interface)** に準拠している任意の言語に対応しています：

| 言語 | 対応 | 必要な設定 |
|------|------|-----------|
| **C** | ✅ | そのまま使える |
| **C++** | ✅ | `extern "C"` が必要 |
| **Rust** | ✅ | `#[no_mangle]` + `extern "C"` |
| **Zig** | ✅ | `export fn` で自動対応 |
| **Go** | ✅ | `//export` コメント |
| **Assembly** | ✅ | C ABIに準拠 |

## 基本的な使い方

### 1. C ライブラリの呼び出し

```cb
// 標準Cライブラリ（libm）の使用
use foreign.m {
    double sqrt(double x);
    double pow(double base, double exp);
    double sin(double x);
    double cos(double x);
}

void main() {
    double result = m.sqrt(25.0);
    println("sqrt(25) =", result);  // 5.0
    
    double power = m.pow(2.0, 10.0);
    println("2^10 =", power);  // 1024.0
}
```

### 2. C++ ライブラリの呼び出し

**C++側** (mylib.cpp):
```cpp
#include <cmath>

// extern "C" でC ABIを使用
extern "C" {
    int add(int a, int b) {
        return a + b;
    }
    
    double distance(double x1, double y1, double x2, double y2) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
}
```

**コンパイル**:
```bash
# macOS
clang++ -shared -fPIC mylib.cpp -o libmylib.dylib

# Linux
g++ -shared -fPIC mylib.cpp -o libmylib.so
```

**Cb側**:
```cb
use foreign.mylib {
    int add(int a, int b);
    double distance(double x1, double y1, double x2, double y2);
}

void main() {
    int sum = mylib.add(10, 20);
    println("10 + 20 =", sum);
    
    double dist = mylib.distance(0.0, 0.0, 3.0, 4.0);
    println("Distance =", dist);  // 5.0
}
```

### 3. Rust ライブラリの呼び出し

**Rust側** (lib.rs):
```rust
#[no_mangle]
pub extern "C" fn factorial(n: i32) -> i64 {
    if n <= 1 {
        1
    } else {
        (1..=n as i64).product()
    }
}

#[no_mangle]
pub extern "C" fn is_prime(n: i32) -> bool {
    if n < 2 {
        return false;
    }
    for i in 2..((n as f64).sqrt() as i32 + 1) {
        if n % i == 0 {
            return false;
        }
    }
    true
}
```

**コンパイル**:
```bash
# Cargo.toml
[lib]
crate-type = ["cdylib"]

# ビルド
cargo build --release

# ライブラリをコピー
cp target/release/libmylib.dylib stdlib/foreign/
```

**Cb側**:
```cb
use foreign.mylib {
    long factorial(int n);
    bool is_prime(int n);
}

void main() {
    long fact = mylib.factorial(10);
    println("10! =", fact);  // 3628800
    
    if (mylib.is_prime(17)) {
        println("17 is prime");
    }
}
```

## 対応している型

### プリミティブ型

| Cb型 | C/C++型 | 説明 |
|------|---------|------|
| `int` | `int` | 32ビット整数 |
| `long` | `long` | 64ビット整数 |
| `float` | `float` | 32ビット浮動小数点 |
| `double` | `double` | 64ビット浮動小数点 |
| `bool` | `bool` | 真偽値 |
| `void` | `void` | 戻り値なし |

### 対応している関数シグネチャ

現在対応しているシグネチャ：

**整数型**:
- `int func()`
- `int func(int)`
- `int func(int, int)`

**実数型**:
- `double func(double)`
- `double func(double, double)`
- `double func(double, double, double, double)`

**長整数型**:
- `long func(int)`

**void型**:
- `void func()`
- `void func(int)`

## ライブラリの配置

FFIマネージャーは以下のパスを検索します（優先度順）：

1. `./stdlib/foreign/`
2. `.` (カレントディレクトリ)
3. `/usr/local/lib/`
4. `/usr/lib/`
5. `/opt/homebrew/lib/` (macOS)

### 推奨される配置方法

```bash
# ライブラリをstdlib/foreignに配置
cp libmylib.dylib stdlib/foreign/

# または、カレントディレクトリに配置
cp libmylib.dylib .
```

## ファイル形式

| プラットフォーム | 拡張子 | 例 |
|-----------------|--------|-----|
| macOS | `.dylib` | `libmath.dylib` |
| Linux | `.so` | `libmath.so` |
| Windows | `.dll` | `math.dll` |

**注意**: ライブラリ名は `lib` プレフィックスが必要です（例: `foreign.math` → `libmath.dylib`）

## 実践的な例

### 例1: 複雑な計算をC++で高速化

**C++側** (fast_math.cpp):
```cpp
extern "C" {
    // ベクトルの内積
    double dot_product(double* a, double* b, int size) {
        double sum = 0.0;
        for (int i = 0; i < size; i++) {
            sum += a[i] * b[i];
        }
        return sum;
    }
    
    // 行列の転置（簡略版）
    void matrix_transpose(double* matrix, double* result, int rows, int cols) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                result[j * rows + i] = matrix[i * cols + j];
            }
        }
    }
}
```

### 例2: Rustで並列処理

**Rust側**:
```rust
use rayon::prelude::*;

#[no_mangle]
pub extern "C" fn parallel_sum(arr: *const i32, len: usize) -> i64 {
    let slice = unsafe { std::slice::from_raw_parts(arr, len) };
    slice.par_iter().map(|&x| x as i64).sum()
}
```

## 各言語での実装手順

### Rustでdylibを作成する

#### 1. プロジェクトの作成

```bash
cargo new --lib mylib
cd mylib
```

#### 2. Cargo.tomlの設定

```toml
[package]
name = "mylib"
version = "0.1.0"
edition = "2021"

[lib]
crate-type = ["cdylib"]  # C互換ダイナミックライブラリ

[dependencies]
```

#### 3. ライブラリの実装 (src/lib.rs)

```rust
// 基本的な算術関数
#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}

#[no_mangle]
pub extern "C" fn multiply(a: i32, b: i32) -> i32 {
    a * b
}

// 階乗計算
#[no_mangle]
pub extern "C" fn factorial(n: i32) -> i64 {
    if n <= 1 {
        1
    } else {
        (1..=n as i64).product()
    }
}

// 素数判定
#[no_mangle]
pub extern "C" fn is_prime(n: i32) -> bool {
    if n < 2 {
        return false;
    }
    for i in 2..((n as f64).sqrt() as i32 + 1) {
        if n % i == 0 {
            return false;
        }
    }
    true
}

// 浮動小数点演算
#[no_mangle]
pub extern "C" fn calculate_circle_area(radius: f64) -> f64 {
    std::f64::consts::PI * radius * radius
}

// 文字列の長さ（C文字列）
#[no_mangle]
pub extern "C" fn string_length(s: *const std::os::raw::c_char) -> i32 {
    if s.is_null() {
        return 0;
    }
    unsafe {
        let c_str = std::ffi::CStr::from_ptr(s);
        c_str.to_bytes().len() as i32
    }
}
```

#### 4. ビルド

```bash
# リリースビルド（最適化あり）
cargo build --release

# デバッグビルド
cargo build
```

#### 5. ライブラリのコピー

```bash
# macOS
cp target/release/libmylib.dylib ../Cb/stdlib/foreign/

# Linux
cp target/release/libmylib.so ../Cb/stdlib/foreign/

# Windows
cp target/release/mylib.dll ../Cb/stdlib/foreign/
```

#### 6. Cbから使用

```cb
use foreign.mylib {
    int add(int a, int b);
    int multiply(int a, int b);
    long factorial(int n);
    bool is_prime(int n);
    double calculate_circle_area(double radius);
}

void main() {
    println("5 + 3 =", mylib.add(5, 3));
    println("4 * 7 =", mylib.multiply(4, 7));
    println("10! =", mylib.factorial(10));
    println("17 is prime:", mylib.is_prime(17));
    println("Circle area (r=5):", mylib.calculate_circle_area(5.0));
}
```

---

### Goでdylibを作成する

#### 1. プロジェクトの作成

```bash
mkdir mygolib
cd mygolib
go mod init mygolib
```

#### 2. ライブラリの実装 (main.go)

```go
package main

import "C"
import (
    "math"
)

//export add
func add(a, b C.int) C.int {
    return a + b
}

//export subtract
func subtract(a, b C.int) C.int {
    return a - b
}

//export multiply
func multiply(a, b C.int) C.int {
    return a * b
}

//export divide
func divide(a, b C.int) C.int {
    if b == 0 {
        return 0
    }
    return a / b
}

//export power
func power(base C.double, exp C.double) C.double {
    return C.double(math.Pow(float64(base), float64(exp)))
}

//export sqrt
func sqrt(x C.double) C.double {
    return C.double(math.Sqrt(float64(x)))
}

//export fibonacci
func fibonacci(n C.int) C.long {
    if n <= 1 {
        return C.long(n)
    }
    a, b := 0, 1
    for i := 2; i <= int(n); i++ {
        a, b = b, a+b
    }
    return C.long(b)
}

// main関数は必須（ビルドには使用されない）
func main() {}
```

#### 3. ビルド

```bash
# macOS
go build -buildmode=c-shared -o libmygolib.dylib main.go

# Linux
go build -buildmode=c-shared -o libmygolib.so main.go

# Windows
go build -buildmode=c-shared -o mygolib.dll main.go
```

#### 4. ライブラリのコピー

```bash
# macOS/Linux
cp libmygolib.dylib ../Cb/stdlib/foreign/
# または
cp libmygolib.so ../Cb/stdlib/foreign/
```

#### 5. Cbから使用

```cb
use foreign.mygolib {
    int add(int a, int b);
    int subtract(int a, int b);
    int multiply(int a, int b);
    int divide(int a, int b);
    double power(double base, double exp);
    double sqrt(double x);
    long fibonacci(int n);
}

void main() {
    println("10 + 5 =", mygolib.add(10, 5));
    println("10 - 5 =", mygolib.subtract(10, 5));
    println("2^10 =", mygolib.power(2.0, 10.0));
    println("sqrt(144) =", mygolib.sqrt(144.0));
    println("fibonacci(20) =", mygolib.fibonacci(20));
}
```

---

### Zigでdylibを作成する

#### 1. プロジェクトの作成

```bash
mkdir myziglib
cd myziglib
```

#### 2. ライブラリの実装 (lib.zig)

```zig
const std = @import("std");

// export で自動的に extern "C" 相当になる
export fn add(a: i32, b: i32) i32 {
    return a + b;
}

export fn subtract(a: i32, b: i32) i32 {
    return a - b;
}

export fn multiply(a: i32, b: i32) i32 {
    return a * b;
}

export fn divide(a: i32, b: i32) i32 {
    if (b == 0) return 0;
    return @divTrunc(a, b);
}

export fn factorial(n: i32) i64 {
    if (n <= 1) return 1;
    var result: i64 = 1;
    var i: i32 = 2;
    while (i <= n) : (i += 1) {
        result *= i;
    }
    return result;
}

export fn is_even(n: i32) bool {
    return @mod(n, 2) == 0;
}

export fn abs_value(n: i32) i32 {
    return if (n < 0) -n else n;
}

export fn max(a: i32, b: i32) i32 {
    return if (a > b) a else b;
}

export fn min(a: i32, b: i32) i32 {
    return if (a < b) a else b;
}

// 浮動小数点演算
export fn square_root(x: f64) f64 {
    return @sqrt(x);
}

export fn power(base: f64, exp: f64) f64 {
    return std.math.pow(f64, base, exp);
}
```

#### 3. ビルド

```bash
# macOS
zig build-lib lib.zig -dynamic -O ReleaseFast

# ライブラリ名を変更
mv liblib.dylib libmyziglib.dylib

# Linux
zig build-lib lib.zig -dynamic -O ReleaseFast
mv liblib.so libmyziglib.so
```

#### 4. ライブラリのコピー

```bash
cp libmyziglib.dylib ../Cb/stdlib/foreign/
```

#### 5. Cbから使用

```cb
use foreign.myziglib {
    int add(int a, int b);
    int subtract(int a, int b);
    int multiply(int a, int b);
    int divide(int a, int b);
    long factorial(int n);
    bool is_even(int n);
    int abs_value(int n);
    int max(int a, int b);
    int min(int a, int b);
    double square_root(double x);
    double power(double base, double exp);
}

void main() {
    println("15 + 8 =", myziglib.add(15, 8));
    println("max(10, 20) =", myziglib.max(10, 20));
    println("8! =", myziglib.factorial(8));
    println("is_even(7) =", myziglib.is_even(7));
    println("sqrt(81) =", myziglib.square_root(81.0));
}
```

---

### D言語でdylibを作成する

#### 1. ライブラリの実装 (lib.d)

```d
extern (C):

int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

long factorial(int n) {
    if (n <= 1) return 1;
    long result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

double power(double base, double exp) {
    import std.math : pow;
    return pow(base, exp);
}
```

#### 2. ビルド

```bash
# macOS
dmd -shared -fPIC lib.d -of=libmydlib.dylib

# Linux
dmd -shared -fPIC lib.d -of=libmydlib.so
```

---

### Nimでdylibを作成する

#### 1. ライブラリの実装 (lib.nim)

```nim
# extern "C" 相当のエクスポート
proc add(a: cint, b: cint): cint {.exportc, dynlib.} =
  return a + b

proc multiply(a: cint, b: cint): cint {.exportc, dynlib.} =
  return a * b

proc factorial(n: cint): clonglong {.exportc, dynlib.} =
  if n <= 1:
    return 1
  var result: clonglong = 1
  for i in 2..n:
    result *= i
  return result

proc is_prime(n: cint): bool {.exportc, dynlib.} =
  if n < 2:
    return false
  for i in 2..<n:
    if n mod i == 0:
      return false
  return true
```

#### 2. ビルド

```bash
# macOS
nim c --app:lib --opt:speed lib.nim
mv liblib.dylib libmynimlib.dylib

# Linux
nim c --app:lib --opt:speed lib.nim
mv liblib.so libmynimlib.so
```

---

### Fortranでdylibを作成する

#### 1. ライブラリの実装 (lib.f90)

```fortran
! ISO_C_BINDING で C互換にする
module mathlib
    use iso_c_binding
    implicit none
contains
    ! 2つの整数の加算
    function add(a, b) bind(C, name="add") result(res)
        integer(c_int), value :: a, b
        integer(c_int) :: res
        res = a + b
    end function add

    ! 配列の合計
    function sum_array(arr, n) bind(C, name="sum_array") result(res)
        integer(c_int), value :: n
        real(c_double) :: arr(n)
        real(c_double) :: res
        integer :: i
        res = 0.0d0
        do i = 1, n
            res = res + arr(i)
        end do
    end function sum_array
end module mathlib
```

#### 2. ビルド

```bash
# macOS
gfortran -shared -fPIC lib.f90 -o libfortranlib.dylib

# Linux
gfortran -shared -fPIC lib.f90 -o libfortranlib.so
```

---

## 言語別の比較

| 言語 | 難易度 | パフォーマンス | ビルド速度 | おすすめ用途 |
|------|--------|---------------|-----------|-------------|
| **C** | ⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 低レベル処理 |
| **C++** | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 複雑なアルゴリズム |
| **Rust** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | 安全性重視 |
| **Go** | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 並行処理 |
| **Zig** | ⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 低レベル+安全性 |
| **D** | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 高生産性 |
| **Nim** | ⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐ | Python風構文 |
| **Fortran** | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 数値計算 |

## トラブルシューティング

### エラー: "Failed to load library"

**原因**: ライブラリが見つからない、またはアーキテクチャが一致しない

**解決方法**:
```bash
# ライブラリの存在を確認
ls -la stdlib/foreign/libmylib.dylib

# アーキテクチャを確認（macOS）
file stdlib/foreign/libmylib.dylib
# 出力例: Mach-O 64-bit dynamically linked shared library arm64

# 正しいアーキテクチャでコンパイル
clang++ -shared -fPIC mylib.cpp -o libmylib.dylib
```

### エラー: "Function not registered"

**原因**: 関数がライブラリ内に見つからない

**解決方法**:
```bash
# シンボルを確認（macOS/Linux）
nm -gU stdlib/foreign/libmylib.dylib | grep my_function

# C++の場合、extern "C" が必要
```

### エラー: "Unsupported function signature"

**原因**: 関数のシグネチャが対応していない

**解決方法**: 対応しているシグネチャを使用するか、ラッパー関数を作成

## 制限事項

現在の制限：

1. **ポインタのサポートが限定的**
   - 基本的なポインタ操作は今後のバージョンで拡張予定

2. **構造体の受け渡し**
   - 現在は対応していない
   - ポインタ経由での受け渡しを検討中

3. **可変長引数**
   - `printf` のような可変長引数関数は未対応

4. **コールバック関数**
   - C側からCbの関数を呼び出すことは現在未対応

## ベストプラクティス

1. **型の明示**: FFI宣言では型を明確に指定する

2. **エラーハンドリング**: 外部関数の呼び出しは失敗する可能性があることを考慮

3. **パフォーマンス**: FFI呼び出しにはオーバーヘッドがあるため、頻繁に呼ばれる関数は注意

4. **メモリ管理**: C側で確保したメモリは適切に解放する

## まとめ

CbのFFIを使用すると：

✅ 既存のC/C++/Rustライブラリを活用できる
✅ パフォーマンスクリティカルな部分をネイティブコードで実装できる
✅ 豊富なエコシステムを利用できる

FFIは強力な機能ですが、適切に使用することが重要です。
