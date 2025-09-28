#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_func() {
    const std::string test_file_basic = "../../tests/cases/func/integration_func.cb";
    const std::string test_file_array = "../../tests/cases/func/array_func.cb";
    const std::string test_file_simple_array = "../../tests/cases/func/simple_array_return.cb";
    const std::string test_file_static_string = "../../tests/cases/func/simple_static_string_test.cb";
    const std::string test_file_type_safety_valid = "../../tests/cases/func/array_type_safety_valid.cb";
    const std::string test_file_type_safety_error1 = "../../tests/cases/func/array_type_safety_error1.cb";
    const std::string test_file_type_safety_error2 = "../../tests/cases/func/array_type_safety_error2.cb";
    const std::string test_file_function_call_count = "../../tests/cases/func/function_call_count.cb";
    
    // 基本的な関数テスト (with timing)
    double execution_time_basic;
    run_cb_test_with_output_and_time(test_file_basic, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func basic test");
        }, execution_time_basic);
    integration_test_passed_with_time("func basic test", test_file_basic, execution_time_basic);
    
    // 配列関数テスト (with timing)
    double execution_time_array;
    run_cb_test_with_output_and_time(test_file_array, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func array test");
            INTEGRATION_ASSERT_CONTAINS(output, "Testing array functions:", "should contain test start message");
            INTEGRATION_ASSERT_CONTAINS(output, "Integer array created and assigned", "should contain integer array message");
            INTEGRATION_ASSERT_CONTAINS(output, "Array parameter received successfully", "should contain array parameter message");
            INTEGRATION_ASSERT_CONTAINS(output, "All array function tests completed successfully", "should contain completion message");
        }, execution_time_array);
    integration_test_passed_with_time("func array test", test_file_array, execution_time_array);
    
    // シンプルな配列戻り値テスト (with timing)
    double execution_time_simple_array;
    run_cb_test_with_output_and_time(test_file_simple_array, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func simple array return test");
            INTEGRATION_ASSERT_CONTAINS(output, "Testing simple array return", "should contain simple test message");
            INTEGRATION_ASSERT_CONTAINS(output, "Direct array literal assignment completed", "should contain direct assignment message");
            INTEGRATION_ASSERT_CONTAINS(output, "Function array return assignment completed", "should contain function return message");
        }, execution_time_simple_array);
    integration_test_passed_with_time("func simple array return test", test_file_simple_array, execution_time_simple_array);
    
    // 静的文字列配列テスト (with timing)
    double execution_time_static_string;
    run_cb_test_with_output_and_time(test_file_static_string, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func static string test");
            INTEGRATION_ASSERT_CONTAINS(output, "Alice", "should contain first name");
            INTEGRATION_ASSERT_CONTAINS(output, "Bob", "should contain second name");
            INTEGRATION_ASSERT_CONTAINS(output, "Charlie", "should contain third name");
        }, execution_time_static_string);
    integration_test_passed_with_time("func static string test", test_file_static_string, execution_time_static_string);
    
    // 配列型安全性（有効）テスト
    run_cb_test_with_output_and_time_auto(test_file_type_safety_valid, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func type safety valid test");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "should contain expected output");
            INTEGRATION_ASSERT_CONTAINS(output, "200", "should contain expected output");
            INTEGRATION_ASSERT_CONTAINS(output, "300", "should contain expected output");
        });
    integration_test_passed_with_time_auto("func type safety valid test", test_file_type_safety_valid);
    
    // 配列型安全性エラー1（サイズ不一致）テスト
    run_cb_test_with_output_and_time_auto(test_file_type_safety_error1, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for func type safety error1 test");
            INTEGRATION_ASSERT_CONTAINS(output, "Array literal size", "should contain array literal size error");
        });
    integration_test_passed_with_time_auto("func type safety error1 test", test_file_type_safety_error1);
    
    // 配列型安全性エラー2（サイズ不一致）テスト  
    run_cb_test_with_output_and_time_auto(test_file_type_safety_error2, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for func type safety error2 test");
            INTEGRATION_ASSERT_CONTAINS(output, "Array size mismatch", "should contain size mismatch error");
        });
    integration_test_passed_with_time_auto("func type safety error2 test", test_file_type_safety_error2);
    
    // 関数実行回数テスト（新規追加）
    run_cb_test_with_output_and_time_auto(test_file_function_call_count, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for function call count test");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 関数実行回数テスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Before: test=5, call_count=0", "Should show initial state");
            INTEGRATION_ASSERT_CONTAINS(output, "Function p called, count: 1, value: 5", "Should show function call");
            INTEGRATION_ASSERT_CONTAINS(output, "After: test=6, call_count=1", "Should show result after first test");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ Test 1 passed: Function called exactly once", "Should show test 1 success");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Compound assignment test ---", "Should show compound test section");
            INTEGRATION_ASSERT_CONTAINS(output, "Before: test=10, call_count=0", "Should show second test initial state");
            INTEGRATION_ASSERT_CONTAINS(output, "Function p called, count: 1, value: 10", "Should show second function call");
            INTEGRATION_ASSERT_CONTAINS(output, "After: test=20, call_count=1", "Should show result after second test");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ Test 2 passed: Function in compound assignment called exactly once", "Should show test 2 success");
        });
    integration_test_passed_with_time_auto("func function call count test", test_file_function_call_count);
}
