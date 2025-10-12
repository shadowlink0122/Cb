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
    
    std::cout << "[integration-test] Discard variable tests completed" << std::endl;
}

#endif // DISCARD_VARIABLE_TESTS_HPP
