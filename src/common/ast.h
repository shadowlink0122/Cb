#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// 型定義
enum TypeInfo {
    TYPE_VOID = 0,
    TYPE_TINY = 1,
    TYPE_SHORT = 2,
    TYPE_INT = 3,
    TYPE_LONG = 4,
    TYPE_STRING = 5,
    TYPE_BOOL = 6,
    TYPE_ARRAY_BASE = 100 // 配列型は基底型 + 100
};

// 型名を文字列に変換する関数
const char *type_info_to_string(TypeInfo type);

// bool値を文字列に変換する関数
const char *bool_to_string(bool value);

// ASTノード種別
enum class ASTNodeType {
    // リテラル・変数
    AST_NUMBER,
    AST_VARIABLE,
    AST_STRING_LITERAL,

    // 演算子
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_ASSIGN,

    // 制御構造
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_BREAK_STMT,
    AST_RETURN_STMT,

    // 宣言
    AST_VAR_DECL,
    AST_ARRAY_DECL,
    AST_FUNC_DECL,
    AST_PARAM_DECL,

    // 式
    AST_FUNC_CALL,
    AST_ARRAY_REF,
    AST_PRE_INCDEC,
    AST_POST_INCDEC,

    // その他
    AST_STMT_LIST,
    AST_PRINT_STMT,
    AST_PRINTF_STMT,  // 新しいprintf風print
    AST_PRINT_MULTI_STMT,  // 複数引数print
    AST_COMPOUND_STMT,

    // コンパイラ拡張用
    AST_TYPE_SPEC,
    AST_STORAGE_SPEC,
    STORAGE_SPEC
};

// ASTノードの基底クラス
struct ASTNode {
    ASTNodeType node_type;
    TypeInfo type_info;

    // ストレージ属性
    bool is_const = false;
    bool is_static = false;

    // 値・名前
    int64_t int_value = 0;
    std::string str_value;
    std::string name;
    std::string op;

    // 子ノード
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::unique_ptr<ASTNode> condition;
    std::unique_ptr<ASTNode> init_expr;
    std::unique_ptr<ASTNode> update_expr;
    std::unique_ptr<ASTNode> body;

    // リスト（子ノード群）
    std::vector<std::unique_ptr<ASTNode>> children;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::vector<std::unique_ptr<ASTNode>> arguments;
    std::vector<std::unique_ptr<ASTNode>> statements;
    std::vector<TypeInfo> return_types;

    // 配列関連
    int array_size = -1;
    std::unique_ptr<ASTNode> array_index;
    std::unique_ptr<ASTNode> array_size_expr;

    // コンストラクタ
    ASTNode(ASTNodeType type) : node_type(type), type_info(TYPE_INT) {}

    // デストラクタは自動管理（unique_ptr使用）
    virtual ~ASTNode() = default;

    // コピー/ムーブ禁止（unique_ptrのため）
    ASTNode(const ASTNode &) = delete;
    ASTNode &operator=(const ASTNode &) = delete;
    ASTNode(ASTNode &&) = default;
    ASTNode &operator=(ASTNode &&) = default;
};

// 前方宣言
class Parser;
class Evaluator;
class CodeGenerator;

// パーサーインターフェース
class FrontendInterface {
  public:
    virtual ~FrontendInterface() = default;
    virtual std::unique_ptr<ASTNode> parse(const std::string &source) = 0;
    virtual std::unique_ptr<ASTNode>
    parse_file(const std::string &filename) = 0;
};

// バックエンドインターフェース
class BackendInterface {
  public:
    virtual ~BackendInterface() = default;
    virtual void process(const ASTNode *ast) = 0;
};

// 評価器インターフェース
class EvaluatorInterface : public BackendInterface {
  public:
    virtual int64_t evaluate(const ASTNode *node) = 0;
};

// コード生成器インターフェース
class CodeGeneratorInterface : public BackendInterface {
  public:
    virtual std::string generate_code(const ASTNode *ast) = 0;
    virtual void emit_to_file(const ASTNode *ast,
                              const std::string &filename) = 0;
};
