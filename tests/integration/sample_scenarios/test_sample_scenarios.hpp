#pragma once
#include "../framework/integration_test_framework.hpp"

void test_sample_fizzbuzz() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/sample_scenarios/fizzbuzz.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "sample fizzbuzz test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "1\n2\n3: Fizz", "Expected fizzbuzz output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "5: Buzz", "Expected buzz output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "15: FizzBuzz", "Expected fizzbuzz output not found");
        });
    integration_test_passed_with_time_auto("sample fizzbuzz test", "fizzbuzz.cb");
}

void test_sample_fibonacci() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/sample_scenarios/fibonacci_memo.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "sample fibonacci test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "fib(0) = 0", "Expected fib(0) output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "fib(1) = 1", "Expected fib(1) output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "fib(5) = 5", "Expected fib(5) output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "fib(10) = 55", "Expected fib(10) output not found");
        });
    integration_test_passed_with_time_auto("sample fibonacci test", "fibonacci_memo.cb");
}

void test_sample_boolean() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/sample_scenarios/complex_boolean.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "sample boolean test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Boolean Expression Tests ===", "Expected boolean header not found");
            INTEGRATION_ASSERT_CONTAINS(output, "Condition 1 passed", "Expected condition 1 not found");
            INTEGRATION_ASSERT_CONTAINS(output, "Condition 2 passed", "Expected condition 2 not found");
        });
    integration_test_passed_with_time_auto("sample boolean expressions test", "complex_boolean.cb");
}

void test_sample_output_format() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/sample_scenarios/output_format.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "sample output format test failed");
            INTEGRATION_ASSERT_CONTAINS(output, "Start", "Expected start output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "Value: 42", "Expected value output not found");
            INTEGRATION_ASSERT_CONTAINS(output, "Combined: Test has value 42", "Expected combined output not found");
        });
    integration_test_passed_with_time_auto("sample output format test", "output_format.cb");
}

void test_integration_sample_scenarios() {
    test_sample_fizzbuzz();
    test_sample_fibonacci();
    test_sample_boolean();
    test_sample_output_format();
}
