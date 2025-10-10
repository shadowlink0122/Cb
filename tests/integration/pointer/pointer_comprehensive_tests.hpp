#ifndef POINTER_COMPREHENSIVE_TESTS_HPP
#define POINTER_COMPREHENSIVE_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace PointerComprehensiveTests {

// ============================================================================
// 包括的なポインタ操作テスト（アロー構文・デリファレンス構文）
// ============================================================================

inline void test_comprehensive_pointer_operations() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface/test_comprehensive_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的ポインタ操作テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Comprehensive Pointer Test Suite ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("Passed:  20  /  20") != std::string::npos,
                             "20個のテスト全てが通過していない");
            
            INTEGRATION_ASSERT(output.find("✓ ALL TESTS PASSED!") != std::string::npos,
                             "最終成功メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_comprehensive_pointer_operations passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 包括的なアドレス取得テスト（全変数型）
// ============================================================================

inline void test_comprehensive_address_of() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface/test_comprehensive_address_of.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的アドレス取得テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Comprehensive Address-Of Test Suite ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("Passed:  15  /  15") != std::string::npos,
                             "15個のテスト全てが通過していない");
            
            INTEGRATION_ASSERT(output.find("✓ ALL TESTS PASSED!") != std::string::npos,
                             "最終成功メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_comprehensive_address_of passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 構造体ポインタ操作の包括的テスト
// ============================================================================

inline void test_struct_pointer_operations_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_struct_pointer_operations.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "構造体ポインタ操作包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Struct Pointer Operations Comprehensive Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("=== All struct pointer operations tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_struct_pointer_operations_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// インターフェースとimplブロック内でのポインタ操作
// ============================================================================

inline void test_interface_impl_pointer_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_interface_impl_pointer_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "インターフェースとimplポインタ包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Interface Pointer and Impl Block Pointer Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("=== All interface and impl pointer tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_interface_impl_pointer_comprehensive passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Pointer Comprehensive Tests ===\n");
    test_comprehensive_pointer_operations();
    test_comprehensive_address_of();
    test_struct_pointer_operations_comprehensive();
    test_interface_impl_pointer_comprehensive();
    printf("=== Pointer Comprehensive Tests Completed ===\n\n");
}

} // namespace PointerComprehensiveTests

#endif // POINTER_COMPREHENSIVE_TESTS_HPP
