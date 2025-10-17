#ifndef TEST_MACRO_DEFINITION_HPP
#define TEST_MACRO_DEFINITION_HPP

#include "../../framework/test_framework.hpp"
#include "frontend/preprocessor/macro_definition.h"

using namespace CbPreprocessor;

// テスト: オブジェクト形式マクロ
void test_macro_definition_object_like() {
    MacroDefinition macro;
    macro.name = "PI";
    macro.type = MacroType::OBJECT_LIKE;
    macro.body = "3.14159";
    
    ASSERT_TRUE(macro.isObjectLike());
    ASSERT_FALSE(macro.isFunctionLike());
    ASSERT_EQ(0, macro.getParameterCount());
    ASSERT_STREQ("#define PI 3.14159", macro.toString());
}

// テスト: 関数形式マクロ
void test_macro_definition_function_like() {
    MacroDefinition macro;
    macro.name = "SQUARE";
    macro.type = MacroType::FUNCTION_LIKE;
    macro.parameters = {"x"};
    macro.body = "((x) * (x))";
    
    ASSERT_FALSE(macro.isObjectLike());
    ASSERT_TRUE(macro.isFunctionLike());
    ASSERT_EQ(1, macro.getParameterCount());
    ASSERT_STREQ("#define SQUARE(x) ((x) * (x))", macro.toString());
}

// テスト: 複数パラメータ
void test_macro_definition_multiple_parameters() {
    MacroDefinition macro;
    macro.name = "MAX";
    macro.type = MacroType::FUNCTION_LIKE;
    macro.parameters = {"a", "b"};
    macro.body = "((a) > (b) ? (a) : (b))";
    
    ASSERT_TRUE(macro.isFunctionLike());
    ASSERT_EQ(2, macro.getParameterCount());
}

// すべてのテストを登録
void register_macro_definition_tests() {
    RUN_TEST("MacroDefinition::object_like", test_macro_definition_object_like);
    RUN_TEST("MacroDefinition::function_like", test_macro_definition_function_like);
    RUN_TEST("MacroDefinition::multiple_parameters", test_macro_definition_multiple_parameters);
}

#endif // TEST_MACRO_DEFINITION_HPP
