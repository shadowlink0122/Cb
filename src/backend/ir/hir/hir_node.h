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
        Tiny,
        Short,
        Int,
        Long,
        Char,
        String,
        Bool,
        Float,
        Double,
        Struct,
        Enum,
        Interface,
        Pointer,
        Reference, // 参照型 (&T)
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

    // 配列サイズ（固定長配列の場合）
    int array_size = -1; // -1 = 動的配列

    // 関数型用
    std::vector<HIRType> param_types;
    std::unique_ptr<HIRType> return_type;

    // ジェネリック型パラメータ
    std::vector<HIRType> generic_args;

    // const修飾子
    bool is_const = false;

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
        AddressOf,   // &expr - アドレス取得
        Dereference, // *expr - 間接参照
        SizeOf,      // sizeof(type) or sizeof(expr)
        New,         // new Type - メモリ確保
        Await,       // await expr - async/await
        PreIncDec,   // ++i, --i
        PostIncDec   // i++, i--
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
        // TODO: デフォルト引数は将来実装
        // std::unique_ptr<HIRExpr> default_value;
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
};

// HIR構造体
struct HIRStruct {
    struct Field {
        std::string name;
        HIRType type;
        bool is_private = false;
        // TODO: デフォルト値は将来実装
        // std::unique_ptr<HIRExpr> default_value;
    };

    std::string name;
    std::vector<Field> fields;
    std::vector<std::string> generic_params; // ジェネリック型パラメータ
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
    };

    std::string name;
    std::vector<MethodSignature> methods;
    SourceLocation location;
};

// HIR Impl
struct HIRImpl {
    std::string struct_name;
    std::string interface_name; // empty if not implementing an interface
    std::vector<HIRFunction> methods;
    std::vector<std::string> generic_params; // ジェネリックパラメータ
    SourceLocation location;
};

// HIR Typedef
struct HIRTypedef {
    std::string name;
    HIRType target_type;
    SourceLocation location;
};

// HIR Global Variable
struct HIRGlobalVar {
    std::string name;
    HIRType type;
    bool is_const = false;
    bool is_exported = false;
    std::unique_ptr<HIRExpr> init_expr;
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
    std::vector<HIRGlobalVar> global_vars;
    std::vector<HIRImport> imports;

    // v0.14.0: FFI support
    std::vector<HIRForeignFunction> foreign_functions;
};

} // namespace hir
} // namespace ir
} // namespace cb
