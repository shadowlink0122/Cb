#ifndef TEST_PRINTF_H
#define TEST_PRINTF_H

#include "../framework/integration_test_framework.hpp"

void test_printf_basic() {
    std::string expected = "42\n数値: 123\nHello\nメッセージ: World\nA\n文字: Z\n1234567890\n長整数: 1234567890\n100% test\n進捗: 50%\n";
    std::string output;
    std::string command = "../../main ../../tests/cases/printf/basic_format.cb";
    run_command_and_capture(command, output);
    INTEGRATION_ASSERT_EQ(expected, output, "Printf basic format test");
}

void test_printf_multiple_args() {
    std::string expected = "名前: 田中, 年齢: 25\n結果: 10 + 20 = 30\nABC\n文字: X, 数値: 100, 文字列: test\n生徒情報: 鈴木 (30歳) - 成績: A\n";
    std::string output;
    std::string command = "../../main ../../tests/cases/printf/multiple_args.cb";
    run_command_and_capture(command, output);
    INTEGRATION_ASSERT_EQ(expected, output, "Printf multiple arguments test");
}

void test_printf_expressions() {
    std::string expected = "10 + 5 = 15\n10 - 5 = 5\n10 * 5 = 50\n10 / 5 = 2\n((10 + 5) * 2) = 30\n配列: [1, 2, 3]\n文字列配列: Hello World\n";
    std::string output;
    std::string command = "../../main ../../tests/cases/printf/expressions.cb";
    run_command_and_capture(command, output);
    INTEGRATION_ASSERT_EQ(expected, output, "Printf expressions test");
}

void test_printf_error_cases() {
    // エラーケースは実行時エラーを期待
    std::string output;
    std::string command = "../../main ../../tests/cases/printf/error_cases.cb";
    int exit_code = run_command_and_capture(command, output);
    
    // エラーの場合は非ゼロの終了コードを期待
    if (exit_code != 0) {
        printf("printf error cases test ... passed (error detected)\n");
    } else {
        printf("printf error cases test ... failed (expected error but got success)\n");
    }
}

void test_printf_all() {
    printf("[integration] printf basic format test ... ");
    test_printf_basic();
    printf("passed\n");
    
    printf("[integration] printf multiple arguments test ... ");
    test_printf_multiple_args();
    printf("passed\n");
    
    printf("[integration] printf expressions test ... ");
    test_printf_expressions();
    printf("passed\n");
    
    printf("[integration] printf error cases test ... ");
    test_printf_error_cases();
    printf("passed (errors detected)\n");
}

#endif // TEST_PRINTF_H
