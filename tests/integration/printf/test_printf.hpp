#ifndef TEST_PRINTF_H
#define TEST_PRINTF_H

#include "../framework/integration_test_framework.hpp"

void test_printf_basic() {
    std::string expected = "42\n数値: 123\nHello\nメッセージ: World\nA\n文字: Z\n1234567890\n長整数: 1234567890\n100% test\n進捗: 50%\n";
    run_cb_test_with_output_and_time_auto("../../tests/cases/printf/basic_format.cb",
        [expected](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Printf basic format test should succeed");
            INTEGRATION_ASSERT_EQ(expected, output, "Printf basic format test");
        });
    integration_test_passed_with_time_auto("printf basic format test", "basic_format.cb");
}

void test_printf_multiple_args() {
    std::string expected = "名前: 田中, 年齢: 25\n結果: 10 + 20 = 30\nABC\n文字: X, 数値: 100, 文字列: test\n生徒情報: 鈴木 (30歳) - 成績: A\n";
    run_cb_test_with_output_and_time_auto("../../tests/cases/printf/multiple_args.cb",
        [expected](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Printf multiple arguments test should succeed");
            INTEGRATION_ASSERT_EQ(expected, output, "Printf multiple arguments test");
        });
    integration_test_passed_with_time_auto("printf multiple arguments test", "multiple_args.cb");
}

void test_printf_expressions() {
    std::string expected = "10 + 5 = 15\n10 - 5 = 5\n10 * 5 = 50\n10 / 5 = 2\n((10 + 5) * 2) = 30\n配列: [1, 2, 3]\n文字列配列: Hello World\n";
    run_cb_test_with_output_and_time_auto("../../tests/cases/printf/expressions.cb",
        [expected](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Printf expressions test should succeed");
            INTEGRATION_ASSERT_EQ(expected, output, "Printf expressions test");
        });
    integration_test_passed_with_time_auto("printf expressions test", "expressions.cb");
}

void test_printf_error_cases() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/printf/error_cases.cb",
        [](const std::string& output, int exit_code) {
            // エラーの場合は非ゼロの終了コードを期待
            INTEGRATION_ASSERT_NE(0, exit_code, "Printf error cases should fail");
        });
    integration_test_passed_with_error_and_time_auto("printf error cases test", "error_cases.cb");
}

void test_printf_all() {
    test_printf_basic();
    test_printf_multiple_args();
    test_printf_expressions();
    test_printf_error_cases();
}

#endif // TEST_PRINTF_H
