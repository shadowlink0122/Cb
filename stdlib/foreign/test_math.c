// test_math.c - テスト用の簡単な数学ライブラリ
// v0.13.0: FFI機能のテスト用

#include <math.h>

// 2つの整数を加算
int add(int a, int b) {
    return a + b;
}

// 2つの整数を乗算
int multiply(int a, int b) {
    return a * b;
}

// 平方根を計算（標準ライブラリのsqrtを使用）
double my_sqrt(double x) {
    return sqrt(x);
}

// 累乗を計算（標準ライブラリのpowを使用）
double my_pow(double base, double exponent) {
    return pow(base, exponent);
}
