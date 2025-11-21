#include "framework/compiler_test_framework.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// 各テストモジュールをインクルード
#include "arithmetic/test_arithmetic.hpp"
#include "basic/test_basic.hpp"
#include "ffi/test_ffi.hpp"
// 他のテストは段階的に追加

using namespace cb::test;

void print_usage() {
    std::cout << "Usage: cb_compiler_tests [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -m, --mode MODE        Execution mode: interpreter or "
                 "compiler (default: interpreter)"
              << std::endl;
    std::cout << "  -o, --output DIR       Compiler output directory (default: "
                 "/tmp/cb_test_compiler_output)"
              << std::endl;
    std::cout << "  -h, --help             Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  ./cb_compiler_tests                           # Run in "
                 "interpreter mode"
              << std::endl;
    std::cout << "  ./cb_compiler_tests -m compiler               # Run in "
                 "compiler mode"
              << std::endl;
    std::cout << "  ./cb_compiler_tests -m compiler -o /tmp/out   # Custom "
                 "output directory"
              << std::endl;
}

int main(int argc, char **argv) {
    // デフォルト設定
    ExecutionMode mode = ExecutionMode::Interpreter;
    std::string output_dir = "/tmp/cb_test_compiler_output";

    // コマンドライン引数の解析
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-m" || arg == "--mode") {
            if (i + 1 < argc) {
                std::string mode_str = argv[++i];
                if (mode_str == "compiler") {
                    mode = ExecutionMode::Compiler;
                } else if (mode_str == "interpreter") {
                    mode = ExecutionMode::Interpreter;
                } else {
                    std::cerr << "Error: Invalid mode: " << mode_str
                              << std::endl;
                    std::cerr << "Valid modes: interpreter, compiler"
                              << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Error: --mode requires an argument" << std::endl;
                return 1;
            }
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                output_dir = argv[++i];
            } else {
                std::cerr << "Error: --output requires an argument"
                          << std::endl;
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option: " << arg << std::endl;
            print_usage();
            return 1;
        }
    }

    // 実行モードを設定
    set_execution_mode(mode);
    set_compiler_output_dir(output_dir);

    std::cout << "======================================" << std::endl;
    std::cout << "Cb Integration Tests" << std::endl;
    if (mode == ExecutionMode::Compiler) {
        std::cout << "Mode: COMPILER (HIR → C++ → Binary)" << std::endl;
        std::cout << "Output directory: " << output_dir << std::endl;
    } else {
        std::cout << "Mode: INTERPRETER" << std::endl;
    }
    std::cout << "======================================" << std::endl;

    int total_failed = 0;
    int total_passed = 0;
    int total_skipped = 0;

    // 各テストスイートを実行
    std::vector<TestSuite *> test_suites;

    // 基本テスト
    test_suites.push_back(new TestSuite("Basic Tests"));
    register_basic_tests(*test_suites.back());

    // 算術演算テスト
    test_suites.push_back(new TestSuite("Arithmetic Tests"));
    register_arithmetic_tests(*test_suites.back());

    // FFIテスト（コンパイラモードでは重要）
    if (mode == ExecutionMode::Compiler) {
        test_suites.push_back(new TestSuite("FFI Tests"));
        register_ffi_tests(*test_suites.back());
    }

    // テストスイートを実行
    for (auto suite : test_suites) {
        suite->run();
        total_failed += suite->get_failed_count();
        total_passed += suite->get_passed_count();
        total_skipped += suite->get_skipped_count();
    }

    // 結果サマリー
    std::cout << "\n======================================" << std::endl;
    std::cout << "Overall Results" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Total Passed:  " << total_passed << std::endl;
    std::cout << "Total Failed:  " << total_failed << std::endl;
    std::cout << "Total Skipped: " << total_skipped << std::endl;
    std::cout << std::endl;

    // クリーンアップ
    for (auto suite : test_suites) {
        delete suite;
    }

    if (total_failed == 0) {
        std::cout << "✅ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some tests failed." << std::endl;
        return 1;
    }
}
