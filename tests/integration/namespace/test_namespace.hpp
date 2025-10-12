#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_namespace() {
    std::cout << "[integration-test] Running Namespace tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: Empty namespace declaration
    run_cb_test_with_output_and_time("../cases/namespace/empty_namespace.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "empty_namespace.cb should execute successfully");
            // Empty namespace should not produce output
        }, execution_time);
    integration_test_passed_with_time("Empty namespace declaration", "empty_namespace.cb", execution_time);
    
    // Test 2: Namespace with function definition
    run_cb_test_with_output_and_time("../cases/namespace/namespace_with_function.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "namespace_with_function.cb should execute successfully");
            // Namespace with function (not called) should not produce output
        }, execution_time);
    integration_test_passed_with_time("Namespace with function definition", "namespace_with_function.cb", execution_time);
    
    // Test 3: Qualified function call (namespace::function)
    run_cb_test_with_output_and_time("../cases/namespace/qualified_call.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "qualified_call.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "3", "math::add(1, 2) should return 3");
        }, execution_time);
    integration_test_passed_with_time("Qualified function call (namespace::function)", "qualified_call.cb", execution_time);
    
    // Test 4: Nested namespace
    run_cb_test_with_output_and_time("../cases/namespace/nested_namespace.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "nested_namespace.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "12", "outer::inner::multiply(3, 4) should return 12");
        }, execution_time);
    integration_test_passed_with_time("Nested namespace (outer::inner::func)", "nested_namespace.cb", execution_time);
    
    // Test 5: using namespace - single namespace
    run_cb_test_with_output_and_time("../cases/namespace/using_namespace.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "using_namespace.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "3", "add(1, 2) should return 3");
            INTEGRATION_ASSERT_CONTAINS(output, "12", "multiply(3, 4) should return 12");
            
            // Check output format: "312" (concatenated)
            auto lines = split_lines(output);
            bool found_312 = false;
            for (const auto& line : lines) {
                if (line.find("3") != std::string::npos && 
                    line.find("12") != std::string::npos) {
                    found_312 = true;
                }
            }
            INTEGRATION_ASSERT(found_312, "Should output both results");
        }, execution_time);
    integration_test_passed_with_time("using namespace (single)", "using_namespace.cb", execution_time);
    
    // Test 6: Multiple using namespace
    run_cb_test_with_output_and_time("../cases/namespace/multiple_using.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_using.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "15", "add(10, 5) should return 15");
            INTEGRATION_ASSERT_CONTAINS(output, "5", "subtract(10, 5) should return 5");
        }, execution_time);
    integration_test_passed_with_time("Multiple using namespace", "multiple_using.cb", execution_time);
    
        // Test 7: Ambiguous function call (should fail)
    run_cb_test_with_output_and_time("../cases/namespace/ambiguous_call.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "should fail due to ambiguous function call");
            INTEGRATION_ASSERT_CONTAINS(output, "Ambiguous", "should contain Ambiguous error message");
        }, execution_time);
    integration_test_passed_with_error_and_time("Ambiguous function call test", "../cases/namespace/ambiguous_call.cb", execution_time);
    
    // Test 8: Resolve ambiguity with qualified names
    run_cb_test_with_output_and_time("../cases/namespace/resolve_ambiguity.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "resolve_ambiguity.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "5", "math::calculate(2, 3) should return 5 (2+3)");
            INTEGRATION_ASSERT_CONTAINS(output, "6", "physics::calculate(2, 3) should return 6 (2*3)");
        }, execution_time);
    integration_test_passed_with_time("Resolve ambiguity with qualified names", "resolve_ambiguity.cb", execution_time);
    
    // Test 9: Comprehensive namespace features
    run_cb_test_with_output_and_time("../cases/namespace/comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "comprehensive.cb should execute successfully");
            
            // Check all expected outputs
            INTEGRATION_ASSERT_CONTAINS(output, "8", "add(5, 3) should return 8");
            INTEGRATION_ASSERT_CONTAINS(output, "12", "multiply(4, 3) should return 12");
            INTEGRATION_ASSERT_CONTAINS(output, "20", "max(10, 20) should return 20");
            INTEGRATION_ASSERT_CONTAINS(output, "15", "math::add(10, 5) should return 15");
            
            // Check nested namespace call
            bool found_power = false;
            auto lines = split_lines(output);
            for (const auto& line : lines) {
                if (line.find("8") != std::string::npos) {
                    // Could be power(2, 3) = 8 or add(5, 3) = 8
                    found_power = true;
                }
            }
            INTEGRATION_ASSERT(found_power, "math::advanced::power(2, 3) should return 8");
        }, execution_time);
    integration_test_passed_with_time("Comprehensive namespace features", "comprehensive.cb", execution_time);
    
    std::cout << "[integration-test] Namespace tests completed" << std::endl;
}
