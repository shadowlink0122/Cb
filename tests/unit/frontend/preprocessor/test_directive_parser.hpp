#ifndef TEST_DIRECTIVE_PARSER_HPP
#define TEST_DIRECTIVE_PARSER_HPP

#include "../../framework/test_framework.hpp"
#include "frontend/preprocessor/directive_parser.h"

using namespace CbPreprocessor;

// テスト: 単純な#defineのパース
void test_parse_simple_define() {
    DirectiveParser parser;
    
    std::string line = "#define PI 3.14159";
    MacroDefinition macro = parser.parseDefine(line);
    
    ASSERT_STREQ("PI", macro.name);
    ASSERT_TRUE(macro.isObjectLike());
    ASSERT_STREQ("3.14159", macro.body);
}

// テスト: 関数形式#defineのパース
void test_parse_function_define() {
    DirectiveParser parser;
    
    std::string line = "#define SQUARE(x) ((x) * (x))";
    MacroDefinition macro = parser.parseDefine(line);
    
    ASSERT_STREQ("SQUARE", macro.name);
    ASSERT_TRUE(macro.isFunctionLike());
    ASSERT_EQ(1, macro.parameters.size());
    ASSERT_STREQ("x", macro.parameters[0]);
    ASSERT_STREQ("((x) * (x))", macro.body);
}

// テスト: 複数パラメータのパース
void test_parse_multiple_parameters() {
    DirectiveParser parser;
    
    std::string line = "#define MAX(a, b) ((a) > (b) ? (a) : (b))";
    MacroDefinition macro = parser.parseDefine(line);
    
    ASSERT_STREQ("MAX", macro.name);
    ASSERT_TRUE(macro.isFunctionLike());
    ASSERT_EQ(2, macro.parameters.size());
    ASSERT_STREQ("a", macro.parameters[0]);
    ASSERT_STREQ("b", macro.parameters[1]);
}

// テスト: マクロ呼び出しのパース（単純）
void test_parse_macro_call_simple() {
    DirectiveParser parser;
    
    std::string name;
    std::vector<std::string> args;
    
    bool result = parser.parseMacroCall("SQUARE(5)", name, args);
    ASSERT_TRUE(result);
    ASSERT_STREQ("SQUARE", name);
    ASSERT_EQ(1, args.size());
    ASSERT_STREQ("5", args[0]);
}

// テスト: マクロ呼び出しのパース（複数引数）
void test_parse_macro_call_multiple_args() {
    DirectiveParser parser;
    
    std::string name;
    std::vector<std::string> args;
    
    bool result = parser.parseMacroCall("MAX(10, 20)", name, args);
    ASSERT_TRUE(result);
    ASSERT_STREQ("MAX", name);
    ASSERT_EQ(2, args.size());
    ASSERT_STREQ("10", args[0]);
    ASSERT_STREQ("20", args[1]);
}

// テスト: ネストした括弧を含むマクロ呼び出し
void test_parse_macro_call_nested_parens() {
    DirectiveParser parser;
    
    std::string name;
    std::vector<std::string> args;
    
    bool result = parser.parseMacroCall("FUNC(foo(1,2), bar(3))", name, args);
    ASSERT_TRUE(result);
    ASSERT_STREQ("FUNC", name);
    ASSERT_EQ(2, args.size());
    ASSERT_STREQ("foo(1,2)", args[0]);
    ASSERT_STREQ("bar(3)", args[1]);
}

// すべてのテストを登録
void register_directive_parser_tests() {
    RUN_TEST("DirectiveParser::parse_simple_define", test_parse_simple_define);
    RUN_TEST("DirectiveParser::parse_function_define", test_parse_function_define);
    RUN_TEST("DirectiveParser::parse_multiple_parameters", test_parse_multiple_parameters);
    RUN_TEST("DirectiveParser::parse_macro_call_simple", test_parse_macro_call_simple);
    RUN_TEST("DirectiveParser::parse_macro_call_multiple_args", test_parse_macro_call_multiple_args);
    RUN_TEST("DirectiveParser::parse_macro_call_nested_parens", test_parse_macro_call_nested_parens);
}

#endif // TEST_DIRECTIVE_PARSER_HPP
