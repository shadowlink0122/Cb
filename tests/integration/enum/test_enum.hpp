#ifndef TEST_ENUM_HPP
#define TEST_ENUM_HPP

#include "../framework/integration_test_framework.hpp"

void test_integration_enum() {
    std::cout << "[integration] Running enum tests..." << std::endl;

    // Basic enum functionality
    run_cb_test_with_output("../cases/enum/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
        });
    integration_test_passed("basic enum functionality", "basic.cb");

    // Explicit values
    run_cb_test_with_output("../cases/enum/explicit_values.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "explicit_values.cb should execute successfully");
        });
    integration_test_passed("enum with explicit integer values", "explicit_values.cb");

    // Negative values
    run_cb_test_with_output("../cases/enum/negative_values.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "negative_values.cb should execute successfully");
        });
    integration_test_passed("enum with negative integer values", "negative_values.cb");

    // Large values
    run_cb_test_with_output("../cases/enum/large_values.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "large_values.cb should execute successfully");
        });
    integration_test_passed("enum with large integer values", "large_values.cb");

    // Array indexing with enums
    run_cb_test_with_output("../cases/enum/array_index.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "array_index.cb should execute successfully");
        });
    integration_test_passed("using enum values as array indices", "array_index.cb");

    // String array indexing
    run_cb_test_with_output("../cases/enum/string_array_index.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "string_array_index.cb should execute successfully");
        });
    integration_test_passed("using enum values with string arrays", "string_array_index.cb");

    // Arithmetic operations
    run_cb_test_with_output("../cases/enum/arithmetic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "arithmetic.cb should execute successfully");
        });
    integration_test_passed("arithmetic operations with enum values", "arithmetic.cb");

    // Conditional usage
    run_cb_test_with_output("../cases/enum/conditional.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "conditional.cb should execute successfully");
        });
    integration_test_passed("using enum values in conditional statements", "conditional.cb");

    // Multiple enums
    run_cb_test_with_output("../cases/enum/multiple_enums.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_enums.cb should execute successfully");
        });
    integration_test_passed("defining and using multiple enums", "multiple_enums.cb");

    // Error cases - duplicate values (should fail)
    run_cb_test_with_output("../cases/enum/error_duplicate_values.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "error_duplicate_values.cb should fail due to duplicate enum values");
        });
    integration_test_passed_with_error("error case: duplicate enum values should be rejected", "error_duplicate_values.cb");

    // Error cases - undefined member (should fail)
    run_cb_test_with_output("../cases/enum/error_undefined_member.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "error_undefined_member.cb should fail due to undefined enum member");
        });
    integration_test_passed_with_error("error case: accessing undefined enum member should fail", "error_undefined_member.cb");

    // Error cases - undefined enum (should fail)
    run_cb_test_with_output("../cases/enum/error_undefined_enum.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "error_undefined_enum.cb should fail due to undefined enum");
        });
    integration_test_passed_with_error("error case: accessing undefined enum should fail", "error_undefined_enum.cb");

    std::cout << "[integration] Enum tests completed" << std::endl;
}

#endif // TEST_ENUM_HPP
