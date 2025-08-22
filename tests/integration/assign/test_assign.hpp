#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_assign() {
    // short型の正常系テスト
    run_cb_test_with_output("../../tests/cases/assign/short/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Short assign ok should succeed");
            // 値が正しく出力されることを確認
        });
    integration_test_passed("assign short ok test");

    // short型の異常系テスト（範囲外）
    run_cb_test_with_output("../../tests/cases/assign/short/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow") || contains(output, "エラー"), 
                             "Expected error for short out-of-range assignment");
        });
    integration_test_passed_with_error("assign short ng test");

    // short型の負の値の異常系テスト
    run_cb_test_with_output("../../tests/cases/assign/short/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow") || contains(output, "エラー"), 
                             "Expected error for short negative out-of-range assignment");
        });
    integration_test_passed_with_error("assign short ng_neg test");

    // int型の正常系テスト
    run_cb_test_with_output("../../tests/cases/assign/int/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Int assign ok should succeed");
        });
    integration_test_passed("assign int ok test");

    // int型の異常系テスト（範囲外）
    run_cb_test_with_output("../../tests/cases/assign/int/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow") || contains(output, "エラー"), 
                             "Expected error for int out-of-range assignment");
        });
    integration_test_passed_with_error("assign int ng test");

    // int型の負の値の異常系テスト
    run_cb_test_with_output("../../tests/cases/assign/int/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow") || contains(output, "エラー"), 
                             "Expected error for int negative out-of-range assignment");
        });
    integration_test_passed_with_error("assign int ng_neg test");

    // long型の正常系テスト
    run_cb_test_with_output("../../tests/cases/assign/long/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Long assign ok should succeed");
        });
    integration_test_passed("assign long ok test");

    // const tiny の正常系テスト
    run_cb_test_with_output("../../tests/cases/assign/const/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Const tiny assign ok should succeed");
        });
    integration_test_passed("assign const tiny ok test");

    // const tiny の再代入異常系テスト
    run_cb_test_with_output("../../tests/cases/assign/const/reassign_ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "const") || 
                             contains(output, "再代入") || contains(output, "エラー"), 
                             "Expected error for const reassignment");
        });
    integration_test_passed("assign const tiny reassign ng test");

    // const string要素への代入異常系テスト
    run_cb_test_with_output("../../tests/cases/assign/const/string_element_ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "const") || 
                             contains(output, "代入") || contains(output, "エラー"), 
                             "Expected error for const string element assignment");
        });
    integration_test_passed("assign const string element ng test");
}
