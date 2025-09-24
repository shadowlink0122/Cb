#pragma once
#include "recursive_lexer.h"
#include "../../common/ast.h"
#include <memory>
#include <unordered_map>

using namespace RecursiveParserNS;

class RecursiveParser {
public:
    RecursiveParser(const std::string& source, const std::string& filename = "");
    ASTNode* parse();
    ASTNode* parseProgram();
    
private:
    RecursiveLexer lexer_;
    Token current_token_;
    std::string filename_;  // ソースファイル名
    std::string source_;    // 元のソースコード
    std::vector<std::string> source_lines_;  // 行ごとに分割されたソース
    std::unordered_map<std::string, std::string> typedef_map_; // typedef alias -> actual type mapping
    
    // Helper methods
    bool match(TokenType type);
    bool check(TokenType type);
    Token advance();
    Token peek();
    bool isAtEnd();
    void consume(TokenType type, const std::string& message);
    void error(const std::string& message);
    
    // 位置情報設定のヘルパー
    void setLocation(ASTNode* node, const Token& token);
    void setLocation(ASTNode* node, int line, int column);
    std::string getSourceLine(int line_number);
    
    // Main parsing methods
    ASTNode* parseStatement();
    ASTNode* parseExpression();
    
    // Type and declaration parsing
    std::string parseType();
    ASTNode* parseTypedefDeclaration();
    ASTNode* parseVariableDeclaration();
    ASTNode* parseFunctionDeclaration();
    ASTNode* parseFunctionDeclarationAfterName(const std::string& return_type, const std::string& function_name);
    
    // Statement parsing helpers
    ASTNode* parseTypeDeclaration(bool isConst);
    ASTNode* parseArrayDeclaration(const std::string& type_name, bool isConst);
    ASTNode* parseVariableDeclarationList(const std::string& type_name, bool isConst);
    ASTNode* parseReturnStatement();
    ASTNode* parseBreakStatement();
    ASTNode* parseContinueStatement();
    ASTNode* parseIfStatement();
    ASTNode* parseForStatement();
    ASTNode* parseWhileStatement();
    ASTNode* parseCompoundStatement();
    ASTNode* parsePrintStatement();
    ASTNode* parsePrintlnStatement();
    ASTNode* parseIdentifierStatement();
    
    // Expression parsing
    ASTNode* parseAssignment();
    ASTNode* parseLogicalOr();
    ASTNode* parseLogicalAnd();
    ASTNode* parseComparison();
    ASTNode* parseAdditive();
    ASTNode* parseMultiplicative();
    ASTNode* parseUnary();
    ASTNode* parsePostfix();
    ASTNode* parsePrimary();
    
    // Utility methods
    TypeInfo getTypeInfoFromString(const std::string& type_name);
    ASTNode* parseArrayLiteral();
};
