#ifndef EXPRESSION_TERNARY_H
#define EXPRESSION_TERNARY_H

#include "../../../../common/ast.h"
#include "../../core/type_inference.h"
#include <functional>

// 前方宣言
class Interpreter;
struct TypedValue;

/**
 * 三項演算子（?:）評価のヘルパー関数群
 *
 * この名前空間は、三項演算子の評価を担当するヘルパー関数を提供します。
 * expression_evaluator.cppから分離して、コードの可読性と保守性を向上させます。
 */
namespace TernaryHelpers {

/**
 * 三項演算子を評価する（typed版）
 *
 * @param node 三項演算子のASTノード
 * @param interpreter インタプリタへの参照
 * @param evaluate_expression_callback 通常評価のコールバック
 * @param evaluate_typed_expression_callback 型付き評価のコールバック
 * @param type_engine 型推論エンジンへの参照
 * @param last_typed_result 最後の型付き結果への参照（更新される）
 * @return 評価結果のTypedValue
 */
TypedValue evaluate_ternary_typed(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_callback,
    std::function<TypedValue(const ASTNode *)>
        evaluate_typed_expression_callback,
    TypeInferenceEngine &type_engine, TypedValue &last_typed_result);

/**
 * v0.12.1: エラー伝播演算子（?）を評価する
 *
 * Result<T, E>のOk(v)の場合はvを返し、Err(e)の場合は関数から早期リターン
 * Option<T>のSome(v)の場合はvを返し、Noneの場合は関数から早期リターン
 *
 * @param node エラー伝播演算子のASTノード
 * @param interpreter インタプリタへの参照
 * @param evaluate_expression_callback 式評価のコールバック
 * @return Ok/Someの場合の関連値
 * @throws ReturnException Err/Noneの場合、関数から早期リターン
 */
int64_t evaluate_error_propagation(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_callback);

} // namespace TernaryHelpers

#endif // EXPRESSION_TERNARY_H
