#pragma once
#include <cstdint>
#include <iostream>
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
    TYPE_FLOAT = 8,
    TYPE_DOUBLE = 9,
    TYPE_BIG = 10,
    TYPE_QUAD = 11,
    TYPE_STRUCT = 12,
    TYPE_ENUM = 13,
    TYPE_INTERFACE = 14,
    TYPE_UNION = 15,   // Union型（リテラル型・複合型）
    TYPE_POINTER = 16, // ポインタ型
    TYPE_NULLPTR = 17, // nullptr型
    TYPE_ARRAY_BASE = 100 // 配列型は基底型 + 100（下位互換のため保持）
};

// 配列次元情報を格納する構造体
struct ArrayDimension {
    int size;              // 配列サイズ（-1は動的サイズ）
    bool is_dynamic;       // 動的サイズかどうか
    std::string size_expr; // サイズ式（定数識別子など）

    ArrayDimension(int s = -1, bool dynamic = true,
                   const std::string &expr = "")
        : size(s), is_dynamic(dynamic), size_expr(expr) {}
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

// enum定義情報を格納する構造体
struct EnumMember {
    std::string name;    // enum メンバー名
    int64_t value;       // enum メンバーの値
    bool explicit_value; // 明示的に値が指定されたかどうか

    EnumMember() : value(0), explicit_value(false) {}
    EnumMember(const std::string &n, int64_t v, bool explicit_val = true)
        : name(n), value(v), explicit_value(explicit_val) {}
};

struct EnumDefinition {
    std::string name;                // enum名
    std::vector<EnumMember> members; // enumメンバーのリスト

    EnumDefinition() {}
    EnumDefinition(const std::string &n) : name(n) {}

    // メンバを追加
    void add_member(const std::string &member_name, int64_t value,
                    bool explicit_value = true) {
        members.emplace_back(member_name, value, explicit_value);
    }

    // メンバを名前で検索
    const EnumMember *find_member(const std::string &member_name) const {
        for (const auto &member : members) {
            if (member.name == member_name) {
                return &member;
            }
        }
        return nullptr;
    }

    // 値の重複チェック
    bool has_duplicate_values() const {
        for (size_t i = 0; i < members.size(); ++i) {
            for (size_t j = i + 1; j < members.size(); ++j) {
                if (members[i].value == members[j].value) {
                    return true;
                }
            }
        }
        return false;
    }
};

// Union型の値を格納する構造体（リテラル型・複合型用）
struct UnionValue {
    TypeInfo value_type;      // 値の型
    int64_t int_value;        // 整数値
    std::string string_value; // 文字列値
    bool bool_value;          // ブール値

    UnionValue() : value_type(TYPE_UNKNOWN), int_value(0), bool_value(false) {}

    // 整数値用コンストラクタ
    UnionValue(int64_t val)
        : value_type(TYPE_INT), int_value(val), bool_value(false) {}

    // 文字値用コンストラクタ
    UnionValue(char val)
        : value_type(TYPE_CHAR), int_value(static_cast<int64_t>(val)),
          bool_value(false) {}

    // 文字列値用コンストラクタ
    UnionValue(const std::string &val)
        : value_type(TYPE_STRING), int_value(0), string_value(val),
          bool_value(false) {}

    // ブール値用コンストラクタ
    UnionValue(bool val)
        : value_type(TYPE_BOOL), int_value(0), bool_value(val) {}

    // 型指定コンストラクタ
    UnionValue(TypeInfo type, int64_t val)
        : value_type(type), int_value(val), bool_value(false) {}
    UnionValue(TypeInfo type, const std::string &val)
        : value_type(type), int_value(0), string_value(val), bool_value(false) {
    }

    // 等価性チェック
    bool equals(const UnionValue &other) const {
        if (value_type != other.value_type)
            return false;

        switch (value_type) {
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_SHORT:
        case TYPE_TINY:
        case TYPE_CHAR:
            return int_value == other.int_value;
        case TYPE_STRING:
            return string_value == other.string_value;
        case TYPE_BOOL:
            return bool_value == other.bool_value;
        default:
            return false;
        }
    }
};

// Union型定義（TypeScriptライクなリテラル型・複合型）
struct UnionDefinition {
    std::string name;                       // union型名
    std::vector<UnionValue> allowed_values; // リテラル値（1 | 2 | "A"など）
    std::vector<TypeInfo> allowed_types; // 許可される型（int | stringなど）
    std::vector<std::string>
        allowed_custom_types; // カスタム型（struct名、typedef名など）
    std::vector<std::string>
        allowed_array_types; // 配列型（int[]、MyStruct[]など）
    bool has_literal_values; // リテラル値があるか
    bool has_type_values;    // 型値があるか
    bool has_custom_types;   // カスタム型があるか
    bool has_array_types;    // 配列型があるか

    UnionDefinition()
        : has_literal_values(false), has_type_values(false),
          has_custom_types(false), has_array_types(false) {}
    UnionDefinition(const std::string &n)
        : name(n), has_literal_values(false), has_type_values(false),
          has_custom_types(false), has_array_types(false) {}

    // コピーコンストラクタ（デバッグ用）
    UnionDefinition(const UnionDefinition &other)
        : name(other.name), allowed_values(other.allowed_values),
          allowed_types(other.allowed_types),
          allowed_custom_types(other.allowed_custom_types),
          allowed_array_types(other.allowed_array_types),
          has_literal_values(other.has_literal_values),
          has_type_values(other.has_type_values),
          has_custom_types(other.has_custom_types),
          has_array_types(other.has_array_types) {}

    // 代入演算子（デバッグ用）
    UnionDefinition &operator=(const UnionDefinition &other) {
        if (this != &other) {
            name = other.name;
            allowed_values = other.allowed_values;
            allowed_types = other.allowed_types;
            allowed_custom_types = other.allowed_custom_types;
            allowed_array_types = other.allowed_array_types;
            has_literal_values = other.has_literal_values;
            has_type_values = other.has_type_values;
            has_custom_types = other.has_custom_types;
            has_array_types = other.has_array_types;
        }
        return *this;
    }

    // リテラル値を追加
    void add_literal_value(const UnionValue &value) {
        allowed_values.push_back(value);
        has_literal_values = true;
    }

    // 許可する型を追加（複合型用）
    void add_allowed_type(TypeInfo type) {
        allowed_types.push_back(type);
        has_type_values = true;
    }

    // カスタム型を追加（struct、typedef等）
    void add_allowed_custom_type(const std::string &type_name) {
        allowed_custom_types.push_back(type_name);
        has_custom_types = true;
    }

    // 配列型を追加（int[]、MyStruct[]等）
    void add_allowed_array_type(const std::string &array_type) {
        allowed_array_types.push_back(array_type);
        has_array_types = true;
    }

    // 混合unionかどうか
    bool is_mixed_union() const {
        int type_count = 0;
        if (has_literal_values)
            type_count++;
        if (has_type_values)
            type_count++;
        if (has_custom_types)
            type_count++;
        if (has_array_types)
            type_count++;
        return type_count > 1;
    }

    // 純粋なリテラルunionかどうか
    bool is_literal_union() const {
        return has_literal_values && !has_type_values && !has_custom_types &&
               !has_array_types;
    }

    // 純粋な型unionかどうか
    bool is_type_union() const {
        return !has_literal_values && has_type_values && !has_custom_types &&
               !has_array_types;
    }

    // カスタム型のみのunionかどうか
    bool is_custom_type_union() const {
        return !has_literal_values && !has_type_values && has_custom_types &&
               !has_array_types;
    }

    // 配列型のみのunionかどうか
    bool is_array_type_union() const {
        return !has_literal_values && !has_type_values && !has_custom_types &&
               has_array_types;
    }

    // 値が許可されているかチェック（リテラル値）
    bool is_literal_value_allowed(const UnionValue &value) const {
        if (!has_literal_values)
            return false;

        for (const auto &allowed : allowed_values) {
            if (allowed.equals(value)) {
                return true;
            }
        }
        return false;
    }

    // 型が許可されているかチェック（複合型）
    bool is_type_allowed(TypeInfo type) const {
        // リテラル値から推定される型をチェック
        if (has_literal_values) {
            for (const auto &allowed : allowed_values) {
                if (allowed.value_type == type) {
                    return true;
                }
            }
        }

        // 明示的な型をチェック
        if (has_type_values) {
            for (TypeInfo allowed_type : allowed_types) {
                if (allowed_type == type) {
                    return true;
                }
            }
        }

        return false;
    }

    // カスタム型が許可されているかチェック
    bool is_custom_type_allowed(const std::string &type_name) const {
        if (has_custom_types) {
            for (const auto &allowed_type : allowed_custom_types) {
                if (allowed_type == type_name) {
                    return true;
                }
            }
        }
        return false;
    }

    // 配列型が許可されているかチェック
    bool is_array_type_allowed(const std::string &array_type) const {
        if (has_array_types) {
            for (const auto &allowed_array : allowed_array_types) {
                if (allowed_array == array_type) {
                    return true;
                }
            }
        }
        return false;
    }

    // 値が許可されているかチェック（統合）
    bool is_value_allowed(TypeInfo type, int64_t int_val) const {
        UnionValue value(type, int_val);
        // リテラル値チェック
        if (has_literal_values && is_literal_value_allowed(value)) {
            return true;
        }
        // 型チェック
        return is_type_allowed(type);
    }

    bool is_value_allowed(TypeInfo type, const std::string &str_val) const {
        UnionValue value(type, str_val);
        // リテラル値チェック
        if (has_literal_values && is_literal_value_allowed(value)) {
            return true;
        }
        // 型チェック
        return is_type_allowed(type);
    }

    bool is_value_allowed(TypeInfo type, bool bool_val) const {
        UnionValue value(bool_val);
        // リテラル値チェック
        if (has_literal_values && is_literal_value_allowed(value)) {
            return true;
        }
        // 型チェック
        return is_type_allowed(type);
    }
};

// struct定義情報を格納する構造体
struct StructMember {
    std::string name;         // メンバ変数名
    TypeInfo type;            // メンバ型
    ArrayTypeInfo array_info; // 配列の場合の詳細情報
    std::string type_alias;   // typedef型の場合のエイリアス名
    bool is_pointer = false;  // ポインタメンバかどうか
    int pointer_depth = 0;    // ポインタの深さ
    std::string pointer_base_type_name;        // ポインタの基底型名
    TypeInfo pointer_base_type = TYPE_UNKNOWN; // ポインタ基底型
    bool is_private = false;                   // private指定かどうか
    bool is_reference = false;                 // 参照メンバかどうか
    bool is_unsigned = false; // unsigned修飾子が付与されているか
    bool is_const = false; // const指定かどうか（Rustのnot mutと同等）

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
                    const std::string &type_alias = "", bool is_pointer = false,
                    int pointer_depth = 0,
                    const std::string &pointer_base_type_name = "",
                    TypeInfo pointer_base_type = TYPE_UNKNOWN,
                    bool is_private = false, bool is_reference = false,
                    bool is_unsigned = false, bool is_const = false) {
        StructMember member(member_name, type, type_alias);
        member.is_pointer = is_pointer;
        member.pointer_depth = pointer_depth;
        member.pointer_base_type_name = pointer_base_type_name;
        member.pointer_base_type = pointer_base_type;
        member.is_private = is_private;
        member.is_reference = is_reference;
        member.is_unsigned = is_unsigned;
        member.is_const = is_const;
        members.emplace_back(std::move(member));
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

// 前方宣言
struct ASTNode;

// Interface定義情報を格納する構造体
struct InterfaceMember {
    std::string name;                // 関数名
    TypeInfo return_type;            // 戻り値の型
    bool return_is_unsigned = false; // 戻り値がunsignedかどうか
    std::vector<std::pair<std::string, TypeInfo>>
        parameters; // パラメータのリスト (名前, 型)
    std::vector<bool> parameter_is_unsigned; // 各パラメータがunsignedかどうか

    InterfaceMember() : return_type(TYPE_UNKNOWN) {}
    InterfaceMember(const std::string &n, TypeInfo ret_type,
                    bool ret_unsigned = false)
        : name(n), return_type(ret_type), return_is_unsigned(ret_unsigned) {}

    void add_parameter(const std::string &param_name, TypeInfo param_type,
                       bool is_unsigned = false) {
        parameters.emplace_back(param_name, param_type);
        parameter_is_unsigned.push_back(is_unsigned);
    }

    bool get_parameter_is_unsigned(size_t index) const {
        if (index >= parameter_is_unsigned.size()) {
            return false;
        }
        return parameter_is_unsigned[index];
    }
};

struct InterfaceDefinition {
    std::string name;                     // interface名
    std::vector<InterfaceMember> methods; // メソッドのリスト

    InterfaceDefinition() {}
    InterfaceDefinition(const std::string &n) : name(n) {}

    // メソッドを追加
    void add_method(const std::string &method_name, TypeInfo return_type) {
        methods.emplace_back(method_name, return_type);
    }

    // メソッドを名前で検索
    const InterfaceMember *find_method(const std::string &method_name) const {
        for (const auto &method : methods) {
            if (method.name == method_name) {
                return &method;
            }
        }
        return nullptr;
    }
};

// Impl定義情報を格納する構造体
struct ImplDefinition {
    std::string interface_name; // 実装するinterface名
    std::string struct_name;    // 実装先のstruct名
    std::vector<const ASTNode *>
        methods; // 実装されたメソッドのASTノード（非所有ポインタ）

    ImplDefinition() {}
    ImplDefinition(const std::string &iface, const std::string &struct_name)
        : interface_name(iface), struct_name(struct_name) {}

    // デフォルトコピー/ムーブで十分（vector<const ASTNode*> はコピー可能）
    ImplDefinition(const ImplDefinition &) = default;
    ImplDefinition &operator=(const ImplDefinition &) = default;
    ImplDefinition(ImplDefinition &&) noexcept = default;
    ImplDefinition &operator=(ImplDefinition &&) noexcept = default;

    void add_method(const ASTNode *method_ast) {
        methods.push_back(method_ast);
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
    AST_NULLPTR, // nullptr リテラル

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
    AST_ENUM_DECL,           // enum宣言
    AST_ENUM_TYPEDEF_DECL,   // typedef enum宣言
    AST_UNION_TYPEDEF_DECL, // typedef union宣言 (TypeScript-like literal types)
    AST_INTERFACE_DECL,     // interface宣言
    AST_IMPL_DECL,          // impl宣言
    AST_ENUM_ACCESS,        // enum値アクセス (EnumName::member)

    // 式
    AST_FUNC_CALL,
    AST_ARRAY_REF,
    AST_ARRAY_SLICE, // 配列スライス (arr[0])
    AST_ARRAY_COPY,  // 配列コピー
    AST_PRE_INCDEC,
    AST_POST_INCDEC,
    AST_MEMBER_ACCESS,       // メンバアクセス (struct.member)
    AST_ARROW_ACCESS,        // アロー演算子アクセス (ptr->member)
    AST_MEMBER_ARRAY_ACCESS, // メンバの配列アクセス (struct.member[index])
    AST_STRUCT_LITERAL,      // 構造体リテラル {a: 1, b: "str"}
    AST_IDENTIFIER,          // 識別子（変数名、self等）

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
    AST_THROW_STMT,   // throw文

    // デバッグ・検証
    AST_ASSERT_STMT // assert文
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
    bool is_array = false;              // 配列パラメータフラグ
    bool is_array_return = false;       // 配列戻り値フラグ
    bool is_private_method = false;     // privateメソッドフラグ
    bool is_private_member = false;     // struct privateメンバフラグ
    bool is_pointer = false;            // ポインタ型フラグ
    int pointer_depth = 0;              // ポインタの深さ
    std::string pointer_base_type_name; // ポインタ基底型名
    TypeInfo pointer_base_type = TYPE_UNKNOWN; // ポインタ基底型
    bool is_reference = false;                 // 参照型フラグ
    bool is_unsigned = false;                  // unsigned修飾子

    // 値・名前
    int64_t int_value = 0;     // 整数リテラル値
    double double_value = 0.0; // 浮動小数点リテラル値（float/double）
    long double quad_value = 0.0L; // 128bit 浮動小数点リテラル値
    bool is_float_literal = false; // 浮動小数点リテラルかどうか
    TypeInfo literal_type = TYPE_UNKNOWN; // リテラル固有の型
    std::string literal_text;             // 元のリテラル文字列表現
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
    bool is_arrow_call = false; // アロー演算子経由の呼び出しか

    // enum関連
    std::string enum_name;          // enum型名 (Job::a の Job部分)
    std::string enum_member;        // enumメンバー名 (Job::a の a部分)
    EnumDefinition enum_definition; // enum定義情報（AST_ENUM_DECLノード用）

    // union関連（TypeScript-like literal types）
    std::string union_name; // union型名
    UnionDefinition
        union_definition; // union定義情報（AST_UNION_TYPEDEF_DECLノード用）

    // ネストしたメンバーアクセス用（obj.member.submember対応）
    std::vector<std::string> member_chain; // メンバーアクセスチェーン

    // コンストラクタ - 全フィールドの明示的初期化
    ASTNode(ASTNodeType type)
        : node_type(type), type_info(TYPE_INT), is_const(false),
          is_static(false), is_array(false), is_array_return(false),
          is_private_method(false), int_value(0), array_size(-1),
          is_exported(false), is_qualified_call(false) {}

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
