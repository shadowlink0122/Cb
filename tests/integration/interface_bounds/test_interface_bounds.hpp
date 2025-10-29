#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_interface_bounds() {
    std::cout << "[integration-test] Running Interface Bounds tests..." << std::endl;
    
    // ========================================================================
    // 正常系テスト - 複数インターフェース境界
    // ========================================================================
    
    // Test 1: Multiple bounds per parameter
    double execution_time_1;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_multiple_bounds_per_param.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "Multiple bounds per parameter test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Multiple Interface Bounds Per Parameter Test ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 1: Container<T, A: Allocator + Clone> - PASSED",
                "Test 1 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 2: AdvancedContainer<T, A: Allocator + Clone + Debug> - PASSED",
                "Test 2 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 3: MultiContainer<K: Clone + Debug, V, A: Allocator + Clone> - PASSED",
                "Test 3 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should show completion message");
        },
        execution_time_1
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_multiple_bounds_per_param.cb", 
        execution_time_1
    );
    
    // Test 2: Function with multiple bounds
    double execution_time_2;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_function_multiple_bounds.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "Function multiple bounds test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Generic Function Multiple Bounds Test ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 1: process<T: Clone + Debug>(T value) - PASSED",
                "Function test 1 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 2: combine<K: Clone, V: Debug, A: Allocator + Clone>(...) - PASSED",
                "Function test 2 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should complete successfully");
        },
        execution_time_2
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_function_multiple_bounds.cb", 
        execution_time_2
    );
    
    // Test 3: Enum with multiple bounds
    double execution_time_3;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_enum_multiple_bounds.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "Enum multiple bounds test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Generic Enum Multiple Bounds Test ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 1: Response<T: Clone + Debug, E> - PASSED",
                "Enum test 1 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 2: Container<T: Allocator + Clone + Debug> - PASSED",
                "Enum test 2 should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should complete successfully");
        },
        execution_time_3
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_enum_multiple_bounds.cb", 
        execution_time_3
    );
    
    // Test 4: No conflict for different types
    double execution_time_4;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_no_conflict_different_types.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "No conflict test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== No Conflict for Different Types Test ===",
                "Should show test header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 1: impl Resettable for TypeA - PASSED",
                "TypeA impl should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 2: impl Resettable for TypeB - PASSED",
                "TypeB impl should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Test 3: No conflict detected (different types) - PASSED",
                "No conflict verification should pass");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should complete successfully");
        },
        execution_time_4
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_no_conflict_different_types.cb", 
        execution_time_4
    );
    
    // ========================================================================
    // エラーケーステスト - メソッド名衝突検出
    // ========================================================================
    
    // Test 5: Method conflict detection (interface bounds)
    double execution_time_5;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_conflict_methods.cb",
        [](const std::string& output, int exit_code) {
            // エラーケースなので exit_code != 0 が期待される
            INTEGRATION_ASSERT_NE(0, exit_code, 
                "Conflict test should fail with error");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Method name conflict",
                "Should show method conflict error");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "reset",
                "Should mention conflicting method name");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Allocator",
                "Should mention Allocator interface");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Resettable",
                "Should mention Resettable interface");
        },
        execution_time_5
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_conflict_methods.cb (error case)", 
        execution_time_5
    );
    
    // Test 6: Duplicate impl methods
    double execution_time_6;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface_bounds/test_duplicate_impl_methods.cb",
        [](const std::string& output, int exit_code) {
            // エラーケースなので exit_code != 0 が期待される
            INTEGRATION_ASSERT_NE(0, exit_code, 
                "Duplicate impl methods test should fail with error");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Method name conflict",
                "Should show method conflict error");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "reset",
                "Should mention conflicting method name");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "already defined",
                "Should indicate method is already defined");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "MyType",
                "Should mention the type name");
        },
        execution_time_6
    );
    integration_test_passed_with_time(
        "Interface Bounds", 
        "test_duplicate_impl_methods.cb (error case)", 
        execution_time_6
    );
    
    std::cout << "[integration-test] Interface Bounds tests completed" << std::endl;
}
