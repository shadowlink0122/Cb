#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_global_vars() {
    std::cout << "[integration] Running global vars tests..." << std::endl;
    
    // Test basic global variables
    run_cb_test_with_output_and_time_auto("../../tests/cases/global_vars/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Global variables test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "g1: 10", "Expected initial g1 value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "inc(): 11", "Expected inc() result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "g1 after inc: 11", "Expected updated g1 value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "sum(): 1014", "Expected sum result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "sum() after arr[2] change: 1041", "Expected updated sum result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "msg: ok", "Expected global string value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Global variables test passed", "Expected success message in output");
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
