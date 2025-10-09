#ifndef ASSERT_TESTS_HPP
#define ASSERT_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <chrono>

namespace AssertTests {

// ============================================================================
// 基本的なassert関数のテスト
// ============================================================================

inline void test_basic_assert() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/assert/test_assert_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "assertテストがエラー終了");
            
            // Header
            INTEGRATION_ASSERT(output.find("=== Assert Function Test ===") != std::string::npos,
                             "テストヘッダーが出力されていない");
            
            // Test 1: assert(true)
            INTEGRATION_ASSERT(output.find("Test 1: assert(true)") != std::string::npos,
                             "Test 1のヘッダーが出力されていない");
            INTEGRATION_ASSERT(output.find("✓ Passed") != std::string::npos,
                             "Test 1のパスメッセージがない");
            
            // Test 2: assert with variable
            INTEGRATION_ASSERT(output.find("Test 2: assert with variable") != std::string::npos,
                             "Test 2のヘッダーが出力されていない");
            
            // Test 3: assert with expression
            INTEGRATION_ASSERT(output.find("Test 3: assert with expression") != std::string::npos,
                             "Test 3のヘッダーが出力されていない");
            
            // Test 4: assert with comparison
            INTEGRATION_ASSERT(output.find("Test 4: assert with comparison") != std::string::npos,
                             "Test 4のヘッダーが出力されていない");
            
            // Final message
            INTEGRATION_ASSERT(output.find("✅ All assertion tests passed!") != std::string::npos,
                             "最終的な成功メッセージがない");
        },
        execution_time
    );
    
    printf("[integration-test] test_basic_assert passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 全てのassertテストを実行
// ============================================================================

inline void run_all_assert_tests() {
    printf("\n=== Assert Tests ===\n");
    
    test_basic_assert();
    
    printf("=== All Assert Tests Passed ===\n\n");
}

} // namespace AssertTests

#endif // ASSERT_TESTS_HPP
