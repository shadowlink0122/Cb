#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_struct_array_assignment() {
    std::cout << "[integration-test] Running Struct Array Assignment tests..." << std::endl;
    
    // Test 1: Basic struct assignment to array element
    double execution_time_basic;
    run_cb_test_with_output_and_time("../../tests/cases/struct_array_assignment/test_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_basic.cb should execute successfully");
            
            // 基本的な構造体変数の代入が動作することを確認
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Basic struct variable assignment - PASSED", 
                                       "Should pass basic struct variable assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "task_id =  42", 
                                       "Should assign correct task_id");
            INTEGRATION_ASSERT_CONTAINS(output, "priority =  5", 
                                       "Should assign correct priority");
        }, execution_time_basic);
    integration_test_passed_with_time("Basic Assignment", "test_basic.cb", execution_time_basic);
    
    // Test 2: Struct literal assignment
    double execution_time_literal;
    run_cb_test_with_output_and_time("../../tests/cases/struct_array_assignment/test_literal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_literal.cb should execute successfully");
            
            // 構造体リテラルの直接代入が動作することを確認
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Direct literal assignment - PASSED", 
                                       "Should pass direct literal assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "task_id =  100", 
                                       "Should assign correct task_id from literal");
            INTEGRATION_ASSERT_CONTAINS(output, "priority =  10", 
                                       "Should assign correct priority from literal");
        }, execution_time_literal);
    integration_test_passed_with_time("Literal Assignment", "test_literal.cb", execution_time_literal);
    
    // Test 3: Loop assignment
    double execution_time_loop;
    run_cb_test_with_output_and_time("../../tests/cases/struct_array_assignment/test_loop.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_loop.cb should execute successfully");
            
            // ループ内での代入が正しく動作することを確認
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Loop assignment - PASSED", 
                                       "Should pass loop assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "All 10 elements correctly assigned", 
                                       "Should assign all elements in loop");
        }, execution_time_loop);
    integration_test_passed_with_time("Loop Assignment", "test_loop.cb", execution_time_loop);
    
    // Test 4: Array element to array element copy
    double execution_time_copy;
    run_cb_test_with_output_and_time("../../tests/cases/struct_array_assignment/test_element_copy.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_element_copy.cb should execute successfully");
            
            // 配列要素間のコピーが動作することを確認
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Array element copy - PASSED", 
                                       "Should pass array element copy");
            INTEGRATION_ASSERT_CONTAINS(output, "dst[0].task_id =  777", 
                                       "Should copy task_id correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "dst[0].priority =  99", 
                                       "Should copy priority correctly");
        }, execution_time_copy);
    integration_test_passed_with_time("Element Copy", "test_element_copy.cb", execution_time_copy);
    
    // Test 5: Function return value assignment
    double execution_time_function;
    run_cb_test_with_output_and_time("../../tests/cases/struct_array_assignment/test_function_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_function_return.cb should execute successfully");
            
            // 関数戻り値の代入が動作することを確認
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Function return assignment - PASSED", 
                                       "Should pass function return assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "task_id =  999", 
                                       "Should assign function return value correctly");
        }, execution_time_function);
    integration_test_passed_with_time("Function Return", "test_function_return.cb", execution_time_function);
    
    // Test 6: Comprehensive test
    double execution_time_comprehensive;
    run_cb_test_with_output_and_time("../../tests/cases/struct_array_assignment/test_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_comprehensive.cb should execute successfully");
            
            // 全てのパターンが動作することを確認
            INTEGRATION_ASSERT_CONTAINS(output, "✓ Test 1: Literal assignment - PASSED", 
                                       "Should pass comprehensive test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ Test 2: Variable assignment - PASSED", 
                                       "Should pass comprehensive test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ Test 3: Function return assignment - PASSED", 
                                       "Should pass comprehensive test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ Test 4: Element copy assignment - PASSED", 
                                       "Should pass comprehensive test 4");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ Test 5: Loop assignment - PASSED", 
                                       "Should pass comprehensive test 5");
            INTEGRATION_ASSERT_CONTAINS(output, "Comprehensive test - ALL PASSED", 
                                       "Should complete all tests successfully");
        }, execution_time_comprehensive);
    integration_test_passed_with_time("Comprehensive", "test_comprehensive.cb", execution_time_comprehensive);
    
    std::cout << "[integration-test] Struct Array Assignment tests completed" << std::endl;
}
