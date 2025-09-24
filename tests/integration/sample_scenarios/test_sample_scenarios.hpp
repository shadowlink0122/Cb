#pragma once
#include "../framework/integration_test_framework.hpp"

void test_sample_fizzbuzz() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/sample_scenarios/fizzbuzz.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "sample fizzbuzz test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "1\n2\n3: Fizz", "Expected fizzbuzz output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "5: Buzz", "Expected buzz output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "15: FizzBuzz", "Expected fizzbuzz output not found");
    std::cout << "[integration] sample fizzbuzz test ... passed" << std::endl;
}

void test_sample_fibonacci() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/sample_scenarios/fibonacci_memo.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "sample fibonacci test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "fib(0) = 0", "Expected fib(0) output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "fib(1) = 1", "Expected fib(1) output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "fib(5) = 5", "Expected fib(5) output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "fib(10) = 55", "Expected fib(10) output not found");
    std::cout << "[integration] sample fibonacci test ... passed" << std::endl;
}

void test_sample_boolean() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/sample_scenarios/complex_boolean.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "sample boolean test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "=== Boolean Expression Tests ===", "Expected boolean header not found");
    INTEGRATION_ASSERT_CONTAINS(output, "Condition 1 passed", "Expected condition 1 not found");
    INTEGRATION_ASSERT_CONTAINS(output, "Condition 2 passed", "Expected condition 2 not found");
    std::cout << "[integration] sample boolean expressions test ... passed" << std::endl;
}

void test_sample_output_format() {
    std::string output;
    int exit_code = run_command_and_capture("../../main ../../tests/cases/sample_scenarios/output_format.cb", output);
    INTEGRATION_ASSERT_EQ(0, exit_code, "sample output format test failed");
    INTEGRATION_ASSERT_CONTAINS(output, "Start", "Expected start output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "Value: 42", "Expected value output not found");
    INTEGRATION_ASSERT_CONTAINS(output, "Combined: Test has value 42", "Expected combined output not found");
    std::cout << "[integration] sample output format test ... passed" << std::endl;
}

void test_integration_sample_scenarios() {
    test_sample_fizzbuzz();
    test_sample_fibonacci();
    test_sample_boolean();
    test_sample_output_format();
}
