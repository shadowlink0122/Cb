#ifndef POINTER_ARRAY_TESTS_HPP
#define POINTER_ARRAY_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace PointerArrayTests {

// ============================================================================
// ポインタ配列ループ代入テスト
// ============================================================================

inline void test_pointer_array_loop_assign() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_array_loop_assign.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ配列ループ代入テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Test 1: Loop assignment to pointer array ===") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("=== All loop assignment tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_array_loop_assign passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタ配列関数引数テスト
// ============================================================================

inline void test_pointer_array_function_arg() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_array_function_arg.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ配列関数引数テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All pointer array function argument tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_array_function_arg passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタ配列構造体メンバテスト
// ============================================================================

inline void test_pointer_array_struct_member() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_array_struct_member.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ配列構造体メンバテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All pointer array struct member tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_array_struct_member passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタ配列関数戻り値テスト
// ============================================================================

inline void test_pointer_array_function_return() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_array_function_return.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ配列関数戻り値テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All pointer array function return tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_array_function_return passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタ配列エッジケーステスト
// ============================================================================

inline void test_pointer_array_edge_cases() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_array_edge_cases.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ配列エッジケーステストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All pointer array edge case tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_array_edge_cases passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタ配列ネストテスト
// ============================================================================

inline void test_pointer_array_nested() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_array_nested.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ配列ネストテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All nested pointer array tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_array_nested passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 深いネスト配列テスト
// ============================================================================

inline void test_deep_nested_array() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_deep_nested_array.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "深いネスト配列テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All deep nested array tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_deep_nested_array passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Pointer Array Tests ===\n");
    test_pointer_array_loop_assign();
    test_pointer_array_function_arg();
    test_pointer_array_struct_member();
    test_pointer_array_function_return();
    test_pointer_array_edge_cases();
    test_pointer_array_nested();
    test_deep_nested_array();
    printf("=== Pointer Array Tests Completed ===\n\n");
}

} // namespace PointerArrayTests

#endif // POINTER_ARRAY_TESTS_HPP
