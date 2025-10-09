#pragma once

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace ImplStaticTests {

// 基本的なParser/登録動作のテスト
inline void test_impl_static_simple() {
    std::cout << "[integration-test] Running test_impl_static_simple..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/impl_static/test_impl_static_simple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "impl static simple test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(1, lines.size(), "Should have 1 output line");
            INTEGRATION_ASSERT_EQ("0", lines[0], "Output should be 0");
        }, execution_time);
}

// 基本的なカウンター動作テスト
inline void test_impl_static_basic() {
    std::cout << "[integration-test] Running test_impl_static_basic..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/impl_static/test_impl_static_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "impl static basic test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(3, lines.size(), "Should have 3 output lines");
            INTEGRATION_ASSERT_EQ("1", lines[0], "First increment should be 1");
            INTEGRATION_ASSERT_EQ("2", lines[1], "Second increment should be 2");
            INTEGRATION_ASSERT_EQ("2", lines[2], "get_count should return 2");
        }, execution_time);
}

// 型ごとの独立性テスト
inline void test_impl_static_separate() {
    std::cout << "[integration-test] Running test_impl_static_separate..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/impl_static/test_impl_static_separate.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "impl static separate test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(3, lines.size(), "Should have 3 output lines");
            INTEGRATION_ASSERT_EQ("2", lines[0], "Circle count should be 2");
            INTEGRATION_ASSERT_EQ("2", lines[1], "Rectangle count should be 2 (independent)");
            INTEGRATION_ASSERT_EQ("1", lines[2], "Triangle count should be 1 (independent)");
        }, execution_time);
}

// static const組み合わせテスト
inline void test_impl_static_const() {
    std::cout << "[integration-test] Running test_impl_static_const..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/impl_static/test_impl_static_const.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "impl static const test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(5, lines.size(), "Should have 5 output lines");
            INTEGRATION_ASSERT_EQ("100", lines[0], "MAX_VALUE should be 100");
            INTEGRATION_ASSERT_EQ("0", lines[1], "Initial count should be 0");
            INTEGRATION_ASSERT_EQ("100", lines[2], "MAX_VALUE should still be 100");
            INTEGRATION_ASSERT_EQ("1", lines[3], "Count after increment should be 1");
            INTEGRATION_ASSERT_EQ("2", lines[4], "Count after second increment should be 2");
        }, execution_time);
}

// static変数なしのimpl（後方互換性）テスト
inline void test_impl_no_static() {
    std::cout << "[integration-test] Running test_impl_no_static..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/impl_static/test_impl_no_static.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "impl no static test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(1, lines.size(), "Should have 1 output line");
            INTEGRATION_ASSERT_EQ("42", lines[0], "Output should be 42");
        }, execution_time);
}

// デバッグ動作テスト
inline void test_impl_static_debug() {
    std::cout << "[integration-test] Running test_impl_static_debug..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/impl_static/test_impl_static_debug.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "impl static debug test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(1, lines.size(), "Should have 1 output line");
            INTEGRATION_ASSERT_EQ("5", lines[0], "Output should be 5");
        }, execution_time);
}

// すべてのimpl static テストを実行
inline void run_all_tests() {
    std::cout << "\n=== Running impl Static Variable Tests ===" << std::endl;
    
    test_impl_static_simple();
    test_impl_static_basic();
    test_impl_static_separate();
    test_impl_static_const();
    test_impl_no_static();
    test_impl_static_debug();
    
    std::cout << "=== impl Static Tests Completed ===" << std::endl;
}

} // namespace ImplStaticTests
