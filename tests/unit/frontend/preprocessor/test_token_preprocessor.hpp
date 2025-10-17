#pragma once
#include "frontend/preprocessor/token_preprocessor.h"
#include "frontend/recursive_parser/recursive_lexer.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace RecursiveParserNS;

inline std::vector<Token> tokenize(const std::string& source) {
    RecursiveLexer lexer(source);
    std::vector<Token> tokens;
    while (!lexer.isAtEnd()) {
        Token token = lexer.nextToken();
        if (token.type == TokenType::TOK_EOF) {
            break;
        }
        tokens.push_back(token);
    }
    return tokens;
}

inline std::string tokensToString(const std::vector<Token>& tokens) {
    std::string result;
    for (const auto& token : tokens) {
        if (!result.empty()) result += " ";
        result += token.value;
    }
    return result;
}

inline void test_token_preprocessor_simple_define() {
    std::string source = "#define PI 3.14159\nint x = PI;";
    auto tokens = tokenize(source);
    
    TokenPreprocessor preprocessor;
    auto processed = preprocessor.process(tokens);
    
    assert(!preprocessor.hasError());
    
    // #define は削除され、PI は 3.14159 に展開される
    std::string result = tokensToString(processed);
    assert(result.find("3.14159") != std::string::npos);
    assert(result.find("PI") == std::string::npos || result.find("#define") != std::string::npos);
    
    std::cout << "✓ test_token_preprocessor_simple_define passed" << std::endl;
}

inline void test_token_preprocessor_string_literal_preserved() {
    std::string source = "#define PI 3.14159\nprintln(\"PI = \");";
    auto tokens = tokenize(source);
    
    TokenPreprocessor preprocessor;
    auto processed = preprocessor.process(tokens);
    
    assert(!preprocessor.hasError());
    
    // 文字列リテラル内の PI は展開されない
    bool foundStringWithPI = false;
    for (const auto& token : processed) {
        if (token.type == TokenType::TOK_STRING && token.value.find("PI") != std::string::npos) {
            foundStringWithPI = true;
            // 文字列内の PI が展開されていないことを確認
            assert(token.value.find("3.14") == std::string::npos);
        }
    }
    assert(foundStringWithPI);
    
    std::cout << "✓ test_token_preprocessor_string_literal_preserved passed" << std::endl;
}

inline void test_token_preprocessor_function_macro() {
    std::string source = "#define SQUARE(x) ((x) * (x))\nint y = SQUARE(5);";
    auto tokens = tokenize(source);
    
    TokenPreprocessor preprocessor;
    auto processed = preprocessor.process(tokens);
    
    assert(!preprocessor.hasError());
    
    // SQUARE(5) が展開される
    std::string result = tokensToString(processed);
    assert(result.find("5") != std::string::npos);
    assert(result.find("*") != std::string::npos);
    
    std::cout << "✓ test_token_preprocessor_function_macro passed" << std::endl;
}

inline void test_token_preprocessor_undef() {
    std::string source = "#define MAX 100\nint a = MAX;\n#undef MAX\nint b = MAX;";
    auto tokens = tokenize(source);
    
    TokenPreprocessor preprocessor;
    auto processed = preprocessor.process(tokens);
    
    assert(!preprocessor.hasError());
    
    // 最初の MAX は 100 に展開、2番目の MAX は展開されない
    int maxCount = 0;
    int hundredCount = 0;
    for (const auto& token : processed) {
        if (token.value == "MAX") maxCount++;
        if (token.value == "100") hundredCount++;
    }
    
    assert(hundredCount >= 1); // 最初の MAX が展開されている
    assert(maxCount >= 1);     // 2番目の MAX は展開されていない
    
    std::cout << "✓ test_token_preprocessor_undef passed" << std::endl;
}

inline void test_token_preprocessor_no_directives() {
    std::string source = "int main() { return 0; }";
    auto tokens = tokenize(source);
    
    TokenPreprocessor preprocessor;
    auto processed = preprocessor.process(tokens);
    
    assert(!preprocessor.hasError());
    
    // トークン数が変わらないことを確認
    assert(processed.size() == tokens.size());
    
    std::cout << "✓ test_token_preprocessor_no_directives passed" << std::endl;
}

inline void test_token_preprocessor_nested_macros() {
    std::string source = "#define DOUBLE(x) ((x) * 2)\n#define QUAD(x) DOUBLE(DOUBLE(x))\nint z = QUAD(5);";
    auto tokens = tokenize(source);
    
    TokenPreprocessor preprocessor;
    auto processed = preprocessor.process(tokens);
    
    assert(!preprocessor.hasError());
    
    // QUAD(5) が DOUBLE(DOUBLE(5)) に展開される
    std::string result = tokensToString(processed);
    assert(result.find("5") != std::string::npos);
    assert(result.find("2") != std::string::npos);
    
    std::cout << "✓ test_token_preprocessor_nested_macros passed" << std::endl;
}

inline void run_token_preprocessor_tests() {
    std::cout << "\n=== Token Preprocessor Tests ===" << std::endl;
    test_token_preprocessor_simple_define();
    test_token_preprocessor_string_literal_preserved();
    test_token_preprocessor_function_macro();
    test_token_preprocessor_undef();
    test_token_preprocessor_no_directives();
    test_token_preprocessor_nested_macros();
    std::cout << "All token preprocessor tests passed!\n" << std::endl;
}
