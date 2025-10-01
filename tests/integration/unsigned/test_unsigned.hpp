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
}
