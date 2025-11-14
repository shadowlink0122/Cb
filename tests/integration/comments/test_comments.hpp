#pragma once
#include "../framework/integration_test_framework.hpp"
#include <fstream>
#include <sstream>

namespace CommentTests {

// 期待出力ファイルを読み込む関数
std::string read_expected_output(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open expected output file: " + filepath);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void runCommentTests() {
    std::cout << "[integration-test] === Comment Tests ===" << std::endl;
    std::cout << "[integration-test] Running Comment Tests..." << std::endl;

    // 行コメントのテスト
    {
        double execution_time;
        std::string expected = read_expected_output("../../tests/cases/comments/test_line_comment.expected");
        run_cb_test_with_output_and_time("../../tests/cases/comments/test_line_comment.cb",
            [&expected](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "line comment test should execute successfully");
                INTEGRATION_ASSERT_EQ(expected, output, "line comment test output should match expected");
            }, execution_time);
        integration_test_passed_with_time("line comment test", "../../tests/cases/comments/test_line_comment.cb", execution_time);
    }

    // ブロックコメントのテスト
    {
        double execution_time;
        std::string expected = read_expected_output("../../tests/cases/comments/test_block_comment.expected");
        run_cb_test_with_output_and_time("../../tests/cases/comments/test_block_comment.cb",
            [&expected](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "block comment test should execute successfully");
                INTEGRATION_ASSERT_EQ(expected, output, "block comment test output should match expected");
            }, execution_time);
        integration_test_passed_with_time("block comment test", "../../tests/cases/comments/test_block_comment.cb", execution_time);
    }

    // 混在コメントのテスト
    {
        double execution_time;
        std::string expected = read_expected_output("../../tests/cases/comments/test_mixed_comments.expected");
        run_cb_test_with_output_and_time("../../tests/cases/comments/test_mixed_comments.cb",
            [&expected](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "mixed comments test should execute successfully");
                INTEGRATION_ASSERT_EQ(expected, output, "mixed comments test output should match expected");
            }, execution_time);
        integration_test_passed_with_time("mixed comments test", "../../tests/cases/comments/test_mixed_comments.cb", execution_time);
    }

    // 文字列内のコメント記号テスト
    {
        double execution_time;
        std::string expected = read_expected_output("../../tests/cases/comments/test_comment_in_string.expected");
        run_cb_test_with_output_and_time("../../tests/cases/comments/test_comment_in_string.cb",
            [&expected](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "comment in string test should execute successfully");
                INTEGRATION_ASSERT_EQ(expected, output, "comment in string test output should match expected");
            }, execution_time);
        integration_test_passed_with_time("comment in string test", "../../tests/cases/comments/test_comment_in_string.cb", execution_time);
    }

    // エッジケーステスト
    {
        double execution_time;
        std::string expected = read_expected_output("../../tests/cases/comments/test_comment_edge_cases.expected");
        run_cb_test_with_output_and_time("../../tests/cases/comments/test_comment_edge_cases.cb",
            [&expected](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "comment edge cases test should execute successfully");
                INTEGRATION_ASSERT_EQ(expected, output, "comment edge cases test output should match expected");
            }, execution_time);
        integration_test_passed_with_time("comment edge cases test", "../../tests/cases/comments/test_comment_edge_cases.cb", execution_time);
    }

    std::cout << "[integration-test] Comment tests completed" << std::endl;
}

} // namespace CommentTests
