#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_incdec() {
    std::cout << "[integration] Running incdec tests..." << std::endl;
    
    // Note: compound assignment operators (+=, -=, *=, /=, %=) are not yet implemented
    // Current test cases use these operators, so testing only pre/post increment which should work
    
    // Test pre/post increment/decrement
    run_cb_test_with_output_and_time_auto("../../tests/cases/incdec/pre_post.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "pre_post.cb should execute successfully");
        });
    integration_test_passed_with_time_auto("incdec pre_post test", "pre_post.cb");
    
    // Test increment/decrement in for loop
    run_cb_test_with_output_and_time_auto("../../tests/cases/incdec/for_loop.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "for_loop.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Increment/decrement in for loop test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "sum after first loop: 3", "Expected sum after first loop: 3 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "sum after second loop: 9", "Expected sum after second loop: 9 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Increment/decrement for loop test passed", "Expected success message in output");
        });
    integration_test_passed_with_time_auto("incdec for_loop test", "for_loop.cb");
    
    std::cout << "[integration] Incdec tests completed" << std::endl;
}
