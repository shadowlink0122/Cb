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
        Array,
        Nullptr,
    };

    TypeKind kind = TypeKind::Unknown;
    std::string name; // struct/enum/interface名など
};

// 前方宣言
struct HIRExpr;
struct HIRStmt;

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
        Block
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
        Match
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
};

// HIR関数
struct HIRFunction {
    struct Parameter {
        std::string name;
        HIRType type;
        bool is_const = false;
    };

    std::string name;
    std::vector<Parameter> parameters;
    HIRType return_type;
    std::unique_ptr<HIRStmt> body;
    bool is_async = false;
    SourceLocation location;
};

// HIR構造体
struct HIRStruct {
    struct Field {
        std::string name;
        HIRType type;
        bool is_private = false;
    };

    std::string name;
    std::vector<Field> fields;
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
    SourceLocation location;
};

// HIR Program
struct HIRProgram {
    std::vector<HIRFunction> functions;
    std::vector<HIRStruct> structs;
    std::vector<HIREnum> enums;
    std::vector<HIRInterface> interfaces;
    std::vector<HIRImpl> impls;
};

} // namespace hir
} // namespace ir
} // namespace cb
