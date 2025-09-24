#ifndef TEST_TYPEDEF_HPP
#define TEST_TYPEDEF_HPP

#include "../framework/integration_test_framework.hpp"

void test_typedef_basic() {
    run_cb_test_with_output("../../tests/cases/typedef/test_basic_typedef.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Typedef integration test completed!", 
                                      "Test should complete successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "int number = 42", 
                                      "Typedef variables should work");
        });
    integration_test_passed("test_typedef_basic", "test_basic_typedef.cb");
}

void test_typedef_advanced() {
    run_cb_test_with_output("../../tests/cases/typedef/test_advanced_typedef.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Advanced typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Advanced typedef test completed!", 
                                      "Test should complete successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Processing: 0 with count: 10", 
                                      "Typedef function parameters should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Total after doubling: 20", 
                                      "Typedef arithmetic operations should work");
        });
    integration_test_passed("test_typedef_advanced", "test_advanced_typedef.cb");
}

void test_typedef_array() {
    run_cb_test_with_output("../../tests/cases/typedef/test_array_typedef.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Array typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Typedef array integration test completed!", 
                                      "Test should complete successfully");
        });
    integration_test_passed("test_typedef_array", "test_array_typedef.cb");
}

void test_typedef_various() {
    // Test basic typedef functionality
    run_cb_test_with_output("../../tests/cases/typedef/basic_ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic typedef should work");
        });
    integration_test_passed("test_typedef_basic_ok", "basic_ok.cb");

    // Test nested typedef functionality  
    run_cb_test_with_output("../../tests/cases/typedef/nested_ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Nested typedef should work");
        });
    integration_test_passed("test_typedef_nested_ok", "nested_ok.cb");

    // Test function parameter typedef
    run_cb_test_with_output("../../tests/cases/typedef/function_param_ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Function parameter typedef should work");
        });
    integration_test_passed("test_typedef_function_param", "function_param_ok.cb");
}

void test_typedef_errors() {
    // Test duplicate definition error
    run_cb_test_with_output("../../tests/cases/typedef/duplicate_definition_error.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "Duplicate typedef definition should fail");
        });
    integration_test_passed("test_typedef_duplicate_error", "duplicate_definition_error.cb");

    // Test undefined type error  
    run_cb_test_with_output("../../tests/cases/typedef/undefined_type_error.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "Undefined type in typedef should fail");
        });
    integration_test_passed("test_typedef_undefined_error", "undefined_type_error.cb");
}

// Main typedef test function
void test_integration_typedef() {
    std::cout << "[integration] Running typedef tests..." << std::endl;
    test_typedef_basic();
    test_typedef_advanced(); 
    test_typedef_array();
    test_typedef_various();
    test_typedef_errors();
    std::cout << "[integration] Typedef tests completed" << std::endl;
}

#endif // TEST_TYPEDEF_HPP
