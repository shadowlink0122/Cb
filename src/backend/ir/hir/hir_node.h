#pragma once

#include "../../../common/ast.h"
#include "../common/ir_types.h"
#include <memory>
#include <string>
#include <vector>

namespace cb {
namespace ir {
namespace hir {

// v0.14.0: 簡略化されたHIR定義（初期実装）
// 将来的により洗練されたvariant-based設計に移行予定

// 型情報
struct HIRType {
    enum class TypeKind {
        Unknown,
        Void,
        // 符号付き整数型
        Tiny,  // 8-bit signed
        Short, // 16-bit signed
        Int,   // 32-bit signed
        Long,  // 64-bit signed
        // 符号なし整数型
        UnsignedTiny,  // 8-bit unsigned
        UnsignedShort, // 16-bit unsigned
        UnsignedInt,   // 32-bit unsigned (unsigned)
        UnsignedLong,  // 64-bit unsigned
        // その他のプリミティブ型
        Char,
        String,
        Bool,
        Float,
        Double,
        // 複合型
        Struct,
        Enum,
        Interface,
        Pointer,
        Reference,       // 参照型 (&T)
        RvalueReference, // 右辺値参照 (&&T)
        Array,
        Nullptr,
        Function, // 関数型
        Generic,  // ジェネリック型パラメータ (T, U, etc.)
        Optional, // Optional型 (T?)
        Result,   // Result型
    };

    TypeKind kind = TypeKind::Unknown;
    std::string name; // struct/enum/interface名など

    // ポインタ・参照・配列・Optional・Result用
    std::unique_ptr<HIRType> inner_type;

    // 多次元配列のサポート
    std::vector<int> array_dimensions; // 各次元のサイズ（-1 = 動的）
    int array_size = -1; // 後方互換性のため保持（1次元目のサイズ）

    // 関数型用
    std::vector<HIRType> param_types;
    std::unique_ptr<HIRType> return_type;

    // ジェネリック型パラメータ
    std::vector<HIRType> generic_args;

    // 修飾子
    bool is_const = false;         // const修飾子
    bool is_static = false;        // static修飾子
    bool is_volatile = false;      // volatile修飾子
    bool is_pointer_const = false; // const pointer (T* const)
    bool is_pointee_const = false; // pointer to const (const T*)

    // unsigned修飾子（TypeKindで表現されるが、互換性のため保持）
    bool is_unsigned = false;

    // コピーコンストラクタとムーブコンストラクタ
    HIRType() = default;
    HIRType(const HIRType &);
    HIRType(HIRType &&) = default;
    HIRType &operator=(const HIRType &);
    HIRType &operator=(HIRType &&) = default;
};

// 前方宣言
struct HIRExpr;
struct HIRStmt;
struct HIRFunction; // v0.14.0: ラムダ式で使用するため前方宣言

// HIR式
struct HIRExpr {
    enum class ExprKind {
        Literal,
        Variable,
        BinaryOp,
        UnaryOp,
        FunctionCall,
        MethodCall,
        MemberAccess,
        ArrayAccess,
        Cast,
        Ternary,
        Lambda,
        StructLiteral,
        ArrayLiteral,
        Block,
        AddressOf,       // &expr - アドレス取得
        Dereference,     // *expr - 間接参照
        SizeOf,          // sizeof(type) or sizeof(expr)
        New,             // new Type - メモリ確保
        Await,           // await expr - async/await
        PreIncDec,       // ++i, --i
        PostIncDec,      // i++, i--
        Range,           // start...end - 範囲式
        ErrorPropagation // expr? - エラー伝播演算子 (v0.12.1)
    };

    ExprKind kind;
    HIRType type;
    SourceLocation location;

    // リテラル
    std::string literal_value;
    HIRType literal_type;

    // 変数
    std::string var_name;

    // 二項演算・単項演算
    std::string op;
    std::unique_ptr<HIRExpr> left;
    std::unique_ptr<HIRExpr> right;
    std::unique_ptr<HIRExpr> operand;

    // 関数呼び出し
    std::string func_name;
    std::vector<HIRExpr> arguments;

    // メソッド呼び出し
    std::unique_ptr<HIRExpr> receiver;
    std::string method_name;

    // メンバーアクセス
    std::unique_ptr<HIRExpr> object;
    std::string member_name;
    bool is_arrow = false;

    // 配列アクセス
    std::unique_ptr<HIRExpr> array;
    std::unique_ptr<HIRExpr> index;

    // キャスト
    std::unique_ptr<HIRExpr> cast_expr;
    HIRType cast_type;

    // 三項演算子
    std::unique_ptr<HIRExpr> condition;
    std::unique_ptr<HIRExpr> then_expr;
    std::unique_ptr<HIRExpr> else_expr;

    // 構造体リテラル
    std::string struct_type_name;
    std::vector<std::string> field_names;
    std::vector<HIRExpr> field_values;

    // 配列リテラル
    std::vector<HIRExpr> array_elements;

    // ブロック式
    std::vector<HIRStmt> block_stmts;
    std::unique_ptr<HIRExpr> result_expr;

    // ラムダ (HIRFunctionを避けるため独自定義)
    struct LambdaParameter {
        std::string name;
        HIRType type;
        bool is_const = false;
        // TODO: デフォルト値は将来実装
        // std::unique_ptr<HIRExpr> default_value;
    };
    std::vector<LambdaParameter> lambda_params;
    HIRType lambda_return_type;
    std::unique_ptr<HIRStmt> lambda_body;

    // sizeof
    std::unique_ptr<HIRExpr> sizeof_expr;
    HIRType sizeof_type;

    // new
    HIRType new_type;
    std::vector<HIRExpr> new_args; // コンストラクタ引数

    // range (start...end)
    std::unique_ptr<HIRExpr> range_start;
    std::unique_ptr<HIRExpr> range_end;
};

// HIR文
struct HIRStmt {
    enum class StmtKind {
        VarDecl,
        Assignment,
        ExprStmt,
        If,
        While,
        For,
        Return,
        Break,
        Continue,
        Block,
        Match,
        Switch, // switch文
        Defer,  // defer文
        Delete, // delete文
        Try,    // try-catch文
        Throw,  // throw/エラー送出
        Assert  // assert文
    };

    StmtKind kind;
    SourceLocation location;

    // 変数宣言
    std::string var_name;
    HIRType var_type;
    bool is_const = false;
    std::unique_ptr<HIRExpr> init_expr;

    // 代入
    std::unique_ptr<HIRExpr> lhs;
    std::unique_ptr<HIRExpr> rhs;

    // 式文
    std::unique_ptr<HIRExpr> expr;

    // if文
    std::unique_ptr<HIRExpr> condition;
    std::unique_ptr<HIRStmt> then_body;
    std::unique_ptr<HIRStmt> else_body;

    // while/for文
    std::unique_ptr<HIRStmt> body;
    std::unique_ptr<HIRStmt> init;
    std::unique_ptr<HIRStmt> update;

    // return文
    std::unique_ptr<HIRExpr> return_expr;

    // ブロック
    std::vector<HIRStmt> block_stmts;

    // match文
    std::unique_ptr<HIRExpr> match_expr;
    struct MatchArm {
        enum class PatternKind {
            Wildcard,     // _
            Literal,      // 42, "hello", true
            Variable,     // x (変数束縛)
            EnumVariant,  // Some(x), None, Ok(value), Err(e)
            StructPattern // Point { x, y }
        };

        PatternKind pattern_kind;
        std::string pattern_name;          // 変数名またはvariant名
        std::vector<std::string> bindings; // パターン内の変数束縛
        std::unique_ptr<HIRExpr> guard;    // when節のガード条件
        std::vector<HIRStmt> body;         // アームの本体

        // Enum型の情報（型チェック用）
        std::string enum_type_name; // "Option<int>", "Result<int, string>"
    };
    std::vector<MatchArm> match_arms;

    // switch文
    std::unique_ptr<HIRExpr> switch_expr;
    struct SwitchCase {
        std::unique_ptr<HIRExpr> case_value; // nullptrの場合はdefault
        std::vector<HIRStmt> case_body;
    };
    std::vector<SwitchCase> switch_cases;

    // defer文
    std::unique_ptr<HIRStmt> defer_stmt;

    // delete文
    std::unique_ptr<HIRExpr> delete_expr;

    // try-catch
    std::vector<HIRStmt> try_block;
    struct CatchClause {
        std::string exception_var;
        HIRType exception_type;
        std::vector<HIRStmt> catch_body;
    };
    std::vector<CatchClause> catch_clauses;
    std::vector<HIRStmt> finally_block;

    // throw
    std::unique_ptr<HIRExpr> throw_expr;

    // assert
    std::unique_ptr<HIRExpr> assert_expr;
    std::string assert_message;
};

// HIR関数
struct HIRFunction {
    struct Parameter {
        std::string name;
        HIRType type;
        bool is_const = false;
        // デフォルト引数サポート (v0.14.0)
        std::unique_ptr<HIRExpr> default_value;
        bool has_default = false;

        // デフォルトコンストラクタ
        Parameter() = default;

        // ムーブコンストラクタとムーブ代入演算子を明示的に定義
        Parameter(Parameter &&) = default;
        Parameter &operator=(Parameter &&) = default;

        // コピーコンストラクタとコピー代入演算子を削除
        Parameter(const Parameter &) = delete;
        Parameter &operator=(const Parameter &) = delete;
    };

    std::string name;
    std::vector<Parameter> parameters;
    HIRType return_type;
    std::unique_ptr<HIRStmt> body;
    bool is_async = false;
    bool is_exported = false;
    std::vector<std::string>
        generic_params; // ジェネリック型パラメータ (T, U, etc.)
    SourceLocation location;

    // v0.14.0: 関数ポインタ型推論のサポート
    bool returns_function_pointer = false; // 関数ポインタを返すかどうか
    // TODO: Add function_pointer_signature field after fixing segmentation
    // fault
};

// HIR構造体
struct HIRStruct {
    struct Field {
        std::string name;
        HIRType type;
        bool is_private = false;
        bool is_default = false; // このフィールドがdefaultメンバーか
        // TODO: デフォルト値は将来実装
        // std::unique_ptr<HIRExpr> default_value;
    };

    std::string name;
    std::vector<Field> fields;
    std::vector<std::string> generic_params; // ジェネリック型パラメータ
    bool has_default_member = false; // デフォルトメンバーを持つか
    std::string default_member_name; // デフォルトメンバーの名前
    SourceLocation location;
};

// HIR Enum
struct HIREnum {
    struct Variant {
        std::string name;
        int64_t value;
        bool has_associated_value = false;
        HIRType associated_type;
    };

    std::string name;
    std::vector<Variant> variants;
    SourceLocation location;
};

// HIR Interface
struct HIRInterface {
    struct MethodSignature {
        std::string name;
        std::vector<HIRFunction::Parameter> parameters;
        HIRType return_type;

        // ムーブコンストラクタとムーブ代入演算子を明示的に定義
        MethodSignature() = default;
        MethodSignature(MethodSignature &&) = default;
        MethodSignature &operator=(MethodSignature &&) = default;

        // コピーコンストラクタとコピー代入演算子を削除
        MethodSignature(const MethodSignature &) = delete;
        MethodSignature &operator=(const MethodSignature &) = delete;
    };

    std::string name;
    std::vector<MethodSignature> methods;
    std::vector<std::string> generic_params; // ジェネリックパラメータ
    SourceLocation location;
    bool generate_value_type =
        true; // 値型interfaceも生成するか（デフォルト: true）
};

// HIR Global Variable (moved before HIRImpl for use in static_variables)
struct HIRGlobalVar {
    std::string name;
    HIRType type;
    bool is_const = false;
    bool is_exported = false;
    std::unique_ptr<HIRExpr> init_expr;
    SourceLocation location;
};

// HIR Impl
struct HIRImpl {
    std::string struct_name;
    std::string interface_name; // empty if not implementing an interface
    std::vector<HIRFunction> methods;
    std::vector<HIRGlobalVar> static_variables; // impl内のstatic変数
    std::vector<std::string> generic_params; // ジェネリックパラメータ
    SourceLocation location;
};

// HIR Typedef
struct HIRTypedef {
    std::string name;
    HIRType target_type;
    SourceLocation location;
};

// HIR Union (TypeScript-like literal/type unions)
struct HIRUnion {
    // Union variant can be a literal value or a type
    struct Variant {
        enum class Kind { LiteralInt, LiteralString, LiteralBool, Type };
        Kind kind;
        int64_t int_value;
        std::string string_value;
        bool bool_value;
        HIRType type; // For type variants (int | string | MyStruct)
    };

    std::string name;
    std::vector<Variant> variants;
    SourceLocation location;
};

// HIR Import
struct HIRImport {
    std::string module_path;
    std::vector<std::string> imported_names; // empty = import all
    SourceLocation location;
};

// v0.14.0: FFI (Foreign Function Interface)
struct HIRForeignFunction {
    std::string module_name; // "m", "c", etc.
    std::string function_name;
    HIRType return_type;
    std::vector<HIRFunction::Parameter> parameters;
    SourceLocation location;

    // C++生成時に使用
    std::string mangled_name() const {
        return module_name + "_" + function_name;
    }
};

// HIR Program
struct HIRProgram {
    std::vector<HIRFunction> functions;
    std::vector<HIRStruct> structs;
    std::vector<HIREnum> enums;
    std::vector<HIRInterface> interfaces;
    std::vector<HIRImpl> impls;
    std::vector<HIRTypedef> typedefs;
    std::vector<HIRUnion> unions;
    std::vector<HIRGlobalVar> global_vars;
    std::vector<HIRImport> imports;

    // v0.14.0: FFI support
    std::vector<HIRForeignFunction> foreign_functions;
};

} // namespace hir
} // namespace ir
} // namespace cb
