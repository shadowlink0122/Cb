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
    
    // 文字列返値三項演算子テスト
    const std::string test_file_string_return = "../../tests/cases/ternary/string_return_ternary.cb";
    run_cb_test_with_output_and_time_auto(test_file_string_return, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for string return ternary test");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Return Value Ternary Test ===", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Literal ternary: pos", "Expected literal ternary result 'pos'");
            INTEGRATION_ASSERT_CONTAINS(output, "Function ternary: positive", "Expected function ternary result 'positive'");
            INTEGRATION_ASSERT_CONTAINS(output, "Function with ternary: positive", "Expected function with ternary result 'positive'");
            INTEGRATION_ASSERT_CONTAINS(output, "Function with ternary 2: negative", "Expected function with ternary 2 result 'negative'");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex ternary: zero", "Expected complex ternary result 'zero'");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Return Value Test Complete ===", "Expected test complete message");
        });
    integration_test_passed_with_time_auto("ternary string return test", test_file_string_return);
    
    // 三項演算子return文テスト
    const std::string test_file_ternary_return = "../../test_ternary_return.cb";
    run_cb_test_with_output_and_time_auto(test_file_ternary_return, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for ternary return test");
            INTEGRATION_ASSERT_CONTAINS(output, "Result1: positive", "Expected Result1: positive");
            INTEGRATION_ASSERT_CONTAINS(output, "Result2: negative", "Expected Result2: negative");
        });
    integration_test_passed_with_time_auto("ternary return statement test", test_file_ternary_return);
    
    // 単純な三項演算子return文テスト
    const std::string test_file_simple_ternary_return = "../../test_simple_ternary_return.cb";
    run_cb_test_with_output_and_time_auto(test_file_simple_ternary_return, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for simple ternary return test");
            INTEGRATION_ASSERT_CONTAINS(output, "Result: pos", "Expected Result: pos");
        });
    integration_test_passed_with_time_auto("simple ternary return test", test_file_simple_ternary_return);
    
    // 文字列三項演算子最小統合テスト  
    const std::string test_file_string_minimal = "../../tests/cases/ternary/test_string_ternary_minimal.cb";
    run_cb_test_with_output_and_time_auto(test_file_string_minimal, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for string ternary minimal test");
            
            // テストヘッダーとフッター
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Ternary Integration Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Expected test complete message");
            
            // 基本テスト
            INTEGRATION_ASSERT_CONTAINS(output, "Basic positive: positive", "Expected basic positive result");
            INTEGRATION_ASSERT_CONTAINS(output, "Basic negative: negative", "Expected basic negative result");
            
            // ネストテスト
            INTEGRATION_ASSERT_CONTAINS(output, "Nested zero: zero", "Expected nested zero result");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested positive: pos", "Expected nested positive result");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested negative: neg", "Expected nested negative result");
        });
    integration_test_passed_with_time_auto("string ternary minimal test", test_file_string_minimal);
    
    // 文字列三項演算子統合テスト（printf内関数呼び出し + 変数参照修正検証）
    const std::string test_file_string_integration = "../../tests/cases/ternary/test_string_ternary_integration.cb";
    run_cb_test_with_output_and_time_auto(test_file_string_integration, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for string ternary integration test");
            
            // テストヘッダーとフッター
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Ternary Integration Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Ternary Integration Test Complete ===", "Expected test complete message");
            
            // 基本テスト（printf内での関数呼び出し）
            INTEGRATION_ASSERT_CONTAINS(output, "basic_string_ternary(15): positive", "Expected basic_string_ternary(15): positive");
            INTEGRATION_ASSERT_CONTAINS(output, "basic_string_ternary(-5): negative", "Expected basic_string_ternary(-5): negative");
            
            // ネストテスト（printf内での関数呼び出し）
            INTEGRATION_ASSERT_CONTAINS(output, "nested_string_ternary(0): zero", "Expected nested_string_ternary(0): zero");
            INTEGRATION_ASSERT_CONTAINS(output, "nested_string_ternary(10): positive", "Expected nested_string_ternary(10): positive");
            INTEGRATION_ASSERT_CONTAINS(output, "nested_string_ternary(-10): negative", "Expected nested_string_ternary(-10): negative");
            
            // 複雑テスト（変数参照 + ネストした三項演算子）
            INTEGRATION_ASSERT_CONTAINS(output, "complex_classification(150): high", "Expected complex_classification(150): high");
            INTEGRATION_ASSERT_CONTAINS(output, "complex_classification(75): medium", "Expected complex_classification(75): medium");
            INTEGRATION_ASSERT_CONTAINS(output, "complex_classification(25): low", "Expected complex_classification(25): low");
            INTEGRATION_ASSERT_CONTAINS(output, "complex_classification(5): minimal", "Expected complex_classification(5): minimal");
            
            // 構造体テスト
            INTEGRATION_ASSERT_CONTAINS(output, "evaluate_performance(Alice, 95): excellent", "Expected evaluate_performance(Alice, 95): excellent");
            INTEGRATION_ASSERT_CONTAINS(output, "evaluate_performance(Bob, 65): average", "Expected evaluate_performance(Bob, 65): average");
            INTEGRATION_ASSERT_CONTAINS(output, "evaluate_performance(Carol, 45): poor", "Expected evaluate_performance(Carol, 45): poor");
            
            // 配列テスト
            INTEGRATION_ASSERT_CONTAINS(output, "array_based_decision([10,20,15], 0): lesser_or_equal", "Expected array_based_decision([10,20,15], 0): lesser_or_equal");
            INTEGRATION_ASSERT_CONTAINS(output, "array_based_decision([10,20,15], 1): greater", "Expected array_based_decision([10,20,15], 1): greater");
            INTEGRATION_ASSERT_CONTAINS(output, "array_based_decision([10,20,15], 2): greater", "Expected array_based_decision([10,20,15], 2): greater");
        });
    integration_test_passed_with_time_auto("string ternary integration test (printf + variable reference)", test_file_string_integration);
    
    // 変数参照修正の検証テスト
    const std::string test_file_variable_fix = "../../tests/cases/ternary/test_variable_reference_fix.cb";
    run_cb_test_with_output_and_time_auto(test_file_variable_fix, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for variable reference fix test");
            
            // テストヘッダーとフッター
            INTEGRATION_ASSERT_CONTAINS(output, "=== Variable Reference and Function Call in Printf Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test Complete ===", "Expected test complete message");
            
            // 変数参照の三項演算子テスト
            INTEGRATION_ASSERT_CONTAINS(output, "Variable reference test: success", "Expected variable reference test: success");
            
            // printf内での関数呼び出しテスト
            INTEGRATION_ASSERT_CONTAINS(output, "Function call in printf: excellent", "Expected Function call in printf: excellent");
            INTEGRATION_ASSERT_CONTAINS(output, "Function call in printf: good", "Expected Function call in printf: good");
            INTEGRATION_ASSERT_CONTAINS(output, "Function call in printf: poor", "Expected Function call in printf: poor");
            
            // 複雑なケース：変数参照 + 関数呼び出し
            INTEGRATION_ASSERT_CONTAINS(output, "Combined test (variable + condition): result", "Expected Combined test (variable + condition): result");
            
            // ネストした変数参照テスト
            INTEGRATION_ASSERT_CONTAINS(output, "Nested variable reference: high", "Expected Nested variable reference: high");
        });
    integration_test_passed_with_time_auto("variable reference fix verification test", test_file_variable_fix);
    
    std::cout << "[integration] Ternary tests completed" << std::endl;
}
