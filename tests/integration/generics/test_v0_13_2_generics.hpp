#pragma once
#include "../framework/integration_test_framework.hpp"

namespace V0132GenericsTests {

// v0.13.2: Generic String Array Tests
void test_generic_string_array_basic() {
    std::cout << "[integration-test] v0.13.2: Generic String Array Basic..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/generics/generic_comprehensive_test.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "generic_comprehensive_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Async Lambda", "Expected async lambda test");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Generic String Arrays", "Expected generic string array test");
            INTEGRATION_ASSERT_CONTAINS(output, "Alpha", "Expected string value 'Alpha'");
            INTEGRATION_ASSERT_CONTAINS(output, "Beta", "Expected string value 'Beta'");
            INTEGRATION_ASSERT_CONTAINS(output, "Gamma", "Expected string value 'Gamma'");
            INTEGRATION_ASSERT_CONTAINS(output, "All v0.13.2 Tests Passed", "Expected success message");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.2 Comprehensive Test", "generic_comprehensive_test.cb", execution_time);
}

// v0.13.2: Edge Cases
void test_generic_array_edge_cases() {
    std::cout << "[integration-test] v0.13.2: Generic Array Edge Cases..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/generics/generic_edge_cases_test.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "generic_edge_cases_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Empty Strings", "Expected empty string test");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Boundary Access", "Expected boundary test");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Sequential Async Lambda", "Expected async test");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Generic Array Reassignment", "Expected reassignment test");
            INTEGRATION_ASSERT_CONTAINS(output, "All Edge Case Tests Passed", "Expected success message");
        }, execution_time);
    
    integration_test_passed_with_time("v0.13.2 Edge Cases Test", "generic_edge_cases_test.cb", execution_time);
}

void run_all_v0_13_2_generics_tests() {
    std::cout << "\n[integration-test] === v0.13.2: Generic String Array Fix ===" << std::endl;
    
    test_generic_string_array_basic();
    test_generic_array_edge_cases();
}

} // namespace V0132GenericsTests
