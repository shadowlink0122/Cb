#include <cassert>
#include <iostream>

void test_assign_basic() {
    // TODO: 実装例: 型ごとの代入テストをここに追加
    int a = 1;
    int b = 2;
    a = b;
    assert(a == 2);
}

void test_unit_assign() {
    test_assign_basic();
    std::cout << "[assign] all tests passed" << std::endl;
}
