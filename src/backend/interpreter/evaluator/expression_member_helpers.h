#ifndef EXPRESSION_MEMBER_HELPERS_H
#define EXPRESSION_MEMBER_HELPERS_H

#include "../../../common/ast.h"
#include "core/type_inference.h"
#include <vector>
#include <string>

// 前方宣言
class Interpreter;
class TypedValue;
class Variable;
class ExpressionEvaluator;

/**
 * メンバーアクセス関連のヘルパー関数群
 * 
 * この名前空間は、構造体メンバーアクセスと関数戻り値からのアクセス処理を
 * 担当するヘルパー関数を提供します。
 */
namespace MemberAccessHelpers {

/**
 * 数値結果を型付き値に変換する
 * 
 * @param node ASTノード
 * @param numeric_result 数値結果
 * @param inferred_type 推論された型
 * @param last_captured_function_value キャプチャされた関数値（optional）
 * @return 型付き値
 */
TypedValue consume_numeric_typed_value(
    const ASTNode* node,
    int64_t numeric_result,
    const InferredType& inferred_type,
    std::optional<std::pair<const ASTNode*, TypedValue>>& last_captured_function_value
);

/**
 * 構造体変数からメンバーを取得する
 * 
 * @param struct_var 構造体変数
 * @param member_name メンバー名
 * @param interpreter インタプリタへの参照
 * @return メンバー変数
 */
Variable get_struct_member_from_variable(
    const Variable& struct_var,
    const std::string& member_name,
    Interpreter& interpreter
);

/**
 * 関数戻り値からのメンバーアクセスを評価する
 * 
 * @param func_node 関数ノード
 * @param member_name メンバー名
 * @param evaluator ExpressionEvaluatorへの参照
 * @return 評価結果のTypedValue
 */
TypedValue evaluate_function_member_access(
    const ASTNode* func_node,
    const std::string& member_name,
    ExpressionEvaluator& evaluator
);

/**
 * 関数戻り値からの配列アクセスを評価する
 * 
 * @param func_node 関数ノード
 * @param index_node インデックスノード
 * @param evaluator ExpressionEvaluatorへの参照
 * @return 評価結果のTypedValue
 */
TypedValue evaluate_function_array_access(
    const ASTNode* func_node,
    const ASTNode* index_node,
    ExpressionEvaluator& evaluator
);

/**
 * 関数戻り値からの複合アクセスを評価する（func()[index].member）
 * 
 * @param func_node 関数ノード
 * @param index_node インデックスノード
 * @param member_name メンバー名
 * @param evaluator ExpressionEvaluatorへの参照
 * @return 評価結果のTypedValue
 */
TypedValue evaluate_function_compound_access(
    const ASTNode* func_node,
    const ASTNode* index_node,
    const std::string& member_name,
    ExpressionEvaluator& evaluator
);

/**
 * 再帰的メンバーアクセスを評価する
 * 
 * @param base_var ベース変数
 * @param member_path メンバーパス
 * @param interpreter インタプリタへの参照
 * @return 評価結果のTypedValue
 */
TypedValue evaluate_recursive_member_access(
    const Variable& base_var,
    const std::vector<std::string>& member_path,
    Interpreter& interpreter
);

} // namespace MemberAccessHelpers

#endif // EXPRESSION_MEMBER_HELPERS_H
