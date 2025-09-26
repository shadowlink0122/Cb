#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_bool_expr_basic() {
    std::cout << "[integration] Running bool expr tests..." << std::endl;
    
    // Test basic boolean expressions
    run_cb_test_with_output_and_time_auto("../../tests/cases/bool_expr/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
        });
    integration_test_passed_with_time_auto("bool expr basic test", "basic.cb");
    
    std::cout << "[integration] Bool expr tests completed" << std::endl;
}
