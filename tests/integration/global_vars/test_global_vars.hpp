#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_global_vars() {
    std::cout << "[integration] Running global vars tests..." << std::endl;
    
    // Test basic global variables
    run_cb_test_with_output_and_time_auto("../../tests/cases/global_vars/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
        });
    integration_test_passed_with_time_auto("global vars basic test", "basic.cb");
    
    // Test global array sharing
    run_cb_test_with_output_and_time_auto("../../tests/cases/global_vars/array_share.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "array_share.cb should execute successfully");
        });
    integration_test_passed_with_time_auto("global vars array_share test", "array_share.cb");
    
    // Test global variable redeclaration (should fail)
    run_cb_test_with_output_and_time_auto("../../tests/cases/global_vars/redeclare.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "redeclare.cb should fail due to redeclaration");
        });
    integration_test_passed_with_error_and_time_auto("global vars redeclare test", "redeclare.cb");
    
    std::cout << "[integration] Global vars tests completed" << std::endl;
}
