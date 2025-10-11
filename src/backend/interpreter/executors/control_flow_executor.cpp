#include "control_flow_executor.h"
#include "../../../common/ast.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include "../evaluator/core/evaluator.h"
#include "../services/debug_service.h"
#include <iostream>

// IF文の実行
void ControlFlowExecutor::execute_if_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_IF_STMT_START, "");

    int64_t cond = interpreter_->expression_evaluator_->evaluate_expression(
        node->condition.get());

    debug_msg(DebugMsgId::INTERPRETER_IF_CONDITION_RESULT, cond);

    if (cond) {
        debug_msg(DebugMsgId::INTERPRETER_IF_THEN_EXEC, "");
        interpreter_->execute_statement(node->left.get());
    } else if (node->right) {
        debug_msg(DebugMsgId::INTERPRETER_IF_ELSE_EXEC, "");
        interpreter_->execute_statement(node->right.get());
    }

    debug_msg(DebugMsgId::INTERPRETER_IF_STMT_END, "");
}

// WHILE文の実行
void ControlFlowExecutor::execute_while_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_START, "");

    // whileループ用のdeferスコープのみを作成（変数スコープは作成しない）
    interpreter_->push_defer_scope();

    try {
        int iteration = 0;
        while (true) {
            debug_msg(DebugMsgId::INTERPRETER_WHILE_CONDITION_CHECK, iteration);

            int64_t cond =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->condition.get());

            debug_msg(DebugMsgId::INTERPRETER_WHILE_CONDITION_RESULT, cond);

            if (!cond)
                break;

            try {
                debug_msg(DebugMsgId::INTERPRETER_WHILE_BODY_EXEC, iteration);
                interpreter_->execute_statement(node->body.get());
                iteration++;
            } catch (const ContinueException &e) {
                // continue文でループ継続
                continue;
            }
        }
    } catch (const BreakException &e) {
        // break文でループ脱出
        debug_msg(DebugMsgId::INTERPRETER_WHILE_BREAK, "");
    }

    // whileループのdeferスコープを終了（deferを実行）
    interpreter_->pop_defer_scope();

    debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_END, "");
}

// FOR文の実行
void ControlFlowExecutor::execute_for_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_FOR_STMT_START, "");

    // forループ用のdeferスコープのみを作成（変数スコープは作成しない）
    // これにより、deferはループ終了時に実行されるが、
    // ループ内で作成された変数は親スコープに属する
    interpreter_->push_defer_scope();

    try {
        if (node->init_expr) {
            debug_msg(DebugMsgId::INTERPRETER_FOR_INIT_EXEC, "");
            interpreter_->execute_statement(node->init_expr.get());
        }

        int iteration = 0;
        while (true) {
            if (node->condition) {
                debug_msg(DebugMsgId::INTERPRETER_FOR_CONDITION_CHECK,
                          iteration);

                int64_t cond =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        node->condition.get());

                debug_msg(DebugMsgId::INTERPRETER_FOR_CONDITION_RESULT, cond);

                if (!cond)
                    break;
            }

            try {
                debug_msg(DebugMsgId::INTERPRETER_FOR_BODY_EXEC, iteration);
                interpreter_->execute_statement(node->body.get());
            } catch (const ContinueException &e) {
                // continue文でループ継続、update部分だけ実行
                debug_msg(DebugMsgId::INTERPRETER_FOR_CONTINUE, iteration);
            }

            if (node->update_expr) {
                debug_msg(DebugMsgId::INTERPRETER_FOR_UPDATE_EXEC, iteration);
                interpreter_->execute_statement(node->update_expr.get());
            }

            iteration++;
        }
    } catch (const BreakException &e) {
        // break文でループ脱出
    }

    // forループのdeferスコープを終了（deferを実行）
    interpreter_->pop_defer_scope();
}

// SWITCH文の実行
void ControlFlowExecutor::execute_switch_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_SWITCH_STMT_START, "");

    // switch対象の式を評価
    int64_t switch_value =
        interpreter_->expression_evaluator_->evaluate_expression(
            node->switch_expr.get());

    debug_msg(DebugMsgId::INTERPRETER_SWITCH_VALUE, switch_value);

    // 各case節をチェック
    bool matched = false;
    for (const auto &case_clause : node->cases) {
        // case値のいずれかにマッチするかチェック
        for (const auto &case_value : case_clause->case_values) {
            if (match_case_value(node->switch_expr.get(), case_value.get())) {
                debug_msg(DebugMsgId::INTERPRETER_SWITCH_CASE_MATCHED, "");
                interpreter_->execute_statement(case_clause->case_body.get());
                matched = true;
                break; // 自動break（fallthrough無し）
            }
        }
        if (matched)
            break;
    }

    // どのcaseにもマッチしなかった場合、else節を実行
    if (!matched && node->else_body) {
        debug_msg(DebugMsgId::INTERPRETER_SWITCH_ELSE_EXEC, "");
        interpreter_->execute_statement(node->else_body.get());
    }

    debug_msg(DebugMsgId::INTERPRETER_SWITCH_STMT_END, "");
}

// Switch文のcase値マッチング
bool ControlFlowExecutor::match_case_value(const ASTNode *switch_expr,
                                           const ASTNode *case_value) {
    // switch対象の値を評価
    int64_t switch_val =
        interpreter_->expression_evaluator_->evaluate_expression(switch_expr);

    // 範囲式の場合
    if (case_value->node_type == ASTNodeType::AST_RANGE_EXPR) {
        int64_t range_start =
            interpreter_->expression_evaluator_->evaluate_expression(
                case_value->range_start.get());
        int64_t range_end =
            interpreter_->expression_evaluator_->evaluate_expression(
                case_value->range_end.get());

        debug_msg(DebugMsgId::INTERPRETER_SWITCH_RANGE_CHECK, range_start,
                  range_end);

        // 閉区間でのチェック [start, end]
        return switch_val >= range_start && switch_val <= range_end;
    }

    // 単一値の場合
    int64_t case_val =
        interpreter_->expression_evaluator_->evaluate_expression(case_value);

    debug_msg(DebugMsgId::INTERPRETER_SWITCH_VALUE_CHECK, switch_val, case_val);

    return switch_val == case_val;
}
