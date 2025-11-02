#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_generic_constructor() {
    std::cout << "[integration-test] Running Generic Constructor/Destructor tests..." << std::endl;
    
    // Test 1: Basic generic constructor
    double execution_time_1;
    run_cb_test_with_output_and_time(
        "../../tests/cases/generic_constructor/basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "basic.cb should execute successfully");
            
            // Header check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Generic Constructor Test ===",
                "Should show test header");
            
            // Constructor calls check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Box<T> constructor called: value= 42",
                "Box<int> constructor should be called with value=42");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Box<T> constructor called: value= 123456789",
                "Box<long> constructor should be called with value=123456789");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Box<T> constructor called: value= 999",
                "Box<short> constructor should be called with value=999");
            
            // Destructor calls check (3 times)
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Box<T> destructor called: value= 999",
                "Box<short> destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Box<T> destructor called: value= 123456789",
                "Box<long> destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "Box<T> destructor called: value= 42",
                "Box<int> destructor should be called");
            
            // Completion check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should show completion message");
        },
        execution_time_1
    );
    integration_test_passed_with_time(
        "Generic Constructor", 
        "basic.cb", 
        execution_time_1
    );
    
    // Test 2: sizeof with nested structs and generic structs
    double execution_time_2;
    run_cb_test_with_output_and_time(
        "../../tests/cases/generic_constructor/sizeof_nested.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "sizeof_nested.cb should execute successfully");
            
            // Header check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== sizeof() Test ===",
                "Should show test header");
            
            // Primitive types sizeof check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(int) =  4",
                "sizeof(int) should be 4");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(long) =  8",
                "sizeof(long) should be 8");
            
            // Struct sizeof check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(Point) =  8",
                "sizeof(Point) should be 8 (int+int)");
            
            // Nested struct sizeof check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(Rectangle) =  16",
                "sizeof(Rectangle) should be 16 (Point+Point)");
            
            // Generic struct sizeof check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(Box<int>) =  8",
                "sizeof(Box<int>) should be 8");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(Box<long>) =  12",
                "sizeof(Box<long>) should be 12");
            
            // Constructor sizeof(self) check
            INTEGRATION_ASSERT_CONTAINS(output,
                "b1.size =  8",
                "b1.size should be 8 (sizeof(Box<int>))");
            INTEGRATION_ASSERT_CONTAINS(output,
                "b2.size =  12",
                "b2.size should be 12 (sizeof(Box<long>))");
            
            // Completion check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should show completion message");
        },
        execution_time_2
    );
    integration_test_passed_with_time(
        "Generic Constructor", 
        "sizeof_nested.cb", 
        execution_time_2
    );
    
    // Test 3: sizeof(T) in constructor
    double execution_time_3;
    run_cb_test_with_output_and_time(
        "../../tests/cases/generic_constructor/sizeof_in_constructor.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "sizeof_in_constructor.cb should execute successfully");
            
            // Header check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== sizeof(T) in Constructor Test ===",
                "Should show test header");
            
            // Container<int> check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(T) =  4",
                "sizeof(T) for int should be 4");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "✓ sizeof(T) correctly evaluated as 4",
                "Container<int> test should pass");
            
            // Container<long> check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(T) =  8",
                "sizeof(T) for long should be 8");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "✓ sizeof(T) correctly evaluated as 8",
                "Container<long> test should pass");
            
            // Container<short> check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(T) =  2",
                "sizeof(T) for short should be 2");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "✓ sizeof(T) correctly evaluated as 2",
                "Container<short> test should pass");
            
            // Completion check
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All Tests Passed ===",
                "Should show completion message");
        },
        execution_time_3
    );
    integration_test_passed_with_time(
        "Generic Constructor", 
        "sizeof_in_constructor.cb", 
        execution_time_3
    );
    
    std::cout << "[integration-test] Generic Constructor/Destructor tests completed" << std::endl;
}
