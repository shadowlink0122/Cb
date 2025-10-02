#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_unsigned() {
    run_cb_test_with_output_and_time_auto(
        "../../tests/cases/unsigned/runtime_clamp.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0,
                               "Unsigned runtime clamp sample should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "global=0",
                                        "Global unsigned initialization should clamp to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "local_init=0",
                                        "Local unsigned initialization should clamp to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "local_assign=0",
                                        "Unsigned reassignment should clamp negative values to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "param=0",
                                        "Unsigned parameter should clamp negative argument to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "positive=15",
                                        "Positive unsigned values should remain unchanged");
            INTEGRATION_ASSERT_CONTAINS(output, "array_init=1,0,3,0",
                                        "Unsigned array literal elements should clamp each negative value to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "array_assign=0",
                                        "Unsigned array element assignment should clamp negative value to 0");
        });
    integration_test_passed_with_time_auto("unsigned runtime clamp test");

    run_cb_test_with_output_and_time_auto(
        "../../tests/cases/unsigned/struct_interface.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0,
                               "Unsigned struct/interface sample should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "struct_init=0",
                                        "Unsigned struct literal should clamp negative initializer to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "struct_assign=0",
                                        "Unsigned struct member assignment should clamp to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "func_param=0",
                                        "Unsigned function parameter should clamp negative argument");
            INTEGRATION_ASSERT_CONTAINS(output, "func_return=0",
                                        "Unsigned function return should clamp negative literal");
            INTEGRATION_ASSERT_CONTAINS(output, "func_return_param=0",
                                        "Unsigned function return via parameter should remain clamped");
            INTEGRATION_ASSERT_CONTAINS(output, "interface_value=0",
                                        "Interface call should reflect clamped struct member");
            INTEGRATION_ASSERT_CONTAINS(output, "interface_negative_return=0",
                                        "Interface method returning negative literal should clamp to 0");
            INTEGRATION_ASSERT_CONTAINS(output, "struct_positive=42",
                                        "Positive struct member assignment should pass through");
            INTEGRATION_ASSERT_CONTAINS(output, "interface_positive=42",
                                        "Interface call should report updated positive value");
        });
    integration_test_passed_with_time_auto(
        "unsigned struct/interface coverage test");

    run_cb_test_with_output_and_time_auto(
        "../../tests/cases/unsigned/boundary_ok.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0,
                               "Unsigned boundary ok should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Unsigned boundary test:",
                                        "Expected unsigned boundary header");
            INTEGRATION_ASSERT_CONTAINS(output, "ut (max unsigned tiny): 255",
                                        "Expected unsigned tiny max value");
            INTEGRATION_ASSERT_CONTAINS(output, "us (max unsigned short): 65535",
                                        "Expected unsigned short max value");
            INTEGRATION_ASSERT_CONTAINS(output, "ui (max unsigned int): 4294967295",
                                        "Expected unsigned int max value");
            INTEGRATION_ASSERT_CONTAINS(output, "ul (max unsigned long): 9223372036854775807",
                                        "Expected unsigned long max value");
            INTEGRATION_ASSERT_CONTAINS(output, "Unsigned boundary test passed",
                                        "Expected unsigned boundary success message");
        });
    integration_test_passed_with_time_auto("unsigned boundary ok test");

    auto overflow_assert = [](const std::string &output, int exit_code,
                              const char *label) {
        INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") ||
                               contains(output, "Value out of range"),
                           label);
    };

    run_cb_test_with_output_and_time_auto(
        "../../tests/cases/unsigned/boundary_overflow_tiny.cb",
        [&](const std::string &output, int exit_code) {
            overflow_assert(output, exit_code,
                            "Unsigned tiny overflow should fail");
        });
    integration_test_passed_with_error_and_time_auto(
        "unsigned tiny overflow test");

    run_cb_test_with_output_and_time_auto(
        "../../tests/cases/unsigned/boundary_overflow_short.cb",
        [&](const std::string &output, int exit_code) {
            overflow_assert(output, exit_code,
                            "Unsigned short overflow should fail");
        });
    integration_test_passed_with_error_and_time_auto(
        "unsigned short overflow test");

    run_cb_test_with_output_and_time_auto(
        "../../tests/cases/unsigned/boundary_overflow_int.cb",
        [&](const std::string &output, int exit_code) {
            overflow_assert(output, exit_code,
                            "Unsigned int overflow should fail");
        });
    integration_test_passed_with_error_and_time_auto(
        "unsigned int overflow test");

    run_cb_test_with_output_and_time_auto(
        "../../tests/cases/unsigned/boundary_overflow_long.cb",
        [&](const std::string &output, int exit_code) {
            overflow_assert(output, exit_code,
                            "Unsigned long overflow should fail");
        });
    integration_test_passed_with_error_and_time_auto(
        "unsigned long overflow test");
}
