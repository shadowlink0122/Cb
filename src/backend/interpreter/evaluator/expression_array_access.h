#pragma once
#include "../../../common/ast.h"
#include <cstdint>
#include <functional>

// 前方宣言
class Interpreter;
struct Variable;

// ============================================================================
// 配列アクセス評価のヘルパー関数群
// ============================================================================
// expression_evaluatorから配列関連の処理を分離
// 多次元配列、配列リテラル、配列要素アクセスなどを処理
// ============================================================================

namespace ArrayAccessHelpers {

// ============================================================================
// 配列アクセス評価
// ============================================================================

// 配列要素アクセスの評価（AST_ARRAY_REF）
// node: 配列参照ノード
// interpreter: インタプリタインスタンス
// evaluate_expression_func: 式評価関数（再帰呼び出し用）
// get_struct_member_func: 構造体メンバー取得関数
int64_t evaluate_array_ref(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<Variable(const Variable &, const std::string &)>
        get_struct_member_func);

// 配列リテラルの評価（AST_ARRAY_LITERAL）
// node: 配列リテラルノード
// interpreter: インタプリタインスタンス
int64_t evaluate_array_literal(const ASTNode *node, Interpreter &interpreter);

} // namespace ArrayAccessHelpers
