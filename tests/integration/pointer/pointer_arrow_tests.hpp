#ifndef POINTER_ARROW_TESTS_HPP
#define POINTER_ARROW_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace PointerArrowTests {

// ============================================================================
// アロー演算子テスト
// ============================================================================

inline void test_arrow_operator() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_arrow_operator.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "アロー演算子テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("Test 1: Struct pointer with arrow operator") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1が成功していない");
            
            INTEGRATION_ASSERT(output.find("Test 3: Struct with pointer member") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3が成功していない");
            
            INTEGRATION_ASSERT(output.find("Test 4: Arrow operator with compound assignment") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 4 passed") != std::string::npos,
                             "Test 4が成功していない");
            
            INTEGRATION_ASSERT(output.find("=== All arrow operator tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_arrow_operator passed (%.3fms)\n", execution_time);
}

// ============================================================================
// implメソッド内でのアロー演算子テスト
// ============================================================================

inline void test_arrow_in_impl() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_arrow_in_impl.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "implアロー演算子テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Arrow Operator in Impl Methods Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("Test 1: Pointer member in self") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1が成功していない");
            
            INTEGRATION_ASSERT(output.find("Test 2: Self with pointer member") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2が成功していない");
            
            INTEGRATION_ASSERT(output.find("=== All impl arrow operator tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_arrow_in_impl passed (%.3fms)\n", execution_time);
}

// ============================================================================
// Interface型ポインタとアロー演算子テスト
// ============================================================================

inline void test_interface_pointer_arrow() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/pointer/test_interface_pointer_arrow.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Interfaceポインタアロー演算子テストがエラー終了");
            
            INTEGRATION_ASSERT(output.find("=== Interface Pointer with Arrow Operator Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            INTEGRATION_ASSERT(output.find("Test 1: Basic arrow operator with interface pointer") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1が成功していない");
            
            INTEGRATION_ASSERT(output.find("Test 2: Method call with arguments") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2が成功していない");
            
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
            INTEGRATION_ASSERT(output.find("Test 1: Assign and dereference self pointer member") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 1 passed") != std::string::npos,
                             "Test 1が成功していない");
            
            INTEGRATION_ASSERT(output.find("Test 2: Member access through self pointer member") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 2 passed") != std::string::npos,
                             "Test 2が成功していない");
            
            INTEGRATION_ASSERT(output.find("Test 3: Reassign self pointer member") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Test 3 passed") != std::string::npos,
                             "Test 3が成功していない");
            
            INTEGRATION_ASSERT(output.find("=== All impl self pointer access tests passed ===") != std::string::npos,
                             "最終メッセージが表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_impl_self_pointer_access passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Pointer Arrow Tests ===\n");
    test_arrow_operator();
    test_arrow_in_impl();
    test_interface_pointer_arrow();
    test_impl_self_pointer_access();
    printf("=== Pointer Arrow Tests Completed ===\n\n");
}

} // namespace PointerArrowTests

#endif // POINTER_ARROW_TESTS_HPP
