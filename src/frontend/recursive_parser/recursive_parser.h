#pragma once
#include "recursive_lexer.h"
#include "../../common/ast.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace RecursiveParserNS;

// 前方宣言 - 分離されたパーサークラス
class ExpressionParser;
class StatementParser;
class DeclarationParser;
class TypeParser;
class StructParser;

struct ParsedTypeInfo {
    std::string full_type;                 // 完全な型表現（ポインタ/配列含む）
    std::string base_type;                 // 基本型（typedef解決後）
    std::string original_type;             // typedef解決前の型名
    bool is_pointer = false;               // ポインタ型かどうか
    int pointer_depth = 0;                 // ポインタの深さ
    TypeInfo base_type_info = TYPE_UNKNOWN;// 基本型のTypeInfo
    bool is_array = false;                 // 配列型かどうか
    ArrayTypeInfo array_info;              // 配列情報（多次元対応）
    bool is_reference = false;             // 参照型かどうか
    bool is_unsigned = false;              // unsigned修飾子かどうか
    bool is_const = false;                 // const修飾子かどうか
};

class RecursiveParser {
    // 分離されたパーサークラスが内部状態にアクセスできるようにする
    friend class ExpressionParser;
    friend class StatementParser;
    friend class DeclarationParser;
    friend class VariableDeclarationParser;
    friend class PrimaryExpressionParser;
    friend class TypeParser;
    friend class StructParser;
public:
    RecursiveParser(const std::string& source, const std::string& filename = "");
    ~RecursiveParser();  // 明示的なデストラクタ宣言（unique_ptrの不完全型対応）
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
    
    // 分離されたパーサーのインスタンス
    std::unique_ptr<ExpressionParser> expression_parser_;
    std::unique_ptr<StatementParser> statement_parser_;
    std::unique_ptr<DeclarationParser> declaration_parser_;
    std::unique_ptr<TypeParser> type_parser_;
    std::unique_ptr<StructParser> struct_parser_;
    
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
    const ParsedTypeInfo& getLastParsedTypeInfo() const { return last_parsed_type_info_; }
    TypeInfo resolveParsedTypeInfo(const ParsedTypeInfo& parsed) const;
    ASTNode* parseTypedefDeclaration();
    ASTNode* parseStructDeclaration();           // struct宣言
    ASTNode* parseStructTypedefDeclaration();    // typedef struct宣言
    ASTNode* parseEnumDeclaration();             // enum宣言
    ASTNode* parseEnumTypedefDeclaration();      // typedef enum宣言
    ASTNode* parseUnionTypedefDeclaration();     // typedef union宣言 (TypeScript-like literal types)
    ASTNode* parseFunctionPointerTypedefDeclaration(); // 関数ポインタtypedef宣言
    ASTNode* parseInterfaceDeclaration();        // interface宣言
    ASTNode* parseImplDeclaration();             // impl宣言
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
    ASTNode* parseAssertStatement();
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
    ASTNode* parseArrowAccess(ASTNode* object);   // アロー演算子アクセス (ptr->member)
    ASTNode* parseStructLiteral();                // 構造体リテラル {a: 1, b: "str"}
    ASTNode* parseEnumAccess();                   // enum値アクセス (EnumName::member)
    
    // Utility methods
    ASTNode* cloneAstNode(const ASTNode* node);
    TypeInfo getTypeInfoFromString(const std::string& type_name);
    ASTNode* parseArrayLiteral();
    
    // struct管理
    std::unordered_map<std::string, StructDefinition> struct_definitions_; // struct定義の保存
    
    // enum管理  
    std::unordered_map<std::string, EnumDefinition> enum_definitions_; // enum定義の保存
    
    // union管理 (TypeScript-like literal types)
    std::unordered_map<std::string, UnionDefinition> union_definitions_; // union定義の保存
    
    // interface管理
    std::unordered_map<std::string, InterfaceDefinition> interface_definitions_; // interface定義の保存
    
    // impl管理
    std::vector<ImplDefinition> impl_definitions_; // impl定義の保存
    
    // 関数ポインタtypedef管理
    std::unordered_map<std::string, FunctionPointerTypeInfo> function_pointer_typedefs_; // 関数ポインタtypedef定義の保存
    
    // Union parsing helper
    bool parseUnionValue(UnionDefinition& union_def);
    
    // 関数ポインタ用ヘルパー
    bool isFunctionPointerTypedef();  // 関数ポインタtypedef構文かチェック
    bool isFunctionPointerType(const std::string& type_name) const {
        return function_pointer_typedefs_.find(type_name) != function_pointer_typedefs_.end();
    }
    const FunctionPointerTypeInfo& getFunctionPointerTypeInfo(const std::string& type_name) const {
        return function_pointer_typedefs_.at(type_name);
    }
    
    // 直近に解析した型情報
    ParsedTypeInfo last_parsed_type_info_;
    
public:
    // enum定義へのアクセサ
    const std::unordered_map<std::string, EnumDefinition>& get_enum_definitions() const {
        return enum_definitions_;
    }
    
    // struct定義へのアクセサ
    const std::unordered_map<std::string, StructDefinition>& get_struct_definitions() const {
        return struct_definitions_;
    }
    
    // interface定義へのアクセサ
    const std::unordered_map<std::string, InterfaceDefinition>& get_interface_definitions() const {
        return interface_definitions_;
    }
    
    // impl定義へのアクセサ
    const std::vector<ImplDefinition>& get_impl_definitions() const {
        return impl_definitions_;
    }
    
    // union定義へのアクセサ
    const std::unordered_map<std::string, UnionDefinition>& get_union_definitions() const {
        return union_definitions_;
    }
    
    // 関数ポインタtypedef定義へのアクセサ
    const std::unordered_map<std::string, FunctionPointerTypeInfo>& get_function_pointer_typedefs() const {
        return function_pointer_typedefs_;
    }
    
private:
    // 循環参照検出用のヘルパー関数
    bool detectCircularReference(const std::string& struct_name, 
                                 const std::string& member_type,
                                 std::unordered_set<std::string>& visited,
                                 std::vector<std::string>& path);
};
