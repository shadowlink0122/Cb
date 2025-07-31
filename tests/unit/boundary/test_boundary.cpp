#include <cassert>
#include <iostream>

extern "C" void test_unit_boundary_tiny();
extern "C" void test_unit_boundary_short();
extern "C" void test_unit_boundary_int();
extern "C" void test_unit_boundary_long();

extern "C" void test_unit_boundary() {
    test_unit_boundary_tiny();
    test_unit_boundary_short();
    test_unit_boundary_int();
    test_unit_boundary_long();
    std::cout << "[boundary] all tests passed" << std::endl;
}
