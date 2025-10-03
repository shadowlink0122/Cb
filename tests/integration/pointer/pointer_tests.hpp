#ifndef POINTER_TESTS_HPP
#define POINTER_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <chrono>

namespace PointerTests {

// ============================================================================
// 基本的なポインタ操作のテスト
// ============================================================================

inline void test_basic_pointer_operations() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_address_and_value_changes.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "基本的なポインタ操作がエラー終了");
            
            // Test 1: Basic pointer operations
            INTEGRATION_ASSERT(output.find("Test 1: Basic pointer operations") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("a = 10") != std::string::npos,
                             "aの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr_a = 10") != std::string::npos,
                             "*ptr_aの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After *ptr_a = 20:") != std::string::npos,
                             "ポインタ経由の代入後のメッセージがない");
            INTEGRATION_ASSERT(output.find("a = 20") != std::string::npos,
                             "ポインタ経由でaが変更されていない");
            INTEGRATION_ASSERT(output.find("*ptr_a = 20") != std::string::npos,
                             "*ptr_aが20に更新されていない");
            
            // Test 2: Pointer reassignment
            INTEGRATION_ASSERT(output.find("Test 2: Pointer reassignment") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("a = 20, b = 30") != std::string::npos,
                             "a,bの値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr_b = 20 (points to a)") != std::string::npos,
                             "ptr_bがaを指していない");
            INTEGRATION_ASSERT(output.find("*ptr_b = 30 (now points to b)") != std::string::npos,
                             "ptr_bの再代入後にbを指していない");
            INTEGRATION_ASSERT(output.find("a = 20 (unchanged)") != std::string::npos,
                             "aが誤って変更されている");
            INTEGRATION_ASSERT(output.find("b = 40 (changed)") != std::string::npos,
                             "bがポインタ経由で変更されていない");
            
            // Test 3: Multiple pointers to same variable
            INTEGRATION_ASSERT(output.find("Test 3: Multiple pointers to same variable") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("c = 50") != std::string::npos,
                             "cの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr1 = 50, *ptr2 = 50") != std::string::npos,
                             "ptr1, ptr2が同じ値を指していない");
            INTEGRATION_ASSERT(output.find("c = 60") != std::string::npos,
                             "cが60に更新されていない");
            INTEGRATION_ASSERT(output.find("*ptr1 = 60, *ptr2 = 60") != std::string::npos,
                             "ptr1, ptr2が両方とも60に更新されていない");
            INTEGRATION_ASSERT(output.find("c = 70") != std::string::npos,
                             "cが70に更新されていない");
            INTEGRATION_ASSERT(output.find("*ptr1 = 70, *ptr2 = 70") != std::string::npos,
                             "ptr1, ptr2が両方とも70に更新されていない");
            
            // Test 4: Double pointer
            INTEGRATION_ASSERT(output.find("Test 4: Double pointer") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("d = 80") != std::string::npos,
                             "dの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr_d = 80") != std::string::npos,
                             "*ptr_dの値が正しくない");
            INTEGRATION_ASSERT(output.find("**ptr_ptr_d = 80") != std::string::npos,
                             "**ptr_ptr_dの値が正しくない");
            INTEGRATION_ASSERT(output.find("d = 90") != std::string::npos,
                             "ダブルポインタ経由でdが更新されていない");
            INTEGRATION_ASSERT(output.find("*ptr_d = 90") != std::string::npos,
                             "ダブルポインタ経由で*ptr_dが更新されていない");
            INTEGRATION_ASSERT(output.find("**ptr_ptr_d = 90") != std::string::npos,
                             "**ptr_ptr_dが更新されていない");
            
            // Test 5: Triple pointer
            INTEGRATION_ASSERT(output.find("Test 5: Triple pointer") != std::string::npos,
                             "Test 5のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("e = 100") != std::string::npos,
                             "eの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr_e = 100") != std::string::npos,
                             "*ptr_eの値が正しくない");
            INTEGRATION_ASSERT(output.find("**ptr_ptr_e = 100") != std::string::npos,
                             "**ptr_ptr_eの値が正しくない");
            INTEGRATION_ASSERT(output.find("***ptr_ptr_ptr_e = 100") != std::string::npos,
                             "***ptr_ptr_ptr_eの値が正しくない");
            INTEGRATION_ASSERT(output.find("e = 110") != std::string::npos,
                             "トリプルポインタ経由でeが更新されていない");
            INTEGRATION_ASSERT(output.find("***ptr_ptr_ptr_e = 110") != std::string::npos,
                             "***ptr_ptr_ptr_eが更新されていない");
        },
        execution_time
    );
    
    printf("[✓] test_basic_pointer_operations passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタを関数パラメータとして使用するテスト
// ============================================================================

inline void test_pointer_function_parameters() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_parameters.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ関数パラメータテストがエラー終了");
            
            // Test 1: Simple pointer parameter
            INTEGRATION_ASSERT(output.find("Test 1: Simple pointer parameter") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: x = 10") != std::string::npos,
                             "xの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After increment: x = 11") != std::string::npos,
                             "increment関数が正しく動作していない");
            
            // Test 2: Multiple modifications
            INTEGRATION_ASSERT(output.find("Test 2: Multiple modifications") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: y = 5") != std::string::npos,
                             "yの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After increment: y = 6") != std::string::npos,
                             "yがインクリメントされていない");
            INTEGRATION_ASSERT(output.find("After double: y = 12") != std::string::npos,
                             "yが倍になっていない");
            
            // Test 3: Swap function
            INTEGRATION_ASSERT(output.find("Test 3: Swap function") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: a = 100, b = 200") != std::string::npos,
                             "a,bの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After swap: a = 200, b = 100") != std::string::npos,
                             "swap関数が正しく動作していない");
            
            // Test 4: Double pointer parameter
            INTEGRATION_ASSERT(output.find("Test 4: Double pointer parameter") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: z = 42") != std::string::npos,
                             "zの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After modification: z = 999") != std::string::npos,
                             "ダブルポインタパラメータ経由でzが変更されていない");
        },
        execution_time
    );
    
    printf("[✓] test_pointer_function_parameters passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタチェーンのテスト
// ============================================================================

inline void test_pointer_chains() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_chains.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタチェーンテストがエラー終了");
            
            // Test 1: Pointer chain modifications
            INTEGRATION_ASSERT(output.find("Test 1: Pointer chain modifications") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Initial: original = 1") != std::string::npos,
                             "originalの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After *p1 = 10: original = 10") != std::string::npos,
                             "*p1経由でoriginalが更新されていない");
            INTEGRATION_ASSERT(output.find("After **p2 = 20: original = 20") != std::string::npos,
                             "**p2経由でoriginalが更新されていない");
            INTEGRATION_ASSERT(output.find("After ***p3 = 30: original = 30") != std::string::npos,
                             "***p3経由でoriginalが更新されていない");
            INTEGRATION_ASSERT(output.find("Verification: *p1 = 30, **p2 = 30, ***p3 = 30") != std::string::npos,
                             "全てのポインタが同じ値を指していない");
            
            // Test 2: Redirecting pointer chains
            INTEGRATION_ASSERT(output.find("Test 2: Redirecting pointer chains") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("var1 = 100, var2 = 200") != std::string::npos,
                             "var1, var2の初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr = 100 (points to var1)") != std::string::npos,
                             "ptrがvar1を指していない");
            INTEGRATION_ASSERT(output.find("After *ptr = 150: var1 = 150") != std::string::npos,
                             "ptr経由でvar1が更新されていない");
            INTEGRATION_ASSERT(output.find("*ptr = 200 (now points to var2)") != std::string::npos,
                             "ptrがvar2に再代入されていない");
            INTEGRATION_ASSERT(output.find("After *ptr = 250: var1 = 150, var2 = 250") != std::string::npos,
                             "ptrの再代入後の値が正しくない");
            
            // Test 3: Sequential assignments
            INTEGRATION_ASSERT(output.find("Test 3: Sequential assignments") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Initial: val = 0") != std::string::npos,
                             "valの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After adding 1: val = 1") != std::string::npos,
                             "1回目の加算が正しくない");
            INTEGRATION_ASSERT(output.find("After adding 2: val = 3") != std::string::npos,
                             "2回目の加算が正しくない");
            INTEGRATION_ASSERT(output.find("After adding 3: val = 6") != std::string::npos,
                             "3回目の加算が正しくない");
            INTEGRATION_ASSERT(output.find("After adding 4: val = 10") != std::string::npos,
                             "4回目の加算が正しくない");
            INTEGRATION_ASSERT(output.find("After adding 5: val = 15") != std::string::npos,
                             "5回目の加算が正しくない");
            INTEGRATION_ASSERT(output.find("Final: val = 15") != std::string::npos,
                             "最終的な値が正しくない");
        },
        execution_time
    );
    
    printf("[✓] test_pointer_chains passed (%.3fms)\n", execution_time);
}

// ============================================================================
// nullptrのテスト
// ============================================================================

inline void test_nullptr_checks() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_nullptr_checks.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "nullptrテストがエラー終了");
            
            // Test 1: nullptr initialization
            INTEGRATION_ASSERT(output.find("Test 1: nullptr initialization") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("ptr1 == nullptr (represented as 0)") != std::string::npos,
                             "ptr1のnullptr初期化メッセージがない");
            
            // Test 2: Reassignment to nullptr
            INTEGRATION_ASSERT(output.find("Test 2: Reassignment to nullptr") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: *ptr2 = 42") != std::string::npos,
                             "ptr2の初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After ptr2 = nullptr (pointer now points to null)") != std::string::npos,
                             "nullptrへの再代入メッセージがない");
            
            // Test 3: Multiple nullptr pointers
            INTEGRATION_ASSERT(output.find("Test 3: Multiple nullptr pointers") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("All pointers initialized to nullptr") != std::string::npos,
                             "複数のnullptrポインタ初期化メッセージがない");
            INTEGRATION_ASSERT(output.find("*p1 = 10, *p2 = 20, *p3 = 30") != std::string::npos,
                             "p1, p2, p3が正しく代入されていない");
            
            // Test 4: Double pointer with nullptr
            INTEGRATION_ASSERT(output.find("Test 4: Double pointer with nullptr") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Double pointer initialized to nullptr") != std::string::npos,
                             "ダブルポインタのnullptr初期化メッセージがない");
            INTEGRATION_ASSERT(output.find("**pp = 100") != std::string::npos,
                             "ダブルポインタ経由の値取得が正しくない");
        },
        execution_time
    );
    
    printf("[✓] test_nullptr_checks passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 全てのポインタテストを実行
// ============================================================================

inline void run_all_pointer_tests() {
    printf("\n=== Pointer Tests ===\n");
    
    test_basic_pointer_operations();
    test_pointer_function_parameters();
    test_pointer_chains();
    test_nullptr_checks();
    
    printf("=== All Pointer Tests Passed ===\n\n");
}

} // namespace PointerTests

#endif // POINTER_TESTS_HPP
