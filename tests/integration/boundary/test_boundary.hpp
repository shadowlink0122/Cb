#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_boundary() {
    // tiny境界値テスト
    run_cb_test_with_output("../../tests/cases/boundary/tiny/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Tiny boundary ok should succeed");
        });
    integration_test_passed("boundary tiny ok test");

    run_cb_test_with_output("../../tests/cases/boundary/tiny/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for tiny boundary overflow");
        });
    integration_test_passed_with_error("boundary tiny ng test");

    run_cb_test_with_output("../../tests/cases/boundary/tiny/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for tiny negative boundary overflow");
        });
    integration_test_passed_with_error("boundary tiny ng_neg test");

    // short境界値テスト
    run_cb_test_with_output("../../tests/cases/boundary/short/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Short boundary ok should succeed");
        });
    integration_test_passed("boundary short ok test");

    run_cb_test_with_output("../../tests/cases/boundary/short/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for short boundary overflow");
        });
    integration_test_passed_with_error("boundary short ng test");

    run_cb_test_with_output("../../tests/cases/boundary/short/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for short negative boundary overflow");
        });
    integration_test_passed_with_error("boundary short ng_neg test");

    // int境界値テスト
    run_cb_test_with_output("../../tests/cases/boundary/int/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Int boundary ok should succeed");
        });
    integration_test_passed("boundary int ok test");

    run_cb_test_with_output("../../tests/cases/boundary/int/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for int boundary overflow");
        });
    integration_test_passed_with_error("boundary int ng test");

    run_cb_test_with_output("../../tests/cases/boundary/int/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for int negative boundary overflow");
        });
    integration_test_passed_with_error("boundary int ng_neg test");

    // long境界値テスト
    run_cb_test_with_output("../../tests/cases/boundary/long/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Long boundary ok should succeed");
        });
    integration_test_passed("boundary long ok test");

    run_cb_test_with_output("../../tests/cases/boundary/long/ng.cb", 
        [](const std::string& output, int exit_code) {
            // long型のオーバーフローは負の値になることがある
            INTEGRATION_ASSERT(exit_code == 0, "Long boundary ng should succeed with overflow to negative");
        });
    integration_test_passed_with_overflow("boundary long ng test");
}
