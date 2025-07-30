#include <cassert>
#include <iostream>

void test_cross_type_basic() {
    // TODO: 型を跨いだ演算や代入のテストをここに追加
    int a = 1;
    long b = 2;
    a = static_cast<int>(b);
    assert(a == 2);
}

void test_unit_cross_type() {
    test_cross_type_basic();
    std::cout << "[cross_type] all tests passed" << std::endl;
}
