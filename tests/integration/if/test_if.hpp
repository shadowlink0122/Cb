#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_if() {
    std::cout << "[integration-test] Running if tests..." << std::endl;
    
        // Test basic if statements (with timing)
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/if/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "If statement test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "x = 4", "Expected x = 4 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "if-else test: ok", "Expected if-else test result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "single line if: ok2", "Expected single line if result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "if-else-if: ok3", "Expected if-else-if result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "If statement test passed", "Expected success message in output");
        }, execution_time);
    integration_test_passed_with_time("if basic test", "basic.cb", execution_time);
    integration_test_passed_with_time("if basic test", "basic.cb", execution_time);
    integration_test_passed_with_time("if basic test", "basic.cb", execution_time);
    
    std::cout << "[integration-test] If tests completed" << std::endl;
}
