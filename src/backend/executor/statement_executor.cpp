#include "statement_executor.h"
#include "../interpreter.h"
#include "../../frontend/debug_messages.h"
#include <stdexcept>

StatementExecutor::StatementExecutor(Interpreter& interpreter) 
    : interpreter_(interpreter) {}

void StatementExecutor::execute_statement(const ASTNode *node) {
    if (!node)
        return;

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        for (const auto &stmt : node->statements) {
            execute_statement(stmt.get());
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        if (node->node_type == ASTNodeType::AST_ASSIGN) {
            if (node->left &&
                node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 配列要素への代入
                int64_t index =
                    interpreter_.evaluate_expression(node->left->array_index.get());

                // 変数の型を確認して文字列か配列かを判断
                Variable *var = interpreter_.find_variable(node->left->name);
                if (var && var->type == TYPE_STRING) {
                    // 文字列要素への代入
                    if (node->right->node_type ==
                        ASTNodeType::AST_STRING_LITERAL) {
                        interpreter_.assign_string_element(node->left->name, index,
                                              node->right->str_value);
                    } else {
                        error_msg(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR);
                        throw std::runtime_error(
                            "Non-string character assignment error");
                    }
                } else {
                    // 通常の配列要素への代入
                    int64_t value = interpreter_.evaluate_expression(node->right.get());
                    interpreter_.assign_array_element(node->left->name, index, value);
                }
            } else {
                // 通常の代入
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    interpreter_.assign_variable(node->name, node->right->str_value,
                                    node->is_const);
                } else {
                    int64_t value = interpreter_.evaluate_expression(node->right.get());
                    interpreter_.assign_variable(node->name, value, node->type_info,
                                    node->is_const);
                }
            }
        } else {
            // 変数宣言のみ
            Variable var;
            var.type = node->type_info;
            var.is_const = node->is_const;
            var.is_assigned = false;
            interpreter_.get_global_scope().variables[node->name] = var;
        }
        break;

    case ASTNodeType::AST_ARRAY_DECL: {
        Variable var;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
        var.is_const = node->is_const;
        var.is_array = true;
        var.is_assigned = false;

        // 配列サイズ決定
        if (node->array_size_expr) {
            var.array_size = static_cast<int>(
                interpreter_.evaluate_expression(node->array_size_expr.get()));
        } else {
            var.array_size = node->array_size;
        }

        if (var.array_size < 0) {
            error_msg(DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR,
                      node->name.c_str());
            throw std::runtime_error("Negative array size error");
        }

        // 配列初期化
        TypeInfo elem_type = node->type_info;
        if (elem_type == TYPE_STRING) {
            var.array_strings.resize(var.array_size, "");
        } else {
            var.array_values.resize(var.array_size, 0);
        }

        // 初期化リストがある場合
        for (size_t i = 0; i < node->children.size() &&
                           i < static_cast<size_t>(var.array_size);
             ++i) {
            const auto &child = node->children[i];
            if (child->node_type == ASTNodeType::AST_STMT_LIST) {
                // 配列リテラル [1,2,3,...] の場合
                size_t j = 0;
                for (const auto &element : child->children) {
                    if (j >= static_cast<size_t>(var.array_size))
                        break;
                    if (elem_type == TYPE_STRING) {
                        var.array_strings[j] = element->str_value;
                    } else {
                        int64_t val = interpreter_.evaluate_expression(element.get());
                        interpreter_.check_type_range(elem_type, val, node->name);
                        var.array_values[j] = val;
                    }
                    j++;
                }
                break; // 配列リテラルは一つだけ
            } else {
                // 単一要素の初期化
                if (elem_type == TYPE_STRING) {
                    var.array_strings[i] = child->str_value;
                } else {
                    int64_t val = interpreter_.evaluate_expression(child.get());
                    interpreter_.check_type_range(elem_type, val, node->name);
                    var.array_values[i] = val;
                }
            }
        }

        interpreter_.current_scope().variables[node->name] = var;
    } break;

    case ASTNodeType::AST_PRINT_STMT:
        interpreter_.get_output_manager().print_value(node->left.get());
        break;

    case ASTNodeType::AST_PRINTLN_STMT:
        interpreter_.get_output_manager().print_value_with_newline(node->left.get());
        break;

    case ASTNodeType::AST_PRINTLN_EMPTY:
        printf("\n");
        break;

    case ASTNodeType::AST_PRINTLN_MULTI_STMT:
        interpreter_.get_output_manager().print_multiple_with_newline(node->right.get());
        break;

    case ASTNodeType::AST_PRINTLNF_STMT:
        interpreter_.get_output_manager().print_formatted_with_newline(node->left.get(),
                                                      node->right.get());
        break;

    case ASTNodeType::AST_PRINTF_STMT:
        interpreter_.get_output_manager().print_formatted(node->left.get(), node->right.get());
        break;

    case ASTNodeType::AST_PRINT_MULTI_STMT:
        interpreter_.get_output_manager().print_multiple(node->right.get());
        break;

    case ASTNodeType::AST_IF_STMT: {
        int64_t cond = interpreter_.evaluate_expression(node->condition.get());
        if (cond) {
            execute_statement(node->left.get());
        } else if (node->right) {
            execute_statement(node->right.get());
        }
    } break;

    case ASTNodeType::AST_WHILE_STMT:
        try {
            while (true) {
                int64_t cond = interpreter_.evaluate_expression(node->condition.get());
                if (!cond)
                    break;
                execute_statement(node->body.get());
            }
        } catch (const BreakException &e) {
            // break文でループ脱出
        }
        break;

    case ASTNodeType::AST_FOR_STMT:
        try {
            if (node->init_expr) {
                execute_statement(node->init_expr.get());
            }
            while (true) {
                if (node->condition) {
                    int64_t cond = interpreter_.evaluate_expression(node->condition.get());
                    if (!cond)
                        break;
                }
                execute_statement(node->body.get());
                if (node->update_expr) {
                    execute_statement(node->update_expr.get());
                }
            }
        } catch (const BreakException &e) {
            // break文でループ脱出
        }
        break;

    case ASTNodeType::AST_RETURN_STMT:
        if (node->left) {
            if (node->left->node_type == ASTNodeType::AST_STRING_LITERAL) {
                throw ReturnException(node->left->str_value);
            } else {
                int64_t value = interpreter_.evaluate_expression(node->left.get());
                throw ReturnException(value);
            }
        } else {
            throw ReturnException(0);
        }
        break;

    case ASTNodeType::AST_BREAK_STMT: {
        int64_t cond = 1;
        if (node->left) {
            cond = interpreter_.evaluate_expression(node->left.get());
        }
        if (cond) {
            throw BreakException(cond);
        }
    } break;

    case ASTNodeType::AST_FUNC_DECL:
        // 実行時の関数定義をグローバルスコープに登録
        interpreter_.get_global_scope().functions[node->name] = node;
        break;

    default:
        interpreter_.evaluate_expression(node); // 式文として評価
        break;
    }
}
