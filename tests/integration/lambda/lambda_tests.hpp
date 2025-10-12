#ifndef LAMBDA_TESTS_HPP
#define LAMBDA_TESTS_HPP

#include "../framework/integration_test_framework.hpp"

void test_lambda_function() {
    std::cout << "[integration-test] Running lambda function tests..." << std::endl;
    
    double execution_time;
    
    // テスト1: 基本的なラムダ関数
    run_cb_test_with_output_and_time("../cases/lambda/basic/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic lambda should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "10", "Should print 10");
        }, execution_time);
    integration_test_passed_with_time("basic lambda function", "basic/basic.cb", execution_time);
    
    // テスト2: 複数のパラメータを持つラムダ
    run_cb_test_with_output_and_time("../cases/lambda/basic/multiple_params.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Lambda with multiple parameters should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "10", "Should print 10");
        }, execution_time);
    integration_test_passed_with_time("lambda with multiple parameters", "basic/multiple_params.cb", execution_time);
    
    // テスト3: void戻り値のラムダ
    run_cb_test_with_output_and_time("../cases/lambda/basic/void_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Lambda with void return should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "42", "Should print 42");
        }, execution_time);
    integration_test_passed_with_time("lambda with void return", "basic/void_return.cb", execution_time);
    
    // テスト4: ラムダの即座実行（immediate invocation）
    run_cb_test_with_output_and_time("../cases/lambda/immediate_invocation/immediate_invocation.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Lambda immediate invocation should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "200", "Should print 200 (10 * 20)");
            INTEGRATION_ASSERT_CONTAINS(output, "49", "Should print 49 (7 * 7)");
            INTEGRATION_ASSERT_CONTAINS(output, "23", "Should print 23 ((5+3)+(5*3))");
            INTEGRATION_ASSERT_CONTAINS(output, "All tests passed!", "Should complete all tests");
        }, execution_time);
    integration_test_passed_with_time("lambda immediate invocation", "immediate_invocation/immediate_invocation.cb", execution_time);
    
    // テスト5: ラムダのチェーン呼び出し
    run_cb_test_with_output_and_time("../cases/lambda/immediate_invocation/chain_invocation.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Lambda chain invocation should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "60", "Should print 60 ((10+20)*2)");
            INTEGRATION_ASSERT_CONTAINS(output, "20", "Should print 20 ((5*2)+10)");
            INTEGRATION_ASSERT_CONTAINS(output, "21", "Should print 21 (7*3)");
            INTEGRATION_ASSERT_CONTAINS(output, "All tests passed!", "Should complete all tests");
        }, execution_time);
    integration_test_passed_with_time("lambda chain invocation", "immediate_invocation/chain_invocation.cb", execution_time);
    
    std::cout << "[integration-test] Lambda function tests completed" << std::endl;
}

#endif // LAMBDA_TESTS_HPP
