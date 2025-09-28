#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_bool_expr_basic() {
    std::cout << "[integration] Running bool expr tests..." << std::endl;
    
    // Test basic boolean expressions
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/bool_expr/basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Boolean expression test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "true == 1: 1", "Expected true == 1 result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "false == 0: 1", "Expected false == 0 result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "true == 0: 0", "Expected true == 0 result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "3 > 2: 1", "Expected 3 > 2 result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "!true: 0", "Expected !true result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "true && !false: 1", "Expected logical expression result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Boolean expression test passed", "Expected success message in output");
        }, execution_time);
    integration_test_passed_with_time("bool_expr basic test", "basic.cb", execution_time);
    
    std::cout << "[integration] Bool expr tests completed" << std::endl;
}
