#ifndef FUNCTION_POINTER_TESTS_HPP
#define FUNCTION_POINTER_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>
#include <sstream>

namespace FunctionPointerTests {

// 出力を行ごとに分割するヘルパー関数
inline std::vector<std::string> split_lines(const std::string& str) {
    std::vector<std::string> lines;
    std::istringstream iss(str);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    return lines;
}

// ============================================================================
// 関数ポインタテスト: コールバック関数
// ============================================================================

inline void test_function_pointer_callback() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_callback.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタコールバックテストがエラー終了");
            
            std::vector<std::string> lines = split_lines(output);
            
            INTEGRATION_ASSERT(lines.size() >= 6, "出力行数が不足している");
            
            INTEGRATION_ASSERT(lines[0].find("15") != std::string::npos,
                             "Test 1: applyOperation(10, 5, &add)が正しくない");
            INTEGRATION_ASSERT(lines[1].find("5") != std::string::npos,
                             "Test 2: applyOperation(10, 5, &subtract)が正しくない");
            INTEGRATION_ASSERT(lines[2].find("50") != std::string::npos,
                             "Test 3: compute(10, 5, &multiply)が正しくない");
            INTEGRATION_ASSERT(lines[3].find("15") != std::string::npos,
                             "Test 4: compute(10, 5, &add)が正しくない");
            INTEGRATION_ASSERT(lines[4].find("56") != std::string::npos,
                             "Test 5: applyOperation(7, 8, myFunc)が正しくない");
            INTEGRATION_ASSERT(lines[5].find("18") != std::string::npos,
                             "Test 6: 連続コールバックが正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_callback passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタテスト: 複数の関数ポインタ管理
// ============================================================================

inline void test_function_pointer_multiple() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_multiple_pointers.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "複数関数ポインタテストがエラー終了");
            
            std::vector<std::string> lines = split_lines(output);
            
            INTEGRATION_ASSERT(lines.size() >= 6, "出力行数が不足している");
            
            INTEGRATION_ASSERT(lines[0].find("15") != std::string::npos,
                             "Test 1: op0(10, 5) [add]が正しくない");
            INTEGRATION_ASSERT(lines[1].find("5") != std::string::npos,
                             "Test 2: op1(10, 5) [subtract]が正しくない");
            INTEGRATION_ASSERT(lines[2].find("50") != std::string::npos,
                             "Test 3: op2(10, 5) [multiply]が正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_multiple passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタテスト: 戻り値として返す・チェーン呼び出し
// ============================================================================

inline void test_function_pointer_return() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_return_function_pointer.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタ戻り値テストがエラー終了");
            
            std::vector<std::string> lines = split_lines(output);
            
            INTEGRATION_ASSERT(lines.size() >= 8, "出力行数が不足している");
            
            INTEGRATION_ASSERT(lines[0].find("15") != std::string::npos,
                             "Test 1: getOperation(1)(10, 5) [add]が正しくない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_return passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタアドレス比較テスト
// ============================================================================

inline void test_function_pointer_address_comparison() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_pointer_address_comparison.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタアドレス比較テストがエラー終了");
            
            std::vector<std::string> lines = split_lines(output);
            
            INTEGRATION_ASSERT(lines.size() >= 9, "出力行数が不足");
            
            for (int i = 0; i < 7; i++) {
                INTEGRATION_ASSERT(lines[i].find("1") != std::string::npos,
                                 "Test " + std::to_string(i + 1) + "が失敗");
            }
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_address_comparison passed (%.3fms)\n", execution_time);
}

// ============================================================================
// 関数ポインタアドレス表示テスト
// ============================================================================

inline void test_function_pointer_address_print() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/function_pointer/test_pointer_address_print.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "関数ポインタアドレス表示テストがエラー終了");
            
            std::vector<std::string> lines = split_lines(output);
            
            INTEGRATION_ASSERT(lines.size() >= 7, "出力行数が不足");
            
            INTEGRATION_ASSERT(lines[0].find("0x") == 0,
                             "fp1のアドレスが16進数で表示されていない");
            INTEGRATION_ASSERT(lines[1].find("0x") == 0,
                             "fp2のアドレスが16進数で表示されていない");
        },
        execution_time
    );
    
    printf("[integration-test] test_function_pointer_address_print passed (%.3fms)\n", execution_time);
}

inline void run_all_tests() {
    printf("\n=== Function Pointer Tests ===\n");
    test_function_pointer_callback();
    test_function_pointer_multiple();
    test_function_pointer_return();
    test_function_pointer_address_comparison();
    test_function_pointer_address_print();
    printf("=== Function Pointer Tests Completed ===\n\n");
}

} // namespace FunctionPointerTests

#endif // FUNCTION_POINTER_TESTS_HPP
