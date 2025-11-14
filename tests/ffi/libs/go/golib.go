package main

import "C"
import "math"

// 基本的な算術関数
//export go_add
func go_add(a, b C.int) C.int {
	return a + b
}

//export go_subtract
func go_subtract(a, b C.int) C.int {
	return a - b
}

//export go_multiply
func go_multiply(a, b C.int) C.int {
	return a * b
}

// 浮動小数点演算
//export go_power
func go_power(base C.double, exp C.double) C.double {
	return C.double(math.Pow(float64(base), float64(exp)))
}

//export go_sqrt
func go_sqrt(x C.double) C.double {
	return C.double(math.Sqrt(float64(x)))
}

//export go_sin
func go_sin(x C.double) C.double {
	return C.double(math.Sin(float64(x)))
}

//export go_cos
func go_cos(x C.double) C.double {
	return C.double(math.Cos(float64(x)))
}

// フィボナッチ数列
//export go_fibonacci
func go_fibonacci(n C.int) C.long {
	if n <= 1 {
		return C.long(n)
	}
	a, b := 0, 1
	for i := 2; i <= int(n); i++ {
		a, b = b, a+b
	}
	return C.long(b)
}

// 階乗
//export go_factorial
func go_factorial(n C.int) C.long {
	if n <= 1 {
		return 1
	}
	result := C.long(1)
	for i := C.long(2); i <= C.long(n); i++ {
		result *= i
	}
	return result
}

// 素数判定
//export go_is_prime
func go_is_prime(n C.int) C.int {
	if n < 2 {
		return 0
	}
	for i := 2; i*i <= int(n); i++ {
		if int(n)%i == 0 {
			return 0
		}
	}
	return 1
}

func main() {}
