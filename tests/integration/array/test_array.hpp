#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_array() {
    const std::string test_file_basic = "../../tests/cases/array/basic.cb";
    const std::string test_file_assign = "../../tests/cases/array/assign.cb";
    const std::string test_file_boundary = "../../tests/cases/array/boundary.cb";
    const std::string test_file_literal = "../../tests/cases/array/literal.cb";
    
    // 基本的な配列テスト
    run_cb_test_with_output(test_file_basic, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array basic test");
            // 出力に関する具体的なチェックが必要に応じて追加可能
        });
    integration_test_passed("array basic test", test_file_basic);
    
    // 配列代入テスト
    run_cb_test_with_output(test_file_assign, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array assign test");
        });
    integration_test_passed("array assign test", test_file_assign);
    
    // 配列境界テスト
    run_cb_test_with_output(test_file_boundary, 
        [](const std::string& output, int exit_code) {
            // 境界テストでエラーが期待される場合もある
            if (exit_code != 0) {
                INTEGRATION_ASSERT(contains(output, "bounds") || contains(output, "境界") || 
                                 contains(output, "エラー"), 
                                 "Expected boundary error message");
            }
        });
    if (true) { // 境界エラーが期待される場合
        integration_test_passed_with_error("array boundary test", test_file_boundary);
    } else {
        integration_test_passed("array boundary test", test_file_boundary);
    }
    
    // 配列リテラルテスト
    run_cb_test_with_output(test_file_literal, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array literal test");
        });
    integration_test_passed("array literal test", test_file_literal);
}
