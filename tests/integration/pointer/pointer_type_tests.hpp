#ifndef POINTER_TYPE_TESTS_HPP
#define POINTER_TYPE_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace PointerTypeTests {

// ============================================================================
// float型ポインタ基本テスト
// ============================================================================

inline void test_float_pointer_basic() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_float_pointer_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "float型ポインタ基本テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Test 1: Basic float pointer ===") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("2.71") != std::string::npos,
                             "float型ポインタ経由の変更が正しくない");
            
            INTEGRATION_ASSERT(output.find("=== All float pointer tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_float_pointer_basic passed (%.3fms)\n", execution_time);
}

// ============================================================================
// double型ポインタ基本テスト
// ============================================================================

inline void test_double_pointer_basic() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_double_pointer_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "double型ポインタ基本テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All double pointer tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_double_pointer_basic passed (%.3fms)\n", execution_time);
}

// ============================================================================
// float/double型ポインタ関数テスト
// ============================================================================

inline void test_float_double_pointer_function() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_float_double_pointer_function.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "float/double型ポインタ関数テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All float/double pointer function tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_float_double_pointer_function passed (%.3fms)\n", execution_time);
}

// ============================================================================
// float/double型混合テスト
// ============================================================================

inline void test_float_double_mixed() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_float_double_mixed.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "float/double型混合テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All float/double mixed tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_float_double_mixed passed (%.3fms)\n", execution_time);
}

// ============================================================================
// enum型ポインタ基本テスト
// ============================================================================

inline void test_enum_pointer_basic() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_enum_pointer_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "enum型ポインタ基本テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All enum pointer tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_enum_pointer_basic passed (%.3fms)\n", execution_time);
}

// ============================================================================
// enum型ポインタ関数テスト
// ============================================================================

inline void test_enum_pointer_function() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_enum_pointer_function.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "enum型ポインタ関数テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All enum pointer function tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_enum_pointer_function passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Pointer Type Tests ===\n");
    test_float_pointer_basic();
    test_double_pointer_basic();
    test_float_double_pointer_function();
    test_float_double_mixed();
    test_enum_pointer_basic();
    test_enum_pointer_function();
    printf("=== Pointer Type Tests Completed ===\n\n");
}

} // namespace PointerTypeTests

#endif // POINTER_TYPE_TESTS_HPP
