#pragma once
#include "../framework/integration_test_framework.hpp"

void test_actual_fibonacci_sample() {
    run_cb_test_with_output_and_time_auto("../../sample/fibonacci.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "actual fibonacci sample failed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Fibonacci Sequence ===", "Expected fibonacci header not found");
            INTEGRATION_ASSERT_CONTAINS(output, "0 : 0", "Expected fibonacci f(0) not found");
            INTEGRATION_ASSERT_CONTAINS(output, "1 : 1", "Expected fibonacci f(1) not found");
            INTEGRATION_ASSERT_CONTAINS(output, "5 : 5", "Expected fibonacci f(5) not found");
            INTEGRATION_ASSERT_CONTAINS(output, "10 : 55", "Expected fibonacci f(10) not found");
            INTEGRATION_ASSERT_CONTAINS(output, "Overflow", "Expected overflow detection for large fibonacci numbers");
        });
    integration_test_passed_with_time_auto("actual fibonacci sample", "fibonacci.cb");
}

void test_actual_fizzbuzz_sample() {
    run_cb_test_with_output_and_time_auto("../../sample/fizzbuzz.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "actual fizzbuzz sample failed");
            INTEGRATION_ASSERT_CONTAINS(output, "1\n2\n3: Fizz", "Expected fizzbuzz sequence not found");
            INTEGRATION_ASSERT_CONTAINS(output, "5: Buzz", "Expected buzz output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "15: FizzBuzz", "Expected fizzbuzz output not found");
        });
    integration_test_passed_with_time_auto("actual fizzbuzz sample", "fizzbuzz.cb");
}

void test_actual_array_sample() {
    run_cb_test_with_output_and_time_auto("../../sample/array.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "actual array sample failed");
            // UTF-8 文字列を含む出力をテスト
        });
    integration_test_passed_with_time_auto("actual array sample", "array.cb");
}

void test_actual_bool_sample() {
    run_cb_test_with_output_and_time_auto("../../sample/bool.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "actual bool sample failed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== bool型のテスト ===", "Expected bool test header not found");
            INTEGRATION_ASSERT_CONTAINS(output, "デモ実行完了", "Expected completion message not found");
        });
    integration_test_passed_with_time_auto("actual bool sample", "bool.cb");
}

void test_actual_comprehensive_demo() {
    run_cb_test_with_output_and_time_auto("../../sample/comprehensive_demo.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "actual comprehensive demo failed");
            INTEGRATION_ASSERT_CONTAINS(output, "Cb Programming Language Demo", "Expected demo header not found");
            INTEGRATION_ASSERT_CONTAINS(output, "Demo Complete!", "Expected completion message not found");
        });
    integration_test_passed_with_time_auto("actual comprehensive demo", "comprehensive_demo.cb");
}

void test_integration_actual_samples() {
    test_actual_fibonacci_sample();
    test_actual_fizzbuzz_sample();
    test_actual_array_sample();
    test_actual_bool_sample();
    test_actual_comprehensive_demo();
}
