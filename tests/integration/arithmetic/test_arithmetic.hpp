#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_arithmetic() {
    const std::string test_file_ok = "../../tests/cases/arithmetic/ok.cb";
    const std::string test_file_ng = "../../tests/cases/arithmetic/ng.cb";
    
    // 正常系テスト (with timing)
    double execution_time_ok;
    run_cb_test_with_output_and_time(test_file_ok, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "Arithmetic operations test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "tiny: t1=15, t2=5, t3=6, t4=5", "Expected tiny results in output");
            INTEGRATION_ASSERT_CONTAINS(output, "short: s1=300, s2=-100, s3=60, s4=50", "Expected short results in output");
            INTEGRATION_ASSERT_CONTAINS(output, "int: i1=3000, i2=-1000, i3=600, i4=500", "Expected int results in output");
            INTEGRATION_ASSERT_CONTAINS(output, "long: l1=30000, l2=-10000, l3=6000, l4=5000", "Expected long results in output");
            INTEGRATION_ASSERT_CONTAINS(output, "All arithmetic tests passed", "Expected success message in output");
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
