#pragma once

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace BasicStructTests {

// 基本的な構造体メンバアクセステスト
inline void test_basic_struct_member_access() {
    std::cout << "[integration-test] Running test_basic_struct_member_access..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/test_basic_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic struct member access test should exit with code 0");
            INTEGRATION_ASSERT(output.find("=== Basic Struct Test ===") != std::string::npos, 
                              "Output should contain test header");
            INTEGRATION_ASSERT(output.find("Point p1:") != std::string::npos, 
                              "Output should contain point header");
            INTEGRATION_ASSERT(output.find("10") != std::string::npos, 
                              "Output should contain x coordinate value");
            INTEGRATION_ASSERT(output.find("20") != std::string::npos, 
                              "Output should contain y coordinate value");
            INTEGRATION_ASSERT(output.find("Point array:") != std::string::npos, 
                              "Output should contain array header");
            INTEGRATION_ASSERT(output.find("=== Test completed ===") != std::string::npos, 
                              "Output should contain completion message");
        }, execution_time);
    
    std::cout << "[integration-test] Basic struct member access test completed in " 
              << std::fixed << std::setprecision(3) << (execution_time / 1000.0) << " seconds" << std::endl;
}

// 構造体配列のメンバアクセステスト
inline void test_struct_array_member_access() {
    std::cout << "[integration-test] Running test_struct_array_member_access..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/struct_array_access.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct array member access test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Point:") != std::string::npos, 
                              "Output should contain point headers");
            INTEGRATION_ASSERT(output.find("1") != std::string::npos, 
                              "Output should contain x coordinate value 1");
            INTEGRATION_ASSERT(output.find("2") != std::string::npos, 
                              "Output should contain y coordinate value 2");
            INTEGRATION_ASSERT(output.find("3") != std::string::npos, 
                              "Output should contain x coordinate value 3");
            INTEGRATION_ASSERT(output.find("4") != std::string::npos, 
                              "Output should contain y coordinate value 4");
        }, execution_time);
    
    std::cout << "[integration-test] Struct array member access test completed in " 
              << std::fixed << std::setprecision(3) << (execution_time / 1000.0) << " seconds" << std::endl;
}

// 構造体メンバの動的代入テスト
inline void test_struct_member_assignment() {
    std::cout << "[integration-test] Running test_struct_member_assignment..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/struct_member_assignment.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct member assignment test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Initial values:") != std::string::npos, 
                              "Output should contain initial values header");
            INTEGRATION_ASSERT(output.find("After assignment:") != std::string::npos, 
                              "Output should contain after assignment header");
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, 
                              "Output should contain updated value 100");
            INTEGRATION_ASSERT(output.find("200") != std::string::npos, 
                              "Output should contain updated value 200");
            INTEGRATION_ASSERT(output.find("300") != std::string::npos, 
                              "Output should contain updated value 300");
            INTEGRATION_ASSERT(output.find("400") != std::string::npos, 
                              "Output should contain updated value 400");
        }, execution_time);
    
    std::cout << "[integration-test] Struct member assignment test completed in " 
              << std::fixed << std::setprecision(3) << (execution_time / 1000.0) << " seconds" << std::endl;
}

// 混合型構造体メンバテスト
inline void test_mixed_type_struct_members() {
    std::cout << "[integration-test] Running test_mixed_type_struct_members..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/mixed_type_members.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Mixed type struct members test should exit with code 0");
            INTEGRATION_ASSERT(output.find("ID:") != std::string::npos, 
                              "Output should contain ID header");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, 
                              "Output should contain integer member value 42");
            INTEGRATION_ASSERT(output.find("Active:") != std::string::npos, 
                              "Output should contain Active header");
            INTEGRATION_ASSERT(output.find("Name:") != std::string::npos, 
                              "Output should contain Name header");
        }, execution_time);
    
    std::cout << "[integration-test] Mixed type struct members test completed in " 
              << std::fixed << std::setprecision(3) << (execution_time / 1000.0) << " seconds" << std::endl;
}

// 構造体配列とループの組み合わせテスト
inline void test_struct_array_loop() {
    std::cout << "[integration-test] Running test_struct_array_loop..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/struct_array_loop.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct array loop test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Processing 3 points") != std::string::npos, 
                              "Output should contain processing header");
            INTEGRATION_ASSERT(output.find("Point:") != std::string::npos, 
                              "Output should contain Point headers");
            // ループ内での各構造体アクセスを確認
            INTEGRATION_ASSERT(output.find("0") != std::string::npos, 
                              "Output should contain index 0");
            INTEGRATION_ASSERT(output.find("20") != std::string::npos, 
                              "Output should contain calculated value 20");
        }, execution_time);
    
    std::cout << "[integration-test] Struct array loop test completed in " 
              << std::fixed << std::setprecision(3) << (execution_time / 1000.0) << " seconds" << std::endl;
}

// TypedValue構造体データ保持機能テスト
inline void test_typed_value_struct_data() {
    std::cout << "[integration-test] Running test_typed_value_struct_data..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/typed_value_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "TypedValue struct data test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Struct data preserved") != std::string::npos, 
                              "Output should confirm struct data preservation");
            INTEGRATION_ASSERT(output.find("Type inference working") != std::string::npos, 
                              "Output should confirm type inference functionality");
        }, execution_time);
    
    std::cout << "[integration-test] TypedValue struct data test completed in " 
              << std::fixed << std::setprecision(3) << (execution_time / 1000.0) << " seconds" << std::endl;
}

// パフォーマンステスト（基本的な構造体操作）
inline void test_basic_struct_performance() {
    std::cout << "[integration-test] Running test_basic_struct_performance..." << std::endl;
    
    double execution_time_ms;
    run_cb_test_with_output_and_time("../../tests/cases/struct/performance_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic struct performance test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Performance test completed") != std::string::npos, 
                              "Output should contain completion message");
        }, execution_time_ms);
    
    // パフォーマンステストでは実行時間も重要
    // 注意：環境によって実行時間は変動するため、合理的な上限を設定
    // execution_time_msはミリ秒なので、30秒 = 30000ミリ秒
    INTEGRATION_ASSERT(execution_time_ms < 30000.0, "Basic struct operations should complete within 30 seconds");
    
    std::cout << "[integration-test] Basic struct performance test completed in " 
              << std::fixed << std::setprecision(3) << (execution_time_ms / 1000.0) << " seconds" << std::endl;
}

// 全基本構造体テストを実行
inline void run_all_basic_struct_tests() {
    std::cout << "[integration-test] ========================================" << std::endl;
    std::cout << "[integration-test] Running Basic Struct Integration Tests" << std::endl;
    std::cout << "[integration-test] ========================================" << std::endl;
    
    try {
        test_basic_struct_member_access();
        test_struct_array_member_access();
        test_struct_member_assignment();
        test_mixed_type_struct_members();
        test_struct_array_loop();
        test_typed_value_struct_data();
        test_basic_struct_performance();
        
        std::cout << "[integration-test] ========================================" << std::endl;
        std::cout << "[integration-test] All Basic Struct tests completed successfully!" << std::endl;
        std::cout << "[integration-test] ========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[integration-test] Basic Struct test suite failed: " << e.what() << std::endl;
        throw;
    }
}

} // namespace BasicStructTests
