#pragma once

#include "../../../../common/ast.h"

class ExpressionEvaluator;
class Interpreter;

namespace ErrorHandlingOperators {

int64_t evaluate_try_expression(const ASTNode *node,
                                ExpressionEvaluator &expression_evaluator,
                                Interpreter &interpreter);

int64_t evaluate_checked_expression(const ASTNode *node,
                                    ExpressionEvaluator &expression_evaluator,
                                    Interpreter &interpreter);

} // namespace ErrorHandlingOperators
