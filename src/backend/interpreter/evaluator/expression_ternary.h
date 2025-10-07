#ifndef EXPRESSION_TERNARY_H
#define EXPRESSION_TERNARY_H

#include "../../../common/ast.h"
#include "core/type_inference.h"
#include <functional>

// 前方宣言
class Interpreter;
class TypedValue;

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
    const ASTNode* node,
    Interpreter& interpreter,
    std::function<int64_t(const ASTNode*)> evaluate_expression_callback,
    std::function<TypedValue(const ASTNode*)> evaluate_typed_expression_callback,
    TypeInferenceEngine& type_engine,
    TypedValue& last_typed_result
);

} // namespace TernaryHelpers

#endif // EXPRESSION_TERNARY_H
