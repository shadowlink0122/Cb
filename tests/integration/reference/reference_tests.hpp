#ifndef REFERENCE_TESTS_HPP
#define REFERENCE_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <chrono>

namespace ReferenceTests {

// ============================================================================
// 基本的な参照型のテスト
// ============================================================================

inline void test_simple_reference() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/reference/test_simple_ref.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "単純な参照型テストがエラー終了");
            
            // Expected output: 10, 20, 20
            INTEGRATION_ASSERT(output.find("10") != std::string::npos, "ref初期値が正しくない");
            INTEGRATION_ASSERT(output.find("20") != std::string::npos, "参照経由でaが変更されていない");
        },
        execution_time
    );
}

inline void test_reference_basic() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/reference/test_reference_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的な参照型テストがエラー終了");
            
            // Test 1: 整数型の参照
            INTEGRATION_ASSERT(output.find("10") != std::string::npos, "ref_a初期値が正しくない");
            INTEGRATION_ASSERT(output.find("20") != std::string::npos, "参照経由でaが変更されていない");
            
            // Test 2: 複数の参照
            INTEGRATION_ASSERT(output.find("40") != std::string::npos, "ref_b経由でbが変更されていない");
            
            // Test 3: 浮動小数点数の参照
            INTEGRATION_ASSERT(output.find("3.14") != std::string::npos, "ref_f初期値が正しくない");
            INTEGRATION_ASSERT(output.find("2.71") != std::string::npos, "参照経由でfが変更されていない");
            
            // Test 4: 参照から参照へ（連鎖）
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, "ref2初期値が正しくない");
            INTEGRATION_ASSERT(output.find("200") != std::string::npos, "参照の連鎖でxが変更されていない");
        },
        execution_time
    );
}

inline void test_reference_function_param() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/reference/test_reference_function_param.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数パラメータとしての参照テストがエラー終了");
            
            // Test 1: 基本的な参照パラメータ
            INTEGRATION_ASSERT(output.find("Test 1: Basic reference parameter") != std::string::npos, "Test 1ヘッダーがない");
            
            // Test 2: 複数の参照パラメータ
            INTEGRATION_ASSERT(output.find("Test 2: Multiple reference parameters") != std::string::npos, "Test 2ヘッダーがない");
            
            // Test 3: 参照とポインタの混在
            INTEGRATION_ASSERT(output.find("Test 3: Reference and pointer mix") != std::string::npos, "Test 3ヘッダーがない");
            INTEGRATION_ASSERT(output.find("110") != std::string::npos, "参照経由の変更が反映されていない");
            INTEGRATION_ASSERT(output.find("220") != std::string::npos, "ポインタ経由の変更が反映されていない");
        },
        execution_time
    );
}

inline void test_reference_return() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/reference/test_simple_reference_return.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "参照戻り値テストがエラー終了");
            
            // 期待出力: Before: 100, After getting reference: 100, After modifying: 150, 150
            INTEGRATION_ASSERT(output.find("Before") != std::string::npos, "Beforeメッセージがない");
            INTEGRATION_ASSERT(output.find("After getting reference") != std::string::npos, "After getting referenceメッセージがない");
            INTEGRATION_ASSERT(output.find("After modifying") != std::string::npos, "After modifyingメッセージがない");
            INTEGRATION_ASSERT(output.find("150") != std::string::npos, "参照経由の変更が反映されていない");
        },
        execution_time
    );
}

inline void test_reference_return_comprehensive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/reference/test_reference_return.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "包括的な参照戻り値テストがエラー終了");
            
            // Test 1: グローバル変数への参照を返す
            INTEGRATION_ASSERT(output.find("Test 1: Return reference to global") != std::string::npos, "Test 1ヘッダーがない");
            INTEGRATION_ASSERT(output.find("175") != std::string::npos, "グローバル変数への参照が正しく動作していない");
            
            // Test 2: 参照の転送
            INTEGRATION_ASSERT(output.find("Test 2: Forward reference") != std::string::npos, "Test 2ヘッダーがない");
            INTEGRATION_ASSERT(output.find("75") != std::string::npos, "参照の転送が正しく動作していない");
            
            // Test 3: 条件に応じた参照
            INTEGRATION_ASSERT(output.find("Test 3: Conditional reference") != std::string::npos, "Test 3ヘッダーがない");
        },
        execution_time
    );
}

// ============================================================================
// すべての参照型テストを実行
// ============================================================================

inline void run_all_reference_tests() {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "Running Reference Tests..." << std::endl;
    std::cout << "============================================================" << std::endl;
    
    test_simple_reference();
    test_reference_basic();
    test_reference_function_param();
    test_reference_return();
    test_reference_return_comprehensive();
    
    std::cout << "✅ PASS: Reference Tests (5 tests)" << std::endl;
}

} // namespace ReferenceTests

#endif // REFERENCE_TESTS_HPP
