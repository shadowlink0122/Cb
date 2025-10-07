#ifndef EXPRESSION_ASSIGNMENT_H
#define EXPRESSION_ASSIGNMENT_H

#include "../../../common/ast.h"
#include <functional>

class Interpreter;
struct TypedValue;

namespace AssignmentHelpers {
    /**
     * 代入演算子（=, +=, -=, *=, /=, %= 等）の評価
     * 
     * 対応ケース：
     * - 変数への代入：var = value
     * - 配列要素への代入：array[index] = value
     * - 配列リテラルの代入：var = [1, 2, 3]
     * - 構造体の代入：var = struct_instance
     * - 関数戻り値の代入（配列・構造体を含む）
     * 
     * @param node AST_ASSIGN ノード
     * @param interpreter インタプリタインスタンス
     * @param evaluate_expression_func 式評価関数
     * @param evaluate_typed_expression_func 型付き式評価関数
     * @return 代入された値（数値の場合）、文字列の場合は0
     */
    int64_t evaluate_assignment(
        const ASTNode* node,
        Interpreter& interpreter,
        std::function<int64_t(const ASTNode*)> evaluate_expression_func,
        std::function<TypedValue(const ASTNode*)> evaluate_typed_expression_func
    );
}

#endif // EXPRESSION_ASSIGNMENT_H
