#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_boundary() {
    // tiny境界値テスト (with timing)
    double execution_time_tiny_ok;
    run_cb_test_with_output_and_time("../../tests/cases/boundary/tiny/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Tiny boundary ok should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Tiny boundary test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "t1 (max tiny): 127", "Expected max tiny value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "t2 (min tiny): -128", "Expected min tiny value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Tiny boundary test passed", "Expected success message in output");
        }, execution_time_tiny_ok);
    integration_test_passed_with_time("boundary tiny ok test", "tiny/ok.cb", execution_time_tiny_ok);

    double execution_time_tiny_ng;
    run_cb_test_with_output_and_time("../../tests/cases/boundary/tiny/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for tiny boundary overflow");
        }, execution_time_tiny_ng);
    integration_test_passed_with_error_and_time("boundary tiny ng test", "tiny/ng.cb", execution_time_tiny_ng);

    double execution_time_tiny_ng_neg;
    run_cb_test_with_output_and_time("../../tests/cases/boundary/tiny/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for tiny negative boundary overflow");
        }, execution_time_tiny_ng_neg);
    integration_test_passed_with_error_and_time("boundary tiny ng_neg test", "tiny/ng_neg.cb", execution_time_tiny_ng_neg);

    // short境界値テスト (with timing)
    double execution_time_short_ok;
    run_cb_test_with_output_and_time("../../tests/cases/boundary/short/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Short boundary ok should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Short boundary test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "s1 (max short): 32767", "Expected max short value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "s2 (min short): -32768", "Expected min short value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Short boundary test passed", "Expected success message in output");
        }, execution_time_short_ok);
    integration_test_passed_with_time("boundary short ok test", "short/ok.cb", execution_time_short_ok);

    run_cb_test_with_output_and_time_auto("../../tests/cases/boundary/short/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for short boundary overflow");
        });
    integration_test_passed_with_error_and_time_auto("boundary short ng test");

    run_cb_test_with_output_and_time_auto("../../tests/cases/boundary/short/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for short negative boundary overflow");
        });
    integration_test_passed_with_error_and_time_auto("boundary short ng_neg test");

    // int境界値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/boundary/int/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Int boundary ok should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Integer boundary test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "i1 (max int): 2147483647", "Expected max int value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "i2 (min int): -2147483648", "Expected min int value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Integer boundary test passed", "Expected success message in output");
        });
    integration_test_passed_with_time_auto("boundary int ok test");

    run_cb_test_with_output_and_time_auto("../../tests/cases/boundary/int/ng.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for int boundary overflow");
        });
    integration_test_passed_with_error_and_time_auto("boundary int ng test");

    run_cb_test_with_output_and_time_auto("../../tests/cases/boundary/int/ng_neg.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0 || contains(output, "型の範囲外") || 
                             contains(output, "overflow"), "Expected error for int negative boundary overflow");
        });
    integration_test_passed_with_error_and_time_auto("boundary int ng_neg test");

    // long境界値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/boundary/long/ok.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Long boundary ok should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Long boundary test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "l1 (max long): 9223372036854775807", "Expected max long value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "l2 (min long): -9223372036854775808", "Expected min long value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Long boundary test passed", "Expected success message in output");
        });
    integration_test_passed_with_time_auto("boundary long ok test");

    run_cb_test_with_output_and_time_auto("../../tests/cases/boundary/long/ng.cb", 
        [](const std::string& output, int exit_code) {
            // long型のオーバーフローは負の値になることがある
            INTEGRATION_ASSERT(exit_code == 0, "Long boundary ng should succeed with overflow to negative");
        });
    integration_test_passed_with_overflow_and_time_auto("boundary long ng test");
}
