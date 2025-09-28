#pragma once

#include "../framework/integration_test_framework.hpp"

// 基本的な自己代入テスト（= 演算子使用）
void test_self_assign_basic() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/self_assign/basic_simple.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic simple self assign test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "5\n", "Should contain result 5 with newline");
            INTEGRATION_ASSERT_CONTAINS(output, "10\n", "Should contain result 10 with newline");
            INTEGRATION_ASSERT_CONTAINS(output, "15\n", "Should contain result 15 with newline");
        });
    integration_test_passed_with_time_auto("basic simple self assign test", "basic_simple.cb");
}

// 配列を使った自己代入テスト
void test_self_assign_array() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/self_assign/array_self_assign.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "array self assign test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "2\n", "Should contain array element 2 with newline");
            INTEGRATION_ASSERT_CONTAINS(output, "4\n", "Should contain array element 4 with newline");
            INTEGRATION_ASSERT_CONTAINS(output, "6\n", "Should contain array element 6 with newline");
        });
    integration_test_passed_with_time_auto("array self assign test", "array_self_assign.cb");
}

// 変数を使った自己代入テスト
void test_self_assign_variable() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/self_assign/variable_self_assign.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "variable self assign test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "Should contain result 100");
            INTEGRATION_ASSERT_CONTAINS(output, "150", "Should contain result 150");
        });
    integration_test_passed_with_time_auto("variable self assign test", "variable_self_assign.cb");
}

// 複合代入演算子テスト（現在は未実装のためエラーを期待）
void test_compound_assign_error() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/self_assign/basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "compound assign test executes successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Self assignment operators test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a += 2: 3", "Expected += result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a -= 1: 2", "Expected -= result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a *= 5: 10", "Expected *= result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a /= 2: 5", "Expected /= result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "a %= 3: 2", "Expected %= result in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Self assignment operators test passed", "Expected success message in output");
        });
    integration_test_passed_with_time_auto("compound assign test (limited support)", "basic.cb");
}

// ビット演算を使った自己代入テスト
void test_bitwise_self_assign() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/self_assign/bitwise_self_assign.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "bitwise self assign test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "12\n", "Should contain initial value 12");
            INTEGRATION_ASSERT_CONTAINS(output, "5\n", "Should contain initial value 5");
            INTEGRATION_ASSERT_CONTAINS(output, "4\n", "Should contain AND result 4");
            INTEGRATION_ASSERT_CONTAINS(output, "1\n", "Should contain XOR result 1");
            INTEGRATION_ASSERT_CONTAINS(output, "2\n", "Should contain shift result 2");
        });
    integration_test_passed_with_time_auto("bitwise self assign test", "bitwise_self_assign.cb");
}

// 複合代入演算子テスト
void test_compound_self_assign() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/self_assign/compound_self_assign.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "compound self assign test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "10\n", "Should contain initial value 10");
            INTEGRATION_ASSERT_CONTAINS(output, "3\n", "Should contain initial/result value 3");
            INTEGRATION_ASSERT_CONTAINS(output, "13\n", "Should contain addition result 13");
            INTEGRATION_ASSERT_CONTAINS(output, "26\n", "Should contain multiplication result 26");
            INTEGRATION_ASSERT_CONTAINS(output, "12\n", "Should contain shift result 12");
        });
    integration_test_passed_with_time_auto("compound self assign test", "compound_self_assign.cb");
}

inline void test_integration_self_assign() {
    std::cout << "[integration] Running self assign tests..." << std::endl;
    
    test_self_assign_basic();
    test_self_assign_array();
    test_self_assign_variable();
    test_compound_assign_error();
    test_bitwise_self_assign();
    test_compound_self_assign();
    
    std::cout << "[integration] Self assign tests completed" << std::endl;
}
