#pragma once
#include "../../../../common/ast.h"

// Forward declarations
class Interpreter;
class StatementExecutor;

namespace AssignmentHandlers {

// 単純な代入を処理するヘルパー関数
void execute_assignment(StatementExecutor *executor, Interpreter &interpreter,
                        const ASTNode *node);

} // namespace AssignmentHandlers
