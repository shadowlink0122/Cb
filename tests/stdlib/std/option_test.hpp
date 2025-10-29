#ifndef OPTION_TEST_HPP
#define OPTION_TEST_HPP

#include "../framework/stdlib_test_framework.hpp"

void test_option_some() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 1: Option<int>::Some(42) - PASSED");
}

void test_option_none() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 2: Option<int>::None - PASSED");
}

void test_option_long_some() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 3: Option<long>::Some(999999) - PASSED");
}

void test_option_long_none() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 4: Option<long>::None - PASSED");
}

void test_option_match_some() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 5a: match Some variant - PASSED");
}

void test_option_match_none() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 5b: match None variant - PASSED");
}

void test_option_match_wildcard() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Test 6: wildcard pattern with Some - PASSED");
}

void test_option_all_passed() {
    auto [output, exit_code] = run_cb_test("../../tests/cases/stdlib/std/option.cb");
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "=== All Option tests passed ===");
}

void register_option_tests(StdlibTestRunner& runner) {
    runner.add_test("Option: Some basic", test_option_some);
    runner.add_test("Option: None basic", test_option_none);
    runner.add_test("Option: Long Some", test_option_long_some);
    runner.add_test("Option: Long None", test_option_long_none);
    runner.add_test("Option: Match Some", test_option_match_some);
    runner.add_test("Option: Match None", test_option_match_none);
    runner.add_test("Option: Match Wildcard", test_option_match_wildcard);
    runner.add_test("Option: All tests passed", test_option_all_passed);
}

#endif // OPTION_TEST_HPP
