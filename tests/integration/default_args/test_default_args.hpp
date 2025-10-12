#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_default_args() {
    std::cout << "[integration-test] Running Default Arguments Tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: Basic default arguments
    run_cb_test_with_output_and_time("../cases/default_args/test_default_args_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_args_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "6", "Should compute add(1, 2, 3) = 6");
            INTEGRATION_ASSERT_CONTAINS(output, "23", "Should compute add(1, 2) = 23 with default c=20");
            INTEGRATION_ASSERT_CONTAINS(output, "31", "Should compute add(1) = 31 with defaults b=10, c=20");
            
            auto lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(3, lines.size(), "Should have exactly 3 output lines");
            INTEGRATION_ASSERT_EQ("6", lines[0], "First output should be 6");
            INTEGRATION_ASSERT_EQ("23", lines[1], "Second output should be 23");
            INTEGRATION_ASSERT_EQ("31", lines[2], "Third output should be 31");
        }, execution_time);
    integration_test_passed_with_time("Basic default arguments functionality", "test_default_args_basic.cb", execution_time);
    
    // Test 2: Various types with default arguments
    run_cb_test_with_output_and_time("../cases/default_args/test_default_args_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_args_types.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "--- All defaults ---", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "42", "Should use default int value");
            INTEGRATION_ASSERT_CONTAINS(output, "hello", "Should use default string value");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Partial defaults ---", "Should print partial test header");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "Should use custom int value");
            INTEGRATION_ASSERT_CONTAINS(output, "world", "Should use custom string value");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Multiply test ---", "Should print multiply test header");
            INTEGRATION_ASSERT_CONTAINS(output, "30", "Should compute multiply(5) = 30");
            INTEGRATION_ASSERT_CONTAINS(output, "60", "Should compute multiply(5, 4) = 60");
            INTEGRATION_ASSERT_CONTAINS(output, "40", "Should compute multiply(5, 4, 2) = 40");
        }, execution_time);
    integration_test_passed_with_time("Default arguments with various types (int, string, bool)", "test_default_args_types.cb", execution_time);
    
    // Test 3: const variables as default values
    run_cb_test_with_output_and_time("../cases/default_args/test_default_args_const.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_args_const.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Default window ---", "Should print default test header");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Custom width ---", "Should print custom width test");
            INTEGRATION_ASSERT_CONTAINS(output, "--- All custom ---", "Should print all custom test");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Compute test ---", "Should print compute test header");
            INTEGRATION_ASSERT_CONTAINS(output, "50", "Should compute with default multiplier");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "Should compute with custom multiplier");
        }, execution_time);
    integration_test_passed_with_time("const variables as default values", "test_default_args_const.cb", execution_time);
    
    // Test 4: struct types with default arguments
    run_cb_test_with_output_and_time("../cases/default_args/test_default_args_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_args_struct.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Function returning struct with default args ---", 
                "Should print struct test header");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Struct parameter with default label ---", 
                "Should print struct parameter test");
            
            auto lines = split_lines(output);
            int numeric_count = 0;
            for (const auto& line : lines) {
                if (line == "30" || line == "120" || line == "300" || line == "125") {
                    numeric_count++;
                }
            }
            INTEGRATION_ASSERT(numeric_count >= 4, 
                "Should have struct calculations (30, 120, 300, 125)");
        }, execution_time);
    integration_test_passed_with_time("struct types with default arguments", "test_default_args_struct.cb", execution_time);
    
    // Test 5: Array parameters with default arguments
    run_cb_test_with_output_and_time("../cases/default_args/test_default_args_array.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_args_array.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Sum with default multiplier ---", 
                "Should print default multiplier test");
            INTEGRATION_ASSERT_CONTAINS(output, "15", "Should compute sum with default multiplier");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Sum with custom multiplier ---", 
                "Should print custom multiplier test");
            INTEGRATION_ASSERT_CONTAINS(output, "30", "Should compute sum with custom multiplier");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Print with default prefix ---", 
                "Should print default prefix test");
            INTEGRATION_ASSERT_CONTAINS(output, "Array:", "Should use default prefix");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Print with custom prefix ---", 
                "Should print custom prefix test");
            INTEGRATION_ASSERT_CONTAINS(output, "Values:", "Should use custom prefix");
            
            // Verify array elements are printed
            INTEGRATION_ASSERT_CONTAINS(output, "10", "Should print array element 10");
            INTEGRATION_ASSERT_CONTAINS(output, "20", "Should print array element 20");
            INTEGRATION_ASSERT_CONTAINS(output, "30", "Should print array element 30");
        }, execution_time);
    integration_test_passed_with_time("Array parameters with default arguments", "test_default_args_array.cb", execution_time);
    
    // Test 6: Error case - Non-default parameter after default parameter
    run_cb_test_with_output_and_time("../cases/default_args/test_default_args_error1.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "test_default_args_error1.cb should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "error", "Should report an error");
            INTEGRATION_ASSERT_CONTAINS(output, "Non-default parameter", 
                "Should report non-default parameter error");
            INTEGRATION_ASSERT_CONTAINS(output, "after default parameter", 
                "Should mention 'after default parameter'");
        }, execution_time);
    integration_test_passed_with_time("Error detection: non-default parameter after default", "test_default_args_error1.cb", execution_time);
    
    // Test 7: Error case - Missing required argument
    run_cb_test_with_output_and_time("../cases/default_args/test_default_args_error2.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "test_default_args_error2.cb should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "Error", "Should report an error");
            INTEGRATION_ASSERT_CONTAINS(output, "Argument count mismatch", 
                "Should report argument count mismatch");
            INTEGRATION_ASSERT_CONTAINS(output, "expected 1 to 2, got 0", 
                "Should show expected argument range");
        }, execution_time);
    integration_test_passed_with_time("Error detection: missing required argument", "test_default_args_error2.cb", execution_time);
    
    std::cout << "[integration-test] ✅ PASS: Default Arguments Tests (7 tests)" << std::endl;
}
