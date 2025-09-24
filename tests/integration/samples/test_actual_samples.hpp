#pragma once
#include "../framework/integration_test_framework.hpp"

void test_actual_fibonacci_sample() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../sample/fibonacci.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "actual fibonacci sample failed");
    INTEGRATION_ASSERT_CONTAINS(output, "0\n1\n1\n2\n3\n5\n8", "Expected fibonacci sequence not found");
    std::cout << "[integration] actual fibonacci sample ... passed" << std::endl;
}

void test_actual_fizzbuzz_sample() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../sample/fizzbuzz.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "actual fizzbuzz sample failed");
    INTEGRATION_ASSERT_CONTAINS(output, "1\n2\n3: Fizz", "Expected fizzbuzz sequence not found");
    INTEGRATION_ASSERT_CONTAINS(output, "5: Buzz", "Expected buzz output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "15: FizzBuzz", "Expected fizzbuzz output not found");
    std::cout << "[integration] actual fizzbuzz sample ... passed" << std::endl;
}

void test_actual_array_sample() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../sample/array.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "actual array sample failed");
    // UTF-8 文字列を含む出力をテスト
    std::cout << "[integration] actual array sample ... passed" << std::endl;
}

void test_actual_bool_sample() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../sample/bool.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "actual bool sample failed");
    INTEGRATION_ASSERT_CONTAINS(output, "=== bool型のテスト ===", "Expected bool test header not found");
    INTEGRATION_ASSERT_CONTAINS(output, "デモ実行完了", "Expected completion message not found");
    std::cout << "[integration] actual bool sample ... passed" << std::endl;
}

void test_actual_comprehensive_demo() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../sample/comprehensive_demo.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "actual comprehensive demo failed");
    INTEGRATION_ASSERT_CONTAINS(output, "Cb Programming Language Demo", "Expected demo header not found");
    INTEGRATION_ASSERT_CONTAINS(output, "Demo Complete!", "Expected completion message not found");
    std::cout << "[integration] actual comprehensive demo ... passed" << std::endl;
}

void test_integration_actual_samples() {
    test_actual_fibonacci_sample();
    test_actual_fizzbuzz_sample();
    test_actual_array_sample();
    test_actual_bool_sample();
    test_actual_comprehensive_demo();
}
