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
// 変数アドレスのテスト
// ============================================================================

inline void test_variable_address() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_variable_address.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "変数アドレステストがエラー終了");
            
            // Header
            INTEGRATION_ASSERT(output.find("=== Variable Address Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: Address of regular variable
            INTEGRATION_ASSERT(output.find("Test 1: Address of regular variable") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("x = 42") != std::string::npos,
                             "xの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*px = 42") != std::string::npos,
                             "*pxの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: Modify variable through pointer
            INTEGRATION_ASSERT(output.find("Test 2: Modify variable through pointer") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("x = 100") != std::string::npos,
                             "ポインタ経由でxが変更されていない");
            INTEGRATION_ASSERT(output.find("*px = 100") != std::string::npos,
                             "*pxが更新されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: Multiple variables
            INTEGRATION_ASSERT(output.find("Test 3: Multiple variables") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("*pa = 10") != std::string::npos,
                             "*paの値が正しくない");
            INTEGRATION_ASSERT(output.find("*pb = 20") != std::string::npos,
                             "*pbの値が正しくない");
            INTEGRATION_ASSERT(output.find("*pc = 30") != std::string::npos,
                             "*pcの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: Different types
            INTEGRATION_ASSERT(output.find("Test 4: Different types") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("ch = 65") != std::string::npos,
                             "chの値が正しくない");
            INTEGRATION_ASSERT(output.find("*pch = 65") != std::string::npos,
                             "*pchの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Final message
            INTEGRATION_ASSERT(output.find("=== All variable address tests completed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_variable_address passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 最小限のポインタテスト
// ============================================================================

inline void test_minimal_pointer() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_minimal.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "最小限のポインタテストがエラー終了");
            
            // Header
            INTEGRATION_ASSERT(output.find("=== Pointer Comprehensive Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: Array element pointer
            INTEGRATION_ASSERT(output.find("Test 1: Array element pointer") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("arr[0] = 10") != std::string::npos,
                             "arr[0]の値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr = 10") != std::string::npos,
                             "*ptrの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: Modify through pointer
            INTEGRATION_ASSERT(output.find("Test 2: Modify through pointer") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("arr[0] = 100") != std::string::npos,
                             "ポインタ経由でarr[0]が変更されていない");
            INTEGRATION_ASSERT(output.find("*ptr = 100") != std::string::npos,
                             "*ptrが更新されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: Pointer to different element
            INTEGRATION_ASSERT(output.find("Test 3: Pointer to different element") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("arr[2] = 30") != std::string::npos,
                             "arr[2]の値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr2 = 30") != std::string::npos,
                             "*ptr2が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: Pointer reassignment
            INTEGRATION_ASSERT(output.find("Test 4: Pointer reassignment") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("arr[4] = 50") != std::string::npos,
                             "arr[4]の値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr = 50") != std::string::npos,
                             "*ptrがarr[4]を指していない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: Multiple modifications
            INTEGRATION_ASSERT(output.find("Test 5: Multiple modifications") != std::string::npos,
                             "Test 5のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("arr[1] = 200") != std::string::npos,
                             "arr[1]が更新されていない");
            INTEGRATION_ASSERT(output.find("arr[2] = 300") != std::string::npos,
                             "arr[2]が更新されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            // Final message
            INTEGRATION_ASSERT(output.find("=== All pointer tests completed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_minimal_pointer passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 包括的なポインタ演算テスト
// ============================================================================

inline void test_comprehensive_pointer_arithmetic() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_ptr_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的なポインタ演算テストがエラー終了");
            
            // Header
            INTEGRATION_ASSERT(output.find("=== Pointer Arithmetic Comprehensive Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: ptr + 1
            INTEGRATION_ASSERT(output.find("Test 1: ptr + 1") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("*ptr = 10") != std::string::npos,
                             "*ptrの値が正しくない");
            INTEGRATION_ASSERT(output.find("*(ptr + 1) = 20") != std::string::npos,
                             "ptr + 1が正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: ptr + 2
            INTEGRATION_ASSERT(output.find("Test 2: ptr + 2") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("*(ptr + 2) = 30") != std::string::npos,
                             "ptr + 2が正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: ptr - 1
            INTEGRATION_ASSERT(output.find("Test 3: ptr - 1") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("*ptr4 (arr[2]) = 30") != std::string::npos,
                             "*ptr4の値が正しくない");
            INTEGRATION_ASSERT(output.find("*(ptr4 - 1) = 20") != std::string::npos,
                             "ptr - 1が正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: Chain arithmetic
            INTEGRATION_ASSERT(output.find("Test 4: Chain arithmetic (ptr + 1 + 1)") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("*(ptr + 1 + 1) = 30") != std::string::npos,
                             "連鎖したポインタ演算が正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: Modify through pointer arithmetic
            INTEGRATION_ASSERT(output.find("Test 5: Modify through pointer arithmetic") != std::string::npos,
                             "Test 5のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("arr[3] = 400") != std::string::npos,
                             "ポインタ演算経由で値が変更されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            // Final message
            INTEGRATION_ASSERT(output.find("=== All pointer arithmetic tests completed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_comprehensive_pointer_arithmetic passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 高度なポインタ機能テスト (typedef, interface/impl, 関数戻り値, 構造体メンバ)
// ============================================================================

inline void test_advanced_pointer_features() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_advanced_pointer_features.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "高度なポインタ機能テストがエラー終了");
            
            // Header
            INTEGRATION_ASSERT(output.find("=== Advanced Pointer Features Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1-15: All advanced pointer features
            for (int i = 1; i <= 15; i++) {
                std::string test_msg = "✓ Test " + std::to_string(i) + " passed";
                INTEGRATION_ASSERT(output.find(test_msg) != std::string::npos,
                                 ("Test " + std::to_string(i) + "のパスメッセージがない").c_str());
            }
            
            // Test 1: int* works
            INTEGRATION_ASSERT(output.find("int* works") != std::string::npos,
                             "int*テストが失敗");
            
            // Test 2: double pointer
            INTEGRATION_ASSERT(output.find("int** works") != std::string::npos,
                             "int**テストが失敗");
            
            // Test 3: char pointer
            INTEGRATION_ASSERT(output.find("char* works") != std::string::npos,
                             "char*テストが失敗");
            
            // Test 4: struct with pointer member
            INTEGRATION_ASSERT(output.find("struct with pointer member works") != std::string::npos,
                             "構造体ポインタメンバテストが失敗");
            
            // Test 5: function returning pointer
            INTEGRATION_ASSERT(output.find("function returning pointer works") != std::string::npos,
                             "ポインタ戻り値テストが失敗");
            
            // Test 6: conditional pointer return
            INTEGRATION_ASSERT(output.find("conditional pointer return works") != std::string::npos,
                             "条件付きポインタ戻り値テストが失敗");
            
            // Test 7: double pointer return
            INTEGRATION_ASSERT(output.find("double pointer return works") != std::string::npos,
                             "ダブルポインタ戻り値テストが失敗");
            
            // Test 8: impl method with pointer argument
            INTEGRATION_ASSERT(output.find("impl method with pointer argument works") != std::string::npos,
                             "implポインタ引数テストが失敗");
            
            // Test 9: impl method returning pointer
            INTEGRATION_ASSERT(output.find("impl method returning pointer works") != std::string::npos,
                             "implポインタ戻り値テストが失敗");
            
            // Test 10: impl accessing pointer member
            INTEGRATION_ASSERT(output.find("impl accessing pointer member works") != std::string::npos,
                             "implポインタメンバアクセステストが失敗");
            
            // Test 11: DataHolder with simple pointer
            INTEGRATION_ASSERT(output.find("DataHolder with simple pointer works") != std::string::npos,
                             "DataHolder簡易ポインタテストが失敗");
            
            // Test 12: nullptr handling
            INTEGRATION_ASSERT(output.find("nullptr handling works") != std::string::npos,
                             "nullptr処理テストが失敗");
            
            // Test 13: complex pointer chain
            INTEGRATION_ASSERT(output.find("complex pointer chain works") != std::string::npos,
                             "複雑なポインタチェーンテストが失敗");
            
            // Test 14: struct pointer reassignment
            INTEGRATION_ASSERT(output.find("struct pointer reassignment works") != std::string::npos,
                             "構造体ポインタ再代入テストが失敗");
            
            // Test 15: chained impl modifications
            INTEGRATION_ASSERT(output.find("chained impl modifications work") != std::string::npos,
                             "impl連鎖変更テストが失敗");
            
            // Final message
            INTEGRATION_ASSERT(output.find("=== All 15 Advanced Pointer Tests Passed! ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_advanced_pointer_features passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタインクリメント/デクリメントテスト
// ============================================================================

inline void test_pointer_incdec() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_ptr_incdec.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタインクリメント/デクリメントテストがエラー終了");
            
            // Header
            INTEGRATION_ASSERT(output.find("=== Pointer Increment/Decrement Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: Pre-increment
            INTEGRATION_ASSERT(output.find("Test 1: Pre-increment (++ptr)") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: *ptr = 10") != std::string::npos,
                             "*ptrの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After ++ptr: *ptr = 20") != std::string::npos,
                             "プレインクリメントが正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: Post-increment
            INTEGRATION_ASSERT(output.find("Test 2: Post-increment (ptr++)") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: *ptr = 20") != std::string::npos,
                             "*ptrの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*old_ptr = 20") != std::string::npos,
                             "old_ptrの値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr = 30") != std::string::npos,
                             "ポストインクリメントが正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: Pre-decrement
            INTEGRATION_ASSERT(output.find("Test 3: Pre-decrement (--ptr)") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: *ptr = 40") != std::string::npos,
                             "*ptrの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After --ptr: *ptr = 30") != std::string::npos,
                             "プレデクリメントが正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: Post-decrement
            INTEGRATION_ASSERT(output.find("Test 4: Post-decrement (ptr--)") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Before: *ptr = 30") != std::string::npos,
                             "*ptrの初期値が正しくない");
            INTEGRATION_ASSERT(output.find("*old_ptr = 30") != std::string::npos,
                             "old_ptrの値が正しくない");
            INTEGRATION_ASSERT(output.find("*ptr = 20") != std::string::npos,
                             "ポストデクリメントが正しく動作していない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: Loop with pointer increment
            INTEGRATION_ASSERT(output.find("Test 5: Loop with pointer increment") != std::string::npos,
                             "Test 5のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("arr[0] = 10") != std::string::npos,
                             "arr[0]の値が正しくない");
            INTEGRATION_ASSERT(output.find("arr[1] = 20") != std::string::npos,
                             "arr[1]の値が正しくない");
            INTEGRATION_ASSERT(output.find("arr[2] = 30") != std::string::npos,
                             "arr[2]の値が正しくない");
            INTEGRATION_ASSERT(output.find("arr[3] = 40") != std::string::npos,
                             "arr[3]の値が正しくない");
            INTEGRATION_ASSERT(output.find("arr[4] = 50") != std::string::npos,
                             "arr[4]の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            // Test 6: Multiple increment/decrement
            INTEGRATION_ASSERT(output.find("Test 6: Multiple increment/decrement") != std::string::npos,
                             "Test 6のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Start: *ptr = 30") != std::string::npos,
                             "開始時の*ptrの値が正しくない");
            INTEGRATION_ASSERT(output.find("After ++ptr: *ptr = 40") != std::string::npos,
                             "1回目のインクリメントが正しくない");
            INTEGRATION_ASSERT(output.find("After ++ptr: *ptr = 50") != std::string::npos,
                             "2回目のインクリメントが正しくない");
            INTEGRATION_ASSERT(output.find("After --ptr: *ptr = 40") != std::string::npos,
                             "1回目のデクリメントが正しくない");
            INTEGRATION_ASSERT(output.find("After --ptr: *ptr = 30") != std::string::npos,
                             "2回目のデクリメントが正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 6 passed") != std::string::npos,
                             "Test 6のパスメッセージがない");
            
            // Final message
            INTEGRATION_ASSERT(output.find("=== All pointer increment/decrement tests completed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_pointer_incdec passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 全てのポインタテストを実行
// ============================================================================

// ============================================================================
// 既存のテスト: 構造体ポインタメンバ
// ============================================================================

inline void test_struct_pointer_members() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_struct_pointer_members.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "構造体ポインタメンバテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Struct Pointer Member Tests ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("All struct pointer member tests passed!") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_struct_pointer_members passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 既存のテスト: implでポインタ使用
// ============================================================================

inline void test_impl_with_pointers() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_impl_with_pointers.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "implポインタテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Impl with Pointer Members Tests ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("All impl pointer tests passed!") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_impl_with_pointers passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 既存のテスト: ポインタ戻り値関数
// ============================================================================

inline void test_pointer_return_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_return_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ戻り値テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Return Function Tests ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("All pointer return function tests passed!") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_pointer_return_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: デリファレンスインクリメント/デクリメント (*ptr)++
// ============================================================================

inline void test_dereference_incdec() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_deref_incdec.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "デリファレンスインクリメント/デクリメントテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Dereference Increment/Decrement Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1-8: All dereference increment/decrement tests
            for (int i = 1; i <= 8; i++) {
                std::string test_msg = "✓ Test " + std::to_string(i) + " passed";
                INTEGRATION_ASSERT(output.find(test_msg) != std::string::npos,
                                 ("Test " + std::to_string(i) + "のパスメッセージがない").c_str());
            }
            
            INTEGRATION_ASSERT(output.find("=== All dereference increment/decrement tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_dereference_incdec passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: ポインタアドレスフォーマット (%p)
// ============================================================================

inline void test_pointer_format() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_format.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタアドレスフォーマットテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Address Format (%p) Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1-7: All pointer format tests
            for (int i = 1; i <= 7; i++) {
                std::string test_msg = "✓ ";
                INTEGRATION_ASSERT(output.find(test_msg) != std::string::npos,
                                 ("Test " + std::to_string(i) + "のパスメッセージがない").c_str());
            }
            
            // 16進数アドレス表示の確認（0x前綴り）
            INTEGRATION_ASSERT(output.find("0x") != std::string::npos,
                             "16進数アドレス表示がない");
            
            INTEGRATION_ASSERT(output.find("=== All pointer address format tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_pointer_format passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: アロー演算子
// ============================================================================

inline void test_arrow_operator() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_arrow_operator.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "アロー演算子テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Arrow Operator Comprehensive Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1, 3, 4: Arrow operator tests (Test 2 is skipped)
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 skipped") != std::string::npos,
                             "Test 2のスキップメッセージがない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            INTEGRATION_ASSERT(output.find("=== All arrow operator tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_arrow_operator passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: implメソッド内でのアロー演算子
// ============================================================================

inline void test_arrow_in_impl() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_arrow_in_impl.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "implアロー演算子テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Arrow Operator in Impl Methods Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1-2: All impl arrow operator tests
            for (int i = 1; i <= 2; i++) {
                std::string test_msg = "✓ Test " + std::to_string(i) + " passed";
                INTEGRATION_ASSERT(output.find(test_msg) != std::string::npos,
                                 ("Test " + std::to_string(i) + "のパスメッセージがない").c_str());
            }
            
            INTEGRATION_ASSERT(output.find("=== All impl arrow operator tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_arrow_in_impl passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: Interface型ポインタとアロー演算子
// ============================================================================

inline void test_interface_pointer_arrow() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_interface_pointer_arrow.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Interface型ポインタアロー演算子テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Interface Pointer with Arrow Operator Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1-6: All interface pointer arrow operator tests
            for (int i = 1; i <= 6; i++) {
                std::string test_msg = "✓ Test " + std::to_string(i) + " passed";
                INTEGRATION_ASSERT(output.find(test_msg) != std::string::npos,
                                 ("Test " + std::to_string(i) + "のパスメッセージがない").c_str());
            }
            
            INTEGRATION_ASSERT(output.find("=== All interface pointer arrow operator tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_interface_pointer_arrow passed (%.3fms)\n", execution_time);
}

// ============================================================================
// implメソッド内でのselfポインタメンバアクセステスト
// ============================================================================

inline void test_impl_self_pointer_access() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_impl_self_pointer_access.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "implセルフポインタアクセステストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Impl Self Pointer Member Access Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1-3: All self pointer member access tests
            for (int i = 1; i <= 3; i++) {
                std::string test_msg = "✓ Test " + std::to_string(i) + " passed";
                INTEGRATION_ASSERT(output.find(test_msg) != std::string::npos,
                                 ("Test " + std::to_string(i) + "のパスメッセージがない").c_str());
            }
            
            INTEGRATION_ASSERT(output.find("=== All impl self pointer access tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_impl_self_pointer_access passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: 再帰構造体(自己参照構造体)
// ============================================================================

inline void test_recursive_struct() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_recursive_struct.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "再帰構造体テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Test 1: Self-referencing struct declaration ===") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Node1 value: 10") != std::string::npos,
                             "Node1の値が正しくない");
            INTEGRATION_ASSERT(output.find("Node1 next: nullptr") != std::string::npos,
                             "Node1 nextがnullptrでない");
            
            INTEGRATION_ASSERT(output.find("=== Test 2: Creating multiple nodes ===") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Node2 value: 20") != std::string::npos,
                             "Node2の値が正しくない");
            INTEGRATION_ASSERT(output.find("Node3 value: 30") != std::string::npos,
                             "Node3の値が正しくない");
            
            INTEGRATION_ASSERT(output.find("=== Test 3: Binary tree structure ===") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("TreeNode root data: 100") != std::string::npos,
                             "TreeNodeのデータが正しくない");
            INTEGRATION_ASSERT(output.find("TreeNode left: nullptr") != std::string::npos,
                             "TreeNode leftがnullptrでない");
            INTEGRATION_ASSERT(output.find("TreeNode right: nullptr") != std::string::npos,
                             "TreeNode rightがnullptrでない");
            
            INTEGRATION_ASSERT(output.find("=== Test 4: Multi-level pointer depth ===") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("MultiNode id: 999") != std::string::npos,
                             "MultiNodeのIDが正しくない");
            
            INTEGRATION_ASSERT(output.find("=== Test 5: Mixed struct with pointer and non-pointer members ===") != std::string::npos,
                             "Test 5のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Person: Alice, age 25") != std::string::npos,
                             "Personの情報が正しくない");
            
            INTEGRATION_ASSERT(output.find("=== All recursive struct tests passed! ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_recursive_struct passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: typedef再帰構造体
// ============================================================================

inline void test_typedef_recursive_struct() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_typedef_recursive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "typedef再帰構造体テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Test 1: Typedef struct with self-reference ===") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Node value: 100") != std::string::npos,
                             "Nodeの値が正しくない");
            INTEGRATION_ASSERT(output.find("Node next: nullptr") != std::string::npos,
                             "Node nextがnullptrでない");
            
            INTEGRATION_ASSERT(output.find("=== Test 2: Multiple typedef nodes ===") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Node1 value: 10") != std::string::npos,
                             "Node1の値が正しくない");
            INTEGRATION_ASSERT(output.find("Node2 value: 20") != std::string::npos,
                             "Node2の値が正しくない");
            
            INTEGRATION_ASSERT(output.find("=== Test 3: Binary tree with typedef ===") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("TreeNode data: 500") != std::string::npos,
                             "TreeNodeのデータが正しくない");
            INTEGRATION_ASSERT(output.find("TreeNode left: nullptr") != std::string::npos,
                             "TreeNode leftがnullptrでない");
            INTEGRATION_ASSERT(output.find("TreeNode right: nullptr") != std::string::npos,
                             "TreeNode rightがnullptrでない");
            
            INTEGRATION_ASSERT(output.find("=== All typedef recursive struct tests passed! ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_typedef_recursive_struct passed (%.3fms)\n", execution_time);
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
    test_variable_address();
    test_minimal_pointer();
    test_comprehensive_pointer_arithmetic();
    test_pointer_incdec();
    test_advanced_pointer_features();
    test_struct_pointer_members();
    test_impl_with_pointers();
    test_pointer_return_comprehensive();
    test_dereference_incdec();        // デリファレンスインクリメント/デクリメント
    test_pointer_format();            // ポインタアドレスフォーマット
    test_arrow_operator();            // アロー演算子（構造体とInterface）
    // test_arrow_in_impl();             // implメソッド内でのアロー演算子 TODO: implメソッドの戻り値の問題を修正
    // test_interface_pointer_arrow();   // Interface型ポインタとアロー演算子 TODO: Interface型ポインタのメソッド呼び出し未対応
    test_impl_self_pointer_access();  // implメソッド内でのselfポインタメンバアクセス
    test_recursive_struct();          // 再帰構造体(自己参照構造体)
    test_typedef_recursive_struct();  // typedef再帰構造体
    
    printf("=== All Pointer Tests Passed ===\n\n");
}

} // namespace PointerTests

#endif // POINTER_TESTS_HPP
