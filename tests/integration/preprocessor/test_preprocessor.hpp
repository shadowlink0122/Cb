#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_preprocessor() {
    std::cout << "[integration-test] Running Preprocessor Tests..." << std::endl;
    
    // Test 1: Simple define
    {
        double execution_time;
        run_cb_test_with_output_and_time("../cases/preprocessor/simple_define.cb",
            [](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "simple_define.cb should execute successfully");
                
                // 期待される出力を確認
                INTEGRATION_ASSERT_CONTAINS(output, "PI = 3.14159", "PI should be expanded to 3.14159");
                INTEGRATION_ASSERT_CONTAINS(output, "TRUE = 1", "TRUE should be expanded to 1");
                INTEGRATION_ASSERT_CONTAINS(output, "FALSE = 0", "FALSE should be expanded to 0");
                INTEGRATION_ASSERT_CONTAINS(output, "VERSION = 0.11.0", "VERSION should be expanded");
            }, execution_time);
        
        integration_test_passed_with_time("simple define macros", "simple_define.cb", execution_time);
    }
    
    // Test 2: Function macros
    {
        double execution_time;
        run_cb_test_with_output_and_time("../cases/preprocessor/function_macro.cb",
            [](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "function_macro.cb should execute successfully");
                
                // 関数マクロの展開を確認
                INTEGRATION_ASSERT_CONTAINS(output, "SQUARE(5) = 25", "SQUARE macro should expand correctly");
                INTEGRATION_ASSERT_CONTAINS(output, "MAX(5, 3) = 5", "MAX macro should return larger value");
                INTEGRATION_ASSERT_CONTAINS(output, "MIN(5, 3) = 3", "MIN macro should return smaller value");
                INTEGRATION_ASSERT_CONTAINS(output, "ABS(-7) = 7", "ABS macro should return absolute value");
            }, execution_time);
        
        integration_test_passed_with_time("function macros", "function_macro.cb", execution_time);
    }
    
    // Test 3: Nested macros
    {
        double execution_time;
        run_cb_test_with_output_and_time("../cases/preprocessor/nested_macro_demo.cb",
            [](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "nested_macro_demo.cb should execute successfully");
                
                // ネストしたマクロの展開を確認
                INTEGRATION_ASSERT_CONTAINS(output, "DOUBLE(5) = 10", "DOUBLE macro should work");
                INTEGRATION_ASSERT_CONTAINS(output, "QUAD(5) = 20", "QUAD (nested DOUBLE) macro should work");
            }, execution_time);
        
        integration_test_passed_with_time("nested macros", "nested_macro_demo.cb", execution_time);
    }
    
    // Test 4: String literal protection (Bug fix verification)
    {
        double execution_time;
        run_cb_test_with_output_and_time("../cases/preprocessor/string_literal_fix_demo.cb",
            [](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "string_literal_fix_demo.cb should execute successfully");
                
                // 文字列リテラル内の PI と MAX が展開されていないことを確認
                INTEGRATION_ASSERT_CONTAINS(output, "PI is a mathematical constant", 
                    "String literal should NOT have PI expanded");
                INTEGRATION_ASSERT_CONTAINS(output, "MAX value should not be replaced", 
                    "String literal should NOT have MAX expanded");
                
                // しかし、変数としては展開されていることを確認
                INTEGRATION_ASSERT_CONTAINS(output, "3.14159", "PI should be expanded outside string");
                INTEGRATION_ASSERT_CONTAINS(output, "100", "MAX should be expanded outside string");
            }, execution_time);
        
        integration_test_passed_with_time("string literal protection", "string_literal_fix_demo.cb", execution_time);
    }
    
    // Test 5: Comprehensive string literal protection demo
    {
        double execution_time;
        run_cb_test_with_output_and_time("../cases/preprocessor/string_literal_protection_demo.cb",
            [](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "string_literal_protection_demo.cb should execute successfully");
                
                // PI値の展開確認
                INTEGRATION_ASSERT_CONTAINS(output, "PI value: 3.14", "PI should be expanded in code");
                
                // 文字列内の PI が展開されていないことを確認
                INTEGRATION_ASSERT_CONTAINS(output, "The constant PI represents", 
                    "String 'PI represents' should NOT be expanded");
                INTEGRATION_ASSERT_CONTAINS(output, "MAX function can be used", 
                    "String 'MAX function' should NOT be expanded");
                
                // 関数マクロの展開確認
                INTEGRATION_ASSERT_CONTAINS(output, "MAX(10, 20) = 20", "MAX function macro should work");
                
                // 成功メッセージ
                INTEGRATION_ASSERT_CONTAINS(output, "Test passed: String literals are protected", 
                    "Success message should be present");
            }, execution_time);
        
        integration_test_passed_with_time("comprehensive string literal protection", 
            "string_literal_protection_demo.cb", execution_time);
    }
    
    // Test 6: Undef test
    {
        double execution_time;
        run_cb_test_with_output_and_time("../cases/preprocessor/undef_test.cb",
            [](const std::string& output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code, "undef_test.cb should execute successfully");
                
                // 最初のDEBUGが展開されていることを確認
                INTEGRATION_ASSERT_CONTAINS(output, "DEBUG mode: 1", "DEBUG should be 1 initially");
                
                // #undef後のメッセージ確認
                INTEGRATION_ASSERT_CONTAINS(output, "DEBUG is now undefined", 
                    "Undef message should be present");
                
                // 再定義後のDEBUGが2になっていることを確認
                INTEGRATION_ASSERT_CONTAINS(output, "DEBUG mode (redefined): 2", 
                    "DEBUG should be 2 after redefinition");
            }, execution_time);
        
        integration_test_passed_with_time("undef directive", "undef_test.cb", execution_time);
    }
    
    std::cout << "[integration-test] Preprocessor tests completed" << std::endl;
}
