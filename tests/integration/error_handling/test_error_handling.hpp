#ifndef TEST_ERROR_HANDLING_HPP
#define TEST_ERROR_HANDLING_HPP

#include "../framework/integration_test_framework.hpp"

inline void test_error_handling_basic() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/error_handling/basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Error Handling Basics ===",
                "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "safe_deref null: Err",
                "Nullptr deref must be reported");
            INTEGRATION_ASSERT_CONTAINS(output, "sum_checked_prefix oob: Err",
                "checked expression must flag OOB");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Error Handling Basics Passed ===",
                "Should print success footer");
        });
    integration_test_passed_with_time_auto("error_handling_basic", "basic.cb");
}

inline void test_runtime_error_enum() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/error_handling/runtime_error_enum.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "runtime_error_enum.cb should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "RuntimeError enum smoke test",
                "Should describe the scenario");
            INTEGRATION_ASSERT_CONTAINS(output, "NullPointerError ->",
                "Should print NullPointerError variant");
            INTEGRATION_ASSERT_CONTAINS(output, "DivisionByZeroError ->",
                "Should print DivisionByZeroError variant");
        });
    integration_test_passed_with_time_auto("runtime_error_enum", "runtime_error_enum.cb");
}

inline void test_try_checked_suite() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/error_handling/try_checked.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "try_checked.cb should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "safe_divide err: Err",
                "Division by zero must be Err");
            INTEGRATION_ASSERT_CONTAINS(output, "safe_index err: Err",
                "Out-of-bounds access must be Err");
            INTEGRATION_ASSERT_CONTAINS(output, "try & checked expression tests passed",
                "Should print suite completion message");
        });
    integration_test_passed_with_time_auto("try_checked_suite", "try_checked.cb");
}

inline void test_integration_error_handling() {
    std::cout << "[integration-test] Running error_handling tests..." << std::endl;
    test_error_handling_basic();
    test_runtime_error_enum();
    test_try_checked_suite();
    std::cout << "[integration-test] Error_handling tests completed" << std::endl;
}

#endif // TEST_ERROR_HANDLING_HPP
