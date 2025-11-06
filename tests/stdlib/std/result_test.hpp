#ifndef RESULT_TEST_HPP
#define RESULT_TEST_HPP

#include "../framework/stdlib_test_framework.hpp"

void test_result_ok() {
    auto [output, exit_code] = run_cb_test("tests/cases/stdlib/std/result.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 1: Result<int, string>::Ok(42) - PASSED");
}

void test_result_err() {
    auto [output, exit_code] = run_cb_test("tests/cases/stdlib/std/result.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 2: Result<int, string>::Err - PASSED");
}

void test_result_long_ok() {
    auto [output, exit_code] = run_cb_test("tests/cases/stdlib/std/result.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 3: Result<long, int>::Ok(1000000) - PASSED");
}

void test_result_long_err() {
    auto [output, exit_code] = run_cb_test("tests/cases/stdlib/std/result.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 4: Result<long, int>::Err(404) - PASSED");
}

void test_result_match_ok() {
    auto [output, exit_code] = run_cb_test("tests/cases/stdlib/std/result.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 5a: match Ok variant - PASSED");
}

void test_result_match_err() {
    auto [output, exit_code] = run_cb_test("tests/cases/stdlib/std/result.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 5b: match Err variant - PASSED");
}

void test_result_all_passed() {
    auto [output, exit_code] = run_cb_test("tests/cases/stdlib/std/result.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "=== All Result tests passed ===");
}

void register_result_tests(StdlibTestRunner& runner) {
    runner.add_test("Result: Ok basic", test_result_ok);
    runner.add_test("Result: Err basic", test_result_err);
    runner.add_test("Result: Long Ok", test_result_long_ok);
    runner.add_test("Result: Long Err", test_result_long_err);
    runner.add_test("Result: Match Ok", test_result_match_ok);
    runner.add_test("Result: Match Err", test_result_match_err);
    runner.add_test("Result: All tests passed", test_result_all_passed);
}

#endif // RESULT_TEST_HPP
