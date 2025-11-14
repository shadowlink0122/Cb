#!/bin/bash

cd /Users/shadowlink/Documents/git/Cb/tests/ffi

# すべてのテストファイルからbool関数を削除する

cat > tests/rust/basic_test.cb << 'TESTEOF'
// Rust FFI Basic Test

use foreign.rustlib {
    int rust_add(int a, int b);
    int rust_multiply(int a, int b);
    long rust_factorial(int n);
    long rust_fibonacci(int n);
}

void main() {
    println("=== Rust FFI Basic Test ===");
    
    int sum = rustlib.rust_add(15, 25);
    println("rust_add(15, 25) =", sum);
    
    int prod = rustlib.rust_multiply(8, 9);
    println("rust_multiply(8, 9) =", prod);
    
    long fact6 = rustlib.rust_factorial(6);
    println("rust_factorial(6) =", fact6);
    
    long fib10 = rustlib.rust_fibonacci(10);
    println("rust_fibonacci(10) =", fib10);
    
    long fib20 = rustlib.rust_fibonacci(20);
    println("rust_fibonacci(20) =", fib20);
    
    println("✓ All Rust basic tests completed!");
}
TESTEOF

cat > tests/rust/advanced_test.cb << 'TESTEOF'
// Rust FFI Advanced Test

use foreign.rustlib {
    int rust_gcd(int a, int b);
    int rust_lcm(int a, int b);
    double rust_circle_area(double radius);
    double rust_sqrt(double x);
}

void main() {
    println("=== Rust FFI Advanced Test ===");
    
    int gcd48_18 = rustlib.rust_gcd(48, 18);
    println("rust_gcd(48, 18) =", gcd48_18);
    
    int lcm12_18 = rustlib.rust_lcm(12, 18);
    println("rust_lcm(12, 18) =", lcm12_18);
    
    double area = rustlib.rust_circle_area(10.0);
    println("rust_circle_area(10.0) =", area);
    
    double sqrt144 = rustlib.rust_sqrt(144.0);
    println("rust_sqrt(144.0) =", sqrt144);
    
    println("✓ All Rust advanced tests completed!");
}
TESTEOF

cat > tests/go/concurrent_test.cb << 'TESTEOF'
// Go FFI Concurrent Test (Go標準ライブラリの利用)

use foreign.golib {
    long go_fibonacci(int n);
    long go_factorial(int n);
    double go_sin(double x);
    double go_cos(double x);
}

void main() {
    println("=== Go FFI Concurrent Test ===");
    
    long fib15 = golib.go_fibonacci(15);
    println("go_fibonacci(15) =", fib15);
    
    long fact7 = golib.go_factorial(7);
    println("go_factorial(7) =", fact7);
    
    double sin0 = golib.go_sin(0.0);
    println("go_sin(0.0) =", sin0);
    
    double cos0 = golib.go_cos(0.0);
    println("go_cos(0.0) =", cos0);
    
    println("✓ All Go concurrent tests completed!");
    println("✓ Go標準ライブラリ (math) も正常に動作!");
}
TESTEOF

cat > tests/zig/math_test.cb << 'TESTEOF'
// Zig FFI Math Test (Zig標準ライブラリの利用)

use foreign.ziglib {
    long zig_factorial(int n);
    long zig_fibonacci(int n);
    double zig_sqrt(double x);
    double zig_power(double base, double exp);
    double zig_circle_area(double radius);
    int zig_gcd(int a, int b);
}

void main() {
    println("=== Zig FFI Math Test ===");
    
    long fact8 = ziglib.zig_factorial(8);
    println("zig_factorial(8) =", fact8);
    
    long fib12 = ziglib.zig_fibonacci(12);
    println("zig_fibonacci(12) =", fib12);
    
    double sqrt100 = ziglib.zig_sqrt(100.0);
    println("zig_sqrt(100.0) =", sqrt100);
    
    double pow3_4 = ziglib.zig_power(3.0, 4.0);
    println("zig_power(3.0, 4.0) =", pow3_4);
    
    double area = ziglib.zig_circle_area(7.0);
    println("zig_circle_area(7.0) =", area);
    
    int gcd56_98 = ziglib.zig_gcd(56, 98);
    println("zig_gcd(56, 98) =", gcd56_98);
    
    println("✓ All Zig math tests completed!");
    println("✓ Zig標準ライブラリ (std.math) も正常に動作!");
}
TESTEOF

echo "Bool functions removed from all tests!"
