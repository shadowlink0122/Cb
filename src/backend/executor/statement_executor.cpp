#include "statement_executor.h"
#include "../interpreter.h"
#include "../../common/debug_messages.h"
#include "../../common/type_alias.h"
#include "../error_handler.h"
#include <stdexcept>
#include <iostream>

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
                
                // デバッグ：配列代入処理での変数情報を出力
                // printf("[DEBUG] Array assignment to '%s': var=%p\n", node->left->name.c_str(), var);
                // if (var) {
                //     printf("[DEBUG] Variable '%s': type=%d, is_array=%s, array_size=%d\n", 
                //            node->left->name.c_str(), (int)var->type, var->is_array ? "true" : "false", 
                //            var->array_size);
                // }
                
                // 配列への代入処理
                if (var && var->is_array) {
                    // 範囲チェック
                    if (index < 0 || index >= var->array_size) {
                        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, node->left->name.c_str());
                        throw std::runtime_error("Array index out of bounds");
                    }
                    
                    // 値を評価して配列要素に代入
                    int64_t rvalue = interpreter_.evaluate_expression(node->right.get());
                    
                    if (var->type == TYPE_STRING) {
                        // 文字列配列への代入（現在未対応）
                        throw std::runtime_error("String array assignment not yet implemented");
                    } else {
                        // 数値配列への代入
                        var->array_values()[index] = rvalue;
                    }
                    
                    return; // 早期リターン
                }
                
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
                // 通常の代入または配列リテラル代入
                if (node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                    // 配列リテラル代入: arr = [1, 2, 3];
                    debug_msg(DebugMsgId::ARRAY_LITERAL_ASSIGN_DEBUG);
                    interpreter_.assign_array_literal(node->name, node->right.get());
                } else if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    // 配列変数かチェック
                    Variable* var = interpreter_.find_variable(node->name);
                    if (var && var->is_array) {
                        throw std::runtime_error("Cannot assign string to array variable. Use array literal: " + node->name);
                    }
                    interpreter_.assign_variable(node->name, node->right->str_value,
                                    node->is_const);
                } else {
                    // 配列変数かチェック
                    Variable* var = interpreter_.find_variable(node->name);
                    if (var && var->is_array) {
                        throw std::runtime_error("Cannot assign scalar to array variable. Use array literal: " + node->name);
                    }
                    int64_t value = interpreter_.evaluate_expression(node->right.get());
                    interpreter_.assign_variable(node->name, value, node->type_info,
                                    node->is_const);
                }
            }
        } else {
            // 変数宣言のみ
            Variable var;
            // 型エイリアス解決を試行
            TypeInfo resolved_type = interpreter_.resolve_type_alias(node->type_info, node->type_name);
            var.type = (resolved_type != TYPE_UNKNOWN) ? resolved_type : node->type_info;
            var.is_const = node->is_const;
            var.is_assigned = false;
            // printf("[DEBUG] AST_VAR_DECL '%s': type_info=%d, str_value='%s', array_type_info.is_array=%s\n", 
            //        node->name.c_str(), (int)node->type_info, node->str_value.c_str(), 
            //        node->array_type_info.is_array() ? "true" : "false");
            
            // デバッグ：型情報の確認
            debug_msg(DebugMsgId::VAR_DECLARATION_DEBUG, node->name.c_str(),
                      node->type_info, node->type_name.c_str(), resolved_type);
            
            // 実行時型エイリアス解決
            if (node->type_info == TYPE_UNKNOWN && !node->str_value.empty()) {
                auto& registry = get_global_type_alias_registry();
                
                // 配列typedef の場合
                if (registry.is_array_alias(node->str_value)) {
                    ArrayTypeInfo array_info = registry.resolve_array_alias(node->str_value);
                    var.array_type_info = array_info;
                    var.type = array_info.base_type; // 基底型を設定
                    var.is_array = true;
                    
                    // 1次元目のサイズを設定
                    if (!array_info.dimensions.empty()) {
                        auto& first_dim = array_info.dimensions[0];
                        var.array_size = first_dim.is_dynamic ? -1 : first_dim.size;
                    } else {
                        var.array_size = -1;
                    }
                    
                    // メモリ初期化
                    if (var.array_size > 0) {
                        if (array_info.base_type == TYPE_STRING) {
                            var.array_strings().resize(var.array_size, "");
                        } else {
                            var.array_values().resize(var.array_size, 0);
                        }
                    }
                    
                    // printf("[DEBUG] Type alias resolved: %s -> %s, is_array=true, array_size=%d\n", 
                    //        node->str_value.c_str(), array_info.to_string().c_str(), var.array_size);
                } else {
                    // 通常の型エイリアス（未実装）
                    // printf("[DEBUG] Normal type alias resolution not yet implemented: %s\n", node->str_value.c_str());
                }
            } else if (node->array_type_info.is_array()) {
                // 既に解決済みの配列typedef
                var.array_type_info = node->array_type_info;
                debug_msg(DebugMsgId::ARRAY_TYPEDEF_DETECTED_DEBUG);
                var.is_array = true;
                var.type = node->array_type_info.base_type;
                
                if (!node->array_type_info.dimensions.empty()) {
                    auto& first_dim = node->array_type_info.dimensions[0];
                    var.array_size = first_dim.is_dynamic ? -1 : first_dim.size;
                } else {
                    var.array_size = -1;
                }
                
                if (var.array_size > 0) {
                    if (node->array_type_info.base_type == TYPE_STRING) {
                        var.array_strings().resize(var.array_size, "");
                    } else {
                        var.array_values().resize(var.array_size, 0);
                    }
                }
            } else if (var.type >= TYPE_ARRAY_BASE) {
                // 通常の配列型: int[3], string[5] など
                debug_msg(DebugMsgId::NORMAL_ARRAY_TYPE_DEBUG);
                var.is_array = true;
                // 基底型を取得
                TypeInfo base_type = static_cast<TypeInfo>(var.type - TYPE_ARRAY_BASE);
                
                // typedef配列の場合、レジストリからサイズを取得
                if (!node->type_name.empty()) {
                    auto& registry = get_global_type_alias_registry();
                    if (registry.is_array_alias(node->type_name)) {
                        ArrayTypeInfo array_info = registry.resolve_array_alias(node->type_name);
                        if (!array_info.dimensions.empty()) {
                            var.array_size = array_info.dimensions[0].is_dynamic ? -1 : array_info.dimensions[0].size;
                        }
                    }
                } else if (node->array_size_expr) {
                    // 直接的な配列宣言の場合
                    var.array_size = static_cast<int>(
                        interpreter_.evaluate_expression(node->array_size_expr.get()));
                } else {
                    var.array_size = -1; // 動的サイズ
                }
                
                debug_msg(DebugMsgId::ARRAY_SIZE_DEBUG);
                
                // メモリ初期化
                if (var.array_size > 0) {
                    if (base_type == TYPE_STRING) {
                        var.array_strings().resize(var.array_size, "");
                    } else {
                        var.array_values().resize(var.array_size, 0);
                    }
                }
            } else {
                var.is_array = false;
            }
            
            interpreter_.get_global_scope().variables.insert_or_assign(node->name, std::move(var));
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
            var.array_strings().resize(var.array_size, "");
        } else {
            var.array_values().resize(var.array_size, 0);
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
                        var.array_strings()[j] = element->str_value;
                    } else {
                        int64_t val = interpreter_.evaluate_expression(element.get());
                        interpreter_.check_type_range(elem_type, val, node->name);
                        var.array_values()[j] = val;
                    }
                    j++;
                }
                break; // 配列リテラルは一つだけ
            } else {
                // 単一要素の初期化
                if (elem_type == TYPE_STRING) {
                    var.array_strings()[i] = child->str_value;
                } else {
                    int64_t val = interpreter_.evaluate_expression(child.get());
                    interpreter_.check_type_range(elem_type, val, node->name);
                    var.array_values()[i] = val;
                }
            }
        }

        interpreter_.current_scope().variables.insert_or_assign(node->name, std::move(var));
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
    
    case ASTNodeType::AST_TYPEDEF_DECL:
        // typedef宣言の処理
        execute_typedef_statement(node);
        break;
    
    case ASTNodeType::AST_IMPORT_STMT:
        // import文の処理
        if (!interpreter_.process_import(node)) {
            throw std::runtime_error("Failed to process import: " + node->module_name);
        }
        break;
    
    case ASTNodeType::AST_TRY_STMT:
        execute_try_statement(node);
        break;
    
    case ASTNodeType::AST_THROW_STMT:
        execute_throw_statement(node);
        break;
    
    case ASTNodeType::AST_CATCH_STMT:
    case ASTNodeType::AST_FINALLY_STMT:
        // これらは通常TRY_STMTの一部として処理される
        // 単独で呼ばれることはないが、安全のため何もしない
        break;

    default:
        interpreter_.evaluate_expression(node); // 式文として評価
        break;
    }
}

void StatementExecutor::execute_try_statement(const ASTNode *node) {
    try {
        // try blockを実行
        if (node->try_body) {
            execute_statement(node->try_body.get());
        }
    } catch (const std::exception& e) {
        // catch blockがある場合は実行
        if (node->catch_body) {
            // TODO: 例外変数をスコープに追加
            execute_statement(node->catch_body.get());
        }
    }
    
    // finally blockがある場合は必ず実行
    if (node->finally_body) {
        execute_statement(node->finally_body.get());
    }
}

void StatementExecutor::execute_throw_statement(const ASTNode *node) {
    if (node->throw_expr) {
        // throw式を評価してメッセージを取得
        if (node->throw_expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
            throw std::runtime_error(node->throw_expr->str_value);
        } else {
            // 数値や他の式の場合は文字列化
            int64_t value = interpreter_.evaluate_expression(node->throw_expr.get());
            throw std::runtime_error("Exception: " + std::to_string(value));
        }
    } else {
        throw std::runtime_error("Unspecified exception");
    }
}

void StatementExecutor::execute_typedef_statement(const ASTNode *node) {
    if (!node || node->node_type != ASTNodeType::AST_TYPEDEF_DECL) {
        throw std::runtime_error("Invalid typedef declaration");
    }

    auto& registry = get_global_type_alias_registry();
    bool success = false;

    // 配列型typedef かどうかをチェック
    if (node->array_type_info.is_array()) {
        // 配列型typedefの場合
        debug_msg(DebugMsgId::TYPEDEF_REGISTER, node->name.c_str(), 
                  node->array_type_info.to_string().c_str());
        success = registry.register_array_alias(node->name, node->array_type_info);
    } else {
        // 通常の型typedef
        debug_msg(DebugMsgId::TYPEDEF_REGISTER, node->name.c_str(), 
                  type_info_to_string(node->type_info));
        success = registry.register_alias(node->name, node->type_info);
    }
    
    if (!success) {
        throw std::runtime_error("Failed to register type alias: " + node->name);
    }
}
