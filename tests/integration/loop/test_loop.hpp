#pragma once

#include "../framework/integration_test_framework.hpp"

void test_break_continue() {
    run_cb_test_with_output("../../tests/cases/loop/break_continue.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Break/continue test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Before break", "Should print before break");
            INTEGRATION_ASSERT_CONTAINS(output, "After break", "Should print after break");
            INTEGRATION_ASSERT_CONTAINS(output, "Before continue", "Should print before continue");
            INTEGRATION_ASSERT_CONTAINS(output, "After continue", "Should print after continue");
            INTEGRATION_ASSERT_CONTAINS(output, "Loop completed", "Should complete loop");
        });
    integration_test_passed("test_break_continue", "break_continue.cb");
}

void test_nested_loop_break() {
    run_cb_test_with_output("../../tests/cases/loop/nested_break.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Nested loop break test should execute successfully");
        });
    integration_test_passed("test_nested_loop_break", "nested_break.cb");
}

void test_while_loop_continue() {
    run_cb_test_with_output("../../tests/cases/loop/while_continue.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "While loop continue test should execute successfully");
        });
    integration_test_passed("test_while_loop_continue", "while_continue.cb");
}

inline void test_integration_loop() {
    std::cout << "[integration] Running loop tests..." << std::endl;
    test_break_continue();
    test_nested_loop_break();
    test_while_loop_continue();
    std::cout << "[integration] Loop tests completed" << std::endl;
}
