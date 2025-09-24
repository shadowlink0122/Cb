#include "framework/integration_test_framework.hpp"

// 各テストモジュールをインクルード
#include "arithmetic/test_arithmetic.hpp"
#include "array/test_array.hpp"
#include "array_copy/test_array_copy.hpp"
#include "array_literal/test_array_literal.hpp"
#include "array_return/test_array_return.hpp"
#include "assign/test_assign.hpp"
#include "bool_expr/test_bool_expr.hpp"
#include "boundary/test_boundary.hpp"
#include "const_array/test_const_array.hpp"
#include "cross_type/test_cross_type.hpp"
#include "dynamic_array_error/test_dynamic_array_error.hpp"
#include "error_handling/test_error_handling.hpp"
#include "func/test_func.hpp"
#include "global_array/test_global_array.hpp"
#include "global_vars/test_global_vars.hpp"
#include "if/test_if.hpp"
#include "import_export/test_import_export.hpp"
#include "incdec/test_incdec.hpp"
#include "loop/test_loop.hpp"
#include "module_functions/test_module_functions.hpp"
#include "multidim_literal/test_multidim_literal.hpp"
#include "multiple_var_decl/test_multiple_var_decl.hpp"
#include "printf/test_printf.hpp"
#include "println/test_println.hpp"
#include "sample_scenarios/test_sample_scenarios.hpp"
#include "samples/test_actual_samples.hpp"
#include "self_assign/test_self_assign.hpp"
#include "string/test_string.hpp"
#include "typedef/test_typedef.hpp"

int main() {
    int fail = 0;

    // Reset test counters
    IntegrationTestCounter::reset();

    try {
        test_integration_arithmetic();
        test_integration_assign();
        test_integration_boundary();
        test_integration_const_array();
        test_integration_cross_type();
        test_integration_func();
        test_integration_string();
        test_integration_array();
        test_integration_array_literal();
        test_array_copy();
        test_array_return();
        test_multidim_literal();
        test_bool_expr_basic();
        test_integration_loop();
        test_integration_multiple_var_decl();
        test_integration_if();
        test_integration_self_assign();
        test_integration_incdec();
        test_integration_global_vars();
        test_printf_all();
        test_integration_println();
        test_integration_global_array();
        test_integration_sample_scenarios();
        test_integration_actual_samples();
        test_integration_error_handling();
        test_integration_import_export();
        test_integration_module_functions();
        test_integration_typedef();
        test_integration_dynamic_array_error();
    } catch (const std::exception &e) {
        std::cerr << "[integration] test failed: " << e.what() << std::endl;
        fail = 1;
    } catch (...) {
        std::cerr << "[integration] test failed: unknown error" << std::endl;
        fail = 1;
    }

    // Print test results summary
    if (fail == 0) {
        std::cout << "[integration] all tests passed" << std::endl;
    }
    IntegrationTestCounter::print_summary();

    return fail;
}
