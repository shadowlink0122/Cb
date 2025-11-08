#pragma once
#include <string>
#include <vector>

namespace RecursiveParserNS {

enum class TokenType {
    // Operators
    TOK_PLUS,   // +
    TOK_MINUS,  // -
    TOK_MUL,    // *
    TOK_DIV,    // /
    TOK_MOD,    // %
    TOK_EQ,     // ==
    TOK_NE,     // !=
    TOK_LT,     // <
    TOK_LE,     // <=
    TOK_GT,     // >
    TOK_GE,     // >=
    TOK_AND,    // &&
    TOK_OR,     // ||
    TOK_NOT,    // !
    TOK_INCR,   // ++
    TOK_DECR,   // --
    TOK_ASSIGN, // =

    // Compound assignment operators
    TOK_PLUS_ASSIGN,   // +=
    TOK_MINUS_ASSIGN,  // -=
    TOK_MUL_ASSIGN,    // *=
    TOK_DIV_ASSIGN,    // /=
    TOK_MOD_ASSIGN,    // %=
    TOK_AND_ASSIGN,    // &=
    TOK_OR_ASSIGN,     // |=
    TOK_XOR_ASSIGN,    // ^=
    TOK_LSHIFT_ASSIGN, // <<=
    TOK_RSHIFT_ASSIGN, // >>=

    // Bitwise operators
    TOK_BIT_AND,           // &
    TOK_BIT_OR,            // |
    TOK_PIPE = TOK_BIT_OR, // | (alias for union types)
    TOK_BIT_XOR,           // ^
    TOK_BIT_NOT,           // ~
    TOK_LEFT_SHIFT,        // <<
    TOK_RIGHT_SHIFT,       // >>

    // Ternary operator
    TOK_QUESTION, // ?
    TOK_COLON,    // :

    // Punctuation
    TOK_SEMICOLON, // ;
    TOK_COMMA,     // ,
    TOK_LPAREN,    // (
    TOK_RPAREN,    // )
    TOK_LBRACE,    // {
    TOK_RBRACE,    // }
    TOK_LBRACKET,  // [
    TOK_RBRACKET,  // ]
    TOK_DOT,       // .
    TOK_ARROW,     // ->
    TOK_SCOPE,     // ::

    // Literals
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_INTERPOLATED_STRING, // v0.11.0 文字列補間
    TOK_CHAR,

    // Keywords
    TOK_MAIN,
    TOK_IF,
    TOK_ELSE,
    TOK_FOR,
    TOK_WHILE,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_RETURN,
    TOK_INT,
    TOK_LONG,
    TOK_SHORT,
    TOK_TINY,
    TOK_VOID,
    TOK_STRING_TYPE,
    TOK_CHAR_TYPE,
    TOK_BOOL,
    TOK_FLOAT,
    TOK_DOUBLE,
    TOK_BIG,
    TOK_QUAD,
    TOK_TRUE,
    TOK_FALSE,
    TOK_PRINT,
    TOK_PRINTLN,
    TOK_PRINTF,
    TOK_TYPEDEF,
    TOK_CONST,
    TOK_STATIC,
    TOK_PRIVATE,
    TOK_STRUCT,
    TOK_ENUM,
    TOK_INTERFACE,
    TOK_IMPL,
    TOK_SELF,
    TOK_NEW,
    TOK_DELETE,
    TOK_NULLPTR,
    TOK_NULL,
    TOK_UNSIGNED,
    TOK_ASSERT,
    TOK_DEFER,
    TOK_DEFAULT,
    TOK_SWITCH,
    TOK_CASE,
    TOK_MATCH,      // v0.11.0: match (pattern matching)
    TOK_FAT_ARROW,  // v0.11.0: => (fat arrow)
    TOK_UNDERSCORE, // v0.11.0: _ (wildcard pattern)
    TOK_RANGE,      // ... (range operator)
    TOK_FUNC,       // func (for lambda expressions)
    TOK_IMPORT,     // import
    TOK_EXPORT,     // export
    TOK_ASYNC,      // async (v0.12.0)
    TOK_AWAIT,      // await (v0.12.0)

    // Special
    TOK_EOF,
    TOK_ERROR
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token(TokenType t, const std::string &val, int l, int c)
        : type(t), value(val), line(l), column(c) {}
};

class RecursiveLexer {
  public:
    explicit RecursiveLexer(const std::string &source);
    Token nextToken();
    bool isAtEnd() const;
    Token peekToken();

  private:
    std::string source_;
    size_t current_;
    int line_;
    int column_;
    Token current_token_;
    bool has_peeked_;

    char peek();
    char peekNext();
    char advance();
    void skipWhitespace();
    void skipComment();
    Token makeToken(TokenType type, const std::string &value);
    Token makeIdentifier();
    Token makeNumber();
    Token makeString();
    Token makeChar();
    TokenType getKeywordType(const std::string &text);
    bool isAlpha(char c);
    bool isDigit(char c);
    bool isAlphaNumeric(char c);
};

} // namespace RecursiveParserNS
