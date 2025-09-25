#pragma once

#include "../framework/integration_test_framework.hpp"
#include <vector>
#include <sstream>

inline void test_integration_ternary() {
    std::cout << "[integration] Running ternary tests..." << std::endl;
    
    // 三項演算子の基本テスト
    const std::string test_file_basic = "../../tests/cases/ternary/basic_ternary.cb";
    run_cb_test_with_output(test_file_basic, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "20", "Expected '20' for max(10, 20)");
            INTEGRATION_ASSERT_CONTAINS(output, "10", "Expected '10' for min(10, 20)");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Expected '1' for score 85 >= 60");
            INTEGRATION_ASSERT_CONTAINS(output, "0", "Expected '0' for score 45 >= 60");
            // ネストした三項演算子の結果を順番にチェック
            std::string::size_type pos = output.find("0");
            pos = output.find("1", pos + 1);
            INTEGRATION_ASSERT(pos != std::string::npos, "Expected nested ternary results");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for ternary basic test");
        });
    integration_test_passed("ternary basic test", test_file_basic);
    
    // 三項演算子の複雑なテスト
    const std::string test_file_complex = "../../tests/cases/ternary/complex_ternary.cb";
    run_cb_test_with_output(test_file_complex, 
        [](const std::string& output, int exit_code) {
            // 出力を行ごとに分割して順序をチェック
            std::vector<std::string> lines;
            std::stringstream ss(output);
            std::string line;
            while (std::getline(ss, line)) {
                if (!line.empty()) {
                    lines.push_back(line);
                }
            }
            
            INTEGRATION_ASSERT(lines.size() >= 11, "Expected at least 11 lines of output");
            
            // 基本的な三項演算子のテスト (行1-6)
            INTEGRATION_ASSERT_EQ("15", lines[0], "Expected '15' for median(15, 10, 20)");
            INTEGRATION_ASSERT_EQ("15", lines[1], "Expected '15' for dynamic size");
            INTEGRATION_ASSERT_EQ("-1", lines[2], "Expected '-1' for sign(-42)");
            INTEGRATION_ASSERT_EQ("0", lines[3], "Expected '0' for sign(0)");
            INTEGRATION_ASSERT_EQ("1", lines[4], "Expected '1' for sign(123)");
            INTEGRATION_ASSERT_EQ("12", lines[5], "Expected '12' for complex bitwise condition");
            
            // 関数を使った三項演算子のテスト (行7-11)
            INTEGRATION_ASSERT_EQ("200", lines[6], "Expected '200' for is_even(-5) ? 100 : 200 (odd number)");
            INTEGRATION_ASSERT_EQ("15", lines[7], "Expected '15' for abs_func(-15)");
            INTEGRATION_ASSERT_EQ("3", lines[8], "Expected '3' for min_func(7, 3) when 7 is odd");
            INTEGRATION_ASSERT_EQ("24", lines[9], "Expected '24' for factorial(4)");
            INTEGRATION_ASSERT_EQ("8", lines[10], "Expected '8' for max_func(abs_func(-8), 6)");
            
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for ternary complex test");
        });
    integration_test_passed("ternary complex test", test_file_complex);
    
    std::cout << "[integration] Ternary tests completed" << std::endl;
}
