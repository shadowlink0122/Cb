#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// 型定義
enum TypeInfo {
    TYPE_UNKNOWN = -1,
    TYPE_VOID = 0,
    TYPE_TINY = 1,
    TYPE_SHORT = 2,
    TYPE_INT = 3,
    TYPE_LONG = 4,
    TYPE_CHAR = 5,
    TYPE_STRING = 6,
    TYPE_BOOL = 7,
    TYPE_ARRAY_BASE = 100 // 配列型は基底型 + 100（下位互換のため保持）
};

// 配列次元情報を格納する構造体
struct ArrayDimension {
    int size;        // 配列サイズ（-1は動的サイズ）
    bool is_dynamic; // 動的サイズかどうか

    ArrayDimension(int s = -1, bool dynamic = true)
        : size(s), is_dynamic(dynamic) {}
};

// 配列型情報を格納する構造体
struct ArrayTypeInfo {
    TypeInfo base_type;                     // 基底型
    std::vector<ArrayDimension> dimensions; // 各次元の情報

    ArrayTypeInfo() : base_type(TYPE_UNKNOWN) {}
    ArrayTypeInfo(TypeInfo base, const std::vector<ArrayDimension> &dims)
        : base_type(base), dimensions(dims) {}

    // 1次元配列として初期化
    ArrayTypeInfo(TypeInfo base, int size = -1, bool dynamic = true)
        : base_type(base) {
        dimensions.push_back(ArrayDimension(size, dynamic));
    }

    // 配列かどうかの判定
    bool is_array() const { return !dimensions.empty(); }

    // 次元数を取得
    size_t get_dimension_count() const { return dimensions.size(); }

    // 型情報文字列を生成（例: "int[10][20]"）
    std::string to_string() const;
};

// 型名を文字列に変換する関数
const char *type_info_to_string(TypeInfo type);
const char *type_info_to_string_basic(TypeInfo type);

// bool値を文字列に変換する関数
const char *bool_to_string(bool value);

// ASTノード種別
enum class ASTNodeType {
    // リテラル・変数
    AST_NUMBER,
    AST_VARIABLE,
    AST_STRING_LITERAL,
    AST_ARRAY_LITERAL,

    // 演算子
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_ASSIGN,

    // 制御構造
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_RETURN_STMT,

    // 宣言
    AST_VAR_DECL,
    AST_MULTIPLE_VAR_DECL, // 複数変数宣言
    AST_ARRAY_DECL,
    AST_FUNC_DECL,
    AST_PARAM_DECL,
    AST_TYPEDEF_DECL, // typedef宣言

    // 式
    AST_FUNC_CALL,
    AST_ARRAY_REF,
    AST_PRE_INCDEC,
    AST_POST_INCDEC,

    // その他
    AST_STMT_LIST,
    AST_PRINT_STMT,
    AST_PRINTLN_STMT,  // 改行付きprint
    AST_PRINTLN_EMPTY, // 引数なし改行のみ
    AST_PRINTF_STMT,   // 新しいprintf風print
    AST_PRINTLNF_STMT, // 改行付きprintf風print
    AST_COMPOUND_STMT,

    // コンパイラ拡張用
    AST_TYPE_SPEC,
    AST_STORAGE_SPEC,
    STORAGE_SPEC,

    // モジュールシステム
    AST_IMPORT_STMT, // import文
    AST_EXPORT_STMT, // export文
    AST_MODULE_DECL, // module宣言

    // 例外処理
    AST_TRY_STMT,     // try文
    AST_CATCH_STMT,   // catch文
    AST_FINALLY_STMT, // finally文
    AST_THROW_STMT    // throw文
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
    std::string type_name; // typedef名など、型の文字列表現
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
    std::vector<std::unique_ptr<ASTNode>>
        array_dimensions;          // 多次元配列の各次元のサイズ式
    ArrayTypeInfo array_type_info; // 詳細な配列型情報

    // モジュール関連
    std::string module_name;               // モジュール名 (std.io等)
    std::vector<std::string> import_items; // インポートする項目リスト
    bool is_exported = false;              // export宣言されているか

    // 例外処理関連
    std::unique_ptr<ASTNode> try_body;     // try block
    std::unique_ptr<ASTNode> catch_body;   // catch block
    std::unique_ptr<ASTNode> finally_body; // finally block
    std::unique_ptr<ASTNode> throw_expr;   // throw expression
    std::string exception_var;             // catch変数名
    std::string exception_type;            // 例外型名

    // 関数呼び出し関連（修飾名対応）
    std::string qualified_name;     // module.function形式の修飾名
    bool is_qualified_call = false; // 修飾された関数呼び出しか

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
