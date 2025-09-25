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
    TYPE_STRUCT = 8,
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

    // ArrayTypeInfoから単一の型IDを生成（TYPE_ARRAY_BASEベースのレガシー互換性用）
    TypeInfo to_legacy_type_id() const;
};

// struct定義情報を格納する構造体
struct StructMember {
    std::string name;         // メンバ変数名
    TypeInfo type;            // メンバ型
    ArrayTypeInfo array_info; // 配列の場合の詳細情報
    std::string type_alias;   // typedef型の場合のエイリアス名

    StructMember() : type(TYPE_UNKNOWN) {}
    StructMember(const std::string &n, TypeInfo t,
                 const std::string &alias = "")
        : name(n), type(t), type_alias(alias) {}
};

struct StructDefinition {
    std::string name;                  // struct名
    std::vector<StructMember> members; // メンバ変数のリスト

    StructDefinition() {}
    StructDefinition(const std::string &n) : name(n) {}

    // メンバを追加
    void add_member(const std::string &member_name, TypeInfo type,
                    const std::string &type_alias = "") {
        members.emplace_back(member_name, type, type_alias);
    }

    // メンバを名前で検索
    const StructMember *find_member(const std::string &member_name) const {
        for (const auto &member : members) {
            if (member.name == member_name) {
                return &member;
            }
        }
        return nullptr;
    }
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
    AST_TERNARY_OP, // 三項演算子 condition ? value1 : value2
    AST_ASSIGN,
    AST_ARRAY_ASSIGN, // 配列代入 (arr1 = arr2)

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
    AST_TYPEDEF_DECL,        // typedef宣言
    AST_STRUCT_DECL,         // struct宣言
    AST_STRUCT_TYPEDEF_DECL, // typedef struct宣言

    // 式
    AST_FUNC_CALL,
    AST_ARRAY_REF,
    AST_ARRAY_SLICE, // 配列スライス (arr[0])
    AST_ARRAY_COPY,  // 配列コピー
    AST_PRE_INCDEC,
    AST_POST_INCDEC,
    AST_MEMBER_ACCESS,       // メンバアクセス (struct.member)
    AST_MEMBER_ARRAY_ACCESS, // メンバの配列アクセス (struct.member[index])
    AST_STRUCT_LITERAL,      // 構造体リテラル {a: 1, b: "str"}

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

// 位置情報構造体
struct SourceLocation {
    std::string filename;
    int line = 0;
    int column = 0;
    std::string source_line; // 該当行の内容

    SourceLocation() = default;
    SourceLocation(const std::string &file, int l, int c,
                   const std::string &line = "")
        : filename(file), line(l), column(c), source_line(line) {}

    std::string to_string() const {
        return filename + ":" + std::to_string(line) + ":" +
               std::to_string(column);
    }
};

// ASTノードの基底クラス
struct ASTNode {
    ASTNodeType node_type;
    TypeInfo type_info;

    // 位置情報
    SourceLocation location;

    // ストレージ属性
    bool is_const = false;
    bool is_static = false;
    bool is_array = false;        // 配列パラメータフラグ
    bool is_array_return = false; // 配列戻り値フラグ

    // 値・名前
    int64_t int_value = 0;
    std::string str_value;
    std::string name;
    std::string type_name; // typedef名など、型の文字列表現
    std::string return_type_name; // 戻り値型の文字列表現（配列型対応）
    std::string op;

    // 子ノード
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;
    std::unique_ptr<ASTNode> third; // 三項演算子用の第三オペランド
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

    // 多次元配列アクセス用
    std::vector<std::unique_ptr<ASTNode>>
        array_indices; // 多次元配列インデックス [i][j][k]

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
