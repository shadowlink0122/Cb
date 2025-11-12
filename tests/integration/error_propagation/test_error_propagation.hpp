#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_error_propagation() {
    std::cout << "[integration-test] Running error propagation (? operator) tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: Basic Result<T, E> with ? operator
    run_cb_test_with_output_and_time("../cases/error_propagation/test_question_basic_result.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_question_basic_result.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Success case completed", "Should complete success case");
            INTEGRATION_ASSERT_CONTAINS(output, "Error case completed", "Should complete error case");
        }, execution_time);
    integration_test_passed_with_time("Basic Result ? operator", "test_question_basic_result.cb", execution_time);
    
    // Test 2: Basic Option<T> with ? operator
    run_cb_test_with_output_and_time("../cases/error_propagation/test_question_basic_option.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_question_basic_option.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Success case: got result", "Should display success case");
            INTEGRATION_ASSERT_CONTAINS(output, "Error case: None propagated", "Should display error case");
        }, execution_time);
    integration_test_passed_with_time("Basic Option ? operator", "test_question_basic_option.cb", execution_time);
    
    // Test 3: Comprehensive ? operator test
    run_cb_test_with_output_and_time("../cases/error_propagation/test_question_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_question_comprehensive.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Result success - PASSED", "Should pass Result success test");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Result error propagation - PASSED", "Should pass Result error test");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Option success - PASSED", "Should pass Option success test");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Option None propagation - PASSED", "Should pass Option None test");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests PASSED ===", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("Comprehensive ? operator", "test_question_comprehensive.cb", execution_time);
    
    // Test 4: ? operator with Option - array search
    run_cb_test_with_output_and_time("../cases/error_propagation/test_question_operator_option.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_question_operator_option.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Success case: found and doubled =  6", "Should find and double value");
            INTEGRATION_ASSERT_CONTAINS(output, "Error case: element not found", "Should handle not found case");
            INTEGRATION_ASSERT_CONTAINS(output, "? operator with Option test passed", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("? operator with Option array search", "test_question_operator_option.cb", execution_time);
    
    // Test 5: ? operator with Result - chain divide
    run_cb_test_with_output_and_time("../cases/error_propagation/test_question_operator_result.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_question_operator_result.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Success case: 240 / 2 / 3 / 4 =  10", "Should chain divide successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Error case: Division by zero detected", "Should detect division by zero");
            INTEGRATION_ASSERT_CONTAINS(output, "? operator with Result test passed", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("? operator with Result chain divide", "test_question_operator_result.cb", execution_time);
    
    // Test 6: Result propagation
    run_cb_test_with_output_and_time("../cases/error_propagation/test_result_propagation.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_result_propagation.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Success: 1", "Should show success count");
            INTEGRATION_ASSERT_CONTAINS(output, "Error: 0", "Should show error count");
        }, execution_time);
    integration_test_passed_with_time("Result propagation", "test_result_propagation.cb", execution_time);
    
    // Test 7: Simple propagation
    run_cb_test_with_output_and_time("../cases/error_propagation/test_simple_propagation.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_simple_propagation.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "x = 42", "Should display x value");
        }, execution_time);
    integration_test_passed_with_time("Simple propagation", "test_simple_propagation.cb", execution_time);
    
    // Test 8: Without propagation
    run_cb_test_with_output_and_time("../cases/error_propagation/test_without_propagation.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_without_propagation.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "x = 0", "Should display x value");
        }, execution_time);
    integration_test_passed_with_time("Without propagation", "test_without_propagation.cb", execution_time);
    
    std::cout << "[integration-test] Error propagation tests completed" << std::endl;
}
