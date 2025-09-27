#ifndef TEST_ENUM_HPP
#define TEST_ENUM_HPP

#include "../framework/integration_test_framework.hpp"

void test_integration_enum() {
    std::cout << "[integration] Running enum tests..." << std::endl;

    // Basic enum functionality (with timing)
    double execution_time_basic;
    run_cb_test_with_output_and_time("../cases/enum/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic.cb should execute successfully");
        }, execution_time_basic);
    integration_test_passed_with_time("basic enum functionality", "basic.cb", execution_time_basic);

    // Explicit values (with timing)
    double execution_time_explicit;
    run_cb_test_with_output_and_time("../cases/enum/explicit_values.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "explicit_values.cb should execute successfully");
        }, execution_time_explicit);
    integration_test_passed_with_time("enum with explicit integer values", "explicit_values.cb", execution_time_explicit);

    // Negative values (with timing)
    double execution_time_negative;
    run_cb_test_with_output_and_time("../cases/enum/negative_values.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "negative_values.cb should execute successfully");
        }, execution_time_negative);
    integration_test_passed_with_time("enum with negative integer values", "negative_values.cb", execution_time_negative);

    // Large values (with timing)
    double execution_time_large;
    run_cb_test_with_output_and_time("../cases/enum/large_values.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "large_values.cb should execute successfully");
        }, execution_time_large);
    integration_test_passed_with_time("enum with large integer values", "large_values.cb", execution_time_large);

    // Array indexing with enums (with timing)
    double execution_time_array;
    run_cb_test_with_output_and_time("../cases/enum/array_index.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "array_index.cb should execute successfully");
        }, execution_time_array);
    integration_test_passed_with_time("using enum values as array indices", "array_index.cb", execution_time_array);

    // String array indexing (with timing)
    double execution_time_string_array;
    run_cb_test_with_output_and_time("../cases/enum/string_array_index.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "string_array_index.cb should execute successfully");
        }, execution_time_string_array);
    integration_test_passed_with_time("using enum values with string arrays", "string_array_index.cb", execution_time_string_array);

    // Arithmetic operations
    run_cb_test_with_output_and_time_auto("../cases/enum/arithmetic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "arithmetic.cb should execute successfully");
        });
    integration_test_passed_with_time_auto("arithmetic operations with enum values", "arithmetic.cb");

    // Conditional usage
    run_cb_test_with_output_and_time_auto("../cases/enum/conditional.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "conditional.cb should execute successfully");
        });
    integration_test_passed_with_time_auto("using enum values in conditional statements", "conditional.cb");

    // Multiple enums
    run_cb_test_with_output_and_time_auto("../cases/enum/multiple_enums.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_enums.cb should execute successfully");
        });
    integration_test_passed_with_time_auto("defining and using multiple enums", "multiple_enums.cb");

    // Error cases - duplicate values (should fail)
    run_cb_test_with_output_and_time_auto("../cases/enum/error_duplicate_values.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "error_duplicate_values.cb should fail due to duplicate enum values");
        });
    integration_test_passed_with_error_and_time_auto("error case: duplicate enum values should be rejected", "error_duplicate_values.cb");

    // Error cases - undefined member (should fail)
    run_cb_test_with_output_and_time_auto("../cases/enum/error_undefined_member.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "error_undefined_member.cb should fail due to undefined enum member");
        });
    integration_test_passed_with_error_and_time_auto("error case: accessing undefined enum member should fail", "error_undefined_member.cb");

    // Error cases - undefined enum (should fail)
    run_cb_test_with_output_and_time_auto("../cases/enum/error_undefined_enum.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "error_undefined_enum.cb should fail due to undefined enum");
        });
    integration_test_passed_with_error_and_time_auto("error case: accessing undefined enum should fail", "error_undefined_enum.cb");

    std::cout << "[integration] Enum tests completed" << std::endl;
}

#endif // TEST_ENUM_HPP
