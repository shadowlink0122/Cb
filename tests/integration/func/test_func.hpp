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
    
    // 基本的な関数テスト
    run_cb_test_with_output(test_file_basic, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func basic test");
        });
    integration_test_passed("func basic test", test_file_basic);
    
    // 配列関数テスト
    run_cb_test_with_output(test_file_array, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func array test");
            INTEGRATION_ASSERT_CONTAINS(output, "Testing array functions:", "should contain test start message");
            INTEGRATION_ASSERT_CONTAINS(output, "Integer array created and assigned", "should contain integer array message");
            INTEGRATION_ASSERT_CONTAINS(output, "Array parameter received successfully", "should contain array parameter message");
            INTEGRATION_ASSERT_CONTAINS(output, "All array function tests completed successfully", "should contain completion message");
        });
    integration_test_passed("func array test", test_file_array);
    
    // シンプルな配列戻り値テスト
    run_cb_test_with_output(test_file_simple_array, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func simple array return test");
            INTEGRATION_ASSERT_CONTAINS(output, "Testing simple array return", "should contain simple test message");
            INTEGRATION_ASSERT_CONTAINS(output, "Direct array literal assignment completed", "should contain direct assignment message");
            INTEGRATION_ASSERT_CONTAINS(output, "Function array return assignment completed", "should contain function return message");
        });
    integration_test_passed("func simple array return test", test_file_simple_array);
    
    // 静的文字列配列テスト
    run_cb_test_with_output(test_file_static_string, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func static string test");
            INTEGRATION_ASSERT_CONTAINS(output, "Alice", "should contain first name");
            INTEGRATION_ASSERT_CONTAINS(output, "Bob", "should contain second name");
            INTEGRATION_ASSERT_CONTAINS(output, "Charlie", "should contain third name");
        });
    integration_test_passed("func static string test", test_file_static_string);
    
    // 配列型安全性（有効）テスト
    run_cb_test_with_output(test_file_type_safety_valid, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for func type safety valid test");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "should contain expected output");
            INTEGRATION_ASSERT_CONTAINS(output, "200", "should contain expected output");
            INTEGRATION_ASSERT_CONTAINS(output, "300", "should contain expected output");
        });
    integration_test_passed("func type safety valid test", test_file_type_safety_valid);
    
    // 配列型安全性エラー1（サイズ不一致）テスト
    run_cb_test_with_output(test_file_type_safety_error1, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for func type safety error1 test");
            INTEGRATION_ASSERT_CONTAINS(output, "Array literal size", "should contain array literal size error");
        });
    integration_test_passed("func type safety error1 test", test_file_type_safety_error1);
    
    // 配列型安全性エラー2（サイズ不一致）テスト  
    run_cb_test_with_output(test_file_type_safety_error2, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for func type safety error2 test");
            INTEGRATION_ASSERT_CONTAINS(output, "Array size mismatch", "should contain size mismatch error");
        });
    integration_test_passed("func type safety error2 test", test_file_type_safety_error2);
}
