#pragma once
#include "../framework/integration_test_framework.hpp"

namespace DefaultMemberTests {

void run_all_default_member_tests() {
    std::cout << "[integration-test] Running Default Member tests..." << std::endl;
    
    // 基本的なデフォルトメンバー動作
    double execution_time;
    run_cb_test_with_output_and_time("../cases/default_member/test_default_member_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_member_basic.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("Basic default member", "test_default_member_basic.cb", execution_time);
    
    // 暗黙的代入のテスト
    run_cb_test_with_output_and_time("../cases/default_member/test_default_member_implicit_assign.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_member_implicit_assign.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("Implicit assignment", "test_default_member_implicit_assign.cb", execution_time);
    
    // P2: Bool型の暗黙的代入の修正テスト
    run_cb_test_with_output_and_time("../cases/default_member/test_bool_fix.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_bool_fix.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Bool test should output 1");
            INTEGRATION_ASSERT_CONTAINS(output, "0", "Bool test should output 0");
        }, execution_time);
    integration_test_passed_with_time("Bool type fix (P2)", "test_bool_fix.cb", execution_time);
    
    // P5: ポインタ経由での文字列アクセスの修正テスト
    run_cb_test_with_output_and_time("../cases/default_member/test_default_array_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_array_pointer.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Hello", "Pointer test should output 'Hello'");
            INTEGRATION_ASSERT_CONTAINS(output, "World", "Pointer test should output 'World'");
            INTEGRATION_ASSERT_CONTAINS(output, "Pointer", "Pointer test should output 'Pointer'");
        }, execution_time);
    integration_test_passed_with_time("Pointer access (P5)", "test_default_array_pointer.cb", execution_time);
    
    // 全型のテスト
    run_cb_test_with_output_and_time("../cases/default_member/test_default_all_types.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_all_types.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("All types test", "test_default_all_types.cb", execution_time);
    
    // インターフェース実装でのデフォルトメンバー
    run_cb_test_with_output_and_time("../cases/default_member/test_default_impl.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_default_impl.cb should execute successfully");
        }, execution_time);
    integration_test_passed_with_time("Interface implementation", "test_default_impl.cb", execution_time);
    
    // 包括的テストスイート（25テスト）
    run_cb_test_with_output_and_time("../cases/default_member/test_suite.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_suite.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Total Tests: 25", "Suite should run 25 tests");
            INTEGRATION_ASSERT_CONTAINS(output, "Passed: 25", "All 25 tests should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Failed: 0", "No tests should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ All tests passed!", "Suite should complete successfully");
        }, execution_time);
    integration_test_passed_with_time("Comprehensive suite (25 tests)", "test_suite.cb", execution_time);
    
    std::cout << "[integration-test] Default Member tests completed" << std::endl;
}

} // namespace DefaultMemberTests
