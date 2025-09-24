#include "interpreter.h"
#include "../common/ast.h"
#include "../common/debug.h"
#include "../common/utf8_utils.h"
#include "array_manager.h"
#include "error_handler.h"
#include "evaluator/expression_evaluator.h"
#include "type_manager.h"
#include "variable_manager.h"
#include <cstdlib>
#include <iostream>
#include <stdexcept>

Interpreter::Interpreter(bool debug)
    : debug_mode(debug), output_manager_(std::make_unique<OutputManager>(this)),
      variable_manager_(std::make_unique<VariableManager>(this)),
      type_manager_(std::make_unique<TypeManager>(this)) {

    // ExpressionEvaluatorを最初に初期化
    expression_evaluator_ = std::make_unique<ExpressionEvaluator>(*this);

    // ArrayManagerはVariableManagerとExpressionEvaluatorが必要なので後で初期化
    array_manager_ = std::make_unique<ArrayManager>(
        variable_manager_.get(), expression_evaluator_.get());

    // StatementExecutorを初期化
    statement_executor_ = std::make_unique<StatementExecutor>(*this);

    // 環境変数からデバッグモード設定
    const char *env_debug = std::getenv("CB_DEBUG_MODE");
    if (env_debug && env_debug[0] == '1') {
        debug_mode = true;
    }

    // グローバルスコープを初期化
    scope_stack.push_back(global_scope);
}

Interpreter::~Interpreter() = default;

void Interpreter::push_scope() { variable_manager_->push_scope(); }

void Interpreter::pop_scope() { variable_manager_->pop_scope(); }

Scope &Interpreter::current_scope() {
    return variable_manager_->current_scope();
}

Variable *Interpreter::find_variable(const std::string &name) {
    return variable_manager_->find_variable(name);
}

const ASTNode *Interpreter::find_function(const std::string &name) {
    // グローバルスコープの関数を検索
    auto func_it = global_scope.functions.find(name);
    if (func_it != global_scope.functions.end()) {
        return func_it->second;
    }
    return nullptr;
}

void Interpreter::register_global_declarations(const ASTNode *node) {
    if (!node)
        return;

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        for (const auto &stmt : node->statements) {
            register_global_declarations(stmt.get());
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        if (node->node_type == ASTNodeType::AST_MULTIPLE_VAR_DECL) {
            // 複数変数宣言の場合、各子ノードを処理
            for (const auto &child : node->children) {
                if (child->node_type == ASTNodeType::AST_VAR_DECL) {
                    register_global_declarations(child.get());
                }
            }
        } else if (node->node_type == ASTNodeType::AST_ASSIGN) {
            // グローバル変数の重複宣言チェック
            if (global_scope.variables.find(node->name) !=
                global_scope.variables.end()) {
                error_msg(DebugMsgId::VAR_REDECLARE_ERROR, node->name.c_str());
                throw std::runtime_error("Variable redeclaration error");
            }

            // グローバル変数の初期化
            Variable var;
            var.type =
                node->type_info != TYPE_VOID ? node->type_info : TYPE_INT;
            var.is_const = node->is_const;
            var.is_assigned = false;

            if (node->right) {
                int64_t value = expression_evaluator_->evaluate_expression(
                    node->right.get());
                if (var.type == TYPE_STRING) {
                    var.str_value = node->right->str_value;
                } else {
                    var.value = value;
                    check_type_range(var.type, value, node->name);
                }
                var.is_assigned = true;
            }

            global_scope.variables[node->name] = var;
        } else if (node->node_type == ASTNodeType::AST_VAR_DECL) {
            // グローバル変数宣言をVariableManagerに委譲
            variable_manager_->declare_global_variable(node);
        }
        break;

    case ASTNodeType::AST_ARRAY_DECL:
        // 配列宣言をArrayManagerに委譲
        array_manager_->declare_array(node);
        break;

    case ASTNodeType::AST_FUNC_DECL:
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER, node->name.c_str());
        global_scope.functions[node->name] = node;
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE, node->name.c_str());
        break;

    case ASTNodeType::AST_TYPEDEF_DECL:
        // typedef宣言をTypeManagerに委譲
        type_manager_->register_typedef(node->name, node->type_name);
        break;

    case ASTNodeType::AST_ARRAY_ASSIGN:
        // 配列代入は実行時に処理
        break;

    default:
        break;
    }
}

void Interpreter::process(const ASTNode *ast) {
    debug_msg(DebugMsgId::INTERPRETER_START);
    if (!ast) {
        debug_msg(DebugMsgId::AST_IS_NULL);
        return;
    }

    debug_msg(DebugMsgId::GLOBAL_DECL_START);
    // まずグローバル宣言を登録
    register_global_declarations(ast);
    debug_msg(DebugMsgId::GLOBAL_DECL_COMPLETE);

    debug_msg(DebugMsgId::MAIN_FUNC_SEARCH);
    // main関数を探して実行
    const ASTNode *main_func = find_function("main");
    if (!main_func) {
        error_msg(DebugMsgId::MAIN_FUNC_NOT_FOUND_ERROR);
        throw std::runtime_error("Main function not found");
    }
    debug_msg(DebugMsgId::MAIN_FUNC_FOUND);

    try {
        push_scope();
        debug_msg(DebugMsgId::MAIN_FUNC_FOUND, "main function execute");

        if (main_func->body) {
            debug_msg(DebugMsgId::MAIN_FUNC_FOUND, "main function body exists");
        } else {
            debug_msg(DebugMsgId::MAIN_FUNC_FOUND,
                      "main function body is null");
        }

        execute_statement(main_func->body.get());
        pop_scope();
    } catch (const ReturnException &e) {
        debug_msg(DebugMsgId::MAIN_FUNC_EXIT, e.value);
    }
}

int64_t Interpreter::evaluate(const ASTNode *node) {
    return expression_evaluator_->evaluate_expression(node);
}

// N次元配列リテラル処理の再帰関数
void Interpreter::process_ndim_array_literal(const ASTNode *literal_node,
                                             Variable &var, TypeInfo elem_type,
                                             int &flat_index, int max_size) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        return;
    }

    for (const auto &element : literal_node->arguments) {
        if (flat_index >= max_size)
            break;

        if (element->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 再帰的に処理（より深い次元）
            process_ndim_array_literal(element.get(), var, elem_type,
                                       flat_index, max_size);
        } else {
            // 最終要素の処理
            if (elem_type == TYPE_STRING) {
                if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    var.multidim_array_strings[flat_index] = element->str_value;
                    if (debug_mode) {
                        debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                                  ("Set string element[" +
                                   std::to_string(flat_index) +
                                   "] = " + element->str_value)
                                      .c_str());
                    }
                }
            } else {
                int64_t val =
                    expression_evaluator_->evaluate_expression(element.get());
                var.multidim_array_values[flat_index] = val;
                if (debug_mode) {
                    debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                              ("Set element[" + std::to_string(flat_index) +
                               "] = " + std::to_string(val))
                                  .c_str());
                }
            }
            flat_index++;
        }
    }
}

void Interpreter::execute_statement(const ASTNode *node) {
    if (!node)
        return;

    if (debug_mode) {
        const char *node_type_name = "UNKNOWN";
        switch (node->node_type) {
        case ASTNodeType::AST_PRINT_STMT:
            node_type_name = "AST_PRINT_STMT";
            break;
        case ASTNodeType::AST_PRINTLN_STMT:
            node_type_name = "AST_PRINTLN_STMT";
            break;
        case ASTNodeType::AST_STMT_LIST:
            node_type_name = "AST_STMT_LIST";
            break;
        case ASTNodeType::AST_VAR_DECL:
            node_type_name = "AST_VAR_DECL";
            break;
        case ASTNodeType::AST_MULTIPLE_VAR_DECL:
            node_type_name = "AST_MULTIPLE_VAR_DECL";
            break;
        case ASTNodeType::AST_ASSIGN:
            node_type_name = "AST_ASSIGN";
            break;
        case ASTNodeType::AST_ARRAY_DECL:
            node_type_name = "AST_ARRAY_DECL";
            break;
        case ASTNodeType::AST_FOR_STMT:
            node_type_name = "AST_FOR_STMT";
            break;
        case ASTNodeType::AST_COMPOUND_STMT:
            node_type_name = "AST_COMPOUND_STMT";
            break;
        default:
            break;
        }
        debug_msg(DebugMsgId::VAR_DECLARATION_DEBUG, node_type_name);
    }

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        for (const auto &stmt : node->statements) {
            execute_statement(stmt.get());
        }
        break;

    case ASTNodeType::AST_COMPOUND_STMT:
        for (const auto &stmt : node->statements) {
            execute_statement(stmt.get());
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        // 変数宣言と代入をVariableManagerに委譲
        variable_manager_->process_var_decl_or_assign(node);
        break;

    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
        statement_executor_->execute_multiple_var_decl(node);
        break;

    case ASTNodeType::AST_ARRAY_DECL:
        statement_executor_->execute_array_decl(node);
        break;

    case ASTNodeType::AST_PRINT_STMT:
        debug_msg(DebugMsgId::PRINT_EXECUTING_STATEMENT);
        if (!node->arguments.empty()) {
            // 複数引数のprint文（再帰下降パーサー対応）
            debug_msg(DebugMsgId::PRINT_STATEMENT_HAS_ARGS);
            output_manager_->print_multiple(node);
        } else if (node->left) {
            // 単一引数のprint文
            if (debug_mode) {
                printf("[DEBUG] Print statement has left node\n");
            }
            print_value(node->left.get());
        } else {
            if (debug_mode) {
                printf("[DEBUG] Print statement has no arguments\n");
            }
        }
        break;

    case ASTNodeType::AST_PRINTLN_STMT:
        if (node->left) {
            // 単一引数のprintln文
            output_manager_->print_value_with_newline(node->left.get());
        } else if (!node->arguments.empty()) {
            // 複数引数のprintln文（再帰下降パーサー対応）
            output_manager_->print_multiple_with_newline(node);
        } else {
            // 引数なしのprintln（改行のみ）
            output_manager_->print_newline();
        }
        break;

    case ASTNodeType::AST_PRINTLN_EMPTY:
        output_manager_->print_newline();
        break;

    case ASTNodeType::AST_PRINTF_STMT:
        output_manager_->print_formatted(node->left.get(), node->right.get());
        break;

    case ASTNodeType::AST_PRINTLNF_STMT:
        output_manager_->print_formatted_with_newline(node->left.get(),
                                                      node->right.get());
        break;

    case ASTNodeType::AST_IF_STMT: {
        int64_t cond =
            expression_evaluator_->evaluate_expression(node->condition.get());
        if (cond) {
            execute_statement(node->left.get());
        } else if (node->right) {
            execute_statement(node->right.get());
        }
    } break;

    case ASTNodeType::AST_WHILE_STMT:
        try {
            while (true) {
                int64_t cond = expression_evaluator_->evaluate_expression(
                    node->condition.get());
                if (!cond)
                    break;
                try {
                    execute_statement(node->body.get());
                } catch (const ContinueException &e) {
                    // continue文でループ継続
                    continue;
                }
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
                    int64_t cond = expression_evaluator_->evaluate_expression(
                        node->condition.get());
                    if (!cond)
                        break;
                }
                try {
                    execute_statement(node->body.get());
                } catch (const ContinueException &e) {
                    // continue文でループ継続、update部分だけ実行
                }
                if (node->update_expr) {
                    execute_statement(node->update_expr.get());
                }
            }
        } catch (const BreakException &e) {
            // break文でループ脱出
        }
        break;

    case ASTNodeType::AST_RETURN_STMT:
        if (debug_mode) {
            std::cerr << "[DEBUG] Processing return statement" << std::endl;
        }
        if (node->left) {
            if (debug_mode) {
                std::cerr << "[DEBUG] Return has expression, type: "
                          << static_cast<int>(node->left->node_type)
                          << std::endl;
            }
            // 配列リテラルの直接返却をサポート
            if (node->left->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                const std::vector<std::unique_ptr<ASTNode>> &elements =
                    node->left->arguments;
                if (debug_mode) {
                    std::cerr << "[DEBUG] Returning array literal with "
                              << elements.size() << " elements" << std::endl;
                }
                // 配列リテラルを処理
                std::vector<int64_t> array_values;
                std::vector<std::string> array_strings;
                bool is_string_array = false;

                // 最初の要素で型を判定
                if (!elements.empty()) {
                    if (elements[0]->node_type ==
                        ASTNodeType::AST_STRING_LITERAL) {
                        is_string_array = true;
                    }
                }

                // 全要素を評価
                for (size_t i = 0; i < elements.size(); i++) {
                    const auto &element = elements[i];
                    if (is_string_array) {
                        if (element->node_type !=
                            ASTNodeType::AST_STRING_LITERAL) {
                            throw std::runtime_error(
                                "Type mismatch in array literal return: "
                                "expected string");
                        }
                        array_strings.push_back(element->str_value);
                    } else {
                        if (element->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            throw std::runtime_error(
                                "Type mismatch in array literal return: "
                                "expected number");
                        }
                        int64_t value =
                            expression_evaluator_->evaluate_expression(
                                element.get());
                        array_values.push_back(value);
                    }
                }

                // ReturnExceptionで配列を返す
                if (is_string_array) {
                    // 文字列配列を3D形式に変換
                    std::vector<std::vector<std::vector<std::string>>>
                        str_array_3d;
                    std::vector<std::vector<std::string>> str_array_2d;
                    str_array_2d.push_back(array_strings);
                    str_array_3d.push_back(str_array_2d);
                    throw ReturnException(str_array_3d, "string[]",
                                          TYPE_STRING);
                } else {
                    // 整数配列を3D形式に変換
                    std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
                    std::vector<std::vector<int64_t>> int_array_2d;
                    int_array_2d.push_back(array_values);
                    int_array_3d.push_back(int_array_2d);
                    throw ReturnException(int_array_3d, "int[]", TYPE_INT);
                }
            } else if (node->left->node_type ==
                       ASTNodeType::AST_STRING_LITERAL) {
                throw ReturnException(node->left->str_value);
            } else if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                if (debug_mode) {
                    std::cerr << "[DEBUG] Return variable: " << node->left->name
                              << std::endl;
                }
                // 変数の場合、配列変数かチェック
                Variable *var = find_variable(node->left->name);
                if (var && var->is_array) {
                    if (debug_mode) {
                        std::cerr << "[DEBUG] Returning array variable, size: "
                                  << var->array_values.size() << std::endl;
                    }
                    // 配列変数のreturn
                    TypeInfo type_info = var->type;
                    std::string type_name =
                        node->left->name; // 配列名を仮の型名として使用

                    if (type_info ==
                            static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT) ||
                        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                           TYPE_LONG) ||
                        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                           TYPE_SHORT) ||
                        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                           TYPE_TINY) ||
                        type_info == static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                           TYPE_BOOL)) {
                        // 整数配列を3D配列として格納
                        std::vector<std::vector<std::vector<int64_t>>>
                            int_array_3d;
                        std::vector<std::vector<int64_t>> int_array_2d;
                        std::vector<int64_t> int_array_1d;

                        for (size_t i = 0; i < var->array_values.size(); ++i) {
                            int_array_1d.push_back(var->array_values[i]);
                            if (debug_mode) {
                                std::cerr << "[DEBUG] Array element[" << i
                                          << "] = " << var->array_values[i]
                                          << std::endl;
                            }
                        }
                        int_array_2d.push_back(int_array_1d);
                        int_array_3d.push_back(int_array_2d);

                        if (debug_mode) {
                            std::cerr
                                << "[DEBUG] Throwing ReturnException with array"
                                << std::endl;
                        }
                        throw ReturnException(int_array_3d, type_name,
                                              type_info);
                    } else if (type_info ==
                                   static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                         TYPE_STRING) ||
                               type_info == static_cast<TypeInfo>(
                                                TYPE_ARRAY_BASE + TYPE_CHAR)) {
                        // 文字列配列を3D配列として格納
                        std::vector<std::vector<std::vector<std::string>>>
                            str_array_3d;
                        std::vector<std::vector<std::string>> str_array_2d;
                        std::vector<std::string> str_array_1d;

                        for (size_t i = 0; i < var->array_strings.size(); ++i) {
                            str_array_1d.push_back(var->array_strings[i]);
                        }
                        str_array_2d.push_back(str_array_1d);
                        str_array_3d.push_back(str_array_2d);

                        throw ReturnException(str_array_3d, type_name,
                                              type_info);
                    }
                } else {
                    if (debug_mode) {
                        std::cerr
                            << "[DEBUG] Variable is not array or not found"
                            << std::endl;
                    }
                }

                // 非配列変数または通常の処理
                int64_t value = expression_evaluator_->evaluate_expression(
                    node->left.get());
                throw ReturnException(value);
            } else {
                int64_t value = expression_evaluator_->evaluate_expression(
                    node->left.get());
                throw ReturnException(value);
            }
        } else {
            throw ReturnException(0);
        }
        break;

    case ASTNodeType::AST_BREAK_STMT: {
        int64_t cond = 1;
        if (node->left) {
            cond = expression_evaluator_->evaluate_expression(node->left.get());
        }
        if (cond) {
            throw BreakException(cond);
        }
    } break;

    case ASTNodeType::AST_CONTINUE_STMT: {
        int64_t cond = 1;
        if (node->left) {
            cond = expression_evaluator_->evaluate_expression(node->left.get());
        }
        if (cond) {
            throw ContinueException(cond);
        }
    } break;

    case ASTNodeType::AST_FUNC_DECL:
        // 実行時の関数定義をグローバルスコープに登録
        global_scope.functions[node->name] = node;
        break;

    default:
        expression_evaluator_->evaluate_expression(node); // 式文として評価
        break;
    }
}

void Interpreter::assign_variable(const std::string &name, int64_t value,
                                  TypeInfo type) {
    variable_manager_->assign_variable(name, value, type, false);
}

void Interpreter::assign_variable(const std::string &name, int64_t value,
                                  TypeInfo type, bool is_const) {
    variable_manager_->assign_variable(name, value, type, is_const);
}

void Interpreter::assign_variable(const std::string &name,
                                  const std::string &value) {
    variable_manager_->assign_variable(name, value);
}

void Interpreter::assign_variable(const std::string &name,
                                  const std::string &value, bool is_const) {
    variable_manager_->assign_variable(name, value, is_const);
}

void Interpreter::assign_function_parameter(const std::string &name,
                                            int64_t value, TypeInfo type) {
    variable_manager_->assign_function_parameter(name, value, type);
}

void Interpreter::assign_array_parameter(const std::string &name,
                                         const Variable &source_array,
                                         TypeInfo type) {
    variable_manager_->assign_array_parameter(name, source_array, type);
}

void Interpreter::assign_array_element(const std::string &name, int64_t index,
                                       int64_t value) {
    debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_DEBUG, name.c_str(), index,
              value);

    Variable *var = find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::VARIABLE_NOT_FOUND, name.c_str());
        error_msg(DebugMsgId::UNDEFINED_ARRAY_ERROR, name.c_str());
        throw std::runtime_error("Undefined array");
    }

    debug_msg(DebugMsgId::ARRAY_INFO, var->is_array, var->array_size,
              var->array_values.size());

    if (!var->is_array) {
        error_msg(DebugMsgId::NON_ARRAY_REF_ERROR, name.c_str());
        throw std::runtime_error("Non-array reference");
    }
    if (var->is_const) {
        error_msg(DebugMsgId::CONST_ARRAY_ASSIGN_ERROR, name.c_str());
        throw std::runtime_error("Assignment to const array");
    }
    if (index < 0 || index >= var->array_size) {
        debug_msg(DebugMsgId::ARRAY_INDEX_OUT_OF_BOUNDS, index,
                  var->array_size);
        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, name.c_str());
        throw std::runtime_error("Array out of bounds");
    }

    debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_START, index);
    TypeInfo elem_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
    check_type_range(elem_type, value, name);
    var->array_values[index] = value;
    debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_SUCCESS);
}

void Interpreter::assign_string_element(const std::string &name, int64_t index,
                                        const std::string &value) {
    debug_msg(DebugMsgId::STRING_ELEMENT_ASSIGN_DEBUG, name.c_str(), index,
              value.c_str());

    Variable *var = find_variable(name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("Undefined variable");
    }
    if (var->type != TYPE_STRING) {
        error_msg(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR);
        throw std::runtime_error("Non-string character assignment");
    }
    if (var->is_const) {
        error_msg(DebugMsgId::CONST_STRING_ELEMENT_ASSIGN_ERROR, name.c_str());
        std::exit(1);
    }

    // UTF-8文字数で範囲チェック
    size_t utf8_length = utf8_utils::utf8_char_count(var->str_value);
    debug_msg(DebugMsgId::STRING_LENGTH_UTF8_DEBUG, utf8_length);

    if (index < 0 || index >= static_cast<int64_t>(utf8_length)) {
        error_msg(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR, name.c_str(), index,
                  utf8_length);
        throw std::runtime_error("String out of bounds");
    }

    // UTF-8文字列の指定位置の文字を置換
    // 新しい文字列を構築
    std::string new_string;
    size_t current_index = 0;
    for (size_t i = 0; i < var->str_value.size();) {
        int len = utf8_utils::utf8_char_length(
            static_cast<unsigned char>(var->str_value[i]));

        if (current_index == static_cast<size_t>(index)) {
            // 置換対象の文字位置
            new_string += value;
            debug_msg(DebugMsgId::STRING_ELEMENT_REPLACE_DEBUG, index,
                      value.c_str());
        } else {
            // 既存の文字をコピー
            new_string += var->str_value.substr(i, len);
        }

        i += len;
        current_index++;
    }

    var->str_value = new_string;
    debug_msg(DebugMsgId::STRING_AFTER_REPLACE_DEBUG, var->str_value.c_str());
}

void Interpreter::print_value(const ASTNode *expr) {
    output_manager_->print_value(expr);
}

void Interpreter::print_formatted(const ASTNode *format_str,
                                  const ASTNode *arg_list) {
    output_manager_->print_formatted(format_str, arg_list);
}

void Interpreter::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &name) {
    type_manager_->check_type_range(type, value, name);
}

// エラー表示ヘルパー関数の実装
void Interpreter::throw_runtime_error_with_location(const std::string &message,
                                                    const ASTNode *node) {
    print_error_with_ast_location(message, node);
    throw std::runtime_error(message);
}

void Interpreter::print_error_at_node(const std::string &message,
                                      const ASTNode *node) {
    print_error_with_ast_location(message, node);
}

int64_t Interpreter::getMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices) {
    return array_manager_->getMultidimensionalArrayElement(var, indices);
}

void Interpreter::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, int64_t value) {
    array_manager_->setMultidimensionalArrayElement(var, indices, value);
}

std::string Interpreter::getMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices) {
    return array_manager_->getMultidimensionalStringArrayElement(var, indices);
}

void Interpreter::setMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices,
    const std::string &value) {
    array_manager_->setMultidimensionalStringArrayElement(var, indices, value);
}

void Interpreter::assign_array_literal(const std::string &name,
                                       const ASTNode *literal_node) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error("Invalid array literal for assignment");
    }

    // 変数を検索
    Variable *var = find_variable(name);
    if (!var) {
        throw std::runtime_error("Variable '" + name + "' not found");
    }

    if (!var->is_array) {
        throw std::runtime_error("Variable '" + name +
                                 "' is not declared as array");
    }

    // 配列リテラルの要素を取得（argumentsフィールドから）
    std::vector<int64_t> values;
    std::vector<std::string> str_values;

    for (const auto &element : literal_node->arguments) {
        if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
            str_values.push_back(element->str_value);
        } else {
            int64_t val =
                expression_evaluator_->evaluate_expression(element.get());
            values.push_back(val);
        }
    }

    // 文字列配列か数値配列かを判定して代入
    if (!str_values.empty()) {
        var->array_strings = str_values;
        var->array_size = str_values.size();
        // 文字列配列の場合、型を適切に設定（配列型を保持）
        if (var->type >= TYPE_ARRAY_BASE) {
            var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
        } else {
            var->type = TYPE_STRING;
        }
        // 数値配列をクリア
        var->array_values.clear();
    } else {
        var->array_values = values;
        var->array_size = values.size();
        // 数値配列の場合、既存の配列型を保持するか、INT配列に設定
        if (var->type < TYPE_ARRAY_BASE) {
            var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
        }
        // 文字列配列をクリア
        var->array_strings.clear();
    }

    var->is_assigned = true;
}

void Interpreter::assign_array_from_return(const std::string &name,
                                           const ReturnException &ret) {
    if (!ret.is_array) {
        throw std::runtime_error("Return value is not an array");
    }

    // 変数を検索
    Variable *var = find_variable(name);
    if (!var) {
        throw std::runtime_error("Variable '" + name + "' not found");
    }

    if (!var->is_array) {
        throw std::runtime_error("Variable '" + name +
                                 "' is not declared as array");
    }

    debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
              ("Assigning array from return to: " + name).c_str());

    // 元の宣言されたサイズを保存
    int declared_array_size = var->array_size;
    int actual_return_size = 0;

    // ReturnExceptionから配列データを取得して変数に代入
    if (!ret.str_array_3d.empty()) {
        // 文字列配列の場合
        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                  "Processing string array return value");

        var->array_strings.clear();

        // 3D配列を1D配列に変換
        for (const auto &plane : ret.str_array_3d) {
            for (const auto &row : plane) {
                for (const auto &element : row) {
                    var->array_strings.push_back(element);
                }
            }
        }

        actual_return_size = var->array_strings.size();
        var->array_size = actual_return_size;
        var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
        var->array_values.clear();

    } else if (!ret.int_array_3d.empty()) {
        // 整数配列の場合
        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                  "Processing integer array return value");

        var->array_values.clear();

        // 3D配列を1D配列に変換
        for (const auto &plane : ret.int_array_3d) {
            for (const auto &row : plane) {
                for (const auto &element : row) {
                    var->array_values.push_back(element);
                }
            }
        }

        actual_return_size = var->array_values.size();
        var->array_size = actual_return_size;
        var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
        var->array_strings.clear();
    } else {
        throw std::runtime_error(
            "Return exception contains no valid array data");
    }

    // 静的配列のサイズチェック:
    // 宣言されたサイズと返された配列のサイズが一致するか確認
    if (declared_array_size > 0 && declared_array_size != actual_return_size) {
        error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                  ("Array size mismatch in assignment: declared " +
                   std::to_string(declared_array_size) +
                   " elements but function returned " +
                   std::to_string(actual_return_size) + " elements")
                      .c_str());
        throw std::runtime_error(
            "Array size mismatch in function return assignment");
    }

    var->is_assigned = true;
    debug_msg(
        DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
        ("Array assignment completed, size: " + std::to_string(var->array_size))
            .c_str());
}

std::string Interpreter::resolve_typedef(const std::string &type_name) {
    return type_manager_->resolve_typedef(type_name);
}

TypeInfo Interpreter::resolve_type_alias(TypeInfo base_type,
                                         const std::string &type_name) {
    // typedefマップを使用してエイリアスを解決
    std::string resolved_type = type_manager_->resolve_typedef(type_name);

    if (resolved_type != type_name) {
        // エイリアスが見つかった場合、新しい型情報を返す
        return type_manager_->string_to_type_info(resolved_type);
    }

    // エイリアスが見つからない場合、元の型を返す
    return base_type;
}

TypeInfo Interpreter::string_to_type_info(const std::string &type_str) {
    return type_manager_->string_to_type_info(type_str);
}

// N次元配列アクセス用のヘルパー関数
std::string Interpreter::extract_array_name(const ASTNode *node) {
    return variable_manager_->extract_array_name(node);
}

std::vector<int64_t> Interpreter::extract_array_indices(const ASTNode *node) {
    return variable_manager_->extract_array_indices(node);
}

// ArrayManagerへのアクセス
int64_t Interpreter::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    return array_manager_->getMultidimensionalArrayElement(var, indices);
}
