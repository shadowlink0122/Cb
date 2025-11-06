#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_pattern_matching() {
    std::cout << "[integration-test] Running Pattern Matching tests..." << std::endl;
    
    // Test 1: Basic Option pattern matching
    double execution_time_1;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_option_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_option_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 1: Option<int> basic pattern matching ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Matched Some with value:",
                "Should match Some variant");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Matched None",
                "Should match None variant");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 1: PASSED",
                "Should show success message");
        },
        execution_time_1
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_option_basic.cb", 
        execution_time_1
    );
    
    // Test 2: Basic Result pattern matching
    double execution_time_2;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_result_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_result_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 2: Result<int, int> basic pattern matching ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 2: PASSED",
                "Should show success message");
        },
        execution_time_2
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_result_basic.cb", 
        execution_time_2
    );
    
    // Test 3: Wildcard pattern matching
    double execution_time_3;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_wildcard.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_wildcard.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 3: Wildcard pattern matching ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Status: Ready with value:",
                "Should match Ready variant");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Status: Other (caught by wildcard)",
                "Should catch with wildcard");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 3: PASSED",
                "Should show success message");
        },
        execution_time_3
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_wildcard.cb", 
        execution_time_3
    );
    
    // Test 4: Nested match statements
    double execution_time_4;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_nested.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_nested.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 4: Nested match statements ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 4: PASSED",
                "Should show success message");
        },
        execution_time_4
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_nested.cb", 
        execution_time_4
    );
    
    // Test 5: Multiple match arms
    double execution_time_5;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_multiple_arms.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_multiple_arms.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 5: Multiple match arms ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "HTTP 200 OK:",
                "Should handle 200 status");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "HTTP 404 Not Found:",
                "Should handle 404 status");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 5: PASSED",
                "Should show success message");
        },
        execution_time_5
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_multiple_arms.cb", 
        execution_time_5
    );
    
    // Test 6: Pattern matching without binding
    double execution_time_6;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_without_binding.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_without_binding.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 6: Pattern matching without binding ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 6: PASSED",
                "Should show success message");
        },
        execution_time_6
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_without_binding.cb", 
        execution_time_6
    );
    
    // Test 7: Match on function return values
    double execution_time_7;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_return_value.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_return_value.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 7: Match on function return values ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Result 1:",
                "Should show result 1");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 7: PASSED",
                "Should show success message");
        },
        execution_time_7
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_return_value.cb", 
        execution_time_7
    );
    
    // Test 8: Switch and Match combined
    double execution_time_8;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_with_switch.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_with_switch.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Test 8: Switch and Match combined ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 8: PASSED",
                "Should show success message");
        },
        execution_time_8
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_with_switch.cb", 
        execution_time_8
    );
    
    // Test 9: Result debug test
    double execution_time_9;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pattern_matching/match_result_debug.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "match_result_debug.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Result Debug Test ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Debug Test: PASSED",
                "Should show success message");
        },
        execution_time_9
    );
    integration_test_passed_with_time(
        "Pattern Matching", 
        "match_result_debug.cb", 
        execution_time_9
    );
    
    std::cout << "[integration-test] Pattern Matching tests completed" << std::endl;
}
