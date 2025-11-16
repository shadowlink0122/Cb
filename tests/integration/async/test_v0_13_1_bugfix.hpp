#pragma once
#include "../framework/integration_test_framework.hpp"

namespace V0131BugFixTests {

void test_async_impl_self_modify() {
    std::cout << "[integration-test] v0.13.1: Async impl self modification..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/async/test_impl_async_self_modify.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_impl_async_self_modify.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Async Impl Self Modification Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - After increment(5):", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - After accumulate(5, 3):", "Expected test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3 - Concurrent increments:", "Expected test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.1 Async Impl Self Modify", "test_impl_async_self_modify.cb", execution_time);
}

void test_impl_async_method() {
    std::cout << "[integration-test] v0.13.1: Async impl method..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/async/test_impl_async_method.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_impl_async_method.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Impl Async Method Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Initial: 10", "Expected initial value");
            INTEGRATION_ASSERT_CONTAINS(output, "After increment: 11", "Expected after increment");
            INTEGRATION_ASSERT_CONTAINS(output, "After set: 100", "Expected after set");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.1 Async Impl Method", "test_impl_async_method.cb", execution_time);
}

void test_impl_async_yield() {
    std::cout << "[integration-test] v0.13.1: Async impl with yield..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/async/test_impl_async_yield.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_impl_async_yield.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Impl Async Method with Yield Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: 60", "Expected result value");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.1 Async Impl Yield", "test_impl_async_yield.cb", execution_time);
}

void test_nested_enum_associated() {
    std::cout << "[integration-test] v0.13.1: Nested enum with associated values..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/enum/test_nested_enum_associated.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_nested_enum_associated.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Nested Enum Associated Value Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - Outer: Some", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Inner: Ok(42)", "Expected inner Ok");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - Outer: Some", "Expected test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Inner: Err(404)", "Expected inner Err");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3 - Outer: Some", "Expected test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "Inner: Some(99)", "Expected nested Some");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.1 Nested Enum Associated", "test_nested_enum_associated.cb", execution_time);
}

void test_enum_copy_semantics() {
    std::cout << "[integration-test] v0.13.1: Enum copy semantics..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/enum/test_enum_copy_semantics.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_enum_copy_semantics.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Enum Associated Value Copy Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - c1.value: 42", "Expected c1 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - c2.value: 42", "Expected c2 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - c3.data:", "Expected c3 data");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - c4.data:", "Expected c4 data");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.1 Enum Copy Semantics", "test_enum_copy_semantics.cb", execution_time);
}

void run_all_v0_13_1_bugfix_tests() {
    std::cout << "\n[integration-test] === v0.13.1 Bug Fix Tests ===" << std::endl;
    
    // Async impl self modification tests (ASYNC-172, 173, 174, 176)
    test_async_impl_self_modify();
    test_impl_async_method();
    test_impl_async_yield();
    
    // Enum associated value deep copy tests
    test_nested_enum_associated();
    test_enum_copy_semantics();
}

} // namespace V0131BugFixTests
