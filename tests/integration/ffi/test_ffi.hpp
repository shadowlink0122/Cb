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
    
    // Note: Tests 3-10 require actual FFI library loading which may not work on all CI environments
    // These tests are skipped in CI but can be run manually in local development
    std::cout << "[integration-test] [SKIP] FFI runtime tests (double_return.cb, math_functions.cb, etc.)" << std::endl;
    std::cout << "[integration-test]        Reason: Requires system libraries with proper ELF headers" << std::endl;
    std::cout << "[integration-test]        Manual test: ./main tests/cases/ffi/double_return.cb" << std::endl;
    
    // Test 11: 統合テスト（カスタムライブラリ）- test_ffi_basic.cb
    // Note: This test requires custom library libtest_math.dylib in stdlib/foreign/
    // It can be run manually from project root with:
    //   DYLD_LIBRARY_PATH=stdlib/foreign ./main tests/cases/ffi/test_ffi_basic.cb
    // Skipped in automated integration tests due to library path complexity
    std::cout << "[integration-test] [SKIP] FFI custom library integration (test_ffi_basic.cb)" << std::endl;
    std::cout << "[integration-test]        Reason: Requires custom libtest_math.dylib with DYLD_LIBRARY_PATH setup" << std::endl;
    std::cout << "[integration-test]        Manual test: DYLD_LIBRARY_PATH=stdlib/foreign ./main tests/cases/ffi/test_ffi_basic.cb" << std::endl;
    
    std::cout << "[integration-test] FFI tests completed (parsing only in CI)" << std::endl;
}
