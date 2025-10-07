#ifndef EXPRESSION_LITERAL_EVAL_H
#define EXPRESSION_LITERAL_EVAL_H

#include "core/interpreter.h"
#include "../../../common/ast.h"

namespace LiteralEvalHelpers {

/**
 * @brief 数値リテラルの評価（typed版）
 * 
 * 整数、浮動小数点数（float, double, quad）のリテラルを評価
 * 
 * @param node AST_NUMBERノード
 * @param inferred_type 推論された型情報
 * @return TypedValue 評価結果
 */
TypedValue evaluate_number_literal_typed(
    const ASTNode* node,
    const InferredType& inferred_type
);

/**
 * @brief 文字列リテラルの評価（typed版）
 * 
 * @param node AST_STRING_LITERALノード
 * @param inferred_type 推論された型情報
 * @return TypedValue 評価結果
 */
TypedValue evaluate_string_literal_typed(
    const ASTNode* node,
    const InferredType& inferred_type
);

/**
 * @brief nullptrリテラルの評価（typed版）
 * 
 * @return TypedValue nullptr評価結果
 */
TypedValue evaluate_nullptr_literal_typed();

/**
 * @brief 変数参照の評価（typed版）
 * 
 * @param node AST_VARIABLEノード
 * @param interpreter インタプリタインスタンス
 * @param inferred_type 推論された型情報
 * @return TypedValue 評価結果
 */
TypedValue evaluate_variable_typed(
    const ASTNode* node,
    Interpreter& interpreter,
    const InferredType& inferred_type
);

} // namespace LiteralEvalHelpers

#endif // EXPRESSION_LITERAL_EVAL_H
