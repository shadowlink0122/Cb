#include "expression_statement_handler.h"
#include "../../../common/ast.h"
#include "../core/interpreter.h"
#include "../evaluator/core/evaluator.h"

ExpressionStatementHandler::ExpressionStatementHandler(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void ExpressionStatementHandler::handle_expression_statement(
    const ASTNode *node) {
    try {
        // 式文として評価
        interpreter_->expression_evaluator_->evaluate_expression(node);
    } catch (const ReturnException &e) {
        // 関数呼び出し文でのreturn値は無視する（void文として扱う）
        // struct、array、stringの戻り値も含めて例外を伝播させない
    }
}
