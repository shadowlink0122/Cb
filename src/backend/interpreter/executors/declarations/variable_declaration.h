#ifndef VARIABLE_DECLARATION_H
#define VARIABLE_DECLARATION_H

struct ASTNode;
class StatementExecutor;
class Interpreter;

namespace DeclarationHandlers {

// Variable declaration execution
void execute_variable_declaration(StatementExecutor *executor,
                                   Interpreter &interpreter,
                                   const ASTNode *node);

void execute_multiple_var_decl(StatementExecutor *executor,
                                Interpreter &interpreter,
                                const ASTNode *node);

} // namespace DeclarationHandlers

#endif // VARIABLE_DECLARATION_H
