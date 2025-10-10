#ifndef POINTER_STRUCT_TESTS_HPP
#define POINTER_STRUCT_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace PointerStructTests {

// ============================================================================
// 構造体ポインタメンバテスト
// ============================================================================

inline void test_struct_pointer_members() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_struct_pointer_members.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "構造体ポインタメンバテストがエラー終了");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, "期待する出力が含まれていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_struct_pointer_members passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 再帰構造体(自己参照構造体)テスト
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
            
            INTEGRATION_ASSERT(output.find("=== All recursive struct tests passed! ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_recursive_struct passed (%.3fms)\n", execution_time);
}

// ============================================================================
// typedef再帰構造体テスト
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
            
            INTEGRATION_ASSERT(output.find("=== All typedef recursive struct tests passed! ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_typedef_recursive_struct passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 深いネスト構造体テスト
// ============================================================================

inline void test_deep_nested_struct() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_deep_nested_struct.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "深いネスト構造体テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== All deep nested struct tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_deep_nested_struct passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Pointer Struct Tests ===\n");
    test_struct_pointer_members();
    test_recursive_struct();
    test_typedef_recursive_struct();
    test_deep_nested_struct();
    printf("=== Pointer Struct Tests Completed ===\n\n");
}

} // namespace PointerStructTests

#endif // POINTER_STRUCT_TESTS_HPP
