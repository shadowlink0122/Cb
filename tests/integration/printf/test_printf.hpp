#ifndef TEST_PRINTF_H
#define TEST_PRINTF_H

#include "../framework/integration_test_framework.hpp"
#include <sstream>

inline void test_printf_basic() {
    run_cb_test_with_output("../../tests/cases/printf/basic_format.cb", 
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(exit_code, 0, "basic printf test should succeed");
        std::string expected = "42\n数値: 123\nHello\nメッセージ: World\nA\n文字: Z\n1234567890\n長整数: 1234567890\n100%%\n進捗: 50%\n";
        INTEGRATION_ASSERT_EQ(output, expected, "printf basic format output check");
    });
}

inline void test_printf_multiple_args() {
    run_cb_test_with_output("../../tests/cases/printf/multiple_args_simple.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(exit_code, 0, "multiple args printf test should succeed");
        std::string expected = "名前: 田中, 年齢: 25\n結果: 10 + 20 = 30\n数値1: 100, 数値2: 200, 文字列: test\n生徒情報: 鈴木 (30歳)\n";
        INTEGRATION_ASSERT_EQ(output, expected, "printf multiple args output check");
    });
}

inline void test_print_multi_args() {
    run_cb_test_with_output("../../tests/cases/printf/test_extra_args.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(exit_code, 0, "multi args print test should succeed");
        std::string expected = "Hello World Extra\n10 20 30\n1 2 3\n";
        INTEGRATION_ASSERT_EQ(output, expected, "print multi args output check with extra args");
    });
}

inline void test_printf_extra_args() {
    run_cb_test_with_output("../../tests/cases/printf/test_extra_args_detailed.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(exit_code, 0, "extra args test should succeed");
        std::string expected = "値: 100 200 300\n名前: 田中 次郎 三郎\n5 + 3 = 8 999\nHello World !\n情報: 佐藤 25 追加情報\n";
        INTEGRATION_ASSERT_EQ(output, expected, "extra args should be displayed with space separation");
    });
}

inline void test_printf_missing_args() {
    run_cb_test_with_output("../../tests/cases/printf/verify_missing_args.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(exit_code, 0, "missing args should still succeed");
        std::string expected = "10 %d\n%s %d\n値: 42, 名前: %s\n";
        INTEGRATION_ASSERT_EQ(output, expected, "missing args should show format specifiers");
    });
}

inline void test_printf_zero_padding() {
    run_cb_test_with_output("../../tests/cases/printf/zero_padding_comprehensive.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_EQ(exit_code, 0, "zero padding test should succeed");
        
        std::vector<std::string> expected_lines = {
            "Basic: 5",
            "Basic: 05", 
            "Basic: 005",
            "Basic: 0005",
            "Basic: 00005",
            "Basic: 0000000005",
            "Large: 42",
            "Large: 042",
            "Large: 0042", 
            "Large: 00042",
            "Overflow: 123",
            "Overflow: 123",
            "Overflow: 123",
            "Negative: -05",
            "Negative: -005", 
            "Negative: -0042",
            "Zero: 000",
            "Zero: 00000",
            "BigWidth: 000000000000123",
            "BigWidth: 00000000000000000001"
        };
        
        std::istringstream iss(output);
        std::string line;
        size_t line_count = 0;
        
        while (std::getline(iss, line) && line_count < expected_lines.size()) {
            if (line != expected_lines[line_count]) {
                throw std::runtime_error("Zero padding line " + std::to_string(line_count + 1) + 
                                       " mismatch. Expected: '" + expected_lines[line_count] + 
                                       "', Got: '" + line + "'");
            }
            line_count++;
        }
        
        if (line_count != expected_lines.size()) {
            throw std::runtime_error("Zero padding: Expected " + std::to_string(expected_lines.size()) + 
                                   " lines, got " + std::to_string(line_count));
        }
    });
}

inline void test_printf_error_cases() {
    // 引数不足のエラーケース（出力を抑制）
    run_cb_test_with_output("../../tests/cases/printf/error_missing_one_arg.cb",
                           [](const std::string& output, int exit_code) {
        // 出力があることだけ確認、標準出力はしない
        INTEGRATION_ASSERT_GT(output.length(), 0, "missing arg test should produce output");
    });
    
    run_cb_test_with_output("../../tests/cases/printf/error_no_args.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_GT(output.length(), 0, "no args test should produce output");
    });
    
    // 型不一致のエラーケース
    run_cb_test_with_output("../../tests/cases/printf/error_string_to_d.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_GT(output.length(), 0, "string to d test should produce output");
    });
    
    run_cb_test_with_output("../../tests/cases/printf/error_number_to_s.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_GT(output.length(), 0, "number to s test should produce output");
    });
    
    // 未サポートフォーマットのエラーケース
    run_cb_test_with_output("../../tests/cases/printf/error_unsupported_x.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_GT(output.length(), 0, "unsupported x test should produce output");
    });
    
    run_cb_test_with_output("../../tests/cases/printf/error_invalid_z.cb",
                           [](const std::string& output, int exit_code) {
        INTEGRATION_ASSERT_GT(output.length(), 0, "invalid z test should produce output");
    });
}

inline void test_printf_all() {
    printf("[integration] printf basic format test ... ");
    test_printf_basic();
    printf("passed\n");
    
    printf("[integration] printf multiple arguments test ... ");
    test_printf_multiple_args();
    printf("passed\n");
    
    printf("[integration] print multi arguments test ... ");
    test_print_multi_args();
    printf("passed\n");
    
    printf("[integration] printf extra arguments test ... ");
    test_printf_extra_args();
    printf("passed\n");
    
    printf("[integration] printf missing arguments test ... ");
    test_printf_missing_args();
    printf("passed\n");
    
    printf("[integration] printf zero padding test ... ");
    test_printf_zero_padding();
    printf("passed\n");
    
    printf("[integration] printf error cases test ... ");
    test_printf_error_cases();
    printf("passed (errors detected)\n");
}

#endif // TEST_PRINTF_H
