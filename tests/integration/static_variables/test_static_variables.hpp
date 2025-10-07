#pragma once

#include "../framework/integration_test_framework.hpp"

void test_integration_static_variables() {
    std::cout << "[integration] Running static_variables tests..." << std::endl;

    // 基本的なstatic変数テスト
    run_cb_test_with_output_and_time_auto("../cases/static_variables/basic_static.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic_static.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("1\n2\n3\n4\n5\nBasic static test passed\n", output, "basic static variable test output");
        });
    integration_test_passed_with_time_auto("basic static variable", "basic_static.cb");
    
    // static const組み合わせテスト
    run_cb_test_with_output_and_time_auto("../cases/static_variables/static_const.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "static_const.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("15\n30\n45\n60\n75\n90\n90\n90\n", output, "static const combination test output");
        });
    integration_test_passed_with_time_auto("static const combination", "static_const.cb");
    
    // 複数関数でのstatic変数スコープテスト
    run_cb_test_with_output_and_time_auto("../cases/static_variables/multiple_functions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_functions.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("15\n90\n2\n20\n80\n4\n25\n70\n8\n", output, "multiple functions scope test output");
        });
    integration_test_passed_with_time_auto("multiple functions scope", "multiple_functions.cb");
    
    // 異なるデータ型でのstatic変数テスト
    run_cb_test_with_output_and_time_auto("../cases/static_variables/different_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "different_types.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("42\n1\n3\n43\n0\n4\n44\n1\n5\nDifferent types static test passed\n", output, "different data types test output");
        });
    integration_test_passed_with_time_auto("different data types", "different_types.cb");
    
    // マルチstatic変数テスト
    run_cb_test_with_output_and_time_auto("../cases/static_variables/static_array.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "static_array.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("1\n2\n3\n11\n12\n13\n21\n22\n23\n", output, "multi static variables test output");
        });
    integration_test_passed_with_time_auto("multi static variables", "static_array.cb");
    
    // 再帰関数でのstatic変数テスト
    run_cb_test_with_output_and_time_auto("../cases/static_variables/recursive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "recursive.cb should execute successfully");
            // Fibonacci(4)の実際の呼び出し順序:
            // fib(4) -> fib(3) -> fib(2) -> fib(1), fib(0), fib(1), fib(2) -> fib(1), fib(0)
            // 合計37回の呼び出し（各呼び出しで call_count++ してから n を出力するため 37*2 + 1 = 75行）
            INTEGRATION_ASSERT_EQ("1\n4\n2\n3\n3\n2\n4\n1\n5\n0\n6\n1\n7\n0\n8\n1\n9\n2\n10\n1\n11\n0\n12\n1\n13\n0\n14\n1\n15\n2\n16\n1\n17\n0\n18\n1\n19\n0\n20\n3\n21\n2\n22\n1\n23\n0\n24\n1\n25\n0\n26\n1\n27\n2\n28\n1\n29\n0\n30\n1\n31\n0\n32\n1\n33\n2\n34\n1\n35\n0\n36\n1\n37\n0\n3\n", output, "recursive static variable test output");
        });
    integration_test_passed_with_time_auto("recursive static variable", "recursive.cb");
    
    // 統合テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/static_variables/static_integration.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "static_integration.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("1\n2\n3\n15\n90\n20\n80\n12\n24\n36\n48\n50\n50\n", output, "static integration test output");
        });
    integration_test_passed_with_time_auto("static integration", "../../tests/cases/static_variables/static_integration.cb");

    std::cout << "[integration] Static variables tests completed" << std::endl;
}
