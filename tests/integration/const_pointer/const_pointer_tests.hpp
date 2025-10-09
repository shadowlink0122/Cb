#ifndef CONST_POINTER_TESTS_HPP
#define CONST_POINTER_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <chrono>

namespace ConstPointerTests {

// ============================================================================
// constポインタ（正常系）の包括的テスト
// ============================================================================

inline void test_const_pointer_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_const_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "constポインタ包括テストがエラー終了");
            
            // Test 1: pointer to const int (const int*)
            INTEGRATION_ASSERT(output.find("=== Test 1: pointer to const int (const int*) ===") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("10") != std::string::npos && output.find("20") != std::string::npos,
                             "Test 1の値が正しくない");
            
            // Test 2: const pointer to int (int* const)
            INTEGRATION_ASSERT(output.find("=== Test 2: const pointer to int (int* const) ===") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("30") != std::string::npos && output.find("35") != std::string::npos,
                             "Test 2の値が正しくない");
            
            // Test 3: const pointer to const int (const int* const)
            INTEGRATION_ASSERT(output.find("=== Test 3: const pointer to const int (const int* const) ===") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("50") != std::string::npos,
                             "Test 3の値が正しくない");
            
            // Test 4: const with regular variables
            INTEGRATION_ASSERT(output.find("=== Test 4: const with regular variables ===") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("100") != std::string::npos && output.find("200") != std::string::npos,
                             "Test 4の値が正しくない");
            
            // Test 5: pointer to const struct (const Point*)
            INTEGRATION_ASSERT(output.find("=== Test 5: pointer to const struct (const Point*) ===") != std::string::npos,
                             "Test 5のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("5") != std::string::npos && output.find("10") != std::string::npos,
                             "Test 5の値が正しくない");
            
            // Test 6: const pointer to struct (Point* const)
            INTEGRATION_ASSERT(output.find("=== Test 6: const pointer to struct (Point* const) ===") != std::string::npos,
                             "Test 6のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("100") != std::string::npos && output.find("200") != std::string::npos,
                             "Test 6の値が正しくない");
            
            // Test 7: const pointer to const struct (const Point* const)
            INTEGRATION_ASSERT(output.find("=== Test 7: const pointer to const struct (const Point* const) ===") != std::string::npos,
                             "Test 7のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("300") != std::string::npos && output.find("400") != std::string::npos,
                             "Test 7の値が正しくない");
            
            // Test 8: pointer to const float (const float*)
            INTEGRATION_ASSERT(output.find("=== Test 8: pointer to const float (const float*) ===") != std::string::npos,
                             "Test 8のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("3.14") != std::string::npos && output.find("2.71") != std::string::npos,
                             "Test 8の値が正しくない");
            
            // Test 9: const pointer to float (float* const)
            INTEGRATION_ASSERT(output.find("=== Test 9: const pointer to float (float* const) ===") != std::string::npos,
                             "Test 9のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("1.5") != std::string::npos,
                             "Test 9の値が正しくない");
            
            // Test 10: pointer to const string (const string*)
            INTEGRATION_ASSERT(output.find("=== Test 10: pointer to const string (const string*) ===") != std::string::npos,
                             "Test 10のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("Hello") != std::string::npos && output.find("World") != std::string::npos,
                             "Test 10の値が正しくない");
            
            // Final message
            INTEGRATION_ASSERT(output.find("=== All const pointer tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[✓] test_const_pointer_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// constポインタエラーケーステスト
// ============================================================================

// エラーテスト: const T* 経由で値を変更
inline void test_error_modify_pointee_const() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer/error_modify_pointee_const.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "const T*経由の値変更エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot modify value through pointer to const") != std::string::npos,
                             "pointee const エラーメッセージが正しくない");
            INTEGRATION_ASSERT(output.find("ERROR: This should not execute") == std::string::npos,
                             "エラー後にコードが実行されている");
        },
        execution_time
    );
    printf("[✓] test_error_modify_pointee_const passed (%.3fms)\n", execution_time);
}

// エラーテスト: T* const の再代入
inline void test_error_reassign_pointer_const() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer/error_reassign_pointer_const.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "T* const再代入エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot reassign const pointer (T* const)") != std::string::npos,
                             "pointer const エラーメッセージが正しくない");
            INTEGRATION_ASSERT(output.find("ERROR: This should not execute") == std::string::npos,
                             "エラー後にコードが実行されている");
        },
        execution_time
    );
    printf("[✓] test_error_reassign_pointer_const passed (%.3fms)\n", execution_time);
}

// エラーテスト: const T* const 経由で値を変更
inline void test_error_modify_both_const() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer/error_modify_both_const.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "const T* const経由の値変更エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot modify value through pointer to const") != std::string::npos,
                             "both const値変更エラーメッセージが正しくない");
            INTEGRATION_ASSERT(output.find("ERROR: This should not execute") == std::string::npos,
                             "エラー後にコードが実行されている");
        },
        execution_time
    );
    printf("[✓] test_error_modify_both_const passed (%.3fms)\n", execution_time);
}

// エラーテスト: const T* const の再代入
inline void test_error_reassign_both_const() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer/error_reassign_both_const.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "const T* const再代入エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot reassign const pointer (T* const)") != std::string::npos,
                             "both const再代入エラーメッセージが正しくない");
            INTEGRATION_ASSERT(output.find("ERROR: This should not execute") == std::string::npos,
                             "エラー後にコードが実行されている");
        },
        execution_time
    );
    printf("[✓] test_error_reassign_both_const passed (%.3fms)\n", execution_time);
}

// エラーテスト: const Point* 経由で構造体メンバーを変更
inline void test_error_modify_struct_pointee_const() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer/error_modify_struct_pointee_const.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "const Point*経由の構造体変更エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot modify value through pointer to const") != std::string::npos,
                             "struct pointee constエラーメッセージが正しくない");
            INTEGRATION_ASSERT(output.find("ERROR: This should not execute") == std::string::npos,
                             "エラー後にコードが実行されている");
        },
        execution_time
    );
    printf("[✓] test_error_modify_struct_pointee_const passed (%.3fms)\n", execution_time);
}

// エラーテスト: Point* const の再代入
inline void test_error_reassign_struct_pointer_const() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer/error_reassign_struct_pointer_const.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "Point* const再代入エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot reassign const pointer (T* const)") != std::string::npos,
                             "struct pointer constエラーメッセージが正しくない");
            INTEGRATION_ASSERT(output.find("ERROR: This should not execute") == std::string::npos,
                             "エラー後にコードが実行されている");
        },
        execution_time
    );
    printf("[✓] test_error_reassign_struct_pointer_const passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 全てのconstポインタテストを実行
// ============================================================================

inline void run_all_const_pointer_tests() {
    printf("\n=== Const Pointer Tests ===\n");
    
    // 正常系テスト
    test_const_pointer_comprehensive();
    
    // エラーケーステスト
    printf("\n--- Error Detection Tests ---\n");
    test_error_modify_pointee_const();
    test_error_reassign_pointer_const();
    test_error_modify_both_const();
    test_error_reassign_both_const();
    test_error_modify_struct_pointee_const();
    test_error_reassign_struct_pointer_const();
    
    printf("=== All Const Pointer Tests Passed ===\n\n");
}

} // namespace ConstPointerTests

#endif // CONST_POINTER_TESTS_HPP
