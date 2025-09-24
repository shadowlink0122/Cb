#pragma once

#include "../framework/integration_test_framework.hpp"

void test_multiple_variable_declaration() {
    run_cb_test_with_output("../../tests/cases/multiple_var_decl/multiple_var_decl.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multiple variable declaration test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Basic multiple declaration:", "Should handle basic multiple declaration");
            INTEGRATION_ASSERT_CONTAINS(output, "With initialization:", "Should handle initialization");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple variable declaration test completed", "Should complete test");
        });
    integration_test_passed("test_multiple_variable_declaration", "multiple_var_decl.cb");
}

inline void test_integration_multiple_var_decl() {
    std::cout << "[integration] Running multiple variable declaration tests..." << std::endl;
    test_multiple_variable_declaration();
    std::cout << "[integration] Multiple variable declaration tests completed" << std::endl;
}
