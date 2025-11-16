#pragma once
#include "../framework/integration_test_framework.hpp"

namespace V0132AsyncTests {

// v0.13.2: Async Lambda Basic Test
void test_async_lambda_basic() {
    std::cout << "[integration-test] v0.13.2: Async Lambda Basic..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/async/test_async_lambda_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_async_lambda_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Async Lambda Basic Test", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: 42", "Expected result value");
            INTEGRATION_ASSERT_CONTAINS(output, "Test Complete", "Expected completion message");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.2 Async Lambda Basic", "test_async_lambda_basic.cb", execution_time);
}

// v0.13.2: Async Lambda Complex Test
void test_async_lambda_complex() {
    std::cout << "[integration-test] v0.13.2: Async Lambda Complex..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/async/test_async_lambda_complex.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_async_lambda_complex.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Sum 1-10: 55", "Expected sum result");
            INTEGRATION_ASSERT_CONTAINS(output, "42 is even: 1", "Expected even check result");
            INTEGRATION_ASSERT_CONTAINS(output, "43 is even: 0", "Expected odd check result");
            INTEGRATION_ASSERT_CONTAINS(output, "Test Complete", "Expected completion message");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.2 Async Lambda Complex", "test_async_lambda_complex.cb", execution_time);
}

// v0.13.2: Async Lambda with Parameters Test
void test_async_lambda_params() {
    std::cout << "[integration-test] v0.13.2: Async Lambda Parameters..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/async/test_async_lambda_params.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_async_lambda_params.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Test Complete", "Expected completion message");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.2 Async Lambda Params", "test_async_lambda_params.cb", execution_time);
}

void run_all_v0_13_2_async_tests() {
    std::cout << "\n[integration-test] === v0.13.2: Async Lambda Support ===" << std::endl;
    
    test_async_lambda_basic();
    test_async_lambda_complex();
    test_async_lambda_params();
}

} // namespace V0132AsyncTests
