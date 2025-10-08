#ifndef MEMBER_ASSIGNMENT_H
#define MEMBER_ASSIGNMENT_H

struct ASTNode;
class StatementExecutor;
class Interpreter;

namespace AssignmentHandlers {

// Member assignment execution
void execute_member_assignment(StatementExecutor *executor,
                               Interpreter &interpreter, const ASTNode *node);

void execute_arrow_assignment(StatementExecutor *executor,
                              Interpreter &interpreter, const ASTNode *node);

} // namespace AssignmentHandlers

#endif // MEMBER_ASSIGNMENT_H
