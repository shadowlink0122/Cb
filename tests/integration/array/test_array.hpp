#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_array() {
    const std::string test_file_basic = "../../tests/cases/array/basic.cb";
    const std::string test_file_assign = "../../tests/cases/array/assign.cb";
    const std::string test_file_boundary = "../../tests/cases/array/boundary.cb";
    const std::string test_file_literal = "../../tests/cases/array/literal.cb";
    
    // 基本的な配列テスト
    double execution_time_basic;
    run_cb_test_with_output_and_time(test_file_basic, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array basic test");
            INTEGRATION_ASSERT_CONTAINS(output, "Array basic test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a[0] = 10", "Expected a[0] = 10 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a[4] = 50", "Expected a[4] = 50 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "After modification a[2] = 99", "Expected modified value in output");
            INTEGRATION_ASSERT_CONTAINS(output, "sum = 219", "Expected sum = 219 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Array basic test passed", "Expected success message in output");
        }, execution_time_basic);
    integration_test_passed_with_time("array basic test", test_file_basic, execution_time_basic);
    
    // 配列代入テスト
    double execution_time_assign;
    run_cb_test_with_output_and_time(test_file_assign, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array assign test");
            INTEGRATION_ASSERT_CONTAINS(output, "Array assignment test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a[0] = 42", "Expected a[0] = 42 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a[1] = 43", "Expected a[1] = 43 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a[2] = 86", "Expected a[2] = 86 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a[3] = 76", "Expected a[3] = 76 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Array assignment test passed", "Expected success message in output");
        }, execution_time_assign);
    integration_test_passed_with_time("array assign test", test_file_assign, execution_time_assign);
    
    // 配列境界テスト (with timing)
    double execution_time_boundary;
    run_cb_test_with_output_and_time(test_file_boundary, 
        [](const std::string& output, int exit_code) {
            // 境界テストでエラーが期待される場合もある
            if (exit_code != 0) {
                INTEGRATION_ASSERT(contains(output, "bounds") || contains(output, "境界") || 
                                 contains(output, "エラー"), 
                                 "Expected boundary error message");
            }
        }, execution_time_boundary);
    if (true) { // 境界エラーが期待される場合
        integration_test_passed_with_error_and_time("array boundary test", test_file_boundary, execution_time_boundary);
    } else {
        integration_test_passed_with_time("array boundary test", test_file_boundary, execution_time_boundary);
    }
    
    // 配列リテラルテスト (with timing)
    double execution_time_literal;
    run_cb_test_with_output_and_time(test_file_literal, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array literal test");
        }, execution_time_literal);
    integration_test_passed_with_time("array literal test", test_file_literal, execution_time_literal);
}
