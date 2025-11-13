#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_preprocessor() {
    std::cout << "[integration-test] Running preprocessor tests..." << std::endl;
    
    // Test 1: Basic #define with numeric values
    double execution_time;
    run_cb_test_with_output_and_time("../cases/preprocessor/define_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "define_basic.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("78.53975") != std::string::npos, 
                "Output should contain PI * RADIUS * RADIUS result");
        }, execution_time);
    integration_test_passed_with_time("basic #define", "define_basic.cb", execution_time);
    
    // Test 2: Simple numeric #define
    run_cb_test_with_output_and_time("../cases/preprocessor/define_number.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "define_number.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, 
                "Output should contain defined value");
        }, execution_time);
    integration_test_passed_with_time("numeric #define", "define_number.cb", execution_time);
    
    // Test 3: #ifdef when macro is defined
    run_cb_test_with_output_and_time("../cases/preprocessor/ifdef_true.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ifdef_true.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Debug enabled") != std::string::npos, 
                "Output should show debug message when DEBUG is defined");
        }, execution_time);
    integration_test_passed_with_time("#ifdef (true)", "ifdef_true.cb", execution_time);
    
    // Test 4: #ifdef when macro is not defined
    run_cb_test_with_output_and_time("../cases/preprocessor/ifdef_false.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ifdef_false.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("This should not appear") == std::string::npos, 
                "Output should not show message when macro is undefined");
        }, execution_time);
    integration_test_passed_with_time("#ifdef (false)", "ifdef_false.cb", execution_time);
    
    // Test 5: #ifndef when macro is not defined
    run_cb_test_with_output_and_time("../cases/preprocessor/ifndef_true.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ifndef_true.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Release mode") != std::string::npos, 
                "Output should show message when DEBUG is not defined");
        }, execution_time);
    integration_test_passed_with_time("#ifndef", "ifndef_true.cb", execution_time);
    
    // Test 6: #else branch
    run_cb_test_with_output_and_time("../cases/preprocessor/else_branch.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "else_branch.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Not in debug mode") != std::string::npos, 
                "Output should show else branch when DEBUG is not defined");
        }, execution_time);
    integration_test_passed_with_time("#else branch", "else_branch.cb", execution_time);
    
    // Test 7: #elseif branch
    run_cb_test_with_output_and_time("../cases/preprocessor/elseif_branch.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "elseif_branch.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Production mode") != std::string::npos, 
                "Output should show elseif branch when PRODUCTION is defined");
        }, execution_time);
    integration_test_passed_with_time("#elseif branch", "elseif_branch.cb", execution_time);
    
    // Test 8: Built-in __VERSION__ macro
    run_cb_test_with_output_and_time("../cases/preprocessor/builtin_version.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "builtin_version.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("0.13.0") != std::string::npos, 
                "Output should contain version number");
        }, execution_time);
    integration_test_passed_with_time("built-in __VERSION__", "builtin_version.cb", execution_time);
    
    // Test 9: String protection - macros should not expand inside strings
    run_cb_test_with_output_and_time("../cases/preprocessor/string_protection.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "string_protection.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("The value of PI is 3.14159") != std::string::npos, 
                "PI in string should not be replaced");
        }, execution_time);
    integration_test_passed_with_time("string protection", "string_protection.cb", execution_time);
    
    // Test 10: Identifier boundary - partial identifiers should not be replaced
    run_cb_test_with_output_and_time("../cases/preprocessor/identifier_boundary.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "identifier_boundary.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, 
                "MAXVALUE should not be affected by MAX macro");
        }, execution_time);
    integration_test_passed_with_time("identifier boundary", "identifier_boundary.cb", execution_time);
    
    // Test 11: Nested #ifdef
    run_cb_test_with_output_and_time("../cases/preprocessor/nested_ifdef.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "nested_ifdef.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Feature A enabled") != std::string::npos, 
                "Output should show Feature A");
            INTEGRATION_ASSERT(output.find("Feature B enabled") != std::string::npos, 
                "Output should show Feature B");
        }, execution_time);
    integration_test_passed_with_time("nested #ifdef", "nested_ifdef.cb", execution_time);
    
    // Test 12: Multiple defines in expressions
    run_cb_test_with_output_and_time("../cases/preprocessor/multiple_defines.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_defines.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("15") != std::string::npos, 
                "Output should contain sum of A + B");
        }, execution_time);
    integration_test_passed_with_time("multiple defines", "multiple_defines.cb", execution_time);
    
    // Test 13: Partial word matching should not replace
    run_cb_test_with_output_and_time("../cases/preprocessor/partial_match.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "partial_match.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, 
                "MAXIMUM should not be affected by MAX macro");
        }, execution_time);
    integration_test_passed_with_time("partial match protection", "partial_match.cb", execution_time);
    
    // Test 14: Underscore boundary test
    run_cb_test_with_output_and_time("../cases/preprocessor/underscore_boundary.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "underscore_boundary.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, 
                "VALUE_MAX should not be affected by VALUE macro");
        }, execution_time);
    integration_test_passed_with_time("underscore boundary", "underscore_boundary.cb", execution_time);
    
    // Test 15: Comments should not be affected by macros
    run_cb_test_with_output_and_time("../cases/preprocessor/comment_protection.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "comment_protection.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, 
                "Macros in comments should not affect code");
        }, execution_time);
    integration_test_passed_with_time("comment protection", "comment_protection.cb", execution_time);
    
    // Test 16: Redefining a macro
    run_cb_test_with_output_and_time("../cases/preprocessor/redefine_warn.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "redefine_warn.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("20") != std::string::npos, 
                "Last definition should win");
        }, execution_time);
    integration_test_passed_with_time("macro redefinition", "redefine_warn.cb", execution_time);
    
    // Test 17: #undef should remove macro definition
    run_cb_test_with_output_and_time("../cases/preprocessor/undef_macro.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "undef_macro.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("This should not appear") == std::string::npos, 
                "#undef should remove macro definition");
        }, execution_time);
    integration_test_passed_with_time("#undef macro", "undef_macro.cb", execution_time);
    
    // Test 18: Built-in __FILE__ macro
    run_cb_test_with_output_and_time("../cases/preprocessor/builtin_file.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "builtin_file.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("builtin_file.cb") != std::string::npos, 
                "Output should contain filename");
        }, execution_time);
    integration_test_passed_with_time("built-in __FILE__", "builtin_file.cb", execution_time);
    
    // Test 19: Built-in __LINE__ macro
    run_cb_test_with_output_and_time("../cases/preprocessor/builtin_line.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "builtin_line.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("5") != std::string::npos, 
                "Output should contain line number");
        }, execution_time);
    integration_test_passed_with_time("built-in __LINE__", "builtin_line.cb", execution_time);
    
    // Test 20: Built-in __DATE__ and __TIME__ macros
    run_cb_test_with_output_and_time("../cases/preprocessor/builtin_date_time.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "builtin_date_time.cb should execute successfully");
            // Just verify it executes - date/time format may vary
        }, execution_time);
    integration_test_passed_with_time("built-in __DATE__/__TIME__", "builtin_date_time.cb", execution_time);
    
    // Test 21: Macro expansion order
    run_cb_test_with_output_and_time("../cases/preprocessor/macro_expansion_order.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "macro_expansion_order.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("15") != std::string::npos, 
                "Macros should expand in correct order");
        }, execution_time);
    integration_test_passed_with_time("macro expansion order", "macro_expansion_order.cb", execution_time);
    
    // Test 22: Nested macro expansion
    run_cb_test_with_output_and_time("../cases/preprocessor/nested_expansion.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "nested_expansion.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, 
                "Nested macros should expand correctly");
        }, execution_time);
    integration_test_passed_with_time("nested expansion", "nested_expansion.cb", execution_time);
    
    // Test 23: Nested #ifdef with #else
    run_cb_test_with_output_and_time("../cases/preprocessor/ifdef_nested_else.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ifdef_nested_else.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Inner defined") != std::string::npos, 
                "Nested ifdef with else should work");
        }, execution_time);
    integration_test_passed_with_time("nested ifdef with else", "ifdef_nested_else.cb", execution_time);
    
    // Test 24: Multiple #elseif branches
    run_cb_test_with_output_and_time("../cases/preprocessor/multiple_elseif.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_elseif.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Option 2") != std::string::npos, 
                "Multiple elseif should work");
        }, execution_time);
    integration_test_passed_with_time("multiple elseif", "multiple_elseif.cb", execution_time);
    
    // Test 25: Empty macro definition (flag)
    run_cb_test_with_output_and_time("../cases/preprocessor/empty_define.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "empty_define.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("Flag is defined") != std::string::npos, 
                "Empty macro should work as flag");
        }, execution_time);
    integration_test_passed_with_time("empty define (flag)", "empty_define.cb", execution_time);
    
    // Test 26: Macro in complex expression
    run_cb_test_with_output_and_time("../cases/preprocessor/macro_in_expression.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "macro_in_expression.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("75") != std::string::npos || output.find("78") != std::string::npos || output.find("79") != std::string::npos, 
                "Macro in expression should work");
        }, execution_time);
    integration_test_passed_with_time("macro in expression", "macro_in_expression.cb", execution_time);
    
    // Test 27: Undefine and redefine
    run_cb_test_with_output_and_time("../cases/preprocessor/undef_redefine.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "undef_redefine.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("20") != std::string::npos, 
                "Undef and redefine should work");
        }, execution_time);
    integration_test_passed_with_time("undef and redefine", "undef_redefine.cb", execution_time);
    
    // Test 28: Macro with operators
    run_cb_test_with_output_and_time("../cases/preprocessor/ifdef_with_operators.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "ifdef_with_operators.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("35") != std::string::npos, 
                "Macro with operators should work");
        }, execution_time);
    integration_test_passed_with_time("macro with operators", "ifdef_with_operators.cb", execution_time);
    
    // Test 29: Whitespace handling
    run_cb_test_with_output_and_time("../cases/preprocessor/whitespace_handling.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "whitespace_handling.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("42") != std::string::npos, 
                "Whitespace in macros should be handled");
        }, execution_time);
    integration_test_passed_with_time("whitespace handling", "whitespace_handling.cb", execution_time);
    
    // Test 30: Different numeric types
    run_cb_test_with_output_and_time("../cases/preprocessor/numeric_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "numeric_types.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("3.14159") != std::string::npos, 
                "Double macro should work");
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, 
                "Int macro should work");
        }, execution_time);
    integration_test_passed_with_time("numeric types", "numeric_types.cb", execution_time);
    
    // Test 31: Case sensitivity
    run_cb_test_with_output_and_time("../cases/preprocessor/case_sensitive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "case_sensitive.cb should execute successfully");
            INTEGRATION_ASSERT(output.find("10") != std::string::npos, 
                "Lowercase macro should work");
            INTEGRATION_ASSERT(output.find("20") != std::string::npos, 
                "Uppercase macro should work");
        }, execution_time);
    integration_test_passed_with_time("case sensitivity", "case_sensitive.cb", execution_time);
    
    std::cout << "[integration-test] Preprocessor tests completed" << std::endl;
}
