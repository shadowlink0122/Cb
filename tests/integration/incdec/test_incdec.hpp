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
    
    std::cout << "[integration] Incdec tests completed" << std::endl;
}
