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

                // v0.12.0: auto_yieldモードの場合、各イテレーション後にyield
                // これにより、whileループが他のタスクを独占しない
                if (interpreter_->is_in_auto_yield_mode()) {
                    throw YieldException(true); // ループ内の自動yield
                }

                // v0.13.0 Phase 2.0:
                // 非auto_yieldモードでも、バックグラウンドタスクがあれば
                // 各イテレーション後に1サイクル実行して協調的マルチタスクを実現
                if (!interpreter_->is_in_auto_yield_mode()) {
                    interpreter_->run_background_tasks_one_cycle();
                }

                iteration++;
            } catch (const ContinueException &e) {
                // continue文でループ継続
                continue;
            } catch (const YieldException &yield_exc) {
                // v0.12.0: auto_yieldモードでのyield
                if (yield_exc.is_from_loop) {
                    throw; // auto_yieldはそのまま再スロー
                }
                // 明示的なyieldはループ継続扱いにしてタスク側で同じステートメントを再開
                throw YieldException(true);
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

    // v0.10.0: forループにもdeferスコープを作成
    // forループ内でdeferが使えるようにする
    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "[FOR_LOOP] About to call push_defer_scope()");
    interpreter_->push_defer_scope();

    // v0.13.0 Phase 2.0 FIX: init式で宣言された変数名を記憶
    std::string init_var_name;
    bool init_var_declared = false;

    try {
        // v0.13.0 Phase 2.0: auto_yieldモードでのループ実行
        // init式は初回のみ実行（変数宣言の重複を避けるため）
        // init式で宣言された変数が既に存在する場合、init式をスキップ
        bool should_execute_init = false;
        if (node->init_expr) {
            if (node->init_expr->node_type == ASTNodeType::AST_VAR_DECL &&
                !node->init_expr->name.empty()) {
                // 変数宣言の場合、現在のスコープに変数が存在しない場合のみ実行
                // v0.13.0 Phase 2.0 FIX: 現在のスコープのみをチェック
                // これにより、複数のforループで同じ変数名を使っても干渉しない
                should_execute_init =
                    !interpreter_->variable_exists_in_current_scope(
                        node->init_expr->name);

                if (should_execute_init) {
                    init_var_name = node->init_expr->name;
                    init_var_declared = true;
                }
            } else {
                // 変数宣言でない場合（代入など）は常に実行
                should_execute_init = true;
            }

            if (should_execute_init) {
                debug_msg(DebugMsgId::INTERPRETER_FOR_INIT_EXEC, "");
                interpreter_->execute_statement(node->init_expr.get());
            }
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
            } catch (const YieldException &yield_exc) {
                // v0.12.0: auto_yieldモードでのyield
                if (yield_exc.is_from_loop) {
                    // 自動yield時のみupdate式を先に実行
                    if (node->update_expr) {
                        debug_msg(DebugMsgId::INTERPRETER_FOR_UPDATE_EXEC,
                                  iteration);
                        interpreter_->execute_statement(
                            node->update_expr.get());
                    }
                    throw; // そのまま再スロー
                }

                // 明示的なyieldはループ継続扱いにしてタスク側で同じステートメントを再開
                throw YieldException(true);
            }

            if (node->update_expr) {
                debug_msg(DebugMsgId::INTERPRETER_FOR_UPDATE_EXEC, iteration);
                interpreter_->execute_statement(node->update_expr.get());
            }

            // v0.12.0: auto_yieldモードの場合、各イテレーション後にyield
            // これにより、forループが他のタスクを独占しない
            if (interpreter_->is_in_auto_yield_mode()) {
                throw YieldException(true); // ループ内の自動yield
            }

            // v0.13.0 Phase 2.0:
            // 非auto_yieldモードでも、バックグラウンドタスクがあれば
            // 各イテレーション後に1サイクル実行して協調的マルチタスクを実現
            if (!interpreter_->is_in_auto_yield_mode()) {
                interpreter_->run_background_tasks_one_cycle();
            }

            iteration++;
        }
    } catch (const BreakException &e) {
        // break文でループ脱出
        debug_msg(DebugMsgId::INTERPRETER_WHILE_BREAK);
    }

    // v0.13.0 Phase 2.0 FIX: init式で宣言された変数を削除
    // これにより、次のforループで同じ変数名を使える
    if (init_var_declared && !init_var_name.empty()) {
        interpreter_->remove_variable_from_current_scope(init_var_name);
    }

    // forループのdeferスコープを終了（deferを実行）
    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "[FOR_LOOP] About to call pop_defer_scope()");
    interpreter_->pop_defer_scope();
    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "[FOR_LOOP] Finished pop_defer_scope()");
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

// ========================================================================
// Match文の実行（v0.11.0 パターンマッチング）
// ========================================================================
void ControlFlowExecutor::execute_match_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_SWITCH_STMT_START, "");

    // match対象の式を評価
    const ASTNode *match_expr = node->match_expr.get();

    // match式を評価してEnum変数を取得
    Variable enum_value;
    // TODO: 将来的にクリーンアップが必要な場合に使用
    // bool needs_cleanup = false;

    if (match_expr->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数の場合、直接取得
        std::string var_name = match_expr->name;
        Variable *enum_var = interpreter_->find_variable(var_name);
        if (!enum_var) {
            throw std::runtime_error(
                "Undefined variable in match expression: " + var_name);
        }
        enum_value = *enum_var;
    } else if (match_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出しの場合、評価してReturnExceptionから値を取得
        try {
            interpreter_->eval_expression(match_expr);
            throw std::runtime_error(
                "Function in match expression did not return a value");
        } catch (const ReturnException &ret) {
            if (ret.is_struct && ret.struct_value.is_enum) {
                enum_value = ret.struct_value;
                // needs_cleanup = true;
            } else {
                throw std::runtime_error(
                    "Function in match expression must return an enum");
            }
        }
    } else if (match_expr->node_type == ASTNodeType::AST_ENUM_CONSTRUCT) {
        // Enum構築式の場合、評価して値を作成
        enum_value.is_enum = true;
        enum_value.enum_variant = match_expr->enum_member;
        if (!match_expr->arguments.empty()) {
            TypedValue typed_result =
                interpreter_->evaluate_typed(match_expr->arguments[0].get());
            enum_value.has_associated_value = true;

            // 文字列型の場合
            if (typed_result.type.type_info == TYPE_STRING) {
                enum_value.associated_str_value = typed_result.string_value;
            }
            // 数値型の場合
            else {
                enum_value.associated_int_value = typed_result.as_numeric();
            }
        } else {
            enum_value.has_associated_value = false;
        }
        // needs_cleanup = true;
    } else {
        throw std::runtime_error("Match expression must be a variable, "
                                 "function call, or enum constructor");
    }

    // Enum型チェック
    if (!enum_value.is_enum) {
        throw std::runtime_error("Match expression must be an enum type");
    }

    debug_msg(DebugMsgId::INTERPRETER_SWITCH_VALUE, 0);

    // 各match armをチェック
    bool matched = false;
    for (const auto &arm : node->match_arms) {
        bool arm_matches = false;

        switch (arm.pattern_type) {
        case PatternType::PATTERN_ENUM_VARIANT:
            // Enumバリアントパターン
            if (enum_value.enum_variant == arm.variant_name) {
                arm_matches = true;

                // バインディングがある場合、関連値を束縛
                if (!arm.bindings.empty() && enum_value.has_associated_value) {
                    const std::string &binding_name = arm.bindings[0];

                    // ワイルドカード(_)の場合は変数を作成しない
                    if (binding_name != "_") {
                        // 型に応じて変数を作成
                        if (!enum_value.associated_str_value.empty()) {
                            // 文字列型の場合
                            interpreter_->assign_variable(
                                binding_name, enum_value.associated_str_value);
                        } else {
                            // 数値型の場合
                            interpreter_->assign_variable(
                                binding_name, enum_value.associated_int_value,
                                TYPE_INT);
                        }
                    }
                }
            }
            break;

        case PatternType::PATTERN_WILDCARD:
            // ワイルドカードパターン（常にマッチ）
            arm_matches = true;
            break;

        default:
            throw std::runtime_error("Unsupported pattern type in match arm");
        }

        if (arm_matches) {
            debug_msg(DebugMsgId::INTERPRETER_SWITCH_CASE_MATCHED, "");

            // armの本体を実行
            interpreter_->execute_statement(arm.body.get());

            // バインディング変数をクリーンアップ
            if (!arm.bindings.empty()) {
                for (const auto &binding : arm.bindings) {
                    // ワイルドカード(_)の場合はクリーンアップ不要
                    if (binding == "_") {
                        continue;
                    }
                    // TODO: スコープ管理を適切に実装（現在は単純削除）
                    Variable *binding_var =
                        interpreter_->find_variable(binding);
                    if (binding_var) {
                        // 変数の削除は慎重に行う必要がある
                        // 現在はスコープが自動管理されるため、ここでは何もしない
                    }
                }
            }

            matched = true;
            break; // 自動break（最初にマッチしたarmのみ実行）
        }
    }

    if (!matched) {
        throw std::runtime_error(
            "Non-exhaustive match: no arm matched the enum variant '" +
            enum_value.enum_variant + "'");
    }

    debug_msg(DebugMsgId::INTERPRETER_SWITCH_STMT_END, "");
}
