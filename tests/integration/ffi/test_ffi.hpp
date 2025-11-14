#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_ffi() {
    std::cout << "[integration-test] Running FFI tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: FFI基本的なパース（外部モジュール宣言）
    run_cb_test_with_output_and_time("../cases/ffi/test_ffi_parse.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "FFI parse test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "FFI parser test - declarations parsed successfully",
                                      "Should parse FFI declarations");
        }, execution_time);
    integration_test_passed_with_time("FFI declaration parsing", "test_ffi_parse.cb", execution_time);
    
    // Test 2: 基本的なFFI解析テスト（複数モジュール）
    run_cb_test_with_output_and_time("../cases/ffi/basic_parse_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic FFI parse test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Multiple foreign modules - PASSED",
                                      "Should support multiple foreign modules");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Multiple function declarations - PASSED",
                                      "Should support multiple function declarations");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Different parameter types - PASSED",
                                      "Should support different parameter types");
        }, execution_time);
    integration_test_passed_with_time("FFI multiple modules parsing", "basic_parse_test.cb", execution_time);
    
    // Test 3: double戻り値の正確な伝播
    run_cb_test_with_output_and_time("../cases/ffi/double_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Double return test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: sqrt(2.0) precision - PASSED",
                                      "Should return accurate double values");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: sqrt(9.0) exact - PASSED",
                                      "Should return exact double values");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: pow(2.5, 2.0) - PASSED",
                                      "Should handle multi-argument double functions");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Nested calls - PASSED",
                                      "Should handle nested FFI calls");
        }, execution_time);
    integration_test_passed_with_time("FFI double return value propagation", "double_return.cb", execution_time);
    
    // Test 4: 数学関数の呼び出し
    run_cb_test_with_output_and_time("../cases/ffi/math_functions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Math functions test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: sqrt - PASSED",
                                      "Should call sqrt correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: pow - PASSED",
                                      "Should call pow correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: sin(0) - PASSED",
                                      "Should call sin correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: cos(0) - PASSED",
                                      "Should call cos correctly");
        }, execution_time);
    integration_test_passed_with_time("FFI math library functions", "math_functions.cb", execution_time);
    
    // Test 5: モジュール名前空間
    run_cb_test_with_output_and_time("../cases/ffi/module_namespace.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Module namespace test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Module prefix - PASSED",
                                      "Should support module.function() syntax");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Multiple functions same module - PASSED",
                                      "Should allow multiple functions from same module");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Variable assignment - PASSED",
                                      "Should assign FFI results to variables");
        }, execution_time);
    integration_test_passed_with_time("FFI module namespace", "module_namespace.cb", execution_time);
    
    // Test 6: 整数関数
    run_cb_test_with_output_and_time("../cases/ffi/int_functions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Int functions test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: abs positive - PASSED",
                                      "Should handle positive abs");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: abs negative - PASSED",
                                      "Should handle negative abs");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: abs zero - PASSED",
                                      "Should handle zero abs");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: abs large - PASSED",
                                      "Should handle large abs");
        }, execution_time);
    integration_test_passed_with_time("FFI integer functions", "int_functions.cb", execution_time);
    
    // Test 7: 三角関数
    run_cb_test_with_output_and_time("../cases/ffi/trigonometric.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Trigonometric test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: sin(0) - PASSED",
                                      "Should calculate sin correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: cos(0) - PASSED",
                                      "Should calculate cos correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: tan(0) - PASSED",
                                      "Should calculate tan correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: asin(0) - PASSED",
                                      "Should calculate asin correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: acos(1) - PASSED",
                                      "Should calculate acos correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: atan(0) - PASSED",
                                      "Should calculate atan correctly");
        }, execution_time);
    integration_test_passed_with_time("FFI trigonometric functions", "trigonometric.cb", execution_time);
    
    // Test 8: 複数モジュール
    run_cb_test_with_output_and_time("../cases/ffi/multi_module.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multiple modules test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: math module - PASSED",
                                      "Should use math module");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: C module - PASSED",
                                      "Should use C module");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: combined modules - PASSED",
                                      "Should use both modules together");
        }, execution_time);
    integration_test_passed_with_time("FFI multiple modules", "multi_module.cb", execution_time);
    
    // Test 9: 文字列関数 (limited support)
    run_cb_test_with_output_and_time("../cases/ffi/string_functions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "String functions test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "String Tests Completed",
                                      "Should complete string tests");
        }, execution_time);
    integration_test_passed_with_time("FFI string functions (limited)", "string_functions.cb", execution_time);
    
    // Test 10: Void戻り値
    run_cb_test_with_output_and_time("../cases/ffi/void_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Void return test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test: void return - PASSED",
                                      "Should handle void return");
        }, execution_time);
    integration_test_passed_with_time("FFI void return", "void_return.cb", execution_time);
    
    // Test 11: 統合テスト（カスタムライブラリ）- test_ffi_basic.cb
    // Note: This test requires custom library libtest_math.dylib in stdlib/foreign/
    // It can be run manually from project root with:
    //   DYLD_LIBRARY_PATH=stdlib/foreign ./main tests/cases/ffi/test_ffi_basic.cb
    // Skipped in automated integration tests due to library path complexity
    std::cout << "[integration-test] [SKIP] FFI custom library integration (test_ffi_basic.cb)" << std::endl;
    std::cout << "[integration-test]        Reason: Requires custom libtest_math.dylib with DYLD_LIBRARY_PATH setup" << std::endl;
    std::cout << "[integration-test]        Manual test: DYLD_LIBRARY_PATH=stdlib/foreign ./main tests/cases/ffi/test_ffi_basic.cb" << std::endl;
    
    std::cout << "[integration-test] FFI tests completed" << std::endl;
}
