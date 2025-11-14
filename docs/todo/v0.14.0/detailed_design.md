# v0.16.0 詳細設計書

**バージョン**: v0.16.0
**作成日**: 2025-11-13
**ステータス**: 設計中

---

## 目次

1. [HIR実装の詳細設計](#1-hir実装の詳細設計)
2. [MIR実装の詳細設計](#2-mir実装の詳細設計)
3. [LIR実装の詳細設計](#3-lir実装の詳細設計)
4. [データフロー解析の詳細設計](#4-データフロー解析の詳細設計)
5. [IRビューワーの詳細設計](#5-irビューワーの詳細設計)
6. [複数バックエンド対応の設計](#6-複数バックエンド対応の設計)
7. [WASM対応の詳細設計](#7-wasm対応の詳細設計)
8. [TypeScript変換の詳細設計](#8-typescript変換の詳細設計)

---

## 1. HIR実装の詳細設計

### 1.1 HIRノード構造の詳細

#### ファイル: `src/backend/ir/hir/hir_node.h`

```cpp
#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include "common/ast.h"
#include "common/type_system.h"

namespace cb {
namespace ir {
namespace hir {

// 前方宣言
struct HIRExpr;
struct HIRStmt;
struct HIRPattern;

// ===== HIR式ノード =====

enum class HIRExprKind {
    Literal,          // リテラル
    Variable,         // 変数参照
    BinaryOp,         // 二項演算
    UnaryOp,          // 単項演算
    FunctionCall,     // 関数呼び出し
    MethodCall,       // メソッド呼び出し
    MemberAccess,     // メンバーアクセス
    ArrayAccess,      // 配列アクセス
    Cast,             // 型キャスト
    Ternary,          // 三項演算子
    Lambda,           // 無名関数
    StructLiteral,    // 構造体リテラル
    ArrayLiteral,     // 配列リテラル
    Block,            // ブロック式
};

// リテラル
struct HIRLiteral {
    enum class Kind {
        Int,
        Float,
        Double,
        Char,
        String,
        Bool,
        Null,
    };
    Kind kind;
    std::variant<int64_t, double, char, std::string, bool> value;
};

// 変数参照
struct HIRVariable {
    std::string name;                // 変数名
    uint32_t local_id;               // ローカル変数ID（HIR生成時に割り当て）
    bool is_mutable;                 // 可変かどうか
};

// 二項演算
struct HIRBinaryOp {
    enum class Op {
        Add, Sub, Mul, Div, Mod,
        Eq, Ne, Lt, Le, Gt, Ge,
        And, Or,
        BitAnd, BitOr, BitXor,
        Shl, Shr,
    };
    Op op;
    std::unique_ptr<HIRExpr> left;
    std::unique_ptr<HIRExpr> right;
};

// 単項演算
struct HIRUnaryOp {
    enum class Op {
        Neg,              // -x
        Not,              // !x
        BitNot,           // ~x
        Deref,            // *x
        AddressOf,        // &x
        PreInc,           // ++x
        PreDec,           // --x
        PostInc,          // x++
        PostDec,          // x--
    };
    Op op;
    std::unique_ptr<HIRExpr> operand;
};

// 関数呼び出し
struct HIRFunctionCall {
    std::string function_name;                  // 関数名
    std::vector<std::unique_ptr<HIRExpr>> args; // 引数
    std::vector<TypeInfo> type_args;            // 型引数（ジェネリクス用）
    bool is_generic_instantiation;              // ジェネリクスのインスタンス化か
};

// メソッド呼び出し
struct HIRMethodCall {
    std::unique_ptr<HIRExpr> receiver;          // レシーバー（self）
    std::string method_name;                    // メソッド名
    std::vector<std::unique_ptr<HIRExpr>> args; // 引数
    std::string struct_name;                    // 構造体名
    bool is_pointer_receiver;                   // ポインタレシーバーか
};

// メンバーアクセス
struct HIRMemberAccess {
    std::unique_ptr<HIRExpr> object;            // オブジェクト
    std::string member_name;                    // メンバー名
    uint32_t member_index;                      // メンバーのインデックス
};

// 配列アクセス
struct HIRArrayAccess {
    std::unique_ptr<HIRExpr> array;             // 配列
    std::unique_ptr<HIRExpr> index;             // インデックス
};

// 型キャスト
struct HIRCast {
    std::unique_ptr<HIRExpr> expr;              // キャスト対象
    TypeInfo target_type;                       // キャスト先の型
    bool is_implicit;                           // 暗黙のキャストか
};

// 三項演算子
struct HIRTernary {
    std::unique_ptr<HIRExpr> condition;         // 条件
    std::unique_ptr<HIRExpr> then_expr;         // 真の場合
    std::unique_ptr<HIRExpr> else_expr;         // 偽の場合
};

// 無名関数
struct HIRLambda {
    std::vector<HIRParam> params;               // パラメータ
    TypeInfo return_type;                       // 戻り値型
    std::unique_ptr<HIRExpr> body;              // 本体
    std::vector<HIRVariable> captures;          // キャプチャ変数
};

// 構造体リテラル
struct HIRStructLiteral {
    std::string struct_name;                    // 構造体名
    std::vector<std::pair<std::string, std::unique_ptr<HIRExpr>>> fields;
};

// 配列リテラル
struct HIRArrayLiteral {
    std::vector<std::unique_ptr<HIRExpr>> elements;
    TypeInfo element_type;
};

// ブロック式（最後の式が値になる）
struct HIRBlock {
    std::vector<std::unique_ptr<HIRStmt>> statements;
    std::unique_ptr<HIRExpr> result_expr;       // 最後の式（オプション）
};

// HIR式
struct HIRExpr {
    HIRExprKind kind;
    TypeInfo type;                              // 型情報（完全に解決済み）
    SourceLocation location;                    // ソースコード位置

    std::variant<
        HIRLiteral,
        HIRVariable,
        HIRBinaryOp,
        HIRUnaryOp,
        HIRFunctionCall,
        HIRMethodCall,
        HIRMemberAccess,
        HIRArrayAccess,
        HIRCast,
        HIRTernary,
        HIRLambda,
        HIRStructLiteral,
        HIRArrayLiteral,
        HIRBlock
    > data;

    // コンストラクタ
    HIRExpr(HIRExprKind k, TypeInfo t, SourceLocation loc)
        : kind(k), type(t), location(loc) {}
};

// ===== HIR文ノード =====

enum class HIRStmtKind {
    VarDecl,          // 変数宣言
    Assignment,       // 代入文
    ExprStmt,         // 式文
    If,               // if文
    While,            // while文
    For,              // for文
    Return,           // return文
    Break,            // break文
    Continue,         // continue文
    Block,            // ブロック
    Match,            // match文（パターンマッチング）
};

// 変数宣言
struct HIRVarDecl {
    std::string name;                           // 変数名
    uint32_t local_id;                          // ローカル変数ID
    TypeInfo type;                              // 型
    std::unique_ptr<HIRExpr> init_expr;         // 初期化式
    bool is_mutable;                            // 可変かどうか
};

// 代入文
struct HIRAssignment {
    std::unique_ptr<HIRExpr> lhs;               // 左辺
    std::unique_ptr<HIRExpr> rhs;               // 右辺
    enum class Op {
        Assign,           // =
        AddAssign,        // +=
        SubAssign,        // -=
        MulAssign,        // *=
        DivAssign,        // /=
        ModAssign,        // %=
    };
    Op op;
};

// 式文
struct HIRExprStmt {
    std::unique_ptr<HIRExpr> expr;
};

// if文
struct HIRIf {
    std::unique_ptr<HIRExpr> condition;
    std::unique_ptr<HIRStmt> then_block;
    std::unique_ptr<HIRStmt> else_block;        // オプション
};

// while文
struct HIRWhile {
    std::unique_ptr<HIRExpr> condition;
    std::unique_ptr<HIRStmt> body;
};

// for文
struct HIRFor {
    std::unique_ptr<HIRStmt> init;              // 初期化
    std::unique_ptr<HIRExpr> condition;         // 条件
    std::unique_ptr<HIRStmt> update;            // 更新
    std::unique_ptr<HIRStmt> body;              // 本体
};

// return文
struct HIRReturn {
    std::unique_ptr<HIRExpr> value;             // 戻り値（オプション）
};

// ブロック
struct HIRStmtBlock {
    std::vector<std::unique_ptr<HIRStmt>> statements;
};

// パターン（match文用）
struct HIRPattern {
    enum class Kind {
        Literal,          // リテラルパターン
        Variable,         // 変数パターン
        Wildcard,         // ワイルドカードパターン (_)
        Struct,           // 構造体パターン
        Enum,             // enumパターン
    };
    Kind kind;
    // パターンの詳細はここに追加
};

// match文のアーム
struct HIRMatchArm {
    std::unique_ptr<HIRPattern> pattern;
    std::unique_ptr<HIRExpr> guard;             // ガード条件（オプション）
    std::unique_ptr<HIRStmt> body;
};

// match文
struct HIRMatch {
    std::unique_ptr<HIRExpr> scrutinee;         // マッチ対象
    std::vector<HIRMatchArm> arms;
};

// HIR文
struct HIRStmt {
    HIRStmtKind kind;
    SourceLocation location;

    std::variant<
        HIRVarDecl,
        HIRAssignment,
        HIRExprStmt,
        HIRIf,
        HIRWhile,
        HIRFor,
        HIRReturn,
        std::monostate,   // Break/Continue用
        HIRStmtBlock,
        HIRMatch
    > data;

    HIRStmt(HIRStmtKind k, SourceLocation loc)
        : kind(k), location(loc) {}
};

// ===== HIR関数・構造体定義 =====

// パラメータ
struct HIRParam {
    std::string name;
    uint32_t local_id;
    TypeInfo type;
    bool is_mutable;
};

// 関数定義
struct HIRFunction {
    std::string name;                           // 関数名
    std::vector<HIRParam> params;               // パラメータ
    TypeInfo return_type;                       // 戻り値型
    std::vector<std::unique_ptr<HIRStmt>> body; // 関数本体
    bool is_generic;                            // ジェネリック関数か
    std::vector<std::string> type_parameters;   // 型パラメータ
    std::unordered_map<std::string, std::vector<std::string>> interface_bounds;
    uint32_t num_locals;                        // ローカル変数の総数
    SourceLocation location;
};

// 構造体フィールド
struct HIRStructField {
    std::string name;
    TypeInfo type;
    bool is_private;
    uint32_t offset;                            // オフセット（バイト）
};

// 構造体定義
struct HIRStruct {
    std::string name;
    std::vector<HIRStructField> fields;
    std::vector<HIRFunction> methods;
    bool is_generic;
    std::vector<std::string> type_parameters;
    size_t size;                                // サイズ（バイト）
    size_t alignment;                           // アライメント（バイト）
    SourceLocation location;
};

// enumバリアント
struct HIREnumVariant {
    std::string name;
    int64_t discriminant;                       // 判別子
    std::optional<TypeInfo> associated_type;    // 関連値の型
};

// enum定義
struct HIREnum {
    std::string name;
    std::vector<HIREnumVariant> variants;
    bool is_generic;
    std::vector<std::string> type_parameters;
    SourceLocation location;
};

// interfaceメソッド
struct HIRInterfaceMethod {
    std::string name;
    std::vector<TypeInfo> param_types;
    TypeInfo return_type;
};

// interface定義
struct HIRInterface {
    std::string name;
    std::vector<HIRInterfaceMethod> methods;
    bool is_generic;
    std::vector<std::string> type_parameters;
    SourceLocation location;
};

// impl定義
struct HIRImpl {
    std::string struct_name;
    std::optional<std::string> interface_name;  // trait/interface名（オプション）
    std::vector<HIRFunction> methods;
    std::vector<std::string> type_arguments;    // ジェネリクスの型引数
    SourceLocation location;
};

// HIRプログラム（トップレベル）
struct HIRProgram {
    std::vector<HIRFunction> functions;
    std::vector<HIRStruct> structs;
    std::vector<HIREnum> enums;
    std::vector<HIRInterface> interfaces;
    std::vector<HIRImpl> impls;
    std::unordered_map<std::string, TypeInfo> type_aliases;

    // ジェネリクスのインスタンス化されたバージョン
    std::unordered_map<std::string, std::vector<HIRFunction>> monomorphized_functions;
    std::unordered_map<std::string, std::vector<HIRStruct>> monomorphized_structs;
};

} // namespace hir
} // namespace ir
} // namespace cb
```

### 1.2 HIRGenerator実装の詳細

#### ファイル: `src/backend/ir/hir/hir_generator.h`

```cpp
#pragma once
#include "hir_node.h"
#include "common/ast.h"
#include "common/type_system.h"
#include <unordered_map>

namespace cb {
namespace ir {
namespace hir {

// シンボル情報
struct Symbol {
    std::string name;
    uint32_t local_id;
    TypeInfo type;
    bool is_mutable;
    bool is_captured;                           // クロージャでキャプチャされているか
};

// スコープ管理
class ScopeManager {
public:
    void enter_scope();
    void exit_scope();

    void define_symbol(const std::string& name, const Symbol& symbol);
    Symbol* lookup_symbol(const std::string& name);

    uint32_t allocate_local_id();
    uint32_t get_num_locals() const { return next_local_id; }

private:
    struct Scope {
        std::unordered_map<std::string, Symbol> symbols;
        Scope* parent;
    };

    Scope* current_scope = nullptr;
    uint32_t next_local_id = 0;
};

// 型コンテキスト（ジェネリクス解決用）
class TypeContext {
public:
    void enter_generic_context(const std::vector<std::string>& type_params);
    void exit_generic_context();

    void bind_type_parameter(const std::string& param, TypeInfo concrete_type);
    TypeInfo resolve_type_parameter(const std::string& param);

    bool is_type_parameter(const std::string& name) const;

private:
    struct Context {
        std::unordered_map<std::string, TypeInfo> bindings;
        Context* parent;
    };

    Context* current_context = nullptr;
};

// HIRジェネレーター
class HIRGenerator {
public:
    HIRGenerator();

    // メインエントリーポイント
    HIRProgram generate(const ASTNode* ast_root);

private:
    // 式の変換
    std::unique_ptr<HIRExpr> lower_expr(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_literal(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_variable(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_binary_op(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_unary_op(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_function_call(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_method_call(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_member_access(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_array_access(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_cast(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_ternary(const ASTNode* node);
    std::unique_ptr<HIRExpr> lower_lambda(const ASTNode* node);

    // 文の変換
    std::unique_ptr<HIRStmt> lower_stmt(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_var_decl(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_assignment(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_expr_stmt(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_if(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_while(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_for(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_return(const ASTNode* node);
    std::unique_ptr<HIRStmt> lower_block(const ASTNode* node);

    // 関数・構造体の変換
    HIRFunction lower_function(const ASTNode* node);
    HIRStruct lower_struct(const ASTNode* node);
    HIREnum lower_enum(const ASTNode* node);
    HIRInterface lower_interface(const ASTNode* node);
    HIRImpl lower_impl(const ASTNode* node);

    // 型解決
    TypeInfo resolve_type(const ASTNode* node);
    TypeInfo resolve_type_name(const std::string& type_name);

    // ジェネリクスの単相化（Monomorphization）
    void monomorphize_generics(HIRProgram& program);
    HIRFunction monomorphize_function(
        const HIRFunction& generic_func,
        const std::vector<TypeInfo>& type_args
    );
    HIRStruct monomorphize_struct(
        const HIRStruct& generic_struct,
        const std::vector<TypeInfo>& type_args
    );

    // 型推論
    void infer_types(HIRFunction& func);
    TypeInfo infer_expr_type(HIRExpr* expr);

    // 暗黙のキャスト挿入
    std::unique_ptr<HIRExpr> insert_implicit_cast(
        std::unique_ptr<HIRExpr> expr,
        TypeInfo target_type
    );

    // ヘルパー
    bool is_compatible_type(TypeInfo from, TypeInfo to);
    std::string mangle_generic_name(
        const std::string& base_name,
        const std::vector<TypeInfo>& type_args
    );

    // メンバ変数
    ScopeManager scope_manager;
    TypeContext type_context;
    HIRProgram current_program;

    // エラー報告
    std::vector<std::string> errors;
    void report_error(const std::string& message, SourceLocation loc);
};

} // namespace hir
} // namespace ir
} // namespace cb
```

#### ファイル: `src/backend/ir/hir/hir_generator.cpp`

```cpp
#include "hir_generator.h"
#include <cassert>

namespace cb {
namespace ir {
namespace hir {

// ===== ScopeManager実装 =====

void ScopeManager::enter_scope() {
    Scope* new_scope = new Scope();
    new_scope->parent = current_scope;
    current_scope = new_scope;
}

void ScopeManager::exit_scope() {
    assert(current_scope != nullptr);
    Scope* old_scope = current_scope;
    current_scope = current_scope->parent;
    delete old_scope;
}

void ScopeManager::define_symbol(const std::string& name, const Symbol& symbol) {
    assert(current_scope != nullptr);
    current_scope->symbols[name] = symbol;
}

Symbol* ScopeManager::lookup_symbol(const std::string& name) {
    Scope* scope = current_scope;
    while (scope != nullptr) {
        auto it = scope->symbols.find(name);
        if (it != scope->symbols.end()) {
            return &it->second;
        }
        scope = scope->parent;
    }
    return nullptr;
}

uint32_t ScopeManager::allocate_local_id() {
    return next_local_id++;
}

// ===== HIRGenerator実装 =====

HIRGenerator::HIRGenerator() {
    scope_manager.enter_scope();  // グローバルスコープ
}

HIRProgram HIRGenerator::generate(const ASTNode* ast_root) {
    assert(ast_root != nullptr);
    assert(ast_root->node_type == AST_PROGRAM);

    current_program = HIRProgram();

    // トップレベルの宣言を処理
    for (const auto& child : ast_root->children) {
        switch (child->node_type) {
        case AST_FUNCTION_DECL:
            current_program.functions.push_back(lower_function(child.get()));
            break;
        case AST_STRUCT_DECL:
            current_program.structs.push_back(lower_struct(child.get()));
            break;
        case AST_ENUM_DECL:
            current_program.enums.push_back(lower_enum(child.get()));
            break;
        case AST_INTERFACE_DECL:
            current_program.interfaces.push_back(lower_interface(child.get()));
            break;
        case AST_IMPL_DECL:
            current_program.impls.push_back(lower_impl(child.get()));
            break;
        // 他のトップレベル宣言...
        default:
            report_error("Unexpected top-level node", child->location);
            break;
        }
    }

    // ジェネリクスの単相化
    monomorphize_generics(current_program);

    return std::move(current_program);
}

// ===== 式の変換 =====

std::unique_ptr<HIRExpr> HIRGenerator::lower_expr(const ASTNode* node) {
    switch (node->node_type) {
    case AST_INTEGER_LITERAL:
    case AST_FLOAT_LITERAL:
    case AST_STRING_LITERAL:
    case AST_CHAR_LITERAL:
    case AST_BOOL_LITERAL:
        return lower_literal(node);

    case AST_IDENTIFIER:
        return lower_variable(node);

    case AST_BINARY_OP:
        return lower_binary_op(node);

    case AST_UNARY_OP:
        return lower_unary_op(node);

    case AST_FUNCTION_CALL:
        return lower_function_call(node);

    case AST_METHOD_CALL:
        return lower_method_call(node);

    case AST_MEMBER_ACCESS:
        return lower_member_access(node);

    case AST_ARRAY_ACCESS:
        return lower_array_access(node);

    case AST_CAST:
        return lower_cast(node);

    case AST_TERNARY:
        return lower_ternary(node);

    case AST_LAMBDA:
        return lower_lambda(node);

    default:
        report_error("Unexpected expression node", node->location);
        return nullptr;
    }
}

std::unique_ptr<HIRExpr> HIRGenerator::lower_literal(const ASTNode* node) {
    auto expr = std::make_unique<HIRExpr>(
        HIRExprKind::Literal,
        node->type_info,
        node->location
    );

    HIRLiteral literal;
    switch (node->node_type) {
    case AST_INTEGER_LITERAL:
        literal.kind = HIRLiteral::Kind::Int;
        literal.value = node->int_value;
        break;
    case AST_FLOAT_LITERAL:
        literal.kind = HIRLiteral::Kind::Float;
        literal.value = node->double_value;
        break;
    case AST_STRING_LITERAL:
        literal.kind = HIRLiteral::Kind::String;
        literal.value = node->str_value;
        break;
    case AST_CHAR_LITERAL:
        literal.kind = HIRLiteral::Kind::Char;
        literal.value = static_cast<char>(node->int_value);
        break;
    case AST_BOOL_LITERAL:
        literal.kind = HIRLiteral::Kind::Bool;
        literal.value = (node->int_value != 0);
        break;
    default:
        report_error("Invalid literal node", node->location);
        return nullptr;
    }

    expr->data = literal;
    return expr;
}

std::unique_ptr<HIRExpr> HIRGenerator::lower_variable(const ASTNode* node) {
    Symbol* symbol = scope_manager.lookup_symbol(node->name);
    if (!symbol) {
        report_error("Undefined variable: " + node->name, node->location);
        return nullptr;
    }

    auto expr = std::make_unique<HIRExpr>(
        HIRExprKind::Variable,
        symbol->type,
        node->location
    );

    HIRVariable var;
    var.name = symbol->name;
    var.local_id = symbol->local_id;
    var.is_mutable = symbol->is_mutable;

    expr->data = var;
    return expr;
}

std::unique_ptr<HIRExpr> HIRGenerator::lower_binary_op(const ASTNode* node) {
    auto left = lower_expr(node->left.get());
    auto right = lower_expr(node->right.get());

    if (!left || !right) {
        return nullptr;
    }

    // 型チェックと暗黙のキャスト
    if (!is_compatible_type(left->type, right->type)) {
        // 必要に応じて暗黙のキャスト
        if (can_implicitly_cast(left->type, right->type)) {
            left = insert_implicit_cast(std::move(left), right->type);
        } else if (can_implicitly_cast(right->type, left->type)) {
            right = insert_implicit_cast(std::move(right), left->type);
        } else {
            report_error("Type mismatch in binary operation", node->location);
            return nullptr;
        }
    }

    auto expr = std::make_unique<HIRExpr>(
        HIRExprKind::BinaryOp,
        left->type,  // 結果の型
        node->location
    );

    HIRBinaryOp binop;
    binop.op = convert_binary_op(node->op);
    binop.left = std::move(left);
    binop.right = std::move(right);

    expr->data = binop;
    return expr;
}

// 他の変換メソッドも同様に実装...

// ===== ジェネリクスの単相化 =====

void HIRGenerator::monomorphize_generics(HIRProgram& program) {
    // 全てのジェネリック関数呼び出しを収集
    std::vector<std::pair<std::string, std::vector<TypeInfo>>> instantiations;

    // ジェネリック関数の使用箇所を探索
    for (const auto& func : program.functions) {
        collect_generic_instantiations(func, instantiations);
    }

    // 各インスタンス化を生成
    for (const auto& [func_name, type_args] : instantiations) {
        // ジェネリック関数を検索
        auto it = std::find_if(
            program.functions.begin(),
            program.functions.end(),
            [&](const HIRFunction& f) { return f.name == func_name && f.is_generic; }
        );

        if (it != program.functions.end()) {
            HIRFunction mono_func = monomorphize_function(*it, type_args);
            program.monomorphized_functions[func_name].push_back(std::move(mono_func));
        }
    }
}

HIRFunction HIRGenerator::monomorphize_function(
    const HIRFunction& generic_func,
    const std::vector<TypeInfo>& type_args
) {
    assert(generic_func.is_generic);
    assert(type_args.size() == generic_func.type_parameters.size());

    // 型パラメータをバインド
    type_context.enter_generic_context(generic_func.type_parameters);
    for (size_t i = 0; i < type_args.size(); ++i) {
        type_context.bind_type_parameter(
            generic_func.type_parameters[i],
            type_args[i]
        );
    }

    // 関数をコピーして型を具体化
    HIRFunction mono_func = generic_func;
    mono_func.name = mangle_generic_name(generic_func.name, type_args);
    mono_func.is_generic = false;

    // パラメータの型を具体化
    for (auto& param : mono_func.params) {
        param.type = type_context.resolve_type_parameter(param.type);
    }

    // 戻り値の型を具体化
    mono_func.return_type = type_context.resolve_type_parameter(mono_func.return_type);

    // 関数本体の型を具体化
    substitute_types_in_body(mono_func.body);

    type_context.exit_generic_context();

    return mono_func;
}

} // namespace hir
} // namespace ir
} // namespace cb
```

### 1.3 HIRVisitorとHIRDumper

#### ファイル: `src/backend/ir/hir/hir_visitor.h`

```cpp
#pragma once
#include "hir_node.h"

namespace cb {
namespace ir {
namespace hir {

// HIRビジターインターフェース
class HIRVisitor {
public:
    virtual ~HIRVisitor() = default;

    // 式のビジット
    virtual void visit_expr(HIRExpr* expr);
    virtual void visit_literal(HIRLiteral* literal) {}
    virtual void visit_variable(HIRVariable* var) {}
    virtual void visit_binary_op(HIRBinaryOp* binop);
    virtual void visit_unary_op(HIRUnaryOp* unop);
    virtual void visit_function_call(HIRFunctionCall* call);
    virtual void visit_method_call(HIRMethodCall* call);
    virtual void visit_member_access(HIRMemberAccess* access);
    virtual void visit_array_access(HIRArrayAccess* access);
    virtual void visit_cast(HIRCast* cast);
    virtual void visit_ternary(HIRTernary* ternary);
    virtual void visit_lambda(HIRLambda* lambda);

    // 文のビジット
    virtual void visit_stmt(HIRStmt* stmt);
    virtual void visit_var_decl(HIRVarDecl* decl);
    virtual void visit_assignment(HIRAssignment* assign);
    virtual void visit_expr_stmt(HIRExprStmt* stmt);
    virtual void visit_if(HIRIf* if_stmt);
    virtual void visit_while(HIRWhile* while_stmt);
    virtual void visit_for(HIRFor* for_stmt);
    virtual void visit_return(HIRReturn* ret);
    virtual void visit_block(HIRStmtBlock* block);

    // 関数・構造体のビジット
    virtual void visit_function(HIRFunction* func);
    virtual void visit_struct(HIRStruct* struct_def);
    virtual void visit_enum(HIREnum* enum_def);
    virtual void visit_interface(HIRInterface* interface_def);
    virtual void visit_impl(HIRImpl* impl);

    // プログラムのビジット
    virtual void visit_program(HIRProgram* program);
};

} // namespace hir
} // namespace ir
} // namespace cb
```

#### ファイル: `src/backend/ir/hir/hir_dumper.h`

```cpp
#pragma once
#include "hir_visitor.h"
#include <sstream>
#include <string>

namespace cb {
namespace ir {
namespace hir {

// HIRダンパー
class HIRDumper : public HIRVisitor {
public:
    std::string dump_program(HIRProgram* program);
    std::string dump_function(HIRFunction* func);
    std::string dump_expr(HIRExpr* expr);
    std::string dump_stmt(HIRStmt* stmt);

    // ビジターメソッドのオーバーライド
    void visit_literal(HIRLiteral* literal) override;
    void visit_variable(HIRVariable* var) override;
    void visit_binary_op(HIRBinaryOp* binop) override;
    void visit_unary_op(HIRUnaryOp* unop) override;
    void visit_function_call(HIRFunctionCall* call) override;
    // 他のメソッド...

private:
    std::ostringstream output;
    int indent_level = 0;

    void write_indent();
    void increase_indent();
    void decrease_indent();

    std::string type_to_string(TypeInfo type);
    std::string op_to_string(HIRBinaryOp::Op op);
};

} // namespace hir
} // namespace ir
} // namespace cb
```

---

## 2. MIR実装の詳細設計

### 2.1 MIRノード構造の詳細

#### ファイル: `src/backend/ir/mir/mir_node.h`

```cpp
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "common/type_system.h"

namespace cb {
namespace ir {
namespace mir {

// ===== MIR基本構造 =====

// ローカル変数
struct MIRLocal {
    uint32_t id;                                // ローカルID
    TypeInfo type;                              // 型
    std::string debug_name;                     // デバッグ用の名前
    bool is_mutable;                            // 可変かどうか
    bool is_arg;                                // 引数かどうか
    bool is_return_slot;                        // 戻り値スロットかどうか
};

// 場所（Place）- 左辺値を表す
enum class MIRProjectionKind {
    Field,            // 構造体フィールドアクセス (.field)
    Index,            // 配列インデックス ([index])
    Deref,            // デリファレンス (*ptr)
};

struct MIRProjection {
    MIRProjectionKind kind;
    std::variant<uint32_t, uint32_t> data;      // フィールドインデックス or インデックス変数
};

struct MIRPlace {
    uint32_t local;                             // ローカル変数ID
    std::vector<MIRProjection> projections;     // プロジェクション（アクセスパス）

    bool is_simple_local() const {
        return projections.empty();
    }
};

// 定数
struct MIRConstant {
    TypeInfo type;
    std::variant<
        int64_t,          // 整数
        double,           // 浮動小数点数
        std::string,      // 文字列
        bool              // ブール
    > value;
};

// オペランド
enum class MIROperandKind {
    Copy,             // コピー
    Move,             // ムーブ
    Constant,         // 定数
};

struct MIROperand {
    MIROperandKind kind;
    std::variant<MIRPlace, MIRConstant> data;
};

// ===== 右辺値（Rvalue） =====

enum class MIRRvalueKind {
    Use,              // オペランドの使用
    BinaryOp,         // 二項演算
    UnaryOp,          // 単項演算
    Ref,              // 参照
    AddressOf,        // アドレス取得
    Cast,             // キャスト
    Aggregate,        // 集約（構造体、配列）
};

// 二項演算
enum class MIRBinOp {
    Add, Sub, Mul, Div, Mod,
    Eq, Ne, Lt, Le, Gt, Ge,
    BitAnd, BitOr, BitXor,
    Shl, Shr,
};

struct MIRRvalueBinaryOp {
    MIRBinOp op;
    MIROperand left;
    MIROperand right;
};

// 単項演算
enum class MIRUnOp {
    Neg,              // -x
    Not,              // !x
    BitNot,           // ~x
};

struct MIRRvalueUnaryOp {
    MIRUnOp op;
    MIROperand operand;
};

// 参照
enum class MIRBorrowKind {
    Shared,           // 共有参照 (&T)
    Mutable,          // 可変参照 (&mut T)
};

struct MIRRvalueRef {
    MIRBorrowKind kind;
    MIRPlace place;
};

// キャスト
enum class MIRCastKind {
    IntToInt,         // 整数から整数
    FloatToFloat,     // 浮動小数点数から浮動小数点数
    IntToFloat,       // 整数から浮動小数点数
    FloatToInt,       // 浮動小数点数から整数
    Pointer,          // ポインタキャスト
};

struct MIRRvalueCast {
    MIRCastKind kind;
    MIROperand operand;
    TypeInfo target_type;
};

// 集約（構造体、配列リテラル）
enum class MIRAggregateKind {
    Struct,
    Array,
};

struct MIRRvalueAggregate {
    MIRAggregateKind kind;
    std::vector<MIROperand> operands;
    TypeInfo type;
};

// 右辺値
struct MIRRvalue {
    MIRRvalueKind kind;
    std::variant<
        MIROperand,                // Use
        MIRRvalueBinaryOp,         // BinaryOp
        MIRRvalueUnaryOp,          // UnaryOp
        MIRRvalueRef,              // Ref/AddressOf
        MIRRvalueCast,             // Cast
        MIRRvalueAggregate         // Aggregate
    > data;
};

// ===== 文（Statement） =====

enum class MIRStatementKind {
    Assign,           // 代入
    StorageLive,      // ストレージの生存開始
    StorageDead,      // ストレージの生存終了
    Nop,              // 何もしない
};

struct MIRStatement {
    MIRStatementKind kind;
    std::optional<MIRPlace> place;              // 左辺（Assign用）
    std::optional<MIRRvalue> rvalue;            // 右辺（Assign用）
    std::optional<uint32_t> local;              // ローカル変数（StorageLive/Dead用）
};

// ===== 終端命令（Terminator） =====

enum class MIRTerminatorKind {
    Goto,             // 無条件ジャンプ
    SwitchInt,        // 整数値による分岐
    Return,           // 関数からの返却
    Unreachable,      // 到達不可能
    Call,             // 関数呼び出し
    Assert,           // アサーション
};

// Switch分岐の条件
struct MIRSwitchTarget {
    uint64_t value;                             // 比較値
    uint32_t target;                            // ジャンプ先ブロックID
};

struct MIRTerminator {
    MIRTerminatorKind kind;

    // Goto用
    std::optional<uint32_t> target;

    // SwitchInt用
    std::optional<MIROperand> discr;            // 判別値
    std::vector<MIRSwitchTarget> targets;
    std::optional<uint32_t> otherwise;          // デフォルトのジャンプ先

    // Call用
    std::optional<std::string> func;            // 関数名
    std::vector<MIROperand> args;               // 引数
    std::optional<MIRPlace> destination;        // 戻り値の格納先
    std::optional<uint32_t> next_block;         // 呼び出し後のブロック

    // Return用
    std::optional<MIROperand> return_value;
};

// ===== 基本ブロック（Basic Block） =====

struct BasicBlock {
    uint32_t id;                                // ブロックID
    std::vector<MIRStatement> statements;       // 命令列
    MIRTerminator terminator;                   // 終端命令

    // CFG情報
    std::vector<uint32_t> predecessors;         // 先行ブロックのID
    std::vector<uint32_t> successors;           // 後続ブロックのID

    // データフロー解析用
    std::set<uint32_t> live_in;                 // 生存変数（入口）
    std::set<uint32_t> live_out;                // 生存変数（出口）
    std::set<uint32_t> def;                     // 定義される変数
    std::set<uint32_t> use;                     // 使用される変数

    // 支配木用
    std::optional<uint32_t> immediate_dominator; // 即座支配者
    std::set<uint32_t> dominance_frontier;      // 支配境界
};

// ===== 制御フローグラフ（CFG） =====

struct ControlFlowGraph {
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    uint32_t entry_block;                       // エントリーブロックID
    uint32_t exit_block;                        // 出口ブロックID（オプション）

    BasicBlock* get_block(uint32_t id);
    const BasicBlock* get_block(uint32_t id) const;

    void add_edge(uint32_t from, uint32_t to);
    void remove_edge(uint32_t from, uint32_t to);
};

// ===== MIR関数 =====

struct MIRFunction {
    std::string name;                           // 関数名
    std::vector<MIRLocal> locals;               // ローカル変数
    ControlFlowGraph cfg;                       // 制御フローグラフ
    TypeInfo return_type;                       // 戻り値型
    size_t arg_count;                           // 引数の数
    bool is_in_ssa_form;                        // SSA形式かどうか
};

// ===== MIRプログラム =====

struct MIRProgram {
    std::vector<MIRFunction> functions;
};

} // namespace mir
} // namespace ir
} // namespace cb
```

### 2.2 CFGとSSA構築の詳細

#### ファイル: `src/backend/ir/mir/cfg.h`

```cpp
#pragma once
#include "mir_node.h"
#include <unordered_map>
#include <unordered_set>

namespace cb {
namespace ir {
namespace mir {

// CFGビルダー
class CFGBuilder {
public:
    ControlFlowGraph build(const std::vector<std::unique_ptr<HIRStmt>>& hir_stmts);

private:
    uint32_t create_block();
    BasicBlock* get_current_block();
    void set_current_block(uint32_t block_id);

    void emit_statement(const MIRStatement& stmt);
    void emit_terminator(const MIRTerminator& term);

    // HIR文の変換
    void lower_stmt(const HIRStmt* stmt);
    void lower_if(const HIRIf* if_stmt);
    void lower_while(const HIRWhile* while_stmt);
    void lower_for(const HIRFor* for_stmt);

    // エッジの追加
    void add_edge(uint32_t from, uint32_t to);

    ControlFlowGraph cfg;
    uint32_t current_block_id;
    uint32_t next_block_id = 0;

    // ループのbreak/continue用
    struct LoopContext {
        uint32_t break_target;
        uint32_t continue_target;
    };
    std::vector<LoopContext> loop_stack;
};

// CFG解析
class CFGAnalysis {
public:
    explicit CFGAnalysis(ControlFlowGraph* cfg) : cfg(cfg) {}

    // 到達可能性解析
    std::unordered_set<uint32_t> compute_reachable_blocks();

    // 到達不可能ブロックの削除
    void remove_unreachable_blocks();

    // CFGの検証
    bool validate();

private:
    ControlFlowGraph* cfg;
};

} // namespace mir
} // namespace ir
} // namespace cb
```

#### ファイル: `src/backend/ir/mir/ssa_builder.h`

```cpp
#pragma once
#include "mir_node.h"
#include "dominator_tree.h"
#include <unordered_map>
#include <stack>

namespace cb {
namespace ir {
namespace mir {

// PHIノード（SSA形式用）
struct PHINode {
    uint32_t target;                            // 代入先ローカル変数
    std::vector<std::pair<uint32_t, uint32_t>> operands; // (ローカル変数, 先行ブロックID)
};

// SSAビルダー
class SSABuilder {
public:
    explicit SSABuilder(MIRFunction* func);

    // SSA形式への変換
    void convert_to_ssa();

private:
    // PHIノードの挿入
    void insert_phi_nodes();

    // 変数のリネーミング
    void rename_variables();
    void rename_block(uint32_t block_id);

    // 支配木の構築
    void build_dominator_tree();

    // 支配境界の計算
    void compute_dominance_frontiers();

    // ヘルパー
    uint32_t new_ssa_name(uint32_t original_local);
    uint32_t current_def(uint32_t original_local);

    MIRFunction* func;
    DominatorTree dom_tree;

    // SSA変数管理
    std::unordered_map<uint32_t, std::stack<uint32_t>> variable_stack;
    std::unordered_map<uint32_t, uint32_t> ssa_counter;
    uint32_t next_local_id;

    // PHIノード
    std::unordered_map<uint32_t, std::vector<PHINode>> phi_nodes;
};

} // namespace mir
} // namespace ir
} // namespace cb
```

#### ファイル: `src/backend/ir/mir/dominator_tree.h`

```cpp
#pragma once
#include "mir_node.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cb {
namespace ir {
namespace mir {

// 支配木
class DominatorTree {
public:
    explicit DominatorTree(ControlFlowGraph* cfg);

    // 支配木の構築
    void build();

    // ブロックAがブロックBを支配するか
    bool dominates(uint32_t a, uint32_t b) const;

    // ブロックAがブロックBを厳密に支配するか（A ≠ B）
    bool strictly_dominates(uint32_t a, uint32_t b) const;

    // 即座支配者を取得
    std::optional<uint32_t> get_immediate_dominator(uint32_t block) const;

    // 支配境界を取得
    const std::set<uint32_t>& get_dominance_frontier(uint32_t block) const;

    // 支配木の子を取得
    const std::vector<uint32_t>& get_children(uint32_t block) const;

private:
    // Lengauer-Tarjanアルゴリズム
    void compute_immediate_dominators();
    void dfs(uint32_t block);
    void link(uint32_t v, uint32_t w);
    uint32_t eval(uint32_t v);
    void compress(uint32_t v);

    // 支配境界の計算
    void compute_dominance_frontiers();

    ControlFlowGraph* cfg;

    // 支配木データ
    std::unordered_map<uint32_t, uint32_t> idom;                    // 即座支配者
    std::unordered_map<uint32_t, std::vector<uint32_t>> children;   // 支配木の子
    std::unordered_map<uint32_t, std::set<uint32_t>> df;            // 支配境界

    // Lengauer-Tarjan用の一時データ
    std::unordered_map<uint32_t, uint32_t> semi;
    std::unordered_map<uint32_t, uint32_t> vertex;
    std::unordered_map<uint32_t, uint32_t> parent;
    std::unordered_map<uint32_t, uint32_t> ancestor;
    std::unordered_map<uint32_t, uint32_t> label;
    std::unordered_map<uint32_t, std::vector<uint32_t>> bucket;
    uint32_t counter;
};

} // namespace mir
} // namespace ir
} // namespace cb
```

---

このドキュメントはかなり長くなるため、続きを別のファイルとして作成します。
