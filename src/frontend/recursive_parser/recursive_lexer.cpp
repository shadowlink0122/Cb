#include "recursive_lexer.h"
#include <iostream>
#include <unordered_map>

namespace RecursiveParserNS {

RecursiveLexer::RecursiveLexer(const std::string& source) 
    : source_(source), current_(0), line_(1), column_(1), 
      current_token_(TokenType::TOK_EOF, "", 0, 0), has_peeked_(false) {
}

Token RecursiveLexer::nextToken() {
    if (has_peeked_) {
        has_peeked_ = false;
        return current_token_;
    }

    skipWhitespace();

    if (isAtEnd()) {
        return makeToken(TokenType::TOK_EOF, "");
    }

    char c = advance();
    
    if (isAlpha(c) || c == '_') {
        return makeIdentifier();
    }

    if (isDigit(c)) {
        return makeNumber();
    }

    switch (c) {
        // Single character tokens
        case '+': 
            if (peek() == '+') {
                advance(); // consume second '+'
                return makeToken(TokenType::TOK_INCR, "++");
            }
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_PLUS_ASSIGN, "+=");
            }
            return makeToken(TokenType::TOK_PLUS, "+");
        case '-': 
            if (peek() == '-') {
                advance(); // consume second '-'
                return makeToken(TokenType::TOK_DECR, "--");
            }
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_MINUS_ASSIGN, "-=");
            }
            return makeToken(TokenType::TOK_MINUS, "-");
        case '*': 
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_MUL_ASSIGN, "*=");
            }
            return makeToken(TokenType::TOK_MUL, "*");
        case '/': 
            if (peek() == '/') {
                skipComment();
                return nextToken();
            }
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_DIV_ASSIGN, "/=");
            }
            return makeToken(TokenType::TOK_DIV, "/");
        case '%': 
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_MOD_ASSIGN, "%=");
            }
            return makeToken(TokenType::TOK_MOD, "%");
        case ';': return makeToken(TokenType::TOK_SEMICOLON, ";");
        case ',': return makeToken(TokenType::TOK_COMMA, ",");
        case '(': return makeToken(TokenType::TOK_LPAREN, "(");
        case ')': return makeToken(TokenType::TOK_RPAREN, ")");
        case '{': return makeToken(TokenType::TOK_LBRACE, "{");
        case '}': return makeToken(TokenType::TOK_RBRACE, "}");
        case '[': return makeToken(TokenType::TOK_LBRACKET, "[");
        case ']': return makeToken(TokenType::TOK_RBRACKET, "]");
        
        // Two character tokens
        case '=':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::TOK_EQ, "==");
            }
            return makeToken(TokenType::TOK_ASSIGN, "=");
            
        case '!':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::TOK_NE, "!=");
            }
            return makeToken(TokenType::TOK_NOT, "!");
            
        case '<':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::TOK_LE, "<=");
            }
            if (peek() == '<') {
                advance();
                if (peek() == '=') {
                    advance(); // consume '='
                    return makeToken(TokenType::TOK_LSHIFT_ASSIGN, "<<=");
                }
                return makeToken(TokenType::TOK_LEFT_SHIFT, "<<");
            }
            return makeToken(TokenType::TOK_LT, "<");
            
        case '>':
            if (peek() == '=') {
                advance();
                return makeToken(TokenType::TOK_GE, ">=");
            }
            if (peek() == '>') {
                advance();
                if (peek() == '=') {
                    advance(); // consume '='
                    return makeToken(TokenType::TOK_RSHIFT_ASSIGN, ">>=");
                }
                return makeToken(TokenType::TOK_RIGHT_SHIFT, ">>");
            }
            return makeToken(TokenType::TOK_GT, ">");
            
        case '&':
            if (peek() == '&') {
                advance();
                return makeToken(TokenType::TOK_AND, "&&");
            }
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_AND_ASSIGN, "&=");
            }
            return makeToken(TokenType::TOK_BIT_AND, "&");
            
        case '|':
            if (peek() == '|') {
                advance();
                return makeToken(TokenType::TOK_OR, "||");
            }
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_OR_ASSIGN, "|=");
            }
            return makeToken(TokenType::TOK_BIT_OR, "|");
            
        case '^':
            if (peek() == '=') {
                advance(); // consume '='
                return makeToken(TokenType::TOK_XOR_ASSIGN, "^=");
            }
            return makeToken(TokenType::TOK_BIT_XOR, "^");
            
        case '~':
            return makeToken(TokenType::TOK_BIT_NOT, "~");
            
        case '?':
            return makeToken(TokenType::TOK_QUESTION, "?");
            
        case ':':
            if (peek() == ':') {
                advance(); // consume second ':'
                return makeToken(TokenType::TOK_SCOPE, "::");
            }
            return makeToken(TokenType::TOK_COLON, ":");
            
        case '.':
            return makeToken(TokenType::TOK_DOT, ".");
            
        case '"': return makeString();
        case '\'': return makeChar();
    }

    return makeToken(TokenType::TOK_ERROR, std::string(1, c));
}

bool RecursiveLexer::isAtEnd() const {
    return current_ >= source_.length();
}

Token RecursiveLexer::peekToken() {
    if (!has_peeked_) {
        current_token_ = nextToken();
        has_peeked_ = true;
    }
    return current_token_;
}

char RecursiveLexer::peek() {
    if (isAtEnd()) return '\0';
    return source_[current_];
}

char RecursiveLexer::peekNext() {
    if (current_ + 1 >= source_.length()) return '\0';
    return source_[current_ + 1];
}

char RecursiveLexer::advance() {
    if (isAtEnd()) return '\0';
    
    char c = source_[current_++];
    if (c == '\n') {
        line_++;
        column_ = 1;
    } else {
        column_++;
    }
    return c;
}

void RecursiveLexer::skipWhitespace() {
    while (!isAtEnd()) {
        char c = peek();
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void RecursiveLexer::skipComment() {
    while (peek() != '\n' && !isAtEnd()) {
        advance();
    }
}

Token RecursiveLexer::makeToken(TokenType type, const std::string& value) {
    return Token(type, value, line_, column_ - value.length());
}

Token RecursiveLexer::makeIdentifier() {
    size_t start = current_ - 1;
    
    while (isAlphaNumeric(peek()) || peek() == '_') {
        advance();
    }
    
    std::string text = source_.substr(start, current_ - start);
    TokenType type = getKeywordType(text);
    
    return makeToken(type, text);
}

Token RecursiveLexer::makeNumber() {
    size_t start = current_ - 1;
    
    while (isDigit(peek())) {
        advance();
    }
    
    // Look for a fractional part
    if (peek() == '.' && isDigit(peekNext())) {
        advance(); // consume '.'
        while (isDigit(peek())) {
            advance();
        }
    }
    
    std::string text = source_.substr(start, current_ - start);
    return makeToken(TokenType::TOK_NUMBER, text);
}

Token RecursiveLexer::makeString() {
    size_t start = current_ - 1;
    
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line_++;
        advance();
    }
    
    if (isAtEnd()) {
        return makeToken(TokenType::TOK_ERROR, "Unterminated string");
    }
    
    advance(); // consume closing "
    
    std::string text = source_.substr(start + 1, current_ - start - 2); // trim quotes
    return makeToken(TokenType::TOK_STRING, text);
}

Token RecursiveLexer::makeChar() {
    if (isAtEnd()) {
        return makeToken(TokenType::TOK_ERROR, "Unterminated character");
    }
    
    char c = advance();
    
    if (peek() != '\'') {
        return makeToken(TokenType::TOK_ERROR, "Unterminated character");
    }
    
    advance(); // consume closing '
    
    return makeToken(TokenType::TOK_CHAR, std::string(1, c));
}

TokenType RecursiveLexer::getKeywordType(const std::string& text) {
    static std::unordered_map<std::string, TokenType> keywords = {
        {"main", TokenType::TOK_MAIN},
        {"if", TokenType::TOK_IF},
        {"else", TokenType::TOK_ELSE},
        {"for", TokenType::TOK_FOR},
        {"while", TokenType::TOK_WHILE},
        {"break", TokenType::TOK_BREAK},
        {"continue", TokenType::TOK_CONTINUE},
        {"return", TokenType::TOK_RETURN},
        {"int", TokenType::TOK_INT},
        {"long", TokenType::TOK_LONG},
        {"short", TokenType::TOK_SHORT},
        {"tiny", TokenType::TOK_TINY},
        {"void", TokenType::TOK_VOID},
        {"string", TokenType::TOK_STRING_TYPE},
        {"char", TokenType::TOK_CHAR_TYPE},
        {"bool", TokenType::TOK_BOOL},
        {"true", TokenType::TOK_TRUE},
        {"false", TokenType::TOK_FALSE},
        {"print", TokenType::TOK_PRINT},
        {"println", TokenType::TOK_PRINTLN},
        {"printf", TokenType::TOK_PRINTF},
        {"typedef", TokenType::TOK_TYPEDEF},
        {"const", TokenType::TOK_CONST},
        {"static", TokenType::TOK_STATIC},
        {"struct", TokenType::TOK_STRUCT},
        {"enum", TokenType::TOK_ENUM}
    };
    
    auto it = keywords.find(text);
    if (it != keywords.end()) {
        return it->second;
    }
    
    return TokenType::TOK_IDENTIFIER;
}

bool RecursiveLexer::isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool RecursiveLexer::isDigit(char c) {
    return c >= '0' && c <= '9';
}

bool RecursiveLexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

} // namespace RecursiveParserNS
