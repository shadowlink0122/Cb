#ifndef TEST_PREPROCESSOR_HPP
#define TEST_PREPROCESSOR_HPP

#include "../../framework/test_framework.hpp"
#include "frontend/preprocessor/preprocessor.h"

using namespace CbPreprocessor;

// テスト: 単純な#defineの処理
void test_process_simple_define() {
    Preprocessor preprocessor;
    
    std::string source = 
        "#define PI 3.14159\n"
        "println(PI);\n";
    
    std::string result = preprocessor.process(source, "test.cb");
    
    ASSERT_FALSE(preprocessor.hasError());
    ASSERT_STREQ("println(3.14159);\n", result);
}

// テスト: 複数の#defineの処理
void test_process_multiple_defines() {
    Preprocessor preprocessor;
    
    std::string source = 
        "#define PI 3.14159\n"
        "#define E 2.71828\n"
        "println(\"PI =\", PI);\n"
        "println(\"E =\", E);\n";
    
    std::string result = preprocessor.process(source, "test.cb");
    
    ASSERT_FALSE(preprocessor.hasError());
    
    auto macros = preprocessor.getDefinedMacros();
    ASSERT_EQ(2, macros.size());
}

// テスト: #undefの処理
void test_process_undef() {
    Preprocessor preprocessor;
    
    std::string source = 
        "#define DEBUG 1\n"
        "println(DEBUG);\n"
        "#undef DEBUG\n"
        "println(DEBUG);\n";
    
    std::string result = preprocessor.process(source, "test.cb");
    
    ASSERT_FALSE(preprocessor.hasError());
    // 最初のprintln(DEBUG)は展開される
    ASSERT_TRUE(result.find("println(1);") != std::string::npos);
    // 2番目のprintln(DEBUG)は展開されない
    ASSERT_TRUE(result.find("println(DEBUG);") != std::string::npos);
}

// テスト: 関数形式マクロの処理
void test_process_function_macro() {
    Preprocessor preprocessor;
    
    std::string source = 
        "#define SQUARE(x) ((x) * (x))\n"
        "int result = SQUARE(5);\n";
    
    std::string result = preprocessor.process(source, "test.cb");
    
    ASSERT_FALSE(preprocessor.hasError());
    ASSERT_TRUE(result.find("((5) * (5))") != std::string::npos);
}

// テスト: ネストしたマクロの処理
void test_process_nested_macros() {
    Preprocessor preprocessor;
    
    std::string source = 
        "#define DOUBLE(x) ((x) * 2)\n"
        "#define QUAD(x) DOUBLE(DOUBLE(x))\n"
        "int result = QUAD(5);\n";
    
    std::string result = preprocessor.process(source, "test.cb");
    
    ASSERT_FALSE(preprocessor.hasError());
    ASSERT_TRUE(result.find("((((5) * 2)) * 2)") != std::string::npos);
}

// テスト: リセット機能
void test_reset() {
    Preprocessor preprocessor;
    
    std::string source = "#define TEST 123\n";
    preprocessor.process(source, "test.cb");
    
    ASSERT_EQ(1, preprocessor.getDefinedMacros().size());
    
    preprocessor.reset();
    ASSERT_EQ(0, preprocessor.getDefinedMacros().size());
}

// テスト: エラーハンドリング
void test_error_handling() {
    Preprocessor preprocessor;
    
    // 正常なケース
    std::string source = "#define PI 3.14\n";
    preprocessor.process(source, "test.cb");
    ASSERT_FALSE(preprocessor.hasError());
    ASSERT_STREQ("", preprocessor.getLastError());
}

// すべてのテストを登録
void register_preprocessor_tests() {
    RUN_TEST("Preprocessor::process_simple_define", test_process_simple_define);
    RUN_TEST("Preprocessor::process_multiple_defines", test_process_multiple_defines);
    RUN_TEST("Preprocessor::process_undef", test_process_undef);
    RUN_TEST("Preprocessor::process_function_macro", test_process_function_macro);
    RUN_TEST("Preprocessor::process_nested_macros", test_process_nested_macros);
    RUN_TEST("Preprocessor::reset", test_reset);
    RUN_TEST("Preprocessor::error_handling", test_error_handling);
}

#endif // TEST_PREPROCESSOR_HPP
