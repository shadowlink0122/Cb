#ifndef EXPRESSION_BINARY_UNARY_TYPED_H
#define EXPRESSION_BINARY_UNARY_TYPED_H

#include "../../../../common/ast.h"
#include "../../core/interpreter.h"

namespace BinaryUnaryTypedHelpers {

/**
 * @brief evaluate_typed_expressionで使用される二項演算子の評価
 *
 * ポインタ演算、浮動小数点演算、整数演算、比較演算、論理演算、ビット演算を処理
 *
 * @param node AST_BINARY_OPノード
 * @param interpreter インタプリタインスタンス
 * @param inferred_type 推論された型情報
 * @param evaluate_typed_func evaluate_typed_expressionの参照（再帰呼び出し用）
 * @return TypedValue 評価結果
 */
TypedValue evaluate_binary_op_typed(
    const ASTNode *node, Interpreter &interpreter,
    const InferredType &inferred_type,
    std::function<TypedValue(const ASTNode *)> evaluate_typed_func);

/**
 * @brief evaluate_typed_expressionで使用される単項演算子の評価
 *
 * アドレス演算子（&）、間接参照演算子（*）、単項+/-、論理否定（!）を処理
 *
 * @param node AST_UNARY_OPノード
 * @param interpreter インタプリタインスタンス
 * @param inferred_type 推論された型情報
 * @param evaluate_typed_func evaluate_typed_expressionの参照（再帰呼び出し用）
 * @param evaluate_expression_func evaluate_expressionの参照（フォールバック用）
 * @return TypedValue 評価結果
 */
TypedValue evaluate_unary_op_typed(
    const ASTNode *node, Interpreter &interpreter,
    const InferredType &inferred_type,
    std::function<TypedValue(const ASTNode *)> evaluate_typed_func,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func);

} // namespace BinaryUnaryTypedHelpers

#endif // EXPRESSION_BINARY_UNARY_TYPED_H
