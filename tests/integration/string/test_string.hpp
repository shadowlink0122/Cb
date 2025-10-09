#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_string() {
    std::cout << "[integration-test] Running string tests..." << std::endl;
    
    // Test string literals (with timing)
    double execution_time_literal;
    run_cb_test_with_output_and_time("../../tests/cases/string/literal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "literal.cb should execute successfully");
        }, execution_time_literal);
    integration_test_passed_with_time("string literal test", "literal.cb", execution_time_literal);
    
    // Test string assignment (with timing)
    double execution_time_assign;
    run_cb_test_with_output_and_time("../../tests/cases/string/assign.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "assign.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "String assignment test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "s: test string", "Expected initial string value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "s after reassignment: second", "Expected reassigned string value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "String assignment test passed", "Expected success message in output");
        }, execution_time_assign);
    integration_test_passed_with_time("string assign test", "assign.cb", execution_time_assign);
    
    // Test empty strings (with timing)
    double execution_time_empty;
    run_cb_test_with_output_and_time("../../tests/cases/string/empty.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "empty.cb should execute successfully");
        }, execution_time_empty);
    integration_test_passed_with_time("string empty test", "empty.cb", execution_time_empty);
    
    // Test string elements (with timing)
    double execution_time_element;
    run_cb_test_with_output_and_time("../../tests/cases/string/element.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "element.cb should execute successfully");
        }, execution_time_element);
    integration_test_passed_with_time("string element test", "element.cb", execution_time_element);
    
    // Test string in functions (with timing)
    double execution_time_func;
    run_cb_test_with_output_and_time("../../tests/cases/string/func.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "func.cb should execute successfully");
        }, execution_time_func);
    integration_test_passed_with_time("string func test", "func.cb", execution_time_func);
    
    // Phase 2 Week 1: Test strlen spec implementation (with timing)
    double execution_time_strlen_spec;
    run_cb_test_with_output_and_time("../../tests/cases/string/test_strlen_spec.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_strlen_spec.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 1: strlen from spec.md ===", "Expected Test 1 header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 2: Various string lengths ===", "Expected Test 2 header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 3: Direct null terminator access ===", "Expected Test 3 header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "=== strlen spec test completed ===", "Expected completion message in output");
        }, execution_time_strlen_spec);
    integration_test_passed_with_time("string strlen spec test", "test_strlen_spec.cb", execution_time_strlen_spec);
    
    std::cout << "[integration-test] String tests completed" << std::endl;
}
