#pragma once

#include "../framework/integration_test_framework.hpp"
#include <vector>
#include <sstream>

inline void test_integration_ternary() {
    std::cout << "[integration] Running ternary tests..." << std::endl;
    
    // 三項演算子の基本テスト
    const std::string test_file_basic = "../../tests/cases/ternary/basic_ternary.cb";
    run_cb_test_with_output_and_time_auto(test_file_basic, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for ternary basic test");
            INTEGRATION_ASSERT_CONTAINS(output, "Ternary operator test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "max(10, 20): 20", "Expected max(10, 20): 20 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "min(10, 20): 10", "Expected min(10, 20): 10 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "score 85 pass: 1", "Expected score 85 pass: 1 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "score 45 pass: 0", "Expected score 45 pass: 0 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "nested ternary x=5: 0", "Expected nested ternary x=5: 0 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "nested ternary x=15: 1", "Expected nested ternary x=15: 1 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "nested ternary x=-5: -1", "Expected nested ternary x=-5: -1 in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Ternary operator test passed", "Expected success message in output");
        });
    integration_test_passed_with_time_auto("ternary basic test", test_file_basic);
    
    // 三項演算子の複雑なテスト
    const std::string test_file_complex = "../../tests/cases/ternary/complex_ternary.cb";
    run_cb_test_with_output_and_time_auto(test_file_complex, 
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
    integration_test_passed_with_time_auto("ternary complex test", test_file_complex);
    
    // 変数宣言・メンバアクセス・配列アクセスの三項演算子テスト
    const std::string test_file_variable_member = "../../tests/cases/ternary/variable_member_ternary.cb";
    run_cb_test_with_output_and_time_auto(test_file_variable_member, 
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
            
            INTEGRATION_ASSERT(lines.size() >= 10, "Expected at least 10 lines of output");
            
            // println内での三項演算子テスト
            INTEGRATION_ASSERT_EQ("Success", lines[0], "Expected 'Success' for println with ternary (string)");
            INTEGRATION_ASSERT_EQ("42", lines[1], "Expected '42' for println with ternary (numeric)");
            
            // 構造体メンバアクセスの三項演算子テスト
            INTEGRATION_ASSERT_EQ("Alice", lines[2], "Expected 'Alice' for struct member access ternary (string)");
            INTEGRATION_ASSERT_EQ("25", lines[3], "Expected '25' for struct member access ternary (numeric)");
            
            // 配列要素アクセスの三項演算子テスト
            INTEGRATION_ASSERT_EQ("4", lines[4], "Expected '4' for array element access ternary");
            INTEGRATION_ASSERT_EQ("5", lines[5], "Expected '5' for array element access ternary");
            INTEGRATION_ASSERT_EQ("6", lines[6], "Expected '6' for array element access ternary");
            
            // 条件を変更したテスト
            INTEGRATION_ASSERT_EQ("Bob", lines[7], "Expected 'Bob' for changed condition struct member ternary");
            INTEGRATION_ASSERT_EQ("30", lines[8], "Expected '30' for changed condition struct member ternary");
            INTEGRATION_ASSERT_EQ("1", lines[9], "Expected '1' for changed condition array element ternary");
            
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for ternary variable/member test");
        });
    integration_test_passed_with_time_auto("ternary member/array access test", test_file_variable_member);
    
    std::cout << "[integration] Ternary tests completed" << std::endl;
}
