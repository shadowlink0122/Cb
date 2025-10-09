#include "executors/statement_list_executor.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "core/interpreter.h"
#include <iostream>

StatementListExecutor::StatementListExecutor(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StatementListExecutor::execute_statement_list(const ASTNode *node) {
    if (!node) {
        return;
    }

    debug_msg(DebugMsgId::INTERPRETER_STMT_LIST_EXEC, node->statements.size());

    if (interpreter_->debug_mode) {
        std::cerr << "[STMT_LIST_DEBUG] Processing " << node->statements.size()
                  << " statements" << std::endl;
    }

    for (size_t i = 0; i < node->statements.size(); ++i) {
        if (interpreter_->debug_mode) {
            std::cerr << "[STMT_LIST_DEBUG] Processing statement " << (i + 1)
                      << "/" << node->statements.size() << ", type="
                      << static_cast<int>(node->statements[i]->node_type)
                      << std::endl;
        }

        interpreter_->execute_statement(node->statements[i].get());

        if (interpreter_->debug_mode) {
            std::cerr << "[STMT_LIST_DEBUG] Completed statement " << (i + 1)
                      << "/" << node->statements.size() << std::endl;
        }
    }

    if (interpreter_->debug_mode) {
        std::cerr << "[STMT_LIST_DEBUG] All " << node->statements.size()
                  << " statements processed" << std::endl;
    }
}

void StatementListExecutor::execute_compound_statement(const ASTNode *node) {
    if (!node) {
        return;
    }

    debug_msg(DebugMsgId::INTERPRETER_COMPOUND_STMT_EXEC,
              node->statements.size());

    for (const auto &stmt : node->statements) {
        interpreter_->execute_statement(stmt.get());
    }
}
