#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_arithmetic() {
    const std::string test_file_ok = "../../tests/cases/arithmetic/ok.cb";
    const std::string test_file_ng = "../../tests/cases/arithmetic/ng.cb";
    
    // 正常系テスト (with timing)
    double execution_time_ok;
    run_cb_test_with_output_and_time(test_file_ok, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "15", "Expected '15' in arithmetic ok output");
            INTEGRATION_ASSERT_CONTAINS(output, "5", "Expected '5' in arithmetic ok output");
            INTEGRATION_ASSERT_CONTAINS(output, "30000", "Expected '30000' in arithmetic ok output");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for arithmetic ok test");
        }, execution_time_ok);
    integration_test_passed_with_time("arithmetic ok test", test_file_ok, execution_time_ok);

    // 異常系テスト - tiny型範囲外 (with timing)
    double execution_time_ng;
    run_cb_test_with_output_and_time(test_file_ng, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(contains(output, "型範囲外") || contains(output, "overflow") || 
                             contains(output, "エラー") || exit_code != 0, 
                             "Expected error for out-of-range arithmetic");
        }, execution_time_ng);
    integration_test_passed_with_error_and_time("arithmetic ng test", test_file_ng, execution_time_ng);
}
