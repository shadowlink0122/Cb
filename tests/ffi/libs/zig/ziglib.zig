const std = @import("std");

// 基本的な算術関数
export fn zig_add(a: i32, b: i32) i32 {
    return a + b;
}

export fn zig_subtract(a: i32, b: i32) i32 {
    return a - b;
}

export fn zig_multiply(a: i32, b: i32) i32 {
    return a * b;
}

export fn zig_divide(a: i32, b: i32) i32 {
    if (b == 0) return 0;
    return @divTrunc(a, b);
}

// 階乗
export fn zig_factorial(n: i32) i64 {
    if (n <= 1) return 1;
    var result: i64 = 1;
    var i: i32 = 2;
    while (i <= n) : (i += 1) {
        result *= i;
    }
    return result;
}

// フィボナッチ数列
export fn zig_fibonacci(n: i32) i64 {
    if (n <= 1) return n;
    var a: i64 = 0;
    var b: i64 = 1;
    var i: i32 = 2;
    while (i <= n) : (i += 1) {
        const temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// 素数判定
export fn zig_is_prime(n: i32) bool {
    if (n < 2) return false;
    var i: i32 = 2;
    while (i * i <= n) : (i += 1) {
        if (@rem(n, i) == 0) return false;
    }
    return true;
}

// 最大値
export fn zig_max(a: i32, b: i32) i32 {
    return if (a > b) a else b;
}

// 最小値
export fn zig_min(a: i32, b: i32) i32 {
    return if (a < b) a else b;
}

// 絶対値
export fn zig_abs(n: i32) i32 {
    return if (n < 0) -n else n;
}

// 浮動小数点演算
export fn zig_sqrt(x: f64) f64 {
    return @sqrt(x);
}

export fn zig_power(base: f64, exp: f64) f64 {
    return std.math.pow(f64, base, exp);
}

export fn zig_circle_area(radius: f64) f64 {
    return std.math.pi * radius * radius;
}

// 最大公約数
export fn zig_gcd(a_in: i32, b_in: i32) i32 {
    var a = a_in;
    var b = b_in;
    while (b != 0) {
        const temp = b;
        b = @rem(a, b);
        a = temp;
    }
    return if (a < 0) -a else a;
}
