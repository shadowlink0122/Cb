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
    
    // 既存のテスト
    test_multiple_variable_declaration();
    
    // 新規追加のテスト群
    run_cb_test_with_output("../../tests/cases/multiple_var_decl/multiple_var_init_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic initialization test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "int a, b, c: 1 2 3", "Should handle basic int initialization");
            INTEGRATION_ASSERT_CONTAINS(output, "negative/zero/positive: -5 0 42", "Should handle negative numbers");
            INTEGRATION_ASSERT_CONTAINS(output, "long p, q: 1000 2000", "Should handle long type");
        });
    integration_test_passed("test_multiple_var_init_basic", "multiple_var_init_basic.cb");
    
    run_cb_test_with_output("../../tests/cases/multiple_var_decl/multiple_var_init_string.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "String initialization test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "strings: hello world test", "Should handle string initialization");
            INTEGRATION_ASSERT_CONTAINS(output, "empty/full:  content", "Should handle empty strings");
        });
    integration_test_passed("test_multiple_var_init_string", "multiple_var_init_string.cb");
    
    run_cb_test_with_output("../../tests/cases/multiple_var_decl/multiple_var_init_mixed.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Mixed type initialization test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "partial init int: 10 0 30", "Should handle partial initialization");
            INTEGRATION_ASSERT_CONTAINS(output, "partial init short: 100 0 300", "Should handle short type");
            INTEGRATION_ASSERT_CONTAINS(output, "tiny complete: 1 2 3", "Should handle tiny type");
            INTEGRATION_ASSERT_CONTAINS(output, "bool flags: 1 0", "Should handle bool type");
        });
    integration_test_passed("test_multiple_var_init_mixed", "multiple_var_init_mixed.cb");
    
    run_cb_test_with_output("../../tests/cases/multiple_var_decl/multiple_var_init_expressions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expression initialization test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid case: 1 2", "Should handle valid cases");
            INTEGRATION_ASSERT_CONTAINS(output, "With expressions: 8 20", "Should handle arithmetic expressions");
            INTEGRATION_ASSERT_CONTAINS(output, "Large numbers: 999999 123456789", "Should handle large numbers");
        });
    integration_test_passed("test_multiple_var_init_expressions", "multiple_var_init_expressions.cb");
    
    std::cout << "[integration] Multiple variable declaration tests completed" << std::endl;
}
