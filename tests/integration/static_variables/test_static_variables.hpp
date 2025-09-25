#pragma once

#include "../framework/integration_test_framework.hpp"

void test_integration_static_variables() {
    std::cout << "[integration] Running static_variables tests..." << std::endl;

    // 基本的なstatic変数テスト
    run_cb_test_with_output("../cases/static_variables/basic_static.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic_static.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("1\n2\n3\n4\n5\n", output, "basic static variable test output");
        });
    integration_test_passed("basic static variable", "basic_static.cb");
    
    // static const組み合わせテスト
    run_cb_test_with_output("../cases/static_variables/static_const.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "static_const.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("15\n30\n45\n60\n75\n90\n90\n90\n", output, "static const combination test output");
        });
    integration_test_passed("static const combination", "static_const.cb");
    
    // 複数関数でのstatic変数スコープテスト
    run_cb_test_with_output("../cases/static_variables/multiple_functions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_functions.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("15\n90\n2\n20\n80\n4\n25\n70\n8\n", output, "multiple functions scope test output");
        });
    integration_test_passed("multiple functions scope", "multiple_functions.cb");
    
    // 異なるデータ型でのstatic変数テスト
    run_cb_test_with_output("../cases/static_variables/different_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "different_types.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("42\n1\n3\n43\n0\n4\n44\n1\n5\n", output, "different data types test output");
        });
    integration_test_passed("different data types", "different_types.cb");
    
    // マルチstatic変数テスト
    run_cb_test_with_output("../cases/static_variables/static_array.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "static_array.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("1\n2\n3\n11\n12\n13\n21\n22\n23\n", output, "multi static variables test output");
        });
    integration_test_passed("multi static variables", "static_array.cb");
    
    // 再帰関数でのstatic変数テスト
    run_cb_test_with_output("../cases/static_variables/recursive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "recursive.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("1\n4\n2\n3\n3\n2\n4\n1\n5\n0\n6\n1\n7\n2\n8\n1\n9\n0\n3\n", output, "recursive static variable test output");
        });
    integration_test_passed("recursive static variable", "recursive.cb");
    
    // 統合テスト
    run_cb_test_with_output("static_variables/static_integration.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "static_integration.cb should execute successfully");
            INTEGRATION_ASSERT_EQ("1\n2\n3\n15\n90\n20\n80\n12\n24\n36\n48\n50\n50\n", output, "static integration test output");
        });
    integration_test_passed("static integration", "static_integration.cb");

    std::cout << "[integration] Static variables tests completed" << std::endl;
}
