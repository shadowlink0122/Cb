#ifndef EXPRESSION_INCDEC_H
#define EXPRESSION_INCDEC_H

#include "../../../../common/ast.h"
#include <functional>

class Interpreter;

namespace IncDecHelpers {
/**
 * 前置・後置インクリメント/デクリメント演算子の評価
 *
 * 対応ケース：
 * - (*ptr)++/--：ポインタデリファレンス＋インクリメント/デクリメント
 *   - メタデータポインタ対応（変数、配列要素）
 *   - 従来ポインタ対応
 * - 変数++/--：通常の変数
 * - array[index]++/--：配列要素
 * - obj.member++/--：構造体メンバー
 *
 * @param node AST_PRE_INCDEC or AST_POST_INCDEC ノード
 * @param interpreter インタプリタインスタンス
 * @param evaluate_expression_func 式評価関数
 * @return インクリメント/デクリメント前（後置）または後（前置）の値
 */
int64_t evaluate_incdec(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func);
} // namespace IncDecHelpers

#endif // EXPRESSION_INCDEC_H
