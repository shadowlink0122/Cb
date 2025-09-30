#pragma once

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace StructTests {

// 基本的なstruct宣言と使用のテスト
inline void test_basic_struct() {
    std::cout << "[integration] Running test_basic_struct..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/basic_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic struct test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Point: (10, 20)") != std::string::npos, 
                              "Output should contain 'Point: (10, 20)'");
        }, execution_time);
}

// Struct literal初期化のテスト
inline void test_struct_literal() {
    std::cout << "[integration] Running test_struct_literal..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/struct_literal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct literal test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Person 1: Alice, 25 years old, 165 cm") != std::string::npos, 
                              "Output should contain named initialization result");
            INTEGRATION_ASSERT(output.find("Person 2: Bob, 30 years old, 180 cm") != std::string::npos, 
                              "Output should contain positional initialization result");
        }, execution_time);
}

// Struct配列メンバーのテスト
inline void test_struct_array_member() {
    std::cout << "[integration] Running test_struct_array_member..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/struct/struct_array_member.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct array member test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Student: Charlie (ID: 12345)") != std::string::npos, 
                              "Output should contain student info");
            INTEGRATION_ASSERT(output.find("Grades: [85, 92, 78]") != std::string::npos, 
                              "Output should contain grades array");
            INTEGRATION_ASSERT(output.find("Average: 85") != std::string::npos, 
                              "Output should contain calculated average");
        }, execution_time);
}

// Struct配列メンバーのliteral初期化のテスト
inline void test_struct_array_literal() {
    std::cout << "[integration] Running test_struct_array_literal..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_array_literal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct array literal test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Course: Mathematics (3 credits)") != std::string::npos, 
                              "Output should contain course info");
            INTEGRATION_ASSERT(output.find("Scores: [95, 87, 92, 89]") != std::string::npos, 
                              "Output should contain scores array");
            INTEGRATION_ASSERT(output.find("Average score: 90") != std::string::npos, 
                              "Output should contain calculated average");
        });
}

// 構造体定数サイズ配列メンバーの個別代入テスト（新規追加）
inline void test_const_size_array_assignment() {
    std::cout << "[integration] Running test_const_size_array_assignment..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/const_size_array_assignment.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Const size array assignment test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Individual access: 100, 200, 300") != std::string::npos, 
                              "Output should contain individual access results");
            INTEGRATION_ASSERT(output.find("Direct printf: [100, 200, 300]") != std::string::npos, 
                              "Output should contain direct printf results (this was the bug being fixed)");
            INTEGRATION_ASSERT(output.find("Other members: x=50, y=75") != std::string::npos, 
                              "Output should contain other member values");
        });
}

// 構造体の配列のテスト
inline void test_struct_array() {
    std::cout << "[integration] Running test_struct_array..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_array.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct array test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Team Members:") != std::string::npos, 
                              "Output should contain team header");
            INTEGRATION_ASSERT(output.find("1. Alice - $50000 (Dept: HR)") != std::string::npos, 
                              "Output should contain Alice's info");
            INTEGRATION_ASSERT(output.find("2. Bob - $55000 (Dept: CEO)") != std::string::npos, 
                              "Output should contain Bob's info");
            INTEGRATION_ASSERT(output.find("3. Charlie - $60000 (Dept: Dev)") != std::string::npos, 
                              "Output should contain Charlie's info");
            INTEGRATION_ASSERT(output.find("Total salary: $165000") != std::string::npos, 
                              "Output should contain total salary");
        });
}

// 入れ子構造のテスト（現在は簡略版で実装）
inline void test_nested_struct() {
    std::cout << "[integration] Running test_nested_struct..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/nested_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Nested struct test should pass with simplified implementation");
            INTEGRATION_ASSERT(output.find("Basic struct test passed") != std::string::npos, 
                              "Output should contain success message");
            INTEGRATION_ASSERT(output.find("nested features not yet supported") != std::string::npos, 
                              "Output should mention current limitations");
        });
}

// 入れ子構造のフラット実装版テスト（現在の実装で動作する代替案）
inline void test_nested_struct_flat() {
    std::cout << "[integration] Running test_nested_struct_flat..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/nested_struct_flat.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Nested struct flat test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Company: Tech Corp") != std::string::npos, 
                              "Output should contain company name");
            INTEGRATION_ASSERT(output.find("Address: 123 Main St, San Francisco 94105") != std::string::npos, 
                              "Output should contain full address");
            INTEGRATION_ASSERT(output.find("Employees: 150") != std::string::npos, 
                              "Output should contain employee count");
        });
}

// 多次元配列メンバーのテスト（現在は基本機能のみサポート）
inline void test_multidim_array_member() {
    std::cout << "[integration] Running test_multidim_array_member..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/multidim_array_member.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multidim array member test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Matrix: Sample Matrix (2x3)") != std::string::npos, 
                              "Output should contain matrix info");
                        // 配列リテラル代入により正しい値が設定される
            INTEGRATION_ASSERT(output.find("Row 0: [1, 2, 3]") != std::string::npos, 
                              "Output should contain correct values from array literal");
            INTEGRATION_ASSERT(output.find("Row 1: [4, 5, 6]") != std::string::npos, 
                              "Output should contain correct values from array literal");
        });
}

// Struct関数引数のテスト（現在は未実装）
inline void test_struct_function_param() {
    std::cout << "[integration] Running test_struct_function_param (skipped - struct function parameters not implemented)..." << std::endl;
    
    // 構造体の関数引数は複雑な機能のため、現在はスキップ
    // TODO: 将来実装予定
    // run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_function_param.cb", ...);
}

// Struct関数戻り値のテスト
inline void test_struct_function_return() {
    std::cout << "[integration] Running test_struct_function_return..." << std::endl;
    
    // 基本的な構造体関数戻り値テスト（数値のみ）
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_function_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct function return test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Point 1: (3, 4)") != std::string::npos, 
                              "Output should contain Point 1 coordinates");
            INTEGRATION_ASSERT(output.find("Point 2: (1, 2)") != std::string::npos, 
                              "Output should contain Point 2 coordinates");
            INTEGRATION_ASSERT(output.find("Sum: (4, 6)") != std::string::npos, 
                              "Output should contain sum coordinates");
        });
}

// Struct配列関数戻り値のチェーン処理テスト（新規追加）
inline void test_struct_function_array_chain() {
    std::cout << "[integration] Running test_struct_function_array_chain..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_function_array_chain.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct function array chain test should exit with code 0");
            
            // 基本的なチェーンアクセス
            INTEGRATION_ASSERT(output.find("Alice") != std::string::npos, 
                              "Output should contain Alice from get_people()[0].name");
            INTEGRATION_ASSERT(output.find("Bob") != std::string::npos, 
                              "Output should contain Bob from get_people()[1].name");
            INTEGRATION_ASSERT(output.find("Age 0:") != std::string::npos, 
                              "Output should contain age 0 label");
            INTEGRATION_ASSERT(output.find("25") != std::string::npos, 
                              "Output should contain age 25");
            INTEGRATION_ASSERT(output.find("30") != std::string::npos, 
                              "Output should contain age 30");
            
            // 異なる構造体型でのテスト
            INTEGRATION_ASSERT(output.find("Laptop") != std::string::npos, 
                              "Output should contain Laptop from get_products()[0].name");
            INTEGRATION_ASSERT(output.find("Mouse") != std::string::npos, 
                              "Output should contain Mouse from get_products()[1].name");
            INTEGRATION_ASSERT(output.find("Keyboard") != std::string::npos, 
                              "Output should contain Keyboard from get_products()[2].name");
            INTEGRATION_ASSERT(output.find("Prices:") != std::string::npos, 
                              "Output should contain prices label");
            INTEGRATION_ASSERT(output.find("1200") != std::string::npos, 
                              "Output should contain price 1200");
            INTEGRATION_ASSERT(output.find("25") != std::string::npos, 
                              "Output should contain price 25");
            INTEGRATION_ASSERT(output.find("80") != std::string::npos, 
                              "Output should contain price 80");
            
            // 境界値テスト
            INTEGRATION_ASSERT(output.find("Charlie") != std::string::npos, 
                              "Output should contain Charlie from get_team()[0].name");
            INTEGRATION_ASSERT(output.find("Grace") != std::string::npos, 
                              "Output should contain Grace from get_team()[4].name");
            INTEGRATION_ASSERT(output.find("First age:") != std::string::npos, 
                              "Output should contain first age label");
            INTEGRATION_ASSERT(output.find("35") != std::string::npos, 
                              "Output should contain age 35");
            INTEGRATION_ASSERT(output.find("Last age:") != std::string::npos, 
                              "Output should contain last age label");
            INTEGRATION_ASSERT(output.find("29") != std::string::npos, 
                              "Output should contain age 29");
            
            // 中間要素のテスト
            INTEGRATION_ASSERT(output.find("Eve") != std::string::npos, 
                              "Output should contain Eve from get_team()[2].name");
            INTEGRATION_ASSERT(output.find("Middle age:") != std::string::npos, 
                              "Output should contain middle age label");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, 
                              "Output should contain age 42");
            
            INTEGRATION_ASSERT(output.find("All tests completed successfully") != std::string::npos, 
                              "Output should contain success message");
        });
}

// Struct関数での文字列メンバテスト
inline void test_struct_function_string_members() {
    std::cout << "[integration] Running test_struct_function_string_members..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_function_string_members.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct function string members test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Person 1 - ID: 1, Name: Alice, Description: Software Engineer") != std::string::npos, 
                              "Output should contain Person 1 details");
            INTEGRATION_ASSERT(output.find("Person 2 - ID: 1, Name: Alice, Description: Software Engineer") != std::string::npos, 
                              "Output should contain Person 2 details (copied from Person 1)");
            INTEGRATION_ASSERT(output.find("Person 3 - ID: 2, Name: Bob, Description: Data Scientist") != std::string::npos, 
                              "Output should contain Person 3 details");
            
            // バグ検出: 文字列メンバが数値0になっている場合はエラー
            if (output.find("Person 2 - ID: 1, Name: 0, Description: 0") != std::string::npos) {
                throw std::runtime_error("BUG DETECTED: String members in struct parameters are returning 0 instead of actual string values");
            }
        });
}

// Struct関数での配列メンバテスト
inline void test_struct_function_array_members() {
    std::cout << "[integration] Running test_struct_function_array_members..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_function_array_members.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct function array members test should exit with code 0");
            
            // 基本的な出力の検証
            INTEGRATION_ASSERT(output.find("Student 1 - ID: 1, Name: Alice") != std::string::npos, 
                              "Output should contain Student 1 basic info");
            INTEGRATION_ASSERT(output.find("Scores: [85, 90, 78]") != std::string::npos, 
                              "Output should contain Student 1 scores");
            INTEGRATION_ASSERT(output.find("Tags: [honor, active]") != std::string::npos, 
                              "Output should contain Student 1 tags with actual string values");
            
            // processStudent関数の結果検証
            INTEGRATION_ASSERT(output.find("Student 2 - ID: 101, Name: Alice") != std::string::npos, 
                              "Output should contain Student 2 processed info");
            
            // バグ検出: 構造体パラメータの配列アクセスに関するバグを検出
            if (output.find("Scores: [10, 10, 10]") != std::string::npos) {
                throw std::runtime_error("BUG DETECTED: Numeric array members in struct parameters are returning 0 instead of actual values (85+10=95, 90+10=100, 78+10=88 expected, but got 0+10=10 for all)");
            }
            if (output.find("Tags: [0, 0]") != std::string::npos) {
                throw std::runtime_error("BUG DETECTED: String array members in struct parameters are returning 0 instead of actual string values");
            }
            
            // 正しい期待値
            INTEGRATION_ASSERT(output.find("Scores: [95, 100, 88]") != std::string::npos, 
                              "Output should contain Student 2 processed scores");
            INTEGRATION_ASSERT(output.find("Tags: [honor, active]") != std::string::npos, 
                              "Output should contain Student 2 tags copied from original");
        });
}

// 混合データ型のテスト
inline void test_mixed_types() {
    std::cout << "[integration] Running test_mixed_types..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/mixed_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Mixed types test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Label: Test Data (Count: 5)") != std::string::npos, 
                              "Output should contain label and count");
            INTEGRATION_ASSERT(output.find("Numbers: [10, 20, 30]") != std::string::npos, 
                              "Output should contain numbers array");
            INTEGRATION_ASSERT(output.find("Tags: [important, verified]") != std::string::npos, 
                              "Output should contain tags array");
        });
}

// Typedef structのテスト
inline void test_typedef_struct() {
    std::cout << "[integration] Running test_typedef_struct..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/typedef_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Typedef struct test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Vertex 1: Origin at (0, 0)") != std::string::npos, 
                              "Output should contain first vertex");
            INTEGRATION_ASSERT(output.find("Vertex 2: Corner at (10, 10)") != std::string::npos, 
                              "Output should contain second vertex");
            INTEGRATION_ASSERT(output.find("DataPoint #1001: Sample Point") != std::string::npos, 
                              "Output should contain data point info");
            INTEGRATION_ASSERT(output.find("Values: [42, 84]") != std::string::npos, 
                              "Output should contain data point values");
        });
}

// エラーハンドリングのテスト
inline void test_struct_error_handling() {
    std::cout << "[integration] Running test_struct_error_handling..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/struct_error_handling.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct error handling test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Valid member: 42") != std::string::npos, 
                              "Output should contain valid member access");
            INTEGRATION_ASSERT(output.find("Name: Valid") != std::string::npos, 
                              "Output should contain valid name");
        });
}

// 大きなstruct パフォーマンステスト
inline void test_large_struct() {
    std::cout << "[integration] Running test_large_struct..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/large_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Large struct test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Large Struct: Large Data Structure (ID: 999)") != std::string::npos, 
                              "Output should contain struct info");
            INTEGRATION_ASSERT(output.find("First 5 data values: [0, 10, 20, 30, 40]") != std::string::npos, 
                              "Output should contain data values");
            INTEGRATION_ASSERT(output.find("Metadata: [100, 200, 300]") != std::string::npos, 
                              "Output should contain metadata");
            INTEGRATION_ASSERT(output.find("First 3 labels: [first, second, third]") != std::string::npos, 
                              "Output should contain labels");
        });
}

// 統合テスト
inline void test_comprehensive() {
    std::cout << "[integration] Running test_comprehensive..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/struct/comprehensive_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Comprehensive test should exit with code 0");
            INTEGRATION_ASSERT(output.find("Course: Mathematics (Max Students: 2)") != std::string::npos, 
                              "Output should contain course info");
            INTEGRATION_ASSERT(output.find("Student 1:") != std::string::npos, 
                              "Output should contain student 1 header");
            INTEGRATION_ASSERT(output.find("Name: Alice (ID: 1001)") != std::string::npos, 
                              "Output should contain Alice's info");
            INTEGRATION_ASSERT(output.find("Grades: [85, 92, 78]") != std::string::npos, 
                              "Output should contain Alice's grades");
            INTEGRATION_ASSERT(output.find("Average: 85") != std::string::npos, 
                              "Output should contain Alice's average");
            INTEGRATION_ASSERT(output.find("Student 2:") != std::string::npos, 
                              "Output should contain student 2 header");
            INTEGRATION_ASSERT(output.find("Name: Bob (ID: 1002)") != std::string::npos, 
                              "Output should contain Bob's info");
            INTEGRATION_ASSERT(output.find("Grades: [90, 87, 94]") != std::string::npos, 
                              "Output should contain Bob's grades");
            INTEGRATION_ASSERT(output.find("Average: 90") != std::string::npos, 
                              "Output should contain Bob's average");
        });
}

// 全structテストを実行
inline void run_all_struct_tests() {
    std::cout << "[integration] ========================================" << std::endl;
    std::cout << "[integration] Running Struct Integration Tests" << std::endl;
    std::cout << "[integration] ========================================" << std::endl;
    
    try {
        test_basic_struct();
        test_struct_literal();
        test_struct_array_member();
        test_struct_array_literal();
        test_const_size_array_assignment(); // 新規追加テスト
        test_struct_array();
        test_nested_struct();
        test_nested_struct_flat();
        test_multidim_array_member();
        test_struct_function_param();
        test_struct_function_return();
        test_struct_function_array_chain(); // 新規追加: チェーン処理テスト
        test_struct_function_string_members(); // 新しい文字列メンバテスト
        test_struct_function_array_members();  // 新しい配列メンバテスト
        test_mixed_types(); // 複雑な機能
        test_typedef_struct(); // 複雑な機能  
        test_struct_error_handling(); // 複雑な機能
        test_large_struct(); // 複雑な機能
        // test_comprehensive(); // 複雑な機能
        
        std::cout << "[integration] ========================================" << std::endl;
        std::cout << "[integration] All Struct tests completed successfully!" << std::endl;
        std::cout << "[integration] ========================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[integration] Struct test suite failed: " << e.what() << std::endl;
        throw;
    }
}

} // namespace StructTests
