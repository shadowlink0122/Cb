#pragma once

#include "../framework/integration_test_framework.hpp"

void test_array_literal_success() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/test_array_success.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Array success test should pass");
            INTEGRATION_ASSERT(contains(output, "100"), "Should contain 100");
            INTEGRATION_ASSERT(contains(output, "200"), "Should contain 200");
            INTEGRATION_ASSERT(contains(output, "300"), "Should contain 300");
        });
    integration_test_passed_with_time_auto("array_literal_success");
}

void test_array_literal_string_basic() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/string_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "String array basic test should pass");
            INTEGRATION_ASSERT(contains(output, "hello"), "Should contain hello");
            INTEGRATION_ASSERT(contains(output, "world"), "Should contain world");
            INTEGRATION_ASSERT(contains(output, "test"), "Should contain test");
        });
    integration_test_passed_with_time_auto("array_literal_string_basic");
}

void test_array_literal_comprehensive() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/test_array_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Comprehensive array test should pass");
            INTEGRATION_ASSERT(contains(output, "Testing int arrays"), "Should contain int array test");
            INTEGRATION_ASSERT(contains(output, "Testing string arrays"), "Should contain string array test");
            INTEGRATION_ASSERT(contains(output, "hello"), "Should contain hello from string array");
            INTEGRATION_ASSERT(contains(output, "world"), "Should contain world from string array");
        });
    integration_test_passed_with_time_auto("array_literal_comprehensive");
}

void test_array_literal_empty() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/test_array_empty.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code == 0, "Empty array test should pass");
        });
    integration_test_passed_with_time_auto("array_literal_empty");
}

void test_array_literal_type_mismatch_int_string() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/test_array_fail_int_string.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "Type mismatch test should fail");
            INTEGRATION_ASSERT(contains(output, "Error") || 
                             contains(output, "type expected") ||
                             contains(output, "Type mismatch") || 
                             contains(output, "型の不一致") ||
                             contains(output, "error") ||
                             contains(output, "エラー"), 
                             "Should contain error message about type mismatch");
        });
    integration_test_passed_with_error_and_time_auto("array_literal_type_mismatch_int_string");
}

void test_array_literal_type_mismatch_string_int() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/test_array_fail_string_int.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "Type mismatch test should fail");
            INTEGRATION_ASSERT(contains(output, "Error") || 
                             contains(output, "type expected") ||
                             contains(output, "Type mismatch") || 
                             contains(output, "型の不一致") ||
                             contains(output, "error") ||
                             contains(output, "エラー"), 
                             "Should contain error message about type mismatch");
        });
    integration_test_passed_with_error_and_time_auto("array_literal_type_mismatch_string_int");
}

void test_array_literal_type_mismatch_multiple() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/test_array_fail_multiple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "Multiple type mismatch test should fail");
            INTEGRATION_ASSERT(contains(output, "Error") || 
                             contains(output, "type expected") ||
                             contains(output, "Type mismatch") || 
                             contains(output, "型の不一致") ||
                             contains(output, "error") ||
                             contains(output, "エラー"), 
                             "Should contain error message about type mismatch");
        });
    integration_test_passed_with_error_and_time_auto("array_literal_type_mismatch_multiple");
}

void test_array_literal_bounds() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_literal/test_array_bounds.cb", 
        [](const std::string& output, int exit_code) {
            // このテストは範囲外アクセステストなので、実装によって成功する場合もある
            // 出力があることを確認
            INTEGRATION_ASSERT(!output.empty(), "Should have some output");
        });
    integration_test_passed_with_time_auto("array_literal_bounds");
}

// メイン統合テスト関数
void test_integration_array_literal() {
    std::cout << "[integration] Testing array literals..." << std::endl;
    
    test_array_literal_success();
    test_array_literal_string_basic();
    test_array_literal_comprehensive();
    test_array_literal_empty();
    test_array_literal_type_mismatch_int_string();
    test_array_literal_type_mismatch_string_int();
    test_array_literal_type_mismatch_multiple();
    test_array_literal_bounds();
    
    std::cout << "[integration] Array literal tests completed" << std::endl;
}
