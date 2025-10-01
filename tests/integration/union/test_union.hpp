#pragma once
#include "../framework/integration_test_framework.hpp"
#include <string>
#include <iostream>
#include <chrono>

namespace UnionTests {

// 基本リテラル値ユニオンテスト
inline void test_literal_union() {
    std::cout << "[integration] Running test_literal_union..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/literal_union.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Literal union test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 基本リテラル値ユニオンテスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "OK: 200", "Should show HTTP OK status");
            INTEGRATION_ASSERT_CONTAINS(output, "Not Found: 404", "Should show HTTP Not Found status");
            INTEGRATION_ASSERT_CONTAINS(output, "Server Error: 500", "Should show HTTP Server Error status");
            INTEGRATION_ASSERT_CONTAINS(output, "Enabled: 1", "Should show boolean true as 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Disabled: 0", "Should show boolean false as 0");
            INTEGRATION_ASSERT_CONTAINS(output, "Excellent: A", "Should show grade A");
            INTEGRATION_ASSERT_CONTAINS(output, "Debug: DEBUG", "Should show debug log level");
            INTEGRATION_ASSERT_CONTAINS(output, "Error: ERROR", "Should show error log level");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 基本リテラル値ユニオンテスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_literal_union", "literal_union.cb");
}

// 基本型ユニオンテスト
inline void test_type_union() {
    std::cout << "[integration] Running test_type_union..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/type_union.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Type union test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 基本型ユニオンテスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Small Number (int): 42", "Should show int type assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Large Number (long): 1000000", "Should show long type assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Integer Value: 123", "Should show primitive int");
            INTEGRATION_ASSERT_CONTAINS(output, "Boolean Value: 1", "Should show primitive bool as 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Character Value: X", "Should show primitive char");
            INTEGRATION_ASSERT_CONTAINS(output, "Text Value: Hello World", "Should show string value");
            INTEGRATION_ASSERT_CONTAINS(output, "Number Value: 999", "Should show number value");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 基本型ユニオンテスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_type_union", "type_union.cb");
}

// カスタム型ユニオンテスト
inline void test_custom_union() {
    std::cout << "[integration] Running test_custom_union..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/custom_union.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Custom union test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== カスタム型ユニオンテスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt: 777", "Should show custom int type");
            INTEGRATION_ASSERT_CONTAINS(output, "MyString: Custom Text", "Should show custom string type");
            INTEGRATION_ASSERT_CONTAINS(output, "MyBool: 1", "Should show custom bool type");
            INTEGRATION_ASSERT_CONTAINS(output, "Union1 (MyInt): 777", "Should show union assignment from custom int");
            INTEGRATION_ASSERT_CONTAINS(output, "Union2 (MyString): Custom Text", "Should show union assignment from custom string");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed1 (MyInt): 777", "Should show mixed union assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed2 (MyBool): 1", "Should show mixed union bool assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Direct1 (int literal): 888", "Should show direct literal assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Direct2 (bool literal): 0", "Should show direct bool literal assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "=== カスタム型ユニオンテスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_custom_union", "custom_union.cb");
}

// 構造体型ユニオンテスト
inline void test_struct_union() {
    std::cout << "[integration] Running test_struct_union..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/struct_union.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct union test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 構造体型ユニオンテスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Origin: (0, 0)", "Should show origin point");
            INTEGRATION_ASSERT_CONTAINS(output, "Target: (10, 20)", "Should show target point");
            INTEGRATION_ASSERT_CONTAINS(output, "Alice: Alice, 25 years old", "Should show Alice person");
            INTEGRATION_ASSERT_CONTAINS(output, "Bob: Bob, 30 years old", "Should show Bob person");
            INTEGRATION_ASSERT_CONTAINS(output, "Item1 (Point): assigned successfully", "Should show successful point assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Item2 (Person): assigned successfully", "Should show successful person assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Geo1 (Point): assigned successfully", "Should show geometry assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 構造体型ユニオンテスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_struct_union", "struct_union.cb");
}

// 配列型ユニオンテスト
inline void test_array_union() {
    std::cout << "[integration] Running test_array_union..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/array_union.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Array union test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 配列型ユニオンテスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Numbers: [1, 2, 3]", "Should show number array");
            INTEGRATION_ASSERT_CONTAINS(output, "Flags: [1, 0]", "Should show boolean array");
            INTEGRATION_ASSERT_CONTAINS(output, "Sequence: [10, 20, 30, 40, 50]", "Should show sequence array");
            INTEGRATION_ASSERT_CONTAINS(output, "Array1 (int[3]): assigned successfully", "Should show int array assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Array2 (bool[2]): assigned successfully", "Should show bool array assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "coords[0]: 100", "Should show array element access");
            INTEGRATION_ASSERT_CONTAINS(output, "coords[1]: 200", "Should show array element access");
            INTEGRATION_ASSERT_CONTAINS(output, "coords[2]: 300", "Should show array element access");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 配列型ユニオンテスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_array_union", "array_union.cb");
}

// 混合複合型ユニオンテスト
inline void test_mixed_union() {
    std::cout << "[integration] Running test_mixed_union..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/mixed_union.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Mixed union test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 混合複合型ユニオンテスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "UserId: 12345", "Should show user id");
            INTEGRATION_ASSERT_CONTAINS(output, "UserName: JohnDoe", "Should show user name");
            INTEGRATION_ASSERT_CONTAINS(output, "Rectangle: 800x600", "Should show rectangle dimensions");
            INTEGRATION_ASSERT_CONTAINS(output, "Coords: [25, 75]", "Should show coordinate array");
            INTEGRATION_ASSERT_CONTAINS(output, "Settings: [1, 0, 1]", "Should show settings array");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed1 (404 literal): 404", "Should show literal assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed2 (error literal): error", "Should show string literal assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed3 (true literal): 1", "Should show bool literal assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed4 (int type): 999", "Should show int type assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed5 (UserId): 12345", "Should show custom type assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed6 (Rectangle): assigned successfully", "Should show struct assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Mixed7 (int[2]): assigned successfully", "Should show array assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 混合複合型ユニオンテスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_mixed_union", "mixed_union.cb");
}

// エラーケーステスト - 許可されていないリテラル値
inline void test_error_invalid_literal() {
    std::cout << "[integration] Running test_error_invalid_literal..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/error_invalid_literal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Invalid literal test should fail with non-zero exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid assignments:", "Should show valid assignments before error");
            INTEGRATION_ASSERT_CONTAINS(output, "StatusCode: 200", "Should show valid status code");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade: A", "Should show valid grade");
            // エラーメッセージの確認
            INTEGRATION_ASSERT(output.find("not allowed for union") != std::string::npos || 
                              output.find("Value 999 is not allowed") != std::string::npos,
                              "Should contain union type error message");
        });
    integration_test_passed_with_time_auto("test_error_invalid_literal", "error_invalid_literal.cb (expected error)");
}

// エラーケーステスト - 型不一致
inline void test_error_type_mismatch() {
    std::cout << "[integration] Running test_error_type_mismatch..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/error_type_mismatch.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Type mismatch test should fail with non-zero exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid assignments:", "Should show valid assignments before error");
            INTEGRATION_ASSERT_CONTAINS(output, "IntOnly: 123", "Should show valid int assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "StringOnly: hello", "Should show valid string assignment");
            // エラーメッセージの確認
            INTEGRATION_ASSERT(output.find("not allowed for union") != std::string::npos || 
                              output.find("Type mismatch") != std::string::npos,
                              "Should contain type mismatch error message");
        });
    integration_test_passed_with_time_auto("test_error_type_mismatch", "error_type_mismatch.cb (expected error)");
}

// エラーケーステスト - 未定義ユニオン型
inline void test_error_undefined_type() {
    std::cout << "[integration] Running test_error_undefined_type..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/error_undefined_type.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Undefined type test should fail with non-zero exit code");
            // パースエラーまたはコンパイルエラーが発生するはず
            INTEGRATION_ASSERT(output.find("UndefinedUnion") != std::string::npos || 
                              output.find("not found") != std::string::npos ||
                              output.find("undefined") != std::string::npos,
                              "Should contain undefined type error message");
        });
    integration_test_passed_with_time_auto("test_error_undefined_type", "error_undefined_type.cb (expected error)");
}

// エラーケーステスト - カスタム型エラー
inline void test_error_custom_type() {
    std::cout << "[integration] Running test_error_custom_type..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/error_custom_type.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Custom type error test should fail with non-zero exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid assignments:", "Should show valid assignments before error");
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt: 777", "Should show valid custom int");
            INTEGRATION_ASSERT_CONTAINS(output, "MyString: test", "Should show valid custom string");
            // エラーメッセージの確認
            INTEGRATION_ASSERT(output.find("not allowed for union") != std::string::npos || 
                              output.find("custom type") != std::string::npos,
                              "Should contain custom type error message");
        });
    integration_test_passed_with_time_auto("test_error_custom_type", "error_custom_type.cb (expected error)");
}

// エラーケーステスト - 構造体型エラー
inline void test_error_struct_type() {
    std::cout << "[integration] Running test_error_struct_type..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/error_struct_type.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Struct type error test should fail with non-zero exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid assignments:", "Should show valid assignments before error");
            INTEGRATION_ASSERT_CONTAINS(output, "Point: (10, 20)", "Should show valid point");
            INTEGRATION_ASSERT_CONTAINS(output, "Person: Alice, 25", "Should show valid person");
            // エラーメッセージの確認
            INTEGRATION_ASSERT(output.find("not allowed for union") != std::string::npos || 
                              output.find("struct type") != std::string::npos,
                              "Should contain struct type error message");
        });
    integration_test_passed_with_time_auto("test_error_struct_type", "error_struct_type.cb (expected error)");
}

// エラーケーステスト - 配列型エラー
inline void test_error_array_type() {
    std::cout << "[integration] Running test_error_array_type..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/error_array_type.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Array type error test should fail with non-zero exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid assignments:", "Should show valid assignments before error");
            INTEGRATION_ASSERT_CONTAINS(output, "int[3]: [1, 2, 3]", "Should show valid int array");
            INTEGRATION_ASSERT_CONTAINS(output, "bool[2]: [1, 0]", "Should show valid bool array");
            // エラーメッセージの確認
            INTEGRATION_ASSERT(output.find("not allowed for union") != std::string::npos || 
                              output.find("array type") != std::string::npos,
                              "Should contain array type error message");
        });
    integration_test_passed_with_time_auto("test_error_array_type", "error_array_type.cb (expected error)");
}

// エラーケーステスト - 複数エラー組み合わせ
inline void test_error_multiple() {
    std::cout << "[integration] Running test_error_multiple..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/error_multiple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Multiple error test should fail with non-zero exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "Valid assignments:", "Should show valid assignments before error");
            INTEGRATION_ASSERT_CONTAINS(output, "ComplexUnion (literal): 200", "Should show valid literal assignment");
            // エラーメッセージの確認
            INTEGRATION_ASSERT(output.find("not allowed for union") != std::string::npos || 
                              output.find("404") != std::string::npos,
                              "Should contain error message about invalid literal 404");
        });
    integration_test_passed_with_time_auto("test_error_multiple", "error_multiple.cb (expected error)");
}

// Union型文字列処理テスト（新規追加）
inline void test_string_processing() {
    std::cout << "[integration] Running test_string_processing..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/string_processing.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "String processing test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Union型文字列処理テスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "String value: Hello World", "Should show string value assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Status: success", "Should show literal type string assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Numeric value: 42", "Should show numeric value after string");
            INTEGRATION_ASSERT_CONTAINS(output, "Back to string: Converted back", "Should show string reassignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Status comparison test:", "Should show comparison section");
            INTEGRATION_ASSERT_CONTAINS(output, "status1 (success) == status2 (error): not equal", "Should show comparison result");
            INTEGRATION_ASSERT_CONTAINS(output, "status1 (success) == status2 (success): equal", "Should show equal comparison result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== テスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_string_processing", "string_processing.cb");
}

// 構造体Union型複合代入テスト（既存テストの確認）
inline void test_struct_union_compound_assignment() {
    std::cout << "[integration] Running test_struct_union_compound_assignment..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/struct_union_compound_assignment.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct union compound assignment test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== 構造体メンバーUnion型複合代入テスト ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Before compound assignment ===", "Should show before state");
            INTEGRATION_ASSERT_CONTAINS(output, "code: 200, value: 5, count: 10", "Should show initial values");
            INTEGRATION_ASSERT_CONTAINS(output, "=== After compound assignment ===", "Should show after state");
            INTEGRATION_ASSERT_CONTAINS(output, "code: 200, value: 15, count: 20", "Should show modified values after compound assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "=== After string assignment ===", "Should show string assignment section");
            INTEGRATION_ASSERT_CONTAINS(output, "code: 200, value: Hello, count: 20", "Should show string value");
            INTEGRATION_ASSERT_CONTAINS(output, "=== After numeric reassignment and compound assignment ===", "Should show final section");
            INTEGRATION_ASSERT_CONTAINS(output, "code: 200, value: 10, count: 20", "Should show final values after compound assignment");
        });
    integration_test_passed_with_time_auto("test_struct_union_compound_assignment", "struct_union_compound_assignment.cb");
}

// 包括的統合テスト
inline void test_comprehensive() {
    std::cout << "[integration] Running test_comprehensive..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/union/comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Comprehensive test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "=== ユニオン型包括的統合テスト ===", "Should contain test header");
            
            // Section 1: リテラル値ユニオン
            INTEGRATION_ASSERT_CONTAINS(output, "1. Literal Value Unions:", "Should contain section 1 header");
            INTEGRATION_ASSERT_CONTAINS(output, "StatusCode: 200", "Should show status code");
            INTEGRATION_ASSERT_CONTAINS(output, "Flag: 1", "Should show flag");
            INTEGRATION_ASSERT_CONTAINS(output, "Grade: A", "Should show grade");
            INTEGRATION_ASSERT_CONTAINS(output, "LogLevel: INFO", "Should show log level");
            
            // Section 2: 基本型ユニオン
            INTEGRATION_ASSERT_CONTAINS(output, "2. Basic Type Unions:", "Should contain section 2 header");
            INTEGRATION_ASSERT_CONTAINS(output, "NumericType (int): 42", "Should show numeric type int");
            INTEGRATION_ASSERT_CONTAINS(output, "NumericType (long): 1000000", "Should show numeric type long");
            INTEGRATION_ASSERT_CONTAINS(output, "PrimitiveType (int): 123", "Should show primitive int");
            INTEGRATION_ASSERT_CONTAINS(output, "PrimitiveType (bool): 1", "Should show primitive bool");
            INTEGRATION_ASSERT_CONTAINS(output, "PrimitiveType (char): X", "Should show primitive char");
            
            // Section 3: カスタム型ユニオン
            INTEGRATION_ASSERT_CONTAINS(output, "3. Custom Type Unions:", "Should contain section 3 header");
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt: 777", "Should show custom int");
            INTEGRATION_ASSERT_CONTAINS(output, "MyString: CustomText", "Should show custom string");
            INTEGRATION_ASSERT_CONTAINS(output, "CustomUnion (MyInt): 777", "Should show custom union int");
            INTEGRATION_ASSERT_CONTAINS(output, "CustomUnion (MyString): CustomText", "Should show custom union string");
            
            // Section 4: 構造体型ユニオン
            INTEGRATION_ASSERT_CONTAINS(output, "4. Struct Type Unions:", "Should contain section 4 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Point origin: (0, 0)", "Should show point origin");
            INTEGRATION_ASSERT_CONTAINS(output, "Point target: (10, 20)", "Should show point target");
            INTEGRATION_ASSERT_CONTAINS(output, "Person Alice: Alice, 25", "Should show person Alice");
            INTEGRATION_ASSERT_CONTAINS(output, "Person Bob: Bob, 30", "Should show person Bob");
            
            // Section 5: 配列型ユニオン
            INTEGRATION_ASSERT_CONTAINS(output, "5. Array Type Unions:", "Should contain section 5 header");
            INTEGRATION_ASSERT_CONTAINS(output, "int[3]: [1, 2, 3]", "Should show int array");
            INTEGRATION_ASSERT_CONTAINS(output, "bool[2]: [1, 0]", "Should show bool array");
            
            // Section 6: 混合複合型ユニオン
            INTEGRATION_ASSERT_CONTAINS(output, "6. Mixed Complex Unions:", "Should contain section 6 header");
            INTEGRATION_ASSERT_CONTAINS(output, "MegaUnion (999): 999", "Should show mega union literal");
            INTEGRATION_ASSERT_CONTAINS(output, "MegaUnion (special): special", "Should show mega union string");
            INTEGRATION_ASSERT_CONTAINS(output, "MegaUnion (true): 1", "Should show mega union bool");
            INTEGRATION_ASSERT_CONTAINS(output, "Regular int variable: 888", "Should show regular int variable");
            INTEGRATION_ASSERT_CONTAINS(output, "MegaUnion (MyInt): 777", "Should show mega union custom");
            INTEGRATION_ASSERT_CONTAINS(output, "MegaUnion (Point): assigned", "Should show mega union point assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "MegaUnion (int[2]): assigned", "Should show mega union array assignment");
            
            // Section 7: 動的代入テスト
            INTEGRATION_ASSERT_CONTAINS(output, "7. Dynamic Assignment Test:", "Should contain section 7 header");
            INTEGRATION_ASSERT_CONTAINS(output, "Updated StatusCode: 404", "Should show updated status code");
            INTEGRATION_ASSERT_CONTAINS(output, "Updated Flag: 0", "Should show updated flag");
            INTEGRATION_ASSERT_CONTAINS(output, "Updated Grade: C", "Should show updated grade");
            INTEGRATION_ASSERT_CONTAINS(output, "Updated LogLevel: ERROR", "Should show updated log level");
            
            // Section 8: 複雑な構造体操作
            INTEGRATION_ASSERT_CONTAINS(output, "8. Complex Struct Operations:", "Should contain section 8 header");
            INTEGRATION_ASSERT_CONTAINS(output, "EntityUnion reassignment: completed", "Should show entity union reassignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Charlie: Charlie, 35", "Should show Charlie person");
            INTEGRATION_ASSERT_CONTAINS(output, "Center: (25, 25)", "Should show center point");
            
            // テスト結果サマリ
            INTEGRATION_ASSERT_CONTAINS(output, "=== 統合テスト結果サマリ ===", "Should contain test summary");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ リテラル値ユニオン: 4/4 成功", "Should show literal union success");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ 基本型ユニオン: 5/5 成功", "Should show basic type union success");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ カスタム型ユニオン: 4/4 成功", "Should show custom type union success");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ 構造体型ユニオン: 6/6 成功", "Should show struct type union success");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ 配列型ユニオン: 4/4 成功", "Should show array type union success");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ 混合複合型ユニオン: 7/7 成功", "Should show mixed union success");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ 動的代入テスト: 4/4 成功", "Should show dynamic assignment success");
            INTEGRATION_ASSERT_CONTAINS(output, "✓ 複雑な構造体操作: 4/4 成功", "Should show complex struct operations success");
            INTEGRATION_ASSERT_CONTAINS(output, "🎉 全38項目のユニオン型機能テスト成功！", "Should show final success message");
            INTEGRATION_ASSERT_CONTAINS(output, "=== ユニオン型包括的統合テスト完了 ===", "Should contain test completion");
        });
    integration_test_passed_with_time_auto("test_comprehensive", "comprehensive.cb");
}

// 全てのユニオンテストを実行
inline void run_all_union_tests() {
    std::cout << "\n=== Running Union Type Integration Tests ===" << std::endl;
    
    test_literal_union();
    test_type_union();
    test_custom_union();
    test_struct_union();
    test_array_union();
    test_mixed_union();
    test_string_processing(); // 新規追加：文字列処理テスト
    test_struct_union_compound_assignment(); // 新規追加：複合代入テスト
    test_error_invalid_literal();
    test_error_type_mismatch();
    test_error_undefined_type();
    test_error_custom_type();
    test_error_struct_type();
    test_error_array_type();
    test_error_multiple();
    test_comprehensive();
    
    std::cout << "=== Union Type Integration Tests Complete ===" << std::endl;
}

} // namespace UnionTests
