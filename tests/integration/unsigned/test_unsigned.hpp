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
}
