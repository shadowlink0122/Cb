#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_builtin_types() {
    std::cout << "[integration-test] Running Builtin Types (Option/Result) tests..." << std::endl;
    
    // Test 1: Option<T> basic usage
    double execution_time_1;
    run_cb_test_with_output_and_time(
        "../../tests/cases/builtin_types/option_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "Option<T> basic test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Some: ", 
                "Should show Some variant");
            INTEGRATION_ASSERT_CONTAINS(output, "42", 
                "Should show Some value");
            INTEGRATION_ASSERT_CONTAINS(output, "None (expected)", 
                "Should show None variant");
            INTEGRATION_ASSERT_CONTAINS(output, "Option<T> builtin test passed!", 
                "Should show completion message");
        },
        execution_time_1
    );
    integration_test_passed_with_time(
        "Builtin Types", 
        "option_basic.cb", 
        execution_time_1
    );
    
    // Test 2: Result<T, E> basic usage
    double execution_time_2;
    run_cb_test_with_output_and_time(
        "../../tests/cases/builtin_types/result_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "Result<T, E> basic test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Ok: ", 
                "Should show Ok variant");
            INTEGRATION_ASSERT_CONTAINS(output, "100", 
                "Should show Ok value");
            INTEGRATION_ASSERT_CONTAINS(output, "Err: ", 
                "Should show Err variant");
            INTEGRATION_ASSERT_CONTAINS(output, "Result<T, E> builtin test passed!", 
                "Should show completion message");
        },
        execution_time_2
    );
    integration_test_passed_with_time(
        "Builtin Types", 
        "result_basic.cb", 
        execution_time_2
    );
    
    // Test 3: Error case - Cannot redefine Option
    double execution_time_3;
    run_cb_test_with_output_and_time(
        "../../tests/cases/builtin_types/error_redefine_option.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, 
                "Redefining Option should fail with error");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot redefine builtin type", 
                "Should show redefinition error");
            INTEGRATION_ASSERT_CONTAINS(output, "Option", 
                "Should mention Option type");
        },
        execution_time_3
    );
    integration_test_passed_with_time(
        "Builtin Types", 
        "error_redefine_option.cb (error case)", 
        execution_time_3
    );
    
    // Test 4: Error case - Cannot redefine Result
    double execution_time_4;
    run_cb_test_with_output_and_time(
        "../../tests/cases/builtin_types/error_redefine_result.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, 
                "Redefining Result should fail with error");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot redefine builtin type", 
                "Should show redefinition error");
            INTEGRATION_ASSERT_CONTAINS(output, "Result", 
                "Should mention Result type");
        },
        execution_time_4
    );
    integration_test_passed_with_time(
        "Builtin Types", 
        "error_redefine_result.cb (error case)", 
        execution_time_4
    );
    
    std::cout << "[integration-test] Builtin Types tests completed" << std::endl;
    std::cout << "[integration-test] ✅ PASS: Builtin Types Tests (4 tests)" << std::endl;
}
