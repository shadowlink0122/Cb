#ifndef TEST_MACRO_EXPANDER_HPP
#define TEST_MACRO_EXPANDER_HPP

#include "../../framework/test_framework.hpp"
#include "frontend/preprocessor/macro_expander.h"

using namespace CbPreprocessor;

// テスト: マクロの定義と確認
void test_define_and_is_defined() {
    MacroExpander expander;
    
    MacroDefinition macro;
    macro.name = "PI";
    macro.type = MacroType::OBJECT_LIKE;
    macro.body = "3.14159";
    
    expander.define(macro);
    
    ASSERT_TRUE(expander.isDefined("PI"));
    ASSERT_FALSE(expander.isDefined("UNDEFINED"));
}

// テスト: オブジェクト形式マクロの展開
void test_expand_object_like() {
    MacroExpander expander;
    
    MacroDefinition macro;
    macro.name = "PI";
    macro.type = MacroType::OBJECT_LIKE;
    macro.body = "3.14159";
    
    expander.define(macro);
    
    std::string result = expander.expand("PI");
    ASSERT_STREQ("3.14159", result);
}

// テスト: 関数形式マクロの展開
void test_expand_function_macro() {
    MacroExpander expander;
    
    MacroDefinition macro;
    macro.name = "SQUARE";
    macro.type = MacroType::FUNCTION_LIKE;
    macro.parameters = {"x"};
    macro.body = "((x) * (x))";
    
    expander.define(macro);
    
    std::string result = expander.expand("SQUARE", {"5"});
    ASSERT_STREQ("((5) * (5))", result);
}

// テスト: マクロの未定義化
void test_undefine() {
    MacroExpander expander;
    
    MacroDefinition macro;
    macro.name = "DEBUG";
    macro.type = MacroType::OBJECT_LIKE;
    macro.body = "1";
    
    expander.define(macro);
    ASSERT_TRUE(expander.isDefined("DEBUG"));
    
    expander.undefine("DEBUG");
    ASSERT_FALSE(expander.isDefined("DEBUG"));
}

// テスト: expandAllでオブジェクト形式マクロを展開
void test_expand_all_object_macros() {
    MacroExpander expander;
    
    MacroDefinition pi;
    pi.name = "PI";
    pi.type = MacroType::OBJECT_LIKE;
    pi.body = "3.14159";
    expander.define(pi);
    
    MacroDefinition true_macro;
    true_macro.name = "TRUE";
    true_macro.type = MacroType::OBJECT_LIKE;
    true_macro.body = "1";
    expander.define(true_macro);
    
    std::string source = "int x = PI; int y = TRUE;";
    std::string result = expander.expandAll(source);
    
    ASSERT_STREQ("int x = 3.14159; int y = 1;", result);
}

// テスト: expandAllで関数形式マクロを展開
void test_expand_all_function_macros() {
    MacroExpander expander;
    
    MacroDefinition square;
    square.name = "SQUARE";
    square.type = MacroType::FUNCTION_LIKE;
    square.parameters = {"x"};
    square.body = "((x) * (x))";
    expander.define(square);
    
    std::string source = "int result = SQUARE(5);";
    std::string result = expander.expandAll(source);
    
    ASSERT_STREQ("int result = ((5) * (5));", result);
}

// テスト: ネストしたマクロの展開
void test_nested_macro_expansion() {
    MacroExpander expander;
    
    MacroDefinition double_macro;
    double_macro.name = "DOUBLE";
    double_macro.type = MacroType::FUNCTION_LIKE;
    double_macro.parameters = {"x"};
    double_macro.body = "((x) * 2)";
    expander.define(double_macro);
    
    MacroDefinition quad;
    quad.name = "QUAD";
    quad.type = MacroType::FUNCTION_LIKE;
    quad.parameters = {"x"};
    quad.body = "DOUBLE(DOUBLE(x))";
    expander.define(quad);
    
    std::string source = "int result = QUAD(5);";
    std::string result = expander.expandAll(source);
    
    ASSERT_STREQ("int result = ((((5) * 2)) * 2);", result);
}

// テスト: オブジェクト形式と関数形式の混在
void test_mixed_macros() {
    MacroExpander expander;
    
    MacroDefinition pi;
    pi.name = "PI";
    pi.type = MacroType::OBJECT_LIKE;
    pi.body = "3.14159";
    expander.define(pi);
    
    MacroDefinition circle_area;
    circle_area.name = "CIRCLE_AREA";
    circle_area.type = MacroType::FUNCTION_LIKE;
    circle_area.parameters = {"r"};
    circle_area.body = "(PI * (r) * (r))";
    expander.define(circle_area);
    
    std::string source = "double area = CIRCLE_AREA(5.0);";
    std::string result = expander.expandAll(source);
    
    // 結果に"3.14159"と"(5.0)"が含まれていることを確認
    ASSERT_TRUE(result.find("3.14159") != std::string::npos);
    ASSERT_TRUE(result.find("(5.0)") != std::string::npos);
}

// すべてのテストを登録
void register_macro_expander_tests() {
    RUN_TEST("MacroExpander::define_and_is_defined", test_define_and_is_defined);
    RUN_TEST("MacroExpander::expand_object_like", test_expand_object_like);
    RUN_TEST("MacroExpander::expand_function_macro", test_expand_function_macro);
    RUN_TEST("MacroExpander::undefine", test_undefine);
    RUN_TEST("MacroExpander::expand_all_object_macros", test_expand_all_object_macros);
    RUN_TEST("MacroExpander::expand_all_function_macros", test_expand_all_function_macros);
    RUN_TEST("MacroExpander::nested_macro_expansion", test_nested_macro_expansion);
    RUN_TEST("MacroExpander::mixed_macros", test_mixed_macros);
}

#endif // TEST_MACRO_EXPANDER_HPP
