#include <cassert>
#include <iostream>

void test_arithmetic_basic() {
    // tiny型
    signed char ta = 10, tb = 3;
    assert(ta + tb == 13);
    assert(ta - tb == 7);
    assert(ta * tb == 30);
    assert(ta / tb == 3);
    // short型
    short sa = 1000, sb = 200;
    assert(sa + sb == 1200);
    assert(sa - sb == 800);
    assert(sa * sb == 200000);
    assert(sa / sb == 5);
    // int型
    int ia = 100000, ib = 20000;
    assert(ia + ib == 120000);
    assert(ia - ib == 80000);
    assert(ia * ib == 2000000000);
    assert(ia / ib == 5);
    // long long型
    long long la = 1000000000LL, lb = 2000000000LL;
    assert(la + lb == 3000000000LL);
    assert(la - lb == -1000000000LL);
    assert(la * lb == 2000000000000000000LL);
    assert(lb / la == 2LL);
}

void test_unit_arithmetic() {
    test_arithmetic_basic();
    std::cout << "[arithmetic] all tests passed" << std::endl;
}
