#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_string() {
    std::cout << "[integration] Running string tests..." << std::endl;
    
    // Test string literals
    run_cb_test_with_output("../../tests/cases/string/literal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "literal.cb should execute successfully");
        });
    integration_test_passed("string literal test", "literal.cb");
    
    // Test string assignment
    run_cb_test_with_output("../../tests/cases/string/assign.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "assign.cb should execute successfully");
        });
    integration_test_passed("string assign test", "assign.cb");
    
    // Test empty strings
    run_cb_test_with_output("../../tests/cases/string/empty.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "empty.cb should execute successfully");
        });
    integration_test_passed("string empty test", "empty.cb");
    
    // Test string elements
    run_cb_test_with_output("../../tests/cases/string/element.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "element.cb should execute successfully");
        });
    integration_test_passed("string element test", "element.cb");
    
    // Test string in functions
    run_cb_test_with_output("../../tests/cases/string/func.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "func.cb should execute successfully");
        });
    integration_test_passed("string func test", "func.cb");
    
    std::cout << "[integration] String tests completed" << std::endl;
}
