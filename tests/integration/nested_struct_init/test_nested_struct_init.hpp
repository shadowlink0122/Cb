#pragma once

#include "framework/integration_test_framework.hpp"
#include <string>

namespace NestedStructInitTests {

inline void test_declaration_member_access() {
    std::cout << "[integration-test] Running test_declaration_member_access..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/nested_struct_init/declaration_member_access.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Declaration member access test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Test 1: Mid-level extraction - PASSED") != std::string::npos, 
                              "Output should contain Test 1 pass message");
            INTEGRATION_ASSERT(output.find("Test 2: Deep-level extraction - PASSED") != std::string::npos, 
                              "Output should contain Test 2 pass message");
            INTEGRATION_ASSERT(output.find("Test 3: From extracted struct - PASSED") != std::string::npos, 
                              "Output should contain Test 3 pass message");
            INTEGRATION_ASSERT(output.find("Test 4: Chained extraction - PASSED") != std::string::npos, 
                              "Output should contain Test 4 pass message");
            INTEGRATION_ASSERT(output.find("All nested struct declaration tests passed!") != std::string::npos, 
                              "Output should contain completion message");
        }, execution_time);
    
    std::cout << "[integration-test] Declaration member access test passed (" 
              << execution_time << "ms)" << std::endl;
}

inline void test_comprehensive() {
    std::cout << "[integration-test] Running test_comprehensive..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/nested_struct_init/comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Comprehensive test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Pattern 1: Deep extraction passed") != std::string::npos, 
                              "Output should contain Pattern 1 pass message");
            INTEGRATION_ASSERT(output.find("Pattern 5: Independence verification passed") != std::string::npos, 
                              "Output should contain Pattern 5 pass message");
            INTEGRATION_ASSERT(output.find("All comprehensive nested struct tests passed!") != std::string::npos, 
                              "Output should contain completion message");
        }, execution_time);
    
    std::cout << "[integration-test] Comprehensive test passed (" 
              << execution_time << "ms)" << std::endl;
}

inline void test_edge_cases() {
    std::cout << "[integration-test] Running test_edge_cases..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/nested_struct_init/edge_cases.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Edge cases test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Test 1: Maximum depth nesting (5 levels)") != std::string::npos, 
                              "Output should contain Test 1 header");
            INTEGRATION_ASSERT(output.find("Test 5: Sequential initialization") != std::string::npos, 
                              "Output should contain Test 5 header");
            INTEGRATION_ASSERT(output.find("=== All Edge Case Tests Passed! ===") != std::string::npos, 
                              "Output should contain completion message");
        }, execution_time);
    
    std::cout << "[integration-test] Edge cases test passed (" 
              << execution_time << "ms)" << std::endl;
}

inline void run_all_tests() {
    test_declaration_member_access();
    test_comprehensive();
    test_edge_cases();
}

} // namespace NestedStructInitTests
