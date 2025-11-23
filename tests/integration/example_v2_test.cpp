// v0.14.0: 新しいテストフレームワークの使用例
//
// 【重要】統合テストの責務について
// ========================================
// 統合テストはCb言語の機能をテストします。
// HIR/MIR/LIRの詳細検証はユニットテスト (tests/unit/) で行います。
//
// ✅ 統合テストでテストすること:
//   - Cbプログラムが正しく実行されるか
//   - 言語機能（構文、セマンティクス）が正しく動作するか
//   - 期待される出力が得られるか
//
// ❌ 統合テストでテストしないこと:
//   - HIR/MIR/LIRの生成内容（→ tests/unit/hir/, mir/, lir/）
//   - 最適化パスの詳細（→ tests/unit/backend/）
//   - 内部データ構造（→ tests/unit/common/）
//
// 詳細は tests/README.md を参照してください。
// ========================================

#include "framework/integration_test_framework_v2.hpp"

// 例1: 基本的な算術演算のテスト（両モードで同じ動作を保証）
void test_arithmetic_both_modes() {
    std::cout << "\n=== Testing Arithmetic Operations ===" << std::endl;

    // インタプリタモード：実際に実行して結果を検証
    std::cout << "[INTERPRETER MODE]" << std::endl;
    run_cb_test_with_output(
        "../cases/hir_test_simple.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "プログラムが正常終了すること");
            INTEGRATION_ASSERT_CONTAINS(output, "30",
                                        "10+20の結果30が出力されること");
        },
        ExecutionMode::Interpreter);
    integration_test_passed("算術演算テスト", ExecutionMode::Interpreter);

    // コンパイラモード：エラーなくコンパイルできることを検証
    // （実行結果は検証しない。あくまでコンパイルが成功することを確認）
    std::cout << "[COMPILER MODE]" << std::endl;
    run_cb_test_with_output(
        "../cases/hir_test_simple.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "コンパイルが正常終了すること");
            INTEGRATION_ASSERT_CONTAINS(
                output, "Compilation completed successfully",
                "コンパイル成功メッセージが出力されること");
        },
        ExecutionMode::Compiler);
    integration_test_passed("算術演算テスト（コンパイル）",
                            ExecutionMode::Compiler);
}

// 例2: if文のテスト（両モードで同じ動作を保証）
void test_if_statement_both_modes() {
    std::cout << "\n=== Testing If Statement ===" << std::endl;

    // テストケースファイルがある場合の例
    // インタプリタモード：実際に実行
    std::cout << "[INTERPRETER MODE]" << std::endl;
    run_cb_test_with_output(
        "../cases/hir_test_simple.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "プログラムが正常終了すること");
        },
        ExecutionMode::Interpreter);
    integration_test_passed("if文テスト", ExecutionMode::Interpreter);

    // コンパイラモード：コンパイルが成功することを確認
    std::cout << "[COMPILER MODE]" << std::endl;
    run_cb_test_with_output(
        "../cases/hir_test_simple.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "コンパイルが正常終了すること");
        },
        ExecutionMode::Compiler);
    integration_test_passed("if文テスト（コンパイル）",
                            ExecutionMode::Compiler);
}

// 例3: パフォーマンス比較（実行時間測定）
void test_with_performance_measurement() {
    std::cout << "\n=== Performance Measurement ===" << std::endl;

    double exec_time_interpreter = 0.0;
    double exec_time_compiler = 0.0;

    // インタプリタモードの実行時間
    run_cb_test_with_output_and_time(
        "../cases/hir_test_simple.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "正常終了すること");
        },
        exec_time_interpreter, ExecutionMode::Interpreter);

    // コンパイラモードの実行時間（コンパイル時間）
    run_cb_test_with_output_and_time(
        "../cases/hir_test_simple.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "正常終了すること");
        },
        exec_time_compiler, ExecutionMode::Compiler);

    std::cout << "[integration-test] Performance comparison:" << std::endl;
    std::cout << "  Interpreter execution: " << exec_time_interpreter << " ms"
              << std::endl;
    std::cout << "  Compiler compile time: " << exec_time_compiler << " ms"
              << std::endl;

    integration_test_passed_with_time(
        "パフォーマンステスト", "hir_test_simple.cb", exec_time_interpreter,
        ExecutionMode::Interpreter);
    integration_test_passed_with_time("パフォーマンステスト（コンパイル）",
                                      "hir_test_simple.cb", exec_time_compiler,
                                      ExecutionMode::Compiler);
}

int main() {
    std::cout << "=== v0.14.0 Integration Test Framework Example ==="
              << std::endl;
    std::cout << "Integration tests focus on Cb language features" << std::endl;
    std::cout << "(HIR/MIR/LIR details are tested in unit tests)" << std::endl;

    IntegrationTestCounter::reset();
    TimingStats::reset();

    try {
        test_arithmetic_both_modes();
        test_if_statement_both_modes();
        test_with_performance_measurement();

        IntegrationTestCounter::print_summary();
        TimingStats::print_timing_summary();

        return IntegrationTestCounter::get_failed() > 0 ? 1 : 0;
    } catch (const std::exception &e) {
        std::cerr << "\n[integration-test] Exception caught: " << e.what()
                  << std::endl;
        IntegrationTestCounter::print_summary();
        return 1;
    }
}
