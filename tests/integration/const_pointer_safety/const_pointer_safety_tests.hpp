#ifndef CONST_POINTER_SAFETY_TESTS_HPP
#define CONST_POINTER_SAFETY_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>

namespace ConstPointerSafetyTests {

// ============================================================================
// Const Pointer Safety - 正常系テスト
// ============================================================================

// 正しいconst使用法のテスト
inline void test_correct_usage() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer_safety/test_correct_usage.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "正しいconst使用法でエラー終了");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos,
                             "変数xの値が出力されていない");
            INTEGRATION_ASSERT(output.find("*ptr") != std::string::npos,
                             "const int*のポインタ値が出力されていない");
        },
        execution_time
    );
    printf("[✓] test_correct_usage passed (%.3fms)\n", execution_time);
}

// 包括的な単一ポインタテスト
inline void test_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer_safety/test_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的constポインタテストがエラー終了");
            INTEGRATION_ASSERT(output.find("Test 1 PASSED") != std::string::npos,
                             "Test 1: const int* ptr = &const_var が失敗");
            INTEGRATION_ASSERT(output.find("Test 2 PASSED") != std::string::npos,
                             "Test 2: int* ptr = &non_const_var が失敗");
            INTEGRATION_ASSERT(output.find("Test 3 PASSED") != std::string::npos,
                             "Test 3: const int* ptr = &non_const_var が失敗");
            INTEGRATION_ASSERT(output.find("All tests PASSED") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    printf("[✓] test_comprehensive passed (%.3fms)\n", execution_time);
}

// ダブルポインタの正常系テスト
inline void test_double_pointer() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer_safety/test_double_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ダブルポインタテストがエラー終了");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos,
                             "変数の値が出力されていない");
            INTEGRATION_ASSERT(output.find("*ptr1") != std::string::npos,
                             "ptr1の参照が出力されていない");
            INTEGRATION_ASSERT(output.find("**ptr2") != std::string::npos,
                             "ptr2の参照が出力されていない");
        },
        execution_time
    );
    printf("[✓] test_double_pointer passed (%.3fms)\n", execution_time);
}

// ダブルポインタの包括的テスト
inline void test_double_pointer_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer_safety/test_double_pointer_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ダブルポインタ包括テストがエラー終了");
            INTEGRATION_ASSERT(output.find("Test 1 PASSED") != std::string::npos,
                             "Test 1: const int* ptr = &const_var が失敗");
            INTEGRATION_ASSERT(output.find("Test 2 PASSED") != std::string::npos,
                             "Test 2: const int** ptr = &(const int*) が失敗");
            INTEGRATION_ASSERT(output.find("Test 3 PASSED") != std::string::npos,
                             "Test 3: int* ptr = &non_const_var が失敗");
            INTEGRATION_ASSERT(output.find("Test 4 PASSED") != std::string::npos,
                             "Test 4: int** ptr = &(int*) が失敗");
            INTEGRATION_ASSERT(output.find("Test 5 PASSED") != std::string::npos,
                             "Test 5: int* const ptr = &var が失敗");
            INTEGRATION_ASSERT(output.find("All tests PASSED") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    printf("[✓] test_double_pointer_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// Const Pointer Safety - エラー検出テスト
// ============================================================================

// エラーテスト: const変数のアドレスを非constポインタに代入
inline void test_error_assign_const_to_nonconst() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer_safety/error_assign_const_to_nonconst.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "const変数→非constポインタエラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot assign address of const variable") != std::string::npos,
                             "constエラーメッセージが正しくない");
        },
        execution_time
    );
    printf("[✓] test_error_assign_const_to_nonconst passed (%.3fms)\n", execution_time);
}

// エラーテスト: const T*のアドレスをT**に代入
inline void test_error_double_pointer() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer_safety/error_double_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "const T*→T**エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot assign address of pointer to const") != std::string::npos ||
                             output.find("const T*") != std::string::npos,
                             "ダブルポインタconstエラーメッセージが正しくない");
        },
        execution_time
    );
    printf("[✓] test_error_double_pointer passed (%.3fms)\n", execution_time);
}

// エラーテスト: T* constのアドレスをT**に代入
inline void test_error_const_pointer_address() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/const_pointer_safety/error_const_pointer_address.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(exit_code != 0, "T* const→T**エラーが検出されていない");
            INTEGRATION_ASSERT(output.find("Cannot assign address of const pointer") != std::string::npos ||
                             output.find("T* const") != std::string::npos,
                             "constポインタアドレスエラーメッセージが正しくない");
        },
        execution_time
    );
    printf("[✓] test_error_const_pointer_address passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 全てのconst pointer safetyテストを実行
// ============================================================================

inline void run_all_const_pointer_safety_tests() {
    printf("\n=== Const Pointer Safety Tests ===\n");
    
    // 正常系テスト
    printf("\n--- Correct Usage Tests ---\n");
    test_correct_usage();
    test_comprehensive();
    test_double_pointer();
    test_double_pointer_comprehensive();
    
    // エラー検出テスト
    printf("\n--- Error Detection Tests ---\n");
    test_error_assign_const_to_nonconst();
    test_error_double_pointer();
    test_error_const_pointer_address();
    
    printf("=== All Const Pointer Safety Tests Passed ===\n\n");
}

} // namespace ConstPointerSafetyTests

#endif // CONST_POINTER_SAFETY_TESTS_HPP
