#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_cross_type() {
    // 正常系：型変換が正しく動作する
    run_cb_test_with_output_and_time_auto("../../tests/cases/cross_type/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Cross type ok should succeed");
        });
    integration_test_passed_with_time_auto("cross_type ok test");

    // 異常系：型変換でエラーが発生する
    run_cb_test_with_output_and_time_auto("../../tests/cases/cross_type/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "型変換") || contains(output, "エラー"), 
                             "Expected error for invalid cross type conversion");
        });
    integration_test_passed_with_error_and_time_auto("cross_type ng test");
}
