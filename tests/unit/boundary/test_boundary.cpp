#include <cassert>
#include <iostream>
#include <limits>

void test_boundary_basic() {
    // TODO: 型ごとの境界値テストをここに追加
    int max = std::numeric_limits<int>::max();
    int min = std::numeric_limits<int>::min();
    assert(max > min);
}

void test_unit_boundary() {
    test_boundary_basic();
    std::cout << "[boundary] all tests passed" << std::endl;
}
