#ifndef EXPRESSION_SPECIAL_ACCESS_H
#define EXPRESSION_SPECIAL_ACCESS_H

#include "core/interpreter.h"
#include "../../../common/ast.h"

namespace SpecialAccessHelpers {

/**
 * @brief アロー演算子によるメンバーアクセス（ptr->member）
 * 
 * ポインタを介した構造体メンバーアクセスを処理
 * ptr->member は (*ptr).member と等価
 * 
 * @param node AST_ARROW_ACCESSノード
 * @param interpreter インタプリタインスタンス
 * @param evaluate_expression_func evaluate_expressionの参照
 * @param get_struct_member_func get_struct_member_from_variableの参照
 * @return int64_t メンバー値
 */
int64_t evaluate_arrow_access(
    const ASTNode* node,
    Interpreter& interpreter,
    std::function<int64_t(const ASTNode*)> evaluate_expression_func,
    std::function<Variable(const Variable&, const std::string&)> get_struct_member_func
);

/**
 * @brief メンバー配列アクセス（obj.member[index]）
 * 
 * 構造体メンバーが配列の場合のアクセスを処理
 * 関数戻り値からのメンバー配列アクセスもサポート
 * 
 * @param node AST_MEMBER_ARRAY_ACCESSノード
 * @param interpreter インタプリタインスタンス
 * @param evaluate_expression_func evaluate_expressionの参照
 * @param get_struct_member_func get_struct_member_from_variableの参照
 * @return int64_t 配列要素の値
 */
int64_t evaluate_member_array_access(
    const ASTNode* node,
    Interpreter& interpreter,
    std::function<int64_t(const ASTNode*)> evaluate_expression_func,
    std::function<Variable(const Variable&, const std::string&)> get_struct_member_func
);

/**
 * @brief 構造体リテラル評価
 * 
 * 構造体リテラルは代入時にのみ処理されるべきため、ここでは0を返す
 * 
 * @param node AST_STRUCT_LITERALノード
 * @return int64_t 0
 */
inline int64_t evaluate_struct_literal(const ASTNode* node) {
    return 0;
}

/**
 * @brief Enum値アクセス（EnumName::member）
 * 
 * Enum定義から値を取得
 * 
 * @param node AST_ENUM_ACCESSノード
 * @param interpreter インタプリタインスタンス
 * @return int64_t Enum値
 */
int64_t evaluate_enum_access(
    const ASTNode* node,
    Interpreter& interpreter
);

} // namespace SpecialAccessHelpers

#endif // EXPRESSION_SPECIAL_ACCESS_H
