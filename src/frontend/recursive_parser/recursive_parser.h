#pragma once
#include "recursive_lexer.h"
#include "../../common/ast.h"
#include <memory>

using namespace RecursiveParserNS;

class RecursiveParser {
public:
    RecursiveParser(const std::string& source);
    ASTNode* parse();
    ASTNode* parseProgram();
    
private:
    RecursiveLexer lexer_;
    Token current_token_;
    
    // Helper methods
    bool match(TokenType type);
    bool check(TokenType type);
    Token advance();
    Token peek();
    bool isAtEnd();
    void consume(TokenType type, const std::string& message);
    void error(const std::string& message);
    
    // Parse methods
    ASTNode* parseStatement();
    ASTNode* parseTypedefDeclaration();
    ASTNode* parseVariableDeclaration();
    ASTNode* parseFunctionDeclaration();
    ASTNode* parseFunctionDeclarationAfterName(const std::string& return_type, const std::string& function_name);
    std::string parseType();
    ASTNode* parseExpression();
    ASTNode* parseAssignment();
    ASTNode* parseLogicalOr();
    ASTNode* parseLogicalAnd();
    ASTNode* parseComparison();
    ASTNode* parseAdditive();
    ASTNode* parseMultiplicative();
    ASTNode* parseUnary();
    ASTNode* parsePostfix();
    ASTNode* parsePrimary();
};
