#ifndef DISCARD_VARIABLE_TESTS_HPP
#define DISCARD_VARIABLE_TESTS_HPP

#include "../framework/integration_test_framework.hpp"

void test_discard_variable() {
    std::cout << "[integration-test] Running discard variable tests..." << std::endl;
    
    double execution_time;
    
    // テスト1: 基本的な無名変数
    run_cb_test_with_output_and_time("../cases/discard_variable/basic/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic discard should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "OK: Basic discard", "Should print success");
        }, execution_time);
    integration_test_passed_with_time("basic discard variable", "basic/basic.cb", execution_time);
    
    // テスト2: 関数戻り値を無視
    run_cb_test_with_output_and_time("../cases/discard_variable/basic/function_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Discard function return should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Computing...", "Function should execute");
            INTEGRATION_ASSERT_CONTAINS(output, "OK: Return discarded", "Should complete");
        }, execution_time);
    integration_test_passed_with_time("discard function return value", "basic/function_return.cb", execution_time);
    
    // テスト3: 複数の無名変数
    run_cb_test_with_output_and_time("../cases/discard_variable/basic/multiple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multiple discards should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "OK: Multiple discards", "Should complete");
        }, execution_time);
    integration_test_passed_with_time("multiple discard variables", "basic/multiple.cb", execution_time);
    
    // エラーテスト1: discard変数を読み込もうとする
    run_cb_test_with_output_and_time("../cases/discard_variable/error/read_discard.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Reading discard variable should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reference discard variable", "Should report discard error");
        }, execution_time);
    integration_test_passed_with_time("error: read discard variable", "error/read_discard.cb", execution_time);
    
    // エラーテスト2: discard変数を式で使用
    run_cb_test_with_output_and_time("../cases/discard_variable/error/use_in_expression.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Using discard in expression should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reference discard variable", "Should report discard error");
        }, execution_time);
    integration_test_passed_with_time("error: use discard in expression", "error/use_in_expression.cb", execution_time);
    
    // エラーテスト3: discard変数を関数引数として渡す
    run_cb_test_with_output_and_time("../cases/discard_variable/error/pass_as_argument.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Passing discard as argument should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reference discard variable", "Should report discard error");
        }, execution_time);
    integration_test_passed_with_time("error: pass discard as argument", "error/pass_as_argument.cb", execution_time);
    
    // エラーテスト4: discard変数をprintで出力
    run_cb_test_with_output_and_time("../cases/discard_variable/error/print_discard.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Printing discard variable should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reference discard variable", "Should report discard error");
        }, execution_time);
    integration_test_passed_with_time("error: print discard variable", "error/print_discard.cb", execution_time);
    
    // エラーテスト5: discard変数を再代入
    run_cb_test_with_output_and_time("../cases/discard_variable/error/reassign_discard.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Reassigning discard variable should fail");
            // パーサーレベルでInvalid assignment targetエラーになる
            INTEGRATION_ASSERT_CONTAINS(output, "error:", "Should report error");
        }, execution_time);
    integration_test_passed_with_time("error: reassign discard variable", "error/reassign_discard.cb", execution_time);
    
    // エラーテスト6: discard変数を配列要素として使用
    run_cb_test_with_output_and_time("../cases/discard_variable/error/use_in_array.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Using discard in array should fail");
            // パーサーレベルでエラーになる
            INTEGRATION_ASSERT_CONTAINS(output, "error:", "Should report error");
        }, execution_time);
    integration_test_passed_with_time("error: use discard in array literal", "error/use_in_array.cb", execution_time);
    
    // エラーテスト7: discard変数をreturnで返す
    run_cb_test_with_output_and_time("../cases/discard_variable/error/return_discard.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Returning discard variable should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reference discard variable", "Should report discard error");
        }, execution_time);
    integration_test_passed_with_time("error: return discard variable", "error/return_discard.cb", execution_time);
    
    std::cout << "[integration-test] Discard variable tests completed (3 success + 7 error cases)" << std::endl;
}

#endif // DISCARD_VARIABLE_TESTS_HPP
