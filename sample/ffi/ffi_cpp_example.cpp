// C++ library example for FFI
// This demonstrates calling C++ functions from Cb using extern "C"

#include <cmath>
#include <cstring>
#include <iostream>

// Simple C++ functions wrapped with extern "C" for FFI
extern "C" {

// Basic arithmetic
int cpp_add(int a, int b) { return a + b; }

int cpp_multiply(int a, int b) { return a * b; }

// Math operations
double cpp_distance(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

double cpp_circle_area(double radius) {
    const double PI = 3.14159265359;
    return PI * radius * radius;
}

// String operations (simplified for demo)
int cpp_string_length(const char *str) {
    return str ? std::strlen(str) : 0;
}

// Factorial (recursive)
long cpp_factorial(int n) {
    if (n <= 1)
        return 1;
    return n * cpp_factorial(n - 1);
}

// Fibonacci
long cpp_fibonacci(int n) {
    if (n <= 1)
        return n;
    long a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        long temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

// Print from C++ (for testing)
void cpp_hello() { std::cout << "Hello from C++!" << std::endl; }

void cpp_print_number(int n) {
    std::cout << "C++ received number: " << n << std::endl;
}
}
