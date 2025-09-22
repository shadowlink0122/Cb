#pragma once
#include "../framework/integration_test_framework.hpp"

void test_global_array_basic() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/global_array/basic.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "global array basic test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "global_array[0] = 10", "Expected output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "global_array[1] = 20", "Expected output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "global_array[2] = 30", "Expected output not found");
    std::cout << "[integration] global array basic test ... passed" << std::endl;
}

void test_global_array_types() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/global_array/types.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "global array types test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "numbers[0] = 100", "Expected output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "numbers[1] = 200", "Expected output not found");
    std::cout << "[integration] global array types test ... passed" << std::endl;
}

void test_global_array_function_access() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/global_array/function_access.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "global array function access test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "Sum of squares 0-9: 285", "Expected output not found");
    std::cout << "[integration] global array function access test ... passed" << std::endl;
}

void test_integration_global_array() {
    test_global_array_basic();
    test_global_array_types();
    test_global_array_function_access();
}
