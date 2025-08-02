#include "arithmetic/test_arithmetic_case.h"
#include "array/test_array.h"
#include "assign/test_assign_case.h"
#include "bool_expr/test_bool_expr.h"
#include "boundary/test_boundary_case.h"
#include "cross_type/test_cross_type_case.h"
#include "func/test_func_case.h"
#include "if/test_if_case.h"
#include "incdec/test_incdec_case.h"
#include "loop/test_loop.h"
#include "self_assign/test_self_assign_case.h"
#include "string/test_string.h"
#include <iostream>

int main() {
    int fail = 0;
    try {
        test_integration_arithmetic();
        test_integration_assign();
        test_integration_boundary();
        test_integration_cross_type();
        test_integration_func();
        test_integration_incdec();
        test_integration_self_assign();
        test_integration_string();
        test_integration_array();
        test_bool_expr_basic();
        test_integration_loop();
        test_integration_if_basic();
    } catch (const std::exception &e) {
        std::cerr << "[integration] test failed: " << e.what() << std::endl;
        fail = 1;
    } catch (...) {
        std::cerr << "[integration] test failed: unknown error" << std::endl;
        fail = 1;
    }
    if (fail == 0) {
        std::cout << "[integration] all tests passed" << std::endl;
    }
    return fail;
}
