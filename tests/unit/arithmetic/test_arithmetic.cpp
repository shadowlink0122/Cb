#include <cassert>
#include <iostream>

void test_arithmetic_basic() {
    // TODO: 型ごとの四則演算テストをここに追加
    int a = 2, b = 3;
    assert(a + b == 5);
    assert(a * b == 6);
}

void test_unit_arithmetic() {
    test_arithmetic_basic();
    std::cout << "[arithmetic] all tests passed" << std::endl;
}
