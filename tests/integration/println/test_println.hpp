#pragma once
#include "../framework/integration_test_framework.hpp"

void test_println_empty() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/println/empty.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "println empty test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "Before\nAfter", "Expected output not found");
        });
    integration_test_passed_with_time_auto("println empty test", "empty.cb");
}

void test_println_single_arg() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/println/single_arg.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "println single arg test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "AB\nC", "Expected output not found");
        });
    integration_test_passed_with_time_auto("println single argument test", "single_arg.cb");
}

void test_println_printf_style() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/println/printf_style.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "println printf style test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "Number: 42", "Expected number output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "String: Hello", "Expected string output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple: 123, World", "Expected multiple args output not found");
        });
    integration_test_passed_with_time_auto("println printf style test", "printf_style.cb");
}

void test_println_mixed() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/println/mixed_output.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "println mixed test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "ABC\nD\nE\nF", "Expected mixed output not found");
        });
    integration_test_passed_with_time_auto("println mixed output test", "mixed_output.cb");
}

void test_integration_println() {
    test_println_empty();
    test_println_single_arg();
    test_println_printf_style();
    test_println_mixed();
}
