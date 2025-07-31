#include <cassert>
#include <iostream>

extern "C" void test_unit_cross_type_tiny();
extern "C" void test_unit_cross_type_short();
extern "C" void test_unit_cross_type_int();
extern "C" void test_unit_cross_type_long();

void test_cross_type_basic() {
    // int→long
    int i = 123;
    long l = i;
    assert(l == 123);
    // short→int
    short s = -100;
    int i2 = s;
    assert(i2 == -100);
    // tiny→short
    signed char t = 42;
    short s2 = t;
    assert(s2 == 42);
    // long→int（値が範囲内の場合のみ）
    long l2 = 77;
    int i3 = static_cast<int>(l2);
    assert(i3 == 77);
}

extern "C" void test_unit_cross_type() {
    test_unit_cross_type_tiny();
    test_unit_cross_type_short();
    test_unit_cross_type_int();
    test_unit_cross_type_long();
    test_cross_type_basic();
    std::cout << "[cross_type] all tests passed" << std::endl;
}
