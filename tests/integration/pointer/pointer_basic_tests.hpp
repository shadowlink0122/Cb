#ifndef POINTER_BASIC_TESTS_HPP
#define POINTER_BASIC_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <chrono>
#include <sstream>
#include <vector>

namespace PointerBasicTests {

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
            INTEGRATION_ASSERT(output.find("After ptr = &var2: *ptr = 200 (now points to var2)") != std::string::npos,
                             "ptrがvar2に再代入された結果が正しく表示されていない");
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
                             "最終値が正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_chains passed (%.3fms)\n", execution_time);
}

// ============================================================================
// nullptr操作のテスト
// ============================================================================

inline void test_nullptr_checks() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_nullptr_checks.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "nullptrテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("nullptr initialization") != std::string::npos,
                             "nullptr初期化テストのメッセージが出力されていない");
            INTEGRATION_ASSERT(output.find("ptr1 == nullptr") != std::string::npos,
                             "nullptr初期化の確認が出力されていない");
            INTEGRATION_ASSERT(output.find("Reassignment to nullptr") != std::string::npos,
                             "nullptr再代入テストのメッセージが出力されていない");
            INTEGRATION_ASSERT(output.find("pointer now points to null") != std::string::npos,
                             "ptr2をnullptrに再代入した結果が表示されていない");
            INTEGRATION_ASSERT(output.find("Multiple nullptr pointers") != std::string::npos,
                             "複数nullptrテストのメッセージが出力されていない");
            INTEGRATION_ASSERT(output.find("*p1 = 10") != std::string::npos &&
                             output.find("*p2 = 20") != std::string::npos &&
                             output.find("*p3 = 30") != std::string::npos,
                             "複数ポインタの代入結果が表示されていない");
            INTEGRATION_ASSERT(output.find("Double pointer with nullptr") != std::string::npos,
                             "ダブルポインタテストのメッセージが出力されていない");
            INTEGRATION_ASSERT(output.find("**pp = 100") != std::string::npos,
                             "ダブルポインタの代入結果が表示されていない");
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
            
            INTEGRATION_ASSERT(output.find("=== Variable Address Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Test 1: Address of regular variable") != std::string::npos,
                             "通常変数のアドレス確認が出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1の完了メッセージが表示されていない");
            INTEGRATION_ASSERT(output.find("Test 2: Modify variable through pointer") != std::string::npos,
                             "ポインタ経由の変更テストが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2の完了メッセージが表示されていない");
            INTEGRATION_ASSERT(output.find("Test 3: Multiple variables") != std::string::npos,
                             "複数変数テストのヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3の完了メッセージが表示されていない");
            INTEGRATION_ASSERT(output.find("Test 4: Different types") != std::string::npos,
                             "異なる型テストのヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4の完了メッセージが表示されていない");
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
            
            INTEGRATION_ASSERT(output.find("=== Pointer Comprehensive Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Test 1: Array element pointer") != std::string::npos,
                             "配列要素ポインタのテストが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1の完了メッセージが表示されていない");
            INTEGRATION_ASSERT(output.find("Test 2: Modify through pointer") != std::string::npos,
                             "ポインタ経由の変更テストが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2の完了メッセージが表示されていない");
            INTEGRATION_ASSERT(output.find("Test 3: Pointer to different element") != std::string::npos,
                             "別要素のポインタテストが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3の完了メッセージが表示されていない");
            INTEGRATION_ASSERT(output.find("Test 4: Pointer reassignment") != std::string::npos,
                             "ポインタ再代入テストが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4の完了メッセージが表示されていない");
            INTEGRATION_ASSERT(output.find("Test 5: Multiple modifications") != std::string::npos,
                             "複数回の変更テストが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5の完了メッセージが表示されていない");
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
        "../../tests/cases/pointer/pointer_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的なポインタ演算テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("PASS: All basic pointer tests") != std::string::npos,
                             "包括的ポインタ演算の結果が表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_comprehensive_pointer_arithmetic passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 高度なポインタ機能テスト
// ============================================================================

inline void test_advanced_pointer_features() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_advanced_pointer_features.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "高度なポインタ機能テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("Advanced Pointer Features Test") != std::string::npos,
                             "テストヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("=== All 15 Advanced Pointer Tests Passed! ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_advanced_pointer_features passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタのインクリメント/デクリメントテスト
// ============================================================================

inline void test_pointer_incdec() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_ptr_incdec.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタインクリメント/デクリメントテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Increment/Decrement Test ===") != std::string::npos,
                             "テストヘッダーが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 1: Pre-increment (++ptr)") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("Before: *ptr = 10") != std::string::npos,
                             "Test 1の初期値が正しくない");
            INTEGRATION_ASSERT(output.find("After ++ptr: *ptr = 20") != std::string::npos,
                             "Test 1のインクリメント結果が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed: Pre-increment works") != std::string::npos,
                             "Test 1の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 2: Post-increment (ptr++)") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("After ptr++:") != std::string::npos,
                             "Test 2の結果ブロックが表示されていない");
            INTEGRATION_ASSERT(output.find("  *old_ptr = 20") != std::string::npos,
                             "Test 2のold_ptr値が正しくない");
            INTEGRATION_ASSERT(output.find("  *ptr = 30") != std::string::npos,
                             "Test 2のptr値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed: Post-increment works") != std::string::npos,
                             "Test 2の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 3: Pre-decrement (--ptr)") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("After --ptr: *ptr = 30") != std::string::npos,
                             "Test 3のデクリメント結果が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed: Pre-decrement works") != std::string::npos,
                             "Test 3の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 4: Post-decrement (ptr--)") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("After ptr--:") != std::string::npos,
                             "Test 4の結果ブロックが表示されていない");
            INTEGRATION_ASSERT(output.find("  *old_ptr = 30") != std::string::npos,
                             "Test 4のold_ptr値が正しくない");
            INTEGRATION_ASSERT(output.find("  *ptr = 20") != std::string::npos,
                             "Test 4のptr値が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed: Post-decrement works") != std::string::npos,
                             "Test 4の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 5: Loop with pointer increment") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed: Loop with increment works") != std::string::npos,
                             "Test 5の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 6: Multiple increment/decrement") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("After ++ptr: *ptr = 40") != std::string::npos,
                             "Test 6のインクリメント結果が正しくない");
            INTEGRATION_ASSERT(output.find("After ++ptr: *ptr = 50") != std::string::npos,
                             "Test 6の2回目のインクリメント結果が正しくない");
            INTEGRATION_ASSERT(output.find("After --ptr: *ptr = 40") != std::string::npos,
                             "Test 6の1回目のデクリメント結果が正しくない");
            INTEGRATION_ASSERT(output.find("After --ptr: *ptr = 30") != std::string::npos,
                             "Test 6のデクリメント結果が正しくない");
            INTEGRATION_ASSERT(output.find("✓ Test 6 passed: Multiple operations work") != std::string::npos,
                             "Test 6の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("=== All pointer increment/decrement tests completed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_incdec passed (%.3fms)\n", execution_time);
}

// ============================================================================
// デリファレンスインクリメント/デクリメントテスト
// ============================================================================

inline void test_dereference_incdec() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_deref_incdec.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "デリファレンスインクリメント/デクリメントテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Dereference Increment/Decrement Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");

            INTEGRATION_ASSERT(output.find("Post-increment (*ptr)++") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Pre-increment ++(*ptr)") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Post-decrement (*ptr)--") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Pre-decrement --(*ptr)") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Float type with (*ptr)++") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Double type with ++(*ptr)") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 6 passed") != std::string::npos,
                             "Test 6の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Multiple operations") != std::string::npos,
                             "Test 7が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 7 passed") != std::string::npos,
                             "Test 7の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Array element pointer") != std::string::npos,
                             "Test 8が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 8 passed") != std::string::npos,
                             "Test 8の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("=== All dereference increment/decrement tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_dereference_incdec passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタアドレスフォーマットテスト
// ============================================================================

inline void test_pointer_format() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_format.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタアドレスフォーマットテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Address Format (%p) Test ===") != std::string::npos,
                             "テストヘッダーが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 1: Basic variable address") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("&x = 0x") != std::string::npos,
                             "変数アドレスが16進数で表示されていない");
            INTEGRATION_ASSERT(output.find("✓ Address displayed in hex format") != std::string::npos,
                             "Test 1の完了メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 2: Different types") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("int a = 1, address: 0x") != std::string::npos,
                             "int型のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("float b = 2.500000, address: 0x") != std::string::npos,
                             "float型のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("double c = 3.140000, address: 0x") != std::string::npos,
                             "double型のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("✓ All types display addresses") != std::string::npos,
                             "Test 2の完了メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 3: Pointer variable address") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("ptr (points to val) = 0x") != std::string::npos,
                             "ポインタ変数のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("&ptr (address of pointer) = 0x") != std::string::npos,
                             "ポインタ自体のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("✓ Pointer variable address works") != std::string::npos,
                             "Test 3の完了メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 4: Multiple addresses") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Multiple %p in one println works") != std::string::npos,
                             "Test 4の完了メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 5: Array element addresses") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("arr[0]=100 @0x") != std::string::npos,
                             "配列要素のアドレスが表示されていない");
            INTEGRATION_ASSERT(output.find("✓ Array element addresses work") != std::string::npos,
                             "Test 5の完了メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 6: Mixed format specifiers") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Mixed format specifiers work") != std::string::npos,
                             "Test 6の完了メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 7: Float/Double with %f and %p") != std::string::npos,
                             "Test 7が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ %f and %p work together") != std::string::npos,
                             "Test 7の完了メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("=== All pointer address format tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_format passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 包括的なポインタ操作テスト
// ============================================================================

inline void test_comprehensive_pointer_operations() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_ptr_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的ポインタ操作テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Arithmetic Comprehensive Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");

            INTEGRATION_ASSERT(output.find("Test 1: ptr + 1") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 2: ptr + 2") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 3: ptr - 1") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 4: Chain arithmetic (ptr + 1 + 1)") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 5: Modify through pointer arithmetic") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5の成功メッセージが表示されていない");

            INTEGRATION_ASSERT(output.find("=== All pointer arithmetic tests completed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_comprehensive_pointer_operations passed (%.3fms)\n", execution_time);
}

// ============================================================================
// アドレス演算の包括的テスト
// ============================================================================

inline void test_comprehensive_address_of() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_address_display_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "アドレス演算包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Address Display and Pointer Arithmetic Detailed Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");

            INTEGRATION_ASSERT(output.find("Test 1: Hexadecimal address display") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("Array base: &arr[0] = 0x") != std::string::npos,
                             "配列基底アドレスが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 2: Pointer arithmetic and address transition") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("After p+1: p = 0x") != std::string::npos,
                             "ポインタ加算後のアドレスが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 3: Pointer subtraction") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("After p-1: p = 0x") != std::string::npos,
                             "ポインタ減算後のアドレスが表示されていない");

            INTEGRATION_ASSERT(output.find("Test 6: Address of pointer variable itself") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("Address of pointer: &p = 0x") != std::string::npos,
                             "ポインタ変数自身のアドレスが表示されていない");

            INTEGRATION_ASSERT(output.find("=== All address display and arithmetic tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_comprehensive_address_of passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 宣言時初期化の包括的テスト
// ============================================================================

inline void test_declaration_init_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_declaration_init_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "宣言時初期化包括テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Declaration with Initialization and Basic Operations Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1が成功していない");
            INTEGRATION_ASSERT(output.find("✓ Test 5 passed") != std::string::npos,
                             "Test 5が成功していない");
            INTEGRATION_ASSERT(output.find("✓ Test 10 passed") != std::string::npos,
                             "Test 10が成功していない");
            INTEGRATION_ASSERT(output.find("=== All declaration and basic operation tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_declaration_init_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// アドレス表示の包括的テスト
// ============================================================================

inline void test_address_display_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_address_display_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "アドレス表示包括テストがエラー終了");
            
                        INTEGRATION_ASSERT(output.find("Test 1: Hexadecimal address display") != std::string::npos,
                                                         "Test 1が実行されていない");
                        INTEGRATION_ASSERT(output.find("Pointer p = 0x") != std::string::npos,
                                                         "ポインタ値が表示されていない");

                        INTEGRATION_ASSERT(output.find("Test 2: Pointer arithmetic and address transition") != std::string::npos,
                                                         "Test 2が実行されていない");
                        INTEGRATION_ASSERT(output.find("After p+1: p = 0x") != std::string::npos,
                                                         "ポインタ加算後のアドレスが表示されていない");
                        INTEGRATION_ASSERT(output.find("After p+1: p = 0x") != std::string::npos &&
                                                             output.find("After p+1: p = 0x", output.find("After p+1: p = 0x") + 1) != std::string::npos,
                                                         "連続したポインタ加算結果が表示されていない");

                        INTEGRATION_ASSERT(output.find("Test 3: Pointer subtraction") != std::string::npos,
                                                         "Test 3が実行されていない");
                        INTEGRATION_ASSERT(output.find("After p-1: p = 0x") != std::string::npos,
                                                         "ポインタ減算後のアドレスが表示されていない");

                        INTEGRATION_ASSERT(output.find("Test 4: Multiple pointers address comparison") != std::string::npos,
                                                         "Test 4が実行されていない");
                        INTEGRATION_ASSERT(output.find("p1 = 0x") != std::string::npos,
                                                         "p1のアドレスが表示されていない");
                        INTEGRATION_ASSERT(output.find("p2 = 0x") != std::string::npos,
                                                         "p2のアドレスが表示されていない");
                        INTEGRATION_ASSERT(output.find("p3 = 0x") != std::string::npos,
                                                         "p3のアドレスが表示されていない");

                        INTEGRATION_ASSERT(output.find("Test 5: Increment/Decrement and address") != std::string::npos,
                                                         "Test 5が実行されていない");
                        INTEGRATION_ASSERT(output.find("After q++: q = 0x") != std::string::npos,
                                                         "q++のアドレスが表示されていない");
                        INTEGRATION_ASSERT(output.find("After q--: q = 0x") != std::string::npos,
                                                         "q--のアドレスが表示されていない");

                        INTEGRATION_ASSERT(output.find("Test 6: Address of pointer variable itself") != std::string::npos,
                                                         "Test 6が実行されていない");
                        INTEGRATION_ASSERT(output.find("Address of pointer: &p = 0x") != std::string::npos,
                                                         "ポインタ変数自身のアドレスが表示されていない");

                        INTEGRATION_ASSERT(output.find("=== All address display and arithmetic tests passed ===") != std::string::npos,
                                                         "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_address_display_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタ境界の包括的テスト
// ============================================================================

inline void test_pointer_boundary_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_boundary_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ境界テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Boundary and Special Operations Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");

            INTEGRATION_ASSERT(output.find("Test 1: Multiple declaration-time initialization") != std::string::npos,
                             "Test 1が実行されていない");
            INTEGRATION_ASSERT(output.find("*p1 = 10, *p2 = 20, *p3 = 50") != std::string::npos,
                             "Test 1の出力が正しくない");

            INTEGRATION_ASSERT(output.find("Test 2: Chained pointer assignment") != std::string::npos,
                             "Test 2が実行されていない");
            INTEGRATION_ASSERT(output.find("Modified arr2[0] via q3: arr2[0] = 999") != std::string::npos,
                             "Test 2の更新結果が表示されていない");

            INTEGRATION_ASSERT(output.find("Test 3: Pointer to zero-th element") != std::string::npos,
                             "Test 3が実行されていない");
            INTEGRATION_ASSERT(output.find("Sum of array[0..9] = 45") != std::string::npos,
                             "Test 3の結果が表示されていない");

            INTEGRATION_ASSERT(output.find("Test 4: Backward pointer movement") != std::string::npos,
                             "Test 4が実行されていない");
            INTEGRATION_ASSERT(output.find("Product of arr3 (backward) = 120") != std::string::npos,
                             "Test 4の結果が表示されていない");

            INTEGRATION_ASSERT(output.find("Test 5: Array palindrome check with pointers") != std::string::npos,
                             "Test 5が実行されていない");
            INTEGRATION_ASSERT(output.find("Palindrome check passed") != std::string::npos,
                             "Test 5の判定結果が表示されていない");

            INTEGRATION_ASSERT(output.find("Test 6: Mixed pointer increment/decrement") != std::string::npos,
                             "Test 6が実行されていない");
            INTEGRATION_ASSERT(output.find("Pointer navigation: 40 -> 50 -> 60 -> 50 -> 30") != std::string::npos,
                             "Test 6の結果が表示されていない");

            INTEGRATION_ASSERT(output.find("Test 7: Hexadecimal address format verification") != std::string::npos,
                             "Test 7が実行されていない");
            INTEGRATION_ASSERT(output.find("addr_ptr = 0x") != std::string::npos,
                             "Test 7のアドレス表示が正しくない");

            INTEGRATION_ASSERT(output.find("=== All boundary and special operations tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_boundary_comprehensive passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Pointer Basic Tests ===\n");
    test_basic_pointer_operations();
    test_pointer_function_parameters();
    test_pointer_chains();
    test_nullptr_checks();
    test_variable_address();
    test_minimal_pointer();
    test_comprehensive_pointer_arithmetic();
    test_advanced_pointer_features();
    test_pointer_incdec();
    test_dereference_incdec();
    test_pointer_format();
    test_comprehensive_pointer_operations();
    test_comprehensive_address_of();
    test_declaration_init_comprehensive();
    test_address_display_comprehensive();
    test_pointer_boundary_comprehensive();
    printf("=== Pointer Basic Tests Completed ===\n\n");
}

} // namespace PointerBasicTests

#endif // POINTER_BASIC_TESTS_HPP
