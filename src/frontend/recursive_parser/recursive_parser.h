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
    void setDebugMode(bool debug) { debug_mode_ = debug; }
    ASTNode* parseProgram();
    
private:
    RecursiveLexer lexer_;
    Token current_token_;
    std::string filename_;  // ソースファイル名
    std::string source_;    // 元のソースコード
    std::vector<std::string> source_lines_;  // 行ごとに分割されたソース
    std::unordered_map<std::string, std::string> typedef_map_; // typedef alias -> actual type mapping
    bool debug_mode_;  // デバッグモードフラグ
    
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
    ASTNode* parseStructDeclaration();           // struct宣言
    ASTNode* parseStructTypedefDeclaration();    // typedef struct宣言
    ASTNode* parseEnumDeclaration();             // enum宣言
    ASTNode* parseEnumTypedefDeclaration();      // typedef enum宣言
    ASTNode* parseVariableDeclaration();
    ASTNode* parseTypedefVariableDeclaration();
    ASTNode* parseFunctionDeclaration();
    ASTNode* parseFunctionDeclarationAfterName(const std::string& return_type, const std::string& function_name);
    
    // Typedef helper methods
    std::string resolveTypedefChain(const std::string& typedef_name);
    std::string extractBaseType(const std::string& type_name);
    
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
    ASTNode* parseTernary();        // 三項演算子 condition ? value1 : value2
    ASTNode* parseLogicalOr();
    ASTNode* parseLogicalAnd();
    ASTNode* parseBitwiseOr();      // ビット OR |
    ASTNode* parseBitwiseXor();     // ビット XOR ^
    ASTNode* parseBitwiseAnd();     // ビット AND &
    ASTNode* parseComparison();
    ASTNode* parseShift();          // ビットシフト << >>
    ASTNode* parseAdditive();
    ASTNode* parseMultiplicative();
    ASTNode* parseUnary();
    ASTNode* parsePostfix();
    ASTNode* parsePrimary();
    ASTNode* parseMemberAccess(ASTNode* object);  // メンバアクセス (.member)
    ASTNode* parseStructLiteral();                // 構造体リテラル {a: 1, b: "str"}
    ASTNode* parseEnumAccess();                   // enum値アクセス (EnumName::member)
    
    // Utility methods
    TypeInfo getTypeInfoFromString(const std::string& type_name);
    ASTNode* parseArrayLiteral();
    
    // struct管理
    std::unordered_map<std::string, StructDefinition> struct_definitions_; // struct定義の保存
    
    // enum管理  
    std::unordered_map<std::string, EnumDefinition> enum_definitions_; // enum定義の保存
    
public:
    // enum定義へのアクセサ
    const std::unordered_map<std::string, EnumDefinition>& get_enum_definitions() const {
        return enum_definitions_;
    }
    
    // struct定義へのアクセサ
    const std::unordered_map<std::string, StructDefinition>& get_struct_definitions() const {
        return struct_definitions_;
    }
};
