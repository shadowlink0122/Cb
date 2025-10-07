#include "handlers/break_continue_handler.h"
#include "../../../common/ast.h"
#include "core/interpreter.h"
#include "evaluator/expression_evaluator.h"

BreakContinueHandler::BreakContinueHandler(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void BreakContinueHandler::handle_break(const ASTNode *node) {
    int64_t cond = 1;
    if (node->left) {
        cond = interpreter_->expression_evaluator_->evaluate_expression(
            node->left.get());
    }
    if (cond) {
        throw BreakException(cond);
    }
}

void BreakContinueHandler::handle_continue(const ASTNode *node) {
    int64_t cond = 1;
    if (node->left) {
        cond = interpreter_->expression_evaluator_->evaluate_expression(
            node->left.get());
    }
    if (cond) {
        throw ContinueException(cond);
    }
}
