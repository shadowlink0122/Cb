#include <math.h>
#include <string.h>
#include <stdlib.h>

// 基本的な算術関数
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

int divide(int a, int b) {
    if (b == 0) return 0;
    return a / b;
}

// 高度な数学関数
long factorial(int n) {
    if (n <= 1) return 1;
    long result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

int is_prime(int n) {
    if (n < 2) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

double power(double base, double exp) {
    return pow(base, exp);
}

double square_root(double x) {
    return sqrt(x);
}

// C標準ライブラリのラッパー
int string_length(const char* str) {
    return str ? strlen(str) : 0;
}

double sine(double x) {
    return sin(x);
}

double cosine(double x) {
    return cos(x);
}

double tangent(double x) {
    return tan(x);
}

double absolute(double x) {
    return fabs(x);
}

double ceiling(double x) {
    return ceil(x);
}

double floor_value(double x) {
    return floor(x);
}
