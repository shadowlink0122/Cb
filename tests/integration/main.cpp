#include "framework/integration_test_framework.hpp"

// 各テストモジュールをインクルード
#include "arithmetic/test_arithmetic.hpp"
#include "array/test_array.hpp"
#include "array_literal/test_array_literal.hpp"
#include "assign/test_assign.hpp"
#include "bool_expr/test_bool_expr.hpp"
#include "boundary/test_boundary.hpp"
#include "cross_type/test_cross_type.hpp"
#include "func/test_func.hpp"
#include "global_vars/test_global_vars.hpp"
#include "if/test_if.hpp"
#include "incdec/test_incdec.hpp"
#include "loop/test_loop.hpp"
#include "printf/test_printf.hpp"
#include "self_assign/test_self_assign.hpp"
#include "string/test_string.hpp"

int main() {
    int fail = 0;
    try {
        test_integration_arithmetic();
        test_integration_assign();
        test_integration_boundary();
        test_integration_cross_type();
        test_integration_func();
        test_integration_string();
        test_integration_array();
        test_integration_array_literal();
        test_bool_expr_basic();
        test_integration_loop();
        test_integration_if();
        test_integration_self_assign();
        test_integration_incdec();
        test_integration_global_vars();
        test_printf_all();
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
