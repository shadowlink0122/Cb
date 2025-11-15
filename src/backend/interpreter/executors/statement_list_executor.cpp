#include "executors/statement_list_executor.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "core/interpreter.h"
#include "event_loop/simple_event_loop.h"

StatementListExecutor::StatementListExecutor(Interpreter *interpreter)
    : interpreter_(interpreter) {}

namespace {
void clear_resume_position(std::map<const ASTNode *, size_t> &positions,
                           const ASTNode *node) {
    positions.erase(node);
}
} // namespace

void StatementListExecutor::execute_statement_list(const ASTNode *node) {
    if (!node) {
        return;
    }

    debug_msg(DebugMsgId::INTERPRETER_STMT_LIST_EXEC, node->statements.size());

    auto stmt_positions = interpreter_->current_statement_positions();
    size_t start_index = 0;
    if (auto it = stmt_positions->find(node); it != stmt_positions->end()) {
        start_index = it->second;
    }

    auto clear_entry = [&]() { clear_resume_position(*stmt_positions, node); };

    try {
        for (size_t i = start_index; i < node->statements.size(); ++i) {
            (*stmt_positions)[node] = i;

            try {
                interpreter_->execute_statement(node->statements[i].get());
            } catch (const YieldException &e) {
                (*stmt_positions)[node] = e.is_from_loop ? i : (i + 1);
                throw;
            }

            (*stmt_positions)[node] = i + 1;

            if (interpreter_->get_simple_event_loop().has_tasks()) {
                interpreter_->get_simple_event_loop().run_one_cycle();
            }
        }

        clear_entry();
    } catch (const ReturnException &) {
        clear_entry();
        throw;
    } catch (const BreakException &) {
        clear_entry();
        throw;
    } catch (const ContinueException &) {
        clear_entry();
        throw;
    }
}

void StatementListExecutor::execute_compound_statement(const ASTNode *node) {
    if (!node) {
        return;
    }

    debug_msg(DebugMsgId::INTERPRETER_COMPOUND_STMT_EXEC,
              node->statements.size());

    bool pushed_scope = false;
    if (!interpreter_->is_calling_destructor()) {
        interpreter_->push_destructor_scope();
        pushed_scope = true;
    }

    auto stmt_positions = interpreter_->current_statement_positions();
    size_t start_index = 0;
    if (auto it = stmt_positions->find(node); it != stmt_positions->end()) {
        start_index = it->second;
    }

    auto clear_entry = [&]() { clear_resume_position(*stmt_positions, node); };

    try {
        for (size_t i = start_index; i < node->statements.size(); ++i) {
            (*stmt_positions)[node] = i;

            try {
                interpreter_->execute_statement(node->statements[i].get());
            } catch (const YieldException &e) {
                (*stmt_positions)[node] = e.is_from_loop ? i : (i + 1);
                if (pushed_scope) {
                    interpreter_->pop_destructor_scope();
                }
                throw;
            }

            (*stmt_positions)[node] = i + 1;
        }

        if (pushed_scope) {
            interpreter_->pop_destructor_scope();
        }
        clear_entry();
    } catch (const ReturnException &) {
        clear_entry();
        if (pushed_scope) {
            interpreter_->pop_destructor_scope();
        }
        throw;
    } catch (const BreakException &) {
        clear_entry();
        if (pushed_scope) {
            interpreter_->pop_destructor_scope();
        }
        throw;
    } catch (const ContinueException &) {
        clear_entry();
        if (pushed_scope) {
            interpreter_->pop_destructor_scope();
        }
        throw;
    }
}
