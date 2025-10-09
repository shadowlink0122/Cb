#ifndef POINTER_TESTS_HPP
#define POINTER_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <chrono>
#include <sstream>
#include <vector>

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
    
    printf("[integration-test] test_basic_pointer_operations passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_pointer_function_parameters passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_pointer_chains passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_nullptr_checks passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_variable_address passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_minimal_pointer passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_comprehensive_pointer_arithmetic passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_advanced_pointer_features passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_pointer_incdec passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_struct_pointer_members passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_impl_with_pointers passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_pointer_return_comprehensive passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_dereference_incdec passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_pointer_format passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_arrow_operator passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_arrow_in_impl passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_interface_pointer_arrow passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_impl_self_pointer_access passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_recursive_struct passed (%.3fms)\n", execution_time);
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
    
    printf("[integration-test] test_typedef_recursive_struct passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 包括的なポインタ操作テスト（アロー構文・デリファレンス構文）
// ============================================================================

inline void test_comprehensive_pointer_operations() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/interface/test_comprehensive_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的ポインタ操作テストがエラー終了");
            
            // Check for test header
            INTEGRATION_ASSERT(output.find("=== Comprehensive Pointer Test Suite ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Check for all 20 tests passed
            INTEGRATION_ASSERT(output.find("Passed:  20  /  20") != std::string::npos,
                             "20個のテスト全てが通過していない");
            
            INTEGRATION_ASSERT(output.find("✓ ALL TESTS PASSED!") != std::string::npos,
                             "最終成功メッセージが表示されていない");
            
            // Verify specific test outputs
            INTEGRATION_ASSERT(output.find("Test 1: Arrow syntax - ptr->field") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("Test 2: Dereference syntax - (*ptr).field") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("Test 5: Method call with arrow - ptr->method()") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("Test 11: Nested dereference - (*(*o.middle).inner).value") != std::string::npos,
                             "Test 11が実行されていない");
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
            
            // Check for test header
            INTEGRATION_ASSERT(output.find("=== Comprehensive Address-Of Test Suite ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Check for all 15 tests passed
            INTEGRATION_ASSERT(output.find("Passed:  15  /  15") != std::string::npos,
                             "15個のテスト全てが通過していない");
            
            INTEGRATION_ASSERT(output.find("✓ ALL TESTS PASSED!") != std::string::npos,
                             "最終成功メッセージが表示されていない");
            
            // Verify specific test outputs
            INTEGRATION_ASSERT(output.find("Test 1: Address of local variable") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("Test 5: Address of struct member") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("Test 10: Address of unsigned variable") != std::string::npos,
                             "Test 10が実行されていない");
            INTEGRATION_ASSERT(output.find("Test 11: Const function parameter") != std::string::npos,
                             "Test 11が実行されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_comprehensive_address_of passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: 宣言時初期化の包括的テスト
// ============================================================================

inline void test_declaration_init_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_declaration_init_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "宣言時初期化包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Declaration with Initialization and Basic Operations Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: 宣言時初期化
            INTEGRATION_ASSERT(output.find("Test 1: Declaration with initialization") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("*p = 10") != std::string::npos,
                             "初期化された値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: ポインタ加算 +1
            INTEGRATION_ASSERT(output.find("Test 2: Pointer addition +1") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("*p = 200") != std::string::npos,
                             "ポインタ演算後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: ポインタ加算 +2
            INTEGRATION_ASSERT(output.find("Test 3: Pointer addition +2") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("*p = 400") != std::string::npos,
                             "ポインタ演算+2後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: ポインタ減算
            INTEGRATION_ASSERT(output.find("Test 4: Pointer subtraction") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("*p = 300") != std::string::npos,
                             "ポインタ演算-1後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: 後置インクリメント
            INTEGRATION_ASSERT(output.find("Test 5: Post-increment") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("After q++: *q = 200") != std::string::npos,
                             "インクリメント後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            // Test 6: 後置デクリメント
            INTEGRATION_ASSERT(output.find("Test 6: Post-decrement") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("After q--: *q = 100") != std::string::npos,
                             "デクリメント後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 6 passed") != std::string::npos,
                             "Test 6のパスメッセージがない");
            
            // Test 7: 複数ポインタ
            INTEGRATION_ASSERT(output.find("Test 7: Multiple pointers to same array") != std::string::npos,
                             "Test 7が実行されていない");
            INTEGRATION_ASSERT(output.find("*r = 300") != std::string::npos,
                             "複数ポインタの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 7 passed") != std::string::npos,
                             "Test 7のパスメッセージがない");
            
            // Test 8: ポインタ経由の値変更
            INTEGRATION_ASSERT(output.find("Test 8: Value modification through pointer") != std::string::npos,
                             "Test 8が実行されていない");
            INTEGRATION_ASSERT(output.find("After *r = 999: arr[2] = 999, *r = 999") != std::string::npos,
                             "変更後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 8 passed") != std::string::npos,
                             "Test 8のパスメッセージがない");
            
            // Test 9: 別ポインタ
            INTEGRATION_ASSERT(output.find("Test 9: Another pointer to same element") != std::string::npos,
                             "Test 9が実行されていない");
            INTEGRATION_ASSERT(output.find("*s = 999") != std::string::npos,
                             "値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 9 passed") != std::string::npos,
                             "Test 9のパスメッセージがない");
            
            // Test 10: アドレス表示
            INTEGRATION_ASSERT(output.find("Test 10: Pointer address display") != std::string::npos,
                             "Test 10が実行されていない");
            INTEGRATION_ASSERT(output.find("Pointer value: p = 0x") != std::string::npos,
                             "アドレスが16進数表示されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 10 passed") != std::string::npos,
                             "Test 10のパスメッセージがない");
            
            INTEGRATION_ASSERT(output.find("=== All declaration and basic operation tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_declaration_init_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: アドレス表示の包括的テスト
// ============================================================================

inline void test_address_display_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_address_display_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "アドレス表示包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Address Display and Pointer Arithmetic Detailed Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: 16進数アドレス表示
            INTEGRATION_ASSERT(output.find("Test 1: Hexadecimal address display") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("p = 0x") != std::string::npos,
                             "アドレスが16進数表示されていない");
            INTEGRATION_ASSERT(output.find("*p = 10") != std::string::npos,
                             "ポインタの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: ポインタ演算とアドレス遷移
            INTEGRATION_ASSERT(output.find("Test 2: Pointer arithmetic and address transition") != std::string::npos,
                             "Test 2が実行されていない");
            // アドレス遷移を複数回確認
            int hex_count = 0;
            size_t pos = 0;
            while ((pos = output.find("0x", pos)) != std::string::npos) {
                hex_count++;
                pos += 2;
            }
            INTEGRATION_ASSERT(hex_count >= 5, "アドレスの16進数表示が不足している");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: ポインタ減算
            INTEGRATION_ASSERT(output.find("Test 3: Pointer subtraction") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("Before: p = 0x") != std::string::npos,
                             "減算前のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("*p = 30") != std::string::npos,
                             "減算前の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: 複数ポインタのアドレス比較
            INTEGRATION_ASSERT(output.find("Test 4: Multiple pointers address comparison") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("p1 = 0x") != std::string::npos,
                             "p1のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("p2 = 0x") != std::string::npos,
                             "p2のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("p3 = 0x") != std::string::npos,
                             "p3のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: インクリメント/デクリメントとアドレス
            INTEGRATION_ASSERT(output.find("Test 5: Increment/Decrement and address") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("Initial q = 0x") != std::string::npos,
                             "初期アドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("After q++: q = 0x") != std::string::npos,
                             "インクリメント後のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("After q--: q = 0x") != std::string::npos,
                             "デクリメント後のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            // Test 6: ポインタ変数自体のアドレス
            INTEGRATION_ASSERT(output.find("Test 6: Address of pointer variable itself") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("Pointer value: p = 0x") != std::string::npos,
                             "ポインタ値のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 6 passed") != std::string::npos,
                             "Test 6のパスメッセージがない");
            
            INTEGRATION_ASSERT(output.find("=== All address display and arithmetic tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_address_display_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: 構造体ポインタ操作の包括的テスト
// ============================================================================

inline void test_struct_pointer_operations_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_struct_pointer_operations.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "構造体ポインタ操作包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Struct Pointer Operations Comprehensive Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: 構造体変数へのポインタ
            INTEGRATION_ASSERT(output.find("Test 1: Pointer to struct variable") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("  (*ptr).x = 10, (*ptr).y = 20") != std::string::npos,
                             "ポインタの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: ポインタ経由の構造体メンバ変更
            INTEGRATION_ASSERT(output.find("Test 2: Modify struct through pointer") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("After modification: p1.x = 100, p1.y = 200") != std::string::npos,
                             "変更後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: 別のPoint構造体へのポインタ
            INTEGRATION_ASSERT(output.find("Test 3: Another Point pointer") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("  (*p3ptr).x = 50, (*p3ptr).y = 75") != std::string::npos,
                             "値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: ポインタ経由での値変更
            INTEGRATION_ASSERT(output.find("Test 4: Modify Point through pointer") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("Modified via pointer: p3.x = 150, p3.y = 250") != std::string::npos,
                             "変更後の値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: 複数の構造体ポインタ
            INTEGRATION_ASSERT(output.find("Test 5: Multiple struct pointers") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("  (*pa_ptr).x = 1, (*pb_ptr).x = 3, (*pc_ptr).x = 5") != std::string::npos,
                             "複数ポインタの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            // Test 6: 構造体ポインタの再代入
            INTEGRATION_ASSERT(output.find("Test 6: Struct pointer reassignment") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("  initially points to s6a: (*ptr6).x = 111") != std::string::npos,
                             "初期状態のポインタが正しくない");
            INTEGRATION_ASSERT(output.find("  after reassignment to s6b: (*ptr6).x = 333") != std::string::npos,
                             "再代入後のポインタが正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 6 passed") != std::string::npos,
                             "Test 6のパスメッセージがない");
            
            INTEGRATION_ASSERT(output.find("=== All struct pointer operations tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_struct_pointer_operations_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: インターフェースとimplブロック内でのポインタ操作
// ============================================================================

inline void test_interface_impl_pointer_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_interface_impl_pointer_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "インターフェースとimplポインタ包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Interface Pointer and Impl Block Pointer Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: インターフェースポインタ経由のメソッド呼び出し
            INTEGRATION_ASSERT(output.find("Test 1: Interface pointer method calls") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("(*shape_ptr).area() = 200") != std::string::npos,
                             "area()の結果が正しくない");
            INTEGRATION_ASSERT(output.find("(*shape_ptr).perimeter() = 60") != std::string::npos,
                             "perimeter()の結果が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: impl内でのポインタ操作
            INTEGRATION_ASSERT(output.find("Test 2: Pointer operations inside impl") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("test_pointer_ops() = 60") != std::string::npos,
                             "implブロック内のポインタ操作結果が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: impl内でのアドレス調査
            INTEGRATION_ASSERT(output.find("Test 3: Address investigation inside impl") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("test_address_investigation() = 100 (success)") != std::string::npos,
                             "アドレス調査の結果が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: インターフェースポインタ経由でimpl内ポインタテストを呼び出し
            INTEGRATION_ASSERT(output.find("Test 4: Impl pointer tests via interface pointer") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("(*sp).test_pointer_ops() = 60") != std::string::npos,
                             "インターフェースポインタ経由の呼び出し結果が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: 複数のimplメソッド呼び出し
            INTEGRATION_ASSERT(output.find("Test 5: Multiple impl method calls") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("Multiple calls succeeded") != std::string::npos,
                             "複数メソッド呼び出しが成功していない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            INTEGRATION_ASSERT(output.find("=== All interface and impl pointer tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_interface_impl_pointer_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 新規テスト: ポインタ境界ケースと特殊操作
// ============================================================================

inline void test_pointer_boundary_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_boundary_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ境界ケース包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Boundary and Special Operations Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: 複数の宣言時初期化
            INTEGRATION_ASSERT(output.find("Test 1: Multiple declaration-time initialization") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("*p1 = 10, *p2 = 20, *p3 = 50") != std::string::npos,
                             "複数ポインタの初期化が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: ポインタ代入の連鎖
            INTEGRATION_ASSERT(output.find("Test 2: Chained pointer assignment") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("*q1 = *q2 = *q3 = 100") != std::string::npos,
                             "連鎖代入の値が正しくない");
            INTEGRATION_ASSERT(output.find("Modified arr2[0] via q3: arr2[0] = 999") != std::string::npos,
                             "値の変更が反映されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2のパスメッセージがない");
            
            // Test 3: ゼロ要素へのポインタと合計計算
            INTEGRATION_ASSERT(output.find("Test 3: Pointer to zero-th element") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("Sum of array[0..9] = 45") != std::string::npos,
                             "配列の合計が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3のパスメッセージがない");
            
            // Test 4: 逆方向ポインタ移動と積計算
            INTEGRATION_ASSERT(output.find("Test 4: Backward pointer movement") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("Product of arr3 (backward) = 120") != std::string::npos,
                             "逆方向の積が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4のパスメッセージがない");
            
            // Test 5: 回文チェック
            INTEGRATION_ASSERT(output.find("Test 5: Array palindrome check with pointers") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("Palindrome check passed") != std::string::npos,
                             "回文チェックが失敗している");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5のパスメッセージがない");
            
            // Test 6: インクリメント/デクリメント複合
            INTEGRATION_ASSERT(output.find("Test 6: Mixed pointer increment/decrement") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("Pointer navigation: 40 -> 50 -> 60 -> 50 -> 30") != std::string::npos,
                             "ポインタナビゲーションが正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 6 passed") != std::string::npos,
                             "Test 6のパスメッセージがない");
            
            // Test 7: 16進数アドレスフォーマット確認
            INTEGRATION_ASSERT(output.find("Test 7: Hexadecimal address format verification") != std::string::npos,
                             "Test 7が実行されていない");
            INTEGRATION_ASSERT(output.find("addr_ptr = 0x") != std::string::npos,
                             "アドレスが16進数表示されていない");
            INTEGRATION_ASSERT(output.find("*addr_ptr = 111") != std::string::npos,
                             "ポインタの値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 7 passed") != std::string::npos,
                             "Test 7のパスメッセージがない");
            
            INTEGRATION_ASSERT(output.find("=== All boundary and special operations tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_boundary_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタテスト: コールバック関数
// ============================================================================

inline void test_function_pointer_callback() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_callback.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタコールバックテストがエラー終了");
            
            // Expected output: 15, 5, 50, 15, 56, 18
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            
            INTEGRATION_ASSERT(lines.size() >= 6, "出力行数が不足している");
            
            INTEGRATION_ASSERT(lines[0].find("15") != std::string::npos,
                             "Test 1: applyOperation(10, 5, &add)が正しくない");
            INTEGRATION_ASSERT(lines[1].find("5") != std::string::npos,
                             "Test 2: applyOperation(10, 5, &subtract)が正しくない");
            INTEGRATION_ASSERT(lines[2].find("50") != std::string::npos,
                             "Test 3: compute(10, 5, &multiply)が正しくない");
            INTEGRATION_ASSERT(lines[3].find("15") != std::string::npos,
                             "Test 4: compute(10, 5, &add)が正しくない");
            INTEGRATION_ASSERT(lines[4].find("56") != std::string::npos,
                             "Test 5: applyOperation(7, 8, myFunc)が正しくない");
            INTEGRATION_ASSERT(lines[5].find("18") != std::string::npos,
                             "Test 6: 連続コールバックが正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_callback passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタテスト: 複数の関数ポインタ管理
// ============================================================================

inline void test_function_pointer_multiple() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_multiple_pointers.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "複数関数ポインタテストがエラー終了");
            
            // Expected output: 15, 5, 50, 12, 32, 20
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            
            INTEGRATION_ASSERT(lines.size() >= 6, "出力行数が不足している");
            
            INTEGRATION_ASSERT(lines[0].find("15") != std::string::npos,
                             "Test 1: op0(10, 5) [add]が正しくない");
            INTEGRATION_ASSERT(lines[1].find("5") != std::string::npos,
                             "Test 2: op1(10, 5) [subtract]が正しくない");
            INTEGRATION_ASSERT(lines[2].find("50") != std::string::npos,
                             "Test 3: op2(10, 5) [multiply]が正しくない");
            INTEGRATION_ASSERT(lines[3].find("12") != std::string::npos,
                             "Test 4: selectedOp(8, 4) [opType=0, add]が正しくない");
            INTEGRATION_ASSERT(lines[4].find("32") != std::string::npos,
                             "Test 5: selectedOp(8, 4) [opType=2, multiply]が正しくない");
            INTEGRATION_ASSERT(lines[5].find("20") != std::string::npos,
                             "Test 6: 連続計算が正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_multiple passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタテスト: 戻り値として返す・チェーン呼び出し
// ============================================================================

inline void test_function_pointer_return() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_return_function_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタ戻り値テストがエラー終了");
            
            // Expected output: 15, 5, 50, 20, 10, 42, 50, 15
            std::vector<std::string> lines;
            std::istringstream iss(output);
            std::string line;
            while (std::getline(iss, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            
            INTEGRATION_ASSERT(lines.size() >= 8, "出力行数が不足している");
            
            INTEGRATION_ASSERT(lines[0].find("15") != std::string::npos,
                             "Test 1: getOperation(1)(10, 5) [add]が正しくない");
            INTEGRATION_ASSERT(lines[1].find("5") != std::string::npos,
                             "Test 2: getOperation(2)(10, 5) [subtract]が正しくない");
            INTEGRATION_ASSERT(lines[2].find("50") != std::string::npos,
                             "Test 3: getOperation(3)(10, 5) [multiply]が正しくない");
            INTEGRATION_ASSERT(lines[3].find("20") != std::string::npos,
                             "Test 4: getOperation(4)(100, 5) [divide]が正しくない");
            INTEGRATION_ASSERT(lines[4].find("10") != std::string::npos,
                             "Test 5: (*op5)(7, 3) [add]が正しくない");
            INTEGRATION_ASSERT(lines[5].find("42") != std::string::npos,
                             "Test 6: getOperation(3)(6, 7) [チェーン呼び出し]が正しくない");
            INTEGRATION_ASSERT(lines[6].find("50") != std::string::npos,
                             "Test 8: selectOperator(10, 5)(10, 5) [multiply]が正しくない");
            INTEGRATION_ASSERT(lines[7].find("15") != std::string::npos,
                             "Test 9: selectOperator(5, 10)(5, 10) [add]が正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_return passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタアドレス比較テスト
// ============================================================================

inline void test_function_pointer_address_comparison() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_pointer_address_comparison.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタアドレス比較テストがエラー終了");
            
            std::vector<std::string> lines = split_lines(output);
            
            // 全ての比較結果が1であることを確認
            INTEGRATION_ASSERT(lines.size() >= 9, "出力行数が不足");
            
            for (int i = 0; i < 7; i++) {
                INTEGRATION_ASSERT(lines[i].find("1") != std::string::npos,
                                 "Test " + std::to_string(i + 1) + "が失敗");
            }
            
            // 関数呼び出し結果の確認
            INTEGRATION_ASSERT(lines[7].find("15") != std::string::npos,
                             "add関数の結果が正しくない");
            INTEGRATION_ASSERT(lines[8].find("50") != std::string::npos,
                             "multiply関数の結果が正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_address_comparison passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタアドレス表示テスト
// ============================================================================

inline void test_function_pointer_address_print() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_pointer_address_print.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタアドレス表示テストがエラー終了");
            
            std::vector<std::string> lines = split_lines(output);
            
            INTEGRATION_ASSERT(lines.size() >= 7, "出力行数が不足");
            
            // アドレスが16進数形式（0xで始まる）であることを確認
            INTEGRATION_ASSERT(lines[0].find("0x") == 0,
                             "fp1のアドレスが16進数で表示されていない");
            INTEGRATION_ASSERT(lines[1].find("0x") == 0,
                             "fp2のアドレスが16進数で表示されていない");
            INTEGRATION_ASSERT(lines[2].find("0x") == 0,
                             "fp3のアドレスが16進数で表示されていない");
            
            // 同じ関数を指すポインタは同じアドレス
            INTEGRATION_ASSERT(lines[0] == lines[2],
                             "同じ関数を指すポインタのアドレスが異なる");
            
            // 異なる関数を指すポインタは異なるアドレス
            INTEGRATION_ASSERT(lines[0] != lines[1],
                             "異なる関数を指すポインタのアドレスが同じ");
            
            // 関数呼び出し結果の確認
            INTEGRATION_ASSERT(lines[3].find("25") != std::string::npos,
                             "square(5)の結果が正しくない");
            INTEGRATION_ASSERT(lines[4].find("27") != std::string::npos,
                             "cube(3)の結果が正しくない");
            
            // 再代入後のアドレスがfp2と同じ
            INTEGRATION_ASSERT(lines[5].find("0x") == 0,
                             "再代入後のアドレスが16進数で表示されていない");
            INTEGRATION_ASSERT(lines[5] == lines[1],
                             "再代入後のアドレスが期待と異なる");
            
            INTEGRATION_ASSERT(lines[6].find("64") != std::string::npos,
                             "cube(4)の結果が正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_address_print passed (%.3fms)\n", execution_time);
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
    test_comprehensive_pointer_operations();  // 包括的なポインタ操作テスト（アロー構文・デリファレンス構文）
    test_comprehensive_address_of();  // 包括的なアドレス取得テスト（全変数型）
    
    // 新規追加: 既知の制限事項実装後の包括的テスト
    test_declaration_init_comprehensive();  // 宣言時初期化の包括的テスト
    test_address_display_comprehensive();  // アドレス表示の包括的テスト
    test_struct_pointer_operations_comprehensive();  // 構造体ポインタ操作の包括的テスト
    test_interface_impl_pointer_comprehensive();  // インターフェースとimplブロックポインタの包括的テスト
    test_pointer_boundary_comprehensive();  // ポインタ境界ケースと特殊操作の包括的テスト
    
    // 関数ポインタテスト
    test_function_pointer_callback();  // 関数ポインタコールバック
    test_function_pointer_multiple();  // 複数の関数ポインタ管理
    test_function_pointer_return();    // 関数ポインタを戻り値として返す・チェーン呼び出し
    test_function_pointer_address_comparison();  // 関数ポインタアドレス比較
    test_function_pointer_address_print();  // 関数ポインタアドレス表示
    
    printf("=== All Pointer Tests Passed ===\n\n");
}

} // namespace PointerTests

#endif // POINTER_TESTS_HPP
