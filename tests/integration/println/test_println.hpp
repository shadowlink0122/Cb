#pragma once
#include "../framework/integration_test_framework.hpp"

void test_println_empty() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/println/empty.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "println empty test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "Before\nAfter", "Expected output not found");
    std::cout << "[integration] println empty test ... passed" << std::endl;
}

void test_println_single_arg() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/println/single_arg.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "println single arg test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "AB\nC", "Expected output not found");
    std::cout << "[integration] println single argument test ... passed" << std::endl;
}

void test_println_printf_style() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/println/printf_style.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "println printf style test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "Number: 42", "Expected number output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "String: Hello", "Expected string output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "Multiple: 123, World", "Expected multiple args output not found");
    std::cout << "[integration] println printf style test ... passed" << std::endl;
}

void test_println_mixed() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/println/mixed_output.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "println mixed test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "ABC\nD\nE\nF", "Expected mixed output not found");
    std::cout << "[integration] println mixed output test ... passed" << std::endl;
}

void test_integration_println() {
    test_println_empty();
    test_println_single_arg();
    test_println_printf_style();
    test_println_mixed();
}
