#include "control_flow_executor.h"
#include "../core/interpreter.h"
#include "../../../common/ast.h"
#include "../../../common/debug_messages.h"
#include "../evaluator/expression_evaluator.h"
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
    
    try {
        int iteration = 0;
        while (true) {
            debug_msg(DebugMsgId::INTERPRETER_WHILE_CONDITION_CHECK,
                          iteration);
            
            int64_t cond = interpreter_->expression_evaluator_->evaluate_expression(
                node->condition.get());
            
            debug_msg(DebugMsgId::INTERPRETER_WHILE_CONDITION_RESULT, cond);
            
            if (!cond)
                break;
            
            try {
                debug_msg(DebugMsgId::INTERPRETER_WHILE_BODY_EXEC,
                              iteration);
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
    
    debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_END, "");
}

// FOR文の実行
void ControlFlowExecutor::execute_for_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_FOR_STMT_START, "");
    
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
                
                int64_t cond = interpreter_->expression_evaluator_->evaluate_expression(
                    node->condition.get());
                
                debug_msg(DebugMsgId::INTERPRETER_FOR_CONDITION_RESULT,
                              cond);
                
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
                debug_msg(DebugMsgId::INTERPRETER_FOR_UPDATE_EXEC,
                              iteration);
                interpreter_->execute_statement(node->update_expr.get());
            }
            
            iteration++;
        }
    } catch (const BreakException &e) {
        // break文でループ脱出
    }
}
