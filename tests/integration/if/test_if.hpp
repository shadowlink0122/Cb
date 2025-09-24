#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_if() {
    std::cout << "[integration] Running if tests..." << std::endl;
    
    // Test basic if statements
    run_cb_test_with_output("../../tests/cases/if/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
        });
    integration_test_passed("if basic test", "basic.cb");
    
    std::cout << "[integration] If tests completed" << std::endl;
}
