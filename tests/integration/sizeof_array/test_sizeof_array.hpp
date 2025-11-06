#pragma once

#include "../framework/integration_test_framework.hpp"
#include <string>

inline void register_sizeof_array_tests() {
    std::cout << "[integration-test] Running sizeof Array tests..." << std::endl;
    
    // Test 1: Comprehensive array sizeof test
    double execution_time_1;
    run_cb_test_with_output_and_time(
        "../../tests/cases/sizeof_array/sizeof_array_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, 
                "sizeof_array_comprehensive.cb should execute successfully");
            
            // 1D Arrays - Integer Types
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== 1D Arrays - Integer Types ===",
                "Should show 1D integer arrays header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(int[5]) =  20  (expected: 20)",
                "sizeof(int[5]) should be 20");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(long[10]) =  80  (expected: 80)",
                "sizeof(long[10]) should be 80");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(short[3]) =  6  (expected: 6)",
                "sizeof(short[3]) should be 6");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(tiny[8]) =  8  (expected: 8)",
                "sizeof(tiny[8]) should be 8");
            
            // 1D Arrays - Floating Point Types
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== 1D Arrays - Floating Point Types ===",
                "Should show 1D float arrays header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(float[4]) =  16  (expected: 16)",
                "sizeof(float[4]) should be 16");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(double[5]) =  40  (expected: 40)",
                "sizeof(double[5]) should be 40");
            
            // 2D Arrays
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== 2D Arrays ===",
                "Should show 2D arrays header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(int[3][4]) =  48  (expected: 48)",
                "sizeof(int[3][4]) should be 48");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(long[2][3]) =  48  (expected: 48)",
                "sizeof(long[2][3]) should be 48");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(short[5][2]) =  20  (expected: 20)",
                "sizeof(short[5][2]) should be 20");
            
            // 3D Arrays
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== 3D Arrays ===",
                "Should show 3D arrays header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(int[2][3][4]) =  96  (expected: 96)",
                "sizeof(int[2][3][4]) should be 96");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(short[2][2][2]) =  16  (expected: 16)",
                "sizeof(short[2][2][2]) should be 16");
            
            // Large Arrays
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Large Arrays ===",
                "Should show large arrays header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(int[100]) =  400  (expected: 400)",
                "sizeof(int[100]) should be 400");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(long[50]) =  400  (expected: 400)",
                "sizeof(long[50]) should be 400");
            
            // Single Element Arrays
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== Single Element Arrays ===",
                "Should show single element arrays header");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(int[1]) =  4  (expected: 4)",
                "sizeof(int[1]) should be 4");
            INTEGRATION_ASSERT_CONTAINS(output, 
                "sizeof(long[1]) =  8  (expected: 8)",
                "sizeof(long[1]) should be 8");
            
            INTEGRATION_ASSERT_CONTAINS(output, 
                "=== All tests completed ===",
                "Should show completion message");
        },
        execution_time_1
    );
    
    std::cout << "[integration-test] sizeof Array tests completed" << std::endl;
}
