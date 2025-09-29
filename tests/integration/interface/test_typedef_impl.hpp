#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_typedef_impl_basic() {
    std::cout << "[integration] Running test_typedef_impl_basic..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/typedef_impl_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Typedef impl test should exit with code 0");
            
            // MyInt型のテスト
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt toString: MyInt value", "MyInt toString should work");
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt getSize: 1", "MyInt getSize should work");
            
            // MyString型のテスト
            INTEGRATION_ASSERT_CONTAINS(output, "MyString toString: MyString value", "MyString toString should work");
            INTEGRATION_ASSERT_CONTAINS(output, "MyString getSize: 10", "MyString getSize should work");
            
            // IntArray型のテスト
            INTEGRATION_ASSERT_CONTAINS(output, "IntArray toString: IntArray[5]", "IntArray toString should work");
            INTEGRATION_ASSERT_CONTAINS(output, "IntArray getSize: 5", "IntArray getSize should work");
            
            // Matrix型のテスト
            INTEGRATION_ASSERT_CONTAINS(output, "Matrix toString: Matrix[2x2]", "Matrix toString should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Matrix getSize: 4", "Matrix getSize should work");
            
            // Comparableインターフェースのテスト
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt equals: 1", "MyInt equals should work");
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt compare: 0", "MyInt compare should work");
            
            INTEGRATION_ASSERT_CONTAINS(output, "=== テスト完了 ===", "Test should complete successfully");
        }, execution_time);
}
