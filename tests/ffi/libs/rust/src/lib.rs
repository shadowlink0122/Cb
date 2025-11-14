// Rust FFI Library for Cb

// 基本的な算術関数
#[no_mangle]
pub extern "C" fn rust_add(a: i32, b: i32) -> i32 {
    a + b
}

#[no_mangle]
pub extern "C" fn rust_multiply(a: i32, b: i32) -> i32 {
    a * b
}

// 階乗（Rustのイテレータを活用）
#[no_mangle]
pub extern "C" fn rust_factorial(n: i32) -> i64 {
    if n <= 1 {
        1
    } else {
        (1..=n as i64).product()
    }
}

// フィボナッチ数列
#[no_mangle]
pub extern "C" fn rust_fibonacci(n: i32) -> i64 {
    match n {
        0 => 0,
        1 => 1,
        _ => {
            let (mut a, mut b) = (0i64, 1i64);
            for _ in 2..=n {
                let temp = a + b;
                a = b;
                b = temp;
            }
            b
        }
    }
}

// 素数判定
#[no_mangle]
pub extern "C" fn rust_is_prime(n: i32) -> bool {
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

// 最大公約数
#[no_mangle]
pub extern "C" fn rust_gcd(mut a: i32, mut b: i32) -> i32 {
    while b != 0 {
        let temp = b;
        b = a % b;
        a = temp;
    }
    a.abs()
}

// 最小公倍数
#[no_mangle]
pub extern "C" fn rust_lcm(a: i32, b: i32) -> i32 {
    if a == 0 || b == 0 {
        return 0;
    }
    (a * b).abs() / rust_gcd(a, b)
}

// 円の面積
#[no_mangle]
pub extern "C" fn rust_circle_area(radius: f64) -> f64 {
    std::f64::consts::PI * radius * radius
}

// 平方根
#[no_mangle]
pub extern "C" fn rust_sqrt(x: f64) -> f64 {
    x.sqrt()
}
