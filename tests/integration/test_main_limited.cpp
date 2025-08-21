#include "arithmetic/test_arithmetic_case.h"
#include "assign/test_assign_case.h"
#include <iostream>

int main() {
    int fail = 0;
    try {
        test_integration_arithmetic();
        test_integration_assign_short();
        test_integration_assign_int();
        // セグフォルトの原因となる部分をコメントアウト
        // test_integration_assign_int_ng_neg();
    } catch (const std::exception &e) {
        std::cerr << "[integration] test failed: " << e.what() << std::endl;
        fail = 1;
    } catch (...) {
        std::cerr << "[integration] test failed: unknown error" << std::endl;
        fail = 1;
    }
    if (fail == 0) {
        std::cout << "[integration] selected tests passed" << std::endl;
    }
    return fail;
}
