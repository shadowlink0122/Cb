#pragma once
#include "frontend/recursive_parser/recursive_lexer.h"
#include <cassert>
#include <iostream>

using namespace RecursiveParserNS;

inline void test_lexer_preprocessor_define() {
    std::string source = "#define PI 3.14159\nint main() {}";
    RecursiveLexer lexer(source);
    
    Token token = lexer.nextToken();
    assert(token.type == TokenType::TOK_PREPROCESSOR_DEFINE);
    assert(token.value == "#define PI 3.14159");
    std::cout << "✓ test_lexer_preprocessor_define passed" << std::endl;
}

inline void test_lexer_preprocessor_undef() {
    std::string source = "#undef MAX\nint x = 0;";
    RecursiveLexer lexer(source);
    
    Token token = lexer.nextToken();
    assert(token.type == TokenType::TOK_PREPROCESSOR_UNDEF);
    assert(token.value == "#undef MAX");
    std::cout << "✓ test_lexer_preprocessor_undef passed" << std::endl;
}

inline void test_lexer_preprocessor_with_spaces() {
    std::string source = "#  define  PI  3.14159\nint main() {}";
    RecursiveLexer lexer(source);
    
    Token token = lexer.nextToken();
    assert(token.type == TokenType::TOK_PREPROCESSOR_DEFINE);
    std::cout << "✓ test_lexer_preprocessor_with_spaces passed" << std::endl;
}

inline void test_lexer_string_literal_not_affected() {
    std::string source = "\"#define PI 3.14\"";
    RecursiveLexer lexer(source);
    
    Token token = lexer.nextToken();
    std::cout << "Token type: " << static_cast<int>(token.type) << ", value: '" << token.value << "'" << std::endl;
    assert(token.type == TokenType::TOK_STRING);
    // String tokenizer may strip quotes, so check both cases
    if (token.value != "\"#define PI 3.14\"" && token.value != "#define PI 3.14") {
        std::cerr << "Expected string value but got: " << token.value << std::endl;
        assert(false);
    }
    std::cout << "✓ test_lexer_string_literal_not_affected passed" << std::endl;
}

inline void test_lexer_normal_hash() {
    std::string source = "# comment\nint x;";
    RecursiveLexer lexer(source);
    
    Token token = lexer.nextToken();
    // Should be TOK_HASH for unknown directive
    assert(token.type == TokenType::TOK_HASH);
    std::cout << "✓ test_lexer_normal_hash passed" << std::endl;
}

inline void run_lexer_preprocessor_tests() {
    std::cout << "\n=== Lexer Preprocessor Tests ===" << std::endl;
    test_lexer_preprocessor_define();
    test_lexer_preprocessor_undef();
    test_lexer_preprocessor_with_spaces();
    test_lexer_string_literal_not_affected();
    test_lexer_normal_hash();
    std::cout << "All lexer preprocessor tests passed!\n" << std::endl;
}
