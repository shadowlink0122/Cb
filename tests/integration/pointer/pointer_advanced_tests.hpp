#ifndef POINTER_ADVANCED_TESTS_HPP
#define POINTER_ADVANCED_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace PointerAdvancedTests {

// ============================================================================
// implでポインタ使用テスト
// ============================================================================

inline void test_impl_with_pointers() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_impl_with_pointers.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "implポインタテストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Impl with Pointer Members Tests ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1-5のチェック
            INTEGRATION_ASSERT(output.find("Test 1: Using pointer member in impl methods") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("counter.get_value() = 10") != std::string::npos,
                             "Test 1の初期値が正しくない");
            
            INTEGRATION_ASSERT(output.find("Test 2: Modify external variable via impl method") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("storage = 11 (should be 11)") != std::string::npos,
                             "Test 2のインクリメント結果が正しくない");
            
            INTEGRATION_ASSERT(output.find("Test 3: Set value via impl method") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("storage = 100 (should be 100)") != std::string::npos,
                             "Test 3のset_value結果が正しくない");
            
            INTEGRATION_ASSERT(output.find("Test 4: Multiple increments") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("storage = 103 (should be 103)") != std::string::npos,
                             "Test 4の複数回インクリメント結果が正しくない");
            
            INTEGRATION_ASSERT(output.find("Test 5: Reassign pointer to different variable") != std::string::npos,
                             "Test 5のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("other_storage = 51 (should be 51)") != std::string::npos,
                             "Test 5のポインタ付け替え結果が正しくない");
            
            INTEGRATION_ASSERT(output.find("All impl pointer tests passed!") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_impl_with_pointers passed (%.3fms)\n", execution_time);
}

// ============================================================================
// ポインタ戻り値関数テスト
// ============================================================================

inline void test_pointer_return_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_pointer_return_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ポインタ戻り値関数テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Pointer Return Function Tests ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("Test 1: Basic pointer return") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("x = 42, *result = 42") != std::string::npos,
                             "Test 1の基本ポインタ戻り値が正しくない");
            
            INTEGRATION_ASSERT(output.find("Test 2: Conditional pointer return") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("All pointer return function tests passed!") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_pointer_return_comprehensive passed (%.3fms)\n", execution_time);
}

// ============================================================================
// implポインタ基本テスト
// ============================================================================

inline void test_impl_pointer_basic_phase2() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_impl_pointer_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "implポインタ基本テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Test 1: Basic impl pointer ===") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("After modification through pointer:") != std::string::npos,
                             "Test 1のポインタ経由修正が正しくない");
            
            INTEGRATION_ASSERT(output.find("=== Test 2: Impl pointer array ===") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("=== All impl pointer basic tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_impl_pointer_basic_phase2 passed (%.3fms)\n", execution_time);
}

// ============================================================================
// implポインタ関数テスト
// ============================================================================

inline void test_impl_pointer_function() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_impl_pointer_function.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "implポインタ関数テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Test 1: Modify impl through function ===") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("After doubling:") != std::string::npos,
                             "Test 1の倍増処理が正しくない");
            
            INTEGRATION_ASSERT(output.find("=== Test 2: Calculate through function ===") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("=== All impl pointer function tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_impl_pointer_function passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Pointer Advanced Tests ===\n");
    test_impl_with_pointers();
    test_pointer_return_comprehensive();
    test_impl_pointer_basic_phase2();
    test_impl_pointer_function();
    printf("=== Pointer Advanced Tests Completed ===\n\n");
}

} // namespace PointerAdvancedTests

#endif // POINTER_ADVANCED_TESTS_HPP
