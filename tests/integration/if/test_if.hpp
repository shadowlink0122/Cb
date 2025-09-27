#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_if() {
    std::cout << "[integration] Running if tests..." << std::endl;
    
    // Test basic if statements (with timing)
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/if/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("if basic test", "basic.cb", execution_time);
    
    std::cout << "[integration] If tests completed" << std::endl;
}
