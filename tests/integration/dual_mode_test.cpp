#include "framework/dual_mode_test_framework.hpp"

void test_simple_main() {
    run_dual_mode_test(
        "Simple Main Test", "../../tests/cases/basic/simple_main.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Should execute successfully");
        });
}

void test_arithmetic() {
    run_dual_mode_test(
        "Simple Arithmetic", "../../tests/cases/arithmetic/ok.cb",
        [](const std::string &output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Should execute successfully");
        });
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Cb Dual-Mode Integration Test                        ║\n";
    std::cout << "║  Testing both Interpreter and Compiler modes          ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";
    std::cout << std::endl;

    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;

    // テスト実行
    try {
        test_simple_main();
        passed_tests++;
    } catch (...) {
        failed_tests++;
    }
    total_tests++;

    try {
        test_arithmetic();
        passed_tests++;
    } catch (...) {
        failed_tests++;
    }
    total_tests++;

    // 結果表示
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Test Results                                          ║\n";
    std::cout << "╠════════════════════════════════════════════════════════╣\n";
    std::cout << "║  Total:  " << total_tests
              << " tests                                       ║\n";
    std::cout << "║  Passed: " << passed_tests
              << " tests                                       ║\n";
    std::cout << "║  Failed: " << failed_tests
              << " tests                                       ║\n";
    std::cout << "╚════════════════════════════════════════════════════════╝\n";

    return failed_tests == 0 ? 0 : 1;
}
