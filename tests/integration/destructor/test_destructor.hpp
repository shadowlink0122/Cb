#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_generic_destructor() {
    std::cout << "[integration-test] Running Generic Destructor tests..." << std::endl;
    
    // Test 1: Basic Destructor
    double execution_time_basic;
    run_cb_test_with_output_and_time("../../tests/cases/destructor/test_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_basic.cb should execute successfully");
            
            // Test 1: Simple destructor
            INTEGRATION_ASSERT_CONTAINS(output, "[Point] Destructor called for (10, 20)", 
                "Should call Point destructor");
            
            // Test 2: Multiple variables (LIFO order)
            INTEGRATION_ASSERT_CONTAINS(output, "[Resource] Cleaning up resource ID: 3",
                "Should clean up resource 3 first");
            INTEGRATION_ASSERT_CONTAINS(output, "[Resource] Cleaning up resource ID: 2",
                "Should clean up resource 2 second");
            INTEGRATION_ASSERT_CONTAINS(output, "[Resource] Cleaning up resource ID: 1",
                "Should clean up resource 1 last");
            
            // Test 3: Nested scopes
            INTEGRATION_ASSERT_CONTAINS(output, "[Point] Destructor called for (2, 2)",
                "Should call inner scope destructor first");
            INTEGRATION_ASSERT_CONTAINS(output, "[Point] Destructor called for (1, 1)",
                "Should call outer scope destructor after inner");
            
            // Success messages
            INTEGRATION_ASSERT_CONTAINS(output, "All Basic Destructor Tests Passed!",
                "Should show all tests passed");
        }, execution_time_basic);
    integration_test_passed_with_time("Basic Destructor", "test_basic.cb", execution_time_basic);
    
    // Test 2: Generic Destructor
    double execution_time_generic;
    run_cb_test_with_output_and_time("../../tests/cases/destructor/test_generic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_generic.cb should execute successfully");
            
            // Test 1: Simple generic
            INTEGRATION_ASSERT_CONTAINS(output, "[Container<int>] Destructor called",
                "Should call generic destructor");
            
            // Test 2: Generic with trait bound
            INTEGRATION_ASSERT_CONTAINS(output, "[Vector<int, SystemAllocator>] Destructor",
                "Should call generic destructor with trait bound");
            INTEGRATION_ASSERT_CONTAINS(output, "Freeing memory at",
                "Should free memory in destructor");
            
            // Test 4: nullptr handling
            INTEGRATION_ASSERT_CONTAINS(output, "data is nullptr",
                "Should handle nullptr correctly");
            
            // Success message
            INTEGRATION_ASSERT_CONTAINS(output, "All Generic Destructor Tests Passed!",
                "Should show all tests passed");
        }, execution_time_generic);
    integration_test_passed_with_time("Generic Destructor", "test_generic.cb", execution_time_generic);
    
    // Test 3: Scope and Order
    double execution_time_scope;
    run_cb_test_with_output_and_time("../../tests/cases/destructor/test_scope.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_scope.cb should execute successfully");
            
            // LIFO order verification
            INTEGRATION_ASSERT_CONTAINS(output, "[Item] Destructor: id=3",
                "Should destroy item 3 first (LIFO)");
            INTEGRATION_ASSERT_CONTAINS(output, "[Item] Destructor: id=2",
                "Should destroy item 2 second");
            INTEGRATION_ASSERT_CONTAINS(output, "[Item] Destructor: id=1",
                "Should destroy item 1 last");
            
            // Nested scopes
            INTEGRATION_ASSERT_CONTAINS(output, "level=3",
                "Should have deepest level destructor");
            INTEGRATION_ASSERT_CONTAINS(output, "level=2",
                "Should have middle level destructor");
            INTEGRATION_ASSERT_CONTAINS(output, "level=1",
                "Should have outer level destructor");
            
            // Success message
            INTEGRATION_ASSERT_CONTAINS(output, "All Scope Tests Passed!",
                "Should show all tests passed");
        }, execution_time_scope);
    integration_test_passed_with_time("Scope and Order", "test_scope.cb", execution_time_scope);
    
    // Test 4: Vector Destructor
    double execution_time_vector;
    run_cb_test_with_output_and_time("../../tests/cases/destructor/test_vector_destructor.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_vector_destructor.cb should execute successfully");
            
            // Memory management
            INTEGRATION_ASSERT_CONTAINS(output, "Freeing memory at",
                "Should free allocated memory");
            INTEGRATION_ASSERT_CONTAINS(output, "capacity=",
                "Should show vector capacity in destructor");
            
            // nullptr handling
            INTEGRATION_ASSERT_CONTAINS(output, "No memory to free (data is nullptr)",
                "Should handle nullptr case");
            
            // Data preservation
            INTEGRATION_ASSERT_CONTAINS(output, "[0] = 10",
                "Should preserve data until destruction");
            INTEGRATION_ASSERT_CONTAINS(output, "[1] = 20",
                "Should preserve data until destruction");
            INTEGRATION_ASSERT_CONTAINS(output, "[2] = 30",
                "Should preserve data until destruction");
            
            // Success message
            INTEGRATION_ASSERT_CONTAINS(output, "All Vector Destructor Tests Passed!",
                "Should show all tests passed");
        }, execution_time_vector);
    integration_test_passed_with_time("Vector Destructor", "test_vector_destructor.cb", execution_time_vector);
    
    std::cout << "[integration-test] Generic Destructor tests completed" << std::endl;
}
