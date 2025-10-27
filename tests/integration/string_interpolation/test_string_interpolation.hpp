#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_string_interpolation() {
    std::cout << "[integration-test] Running string interpolation tests..." << std::endl;
    
    // Test 1: Basic interpolation
    double execution_time_basic;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/test_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Interpolation Basic Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Simple variable - PASSED", "Expected test 1 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Integer value - PASSED", "Expected test 2 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Multiple interpolations - PASSED", "Expected test 3 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Empty string - PASSED", "Expected test 4 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: Interpolation at start - PASSED", "Expected test 5 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: Interpolation at end - PASSED", "Expected test 6 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7: Only interpolation - PASSED", "Expected test 7 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Basic Tests Passed ===", "Expected completion message");
        }, execution_time_basic);
    integration_test_passed_with_time("string interpolation basic test", "test_basic.cb", execution_time_basic);
    
    // Test 2: Expression evaluation
    double execution_time_expressions;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/test_expressions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_expressions.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Interpolation Expression Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Addition - PASSED", "Expected test 1 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Subtraction - PASSED", "Expected test 2 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Multiplication - PASSED", "Expected test 3 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Division - PASSED", "Expected test 4 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: Modulo - PASSED", "Expected test 5 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: Complex expression - PASSED", "Expected test 6 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7: Parentheses - PASSED", "Expected test 7 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 8: Comparison - PASSED", "Expected test 8 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 9: Unary minus - PASSED", "Expected test 9 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 10: Multiple expressions - PASSED", "Expected test 10 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Expression Tests Passed ===", "Expected completion message");
        }, execution_time_expressions);
    integration_test_passed_with_time("string interpolation expressions test", "test_expressions.cb", execution_time_expressions);
    
    // Test 3: Array access in interpolation
    double execution_time_array;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/test_array_access.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_array_access.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Interpolation Array Access Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Simple array - PASSED", "Expected test 1 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Array in expression - PASSED", "Expected test 2 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Multiple accesses - PASSED", "Expected test 3 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: String array - PASSED", "Expected test 4 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: Index variable - PASSED", "Expected test 5 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Array Access Tests Passed ===", "Expected completion message");
        }, execution_time_array);
    integration_test_passed_with_time("string interpolation array access test", "test_array_access.cb", execution_time_array);
    
    // Test 4: Member access in interpolation
    double execution_time_member;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/test_member_access.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_member_access.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Interpolation Member Access Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Simple struct - PASSED", "Expected test 1 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Member in expression - PASSED", "Expected test 2 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Multiple members - PASSED", "Expected test 3 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Member Access Tests Passed ===", "Expected completion message");
        }, execution_time_member);
    integration_test_passed_with_time("string interpolation member access test", "test_member_access.cb", execution_time_member);
    
    // Test 5: Type conversion in interpolation
    double execution_time_types;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/test_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_types.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Interpolation Type Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: int type - PASSED", "Expected test 1 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: long type - PASSED", "Expected test 2 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: double type - PASSED", "Expected test 3 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: float type - PASSED", "Expected test 4 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: Zero value - PASSED", "Expected test 5 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: Negative value - PASSED", "Expected test 6 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7: Multiple types - PASSED", "Expected test 7 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Type Tests Passed ===", "Expected completion message");
        }, execution_time_types);
    integration_test_passed_with_time("string interpolation types test", "test_types.cb", execution_time_types);
    
    // Test 6: Format specifiers
    double execution_time_format;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/format_specifiers.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "format_specifiers.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Integer Format Tests ===", "Expected integer format tests");
            INTEGRATION_ASSERT_CONTAINS(output, "Decimal: 255", "Expected decimal output");
            INTEGRATION_ASSERT_CONTAINS(output, "Hex (lower): ff", "Expected lowercase hex output");
            INTEGRATION_ASSERT_CONTAINS(output, "Hex (upper): FF", "Expected uppercase hex output");
            INTEGRATION_ASSERT_CONTAINS(output, "Binary: 11111111", "Expected binary output");
            INTEGRATION_ASSERT_CONTAINS(output, "Zero padded: 00255", "Expected zero-padded output");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Float Format Tests ===", "Expected float format tests");
            INTEGRATION_ASSERT_CONTAINS(output, "Two decimals: 3.14", "Expected 2-decimal precision");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Format Tests Passed ===", "Expected completion message");
        }, execution_time_format);
    integration_test_passed_with_time("string interpolation format specifiers test", "format_specifiers.cb", execution_time_format);
    
    // Test 7: Edge cases
    double execution_time_edge;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/test_edge_cases.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_edge_cases.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Interpolation Edge Cases Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Zero value - PASSED", "Expected test 1 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Consecutive interpolations - PASSED", "Expected test 3 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Boundary positions - PASSED", "Expected test 4 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: Repeated variable - PASSED", "Expected test 5 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: Negative number - PASSED", "Expected test 6 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7: Complex expression - PASSED", "Expected test 7 to pass");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Edge Cases Tests Passed ===", "Expected completion message");
        }, execution_time_edge);
    integration_test_passed_with_time("string interpolation edge cases test", "test_edge_cases.cb", execution_time_edge);
    
    // Test 8: Comprehensive integration tests
    double execution_time_basic_interp;
    run_cb_test_with_output_and_time("../../tests/cases/string_interpolation/basic_interpolation.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic_interpolation.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Hello, World!", "Expected basic interpolation");
            INTEGRATION_ASSERT_CONTAINS(output, "Welcome to Cb v0.11!", "Expected version string");
            INTEGRATION_ASSERT_CONTAINS(output, "Count: 42", "Expected count output");
            INTEGRATION_ASSERT_CONTAINS(output, "5 * 5 = 25", "Expected calculation output");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Basic Interpolation Tests Passed ===", "Expected completion message");
        }, execution_time_basic_interp);
    integration_test_passed_with_time("string interpolation comprehensive basic test", "basic_interpolation.cb", execution_time_basic_interp);
    
    std::cout << "[integration-test] String interpolation tests completed" << std::endl;
}
