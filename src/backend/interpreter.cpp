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
    // ArrayManagerはVariableManagerが必要なので後で初期化
    array_manager_ = std::make_unique<ArrayManager>(variable_manager_.get());
    // ExpressionEvaluatorを初期化
    expression_evaluator_ = std::make_unique<ExpressionEvaluator>(*this);

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
        // 複数変数宣言の処理
        for (const auto &child : node->children) {
            if (child->node_type == ASTNodeType::AST_VAR_DECL) {
                execute_statement(child.get());
            }
        }
        break;

    case ASTNodeType::AST_ARRAY_DECL: {
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, node->name.c_str());
        debug_msg(DebugMsgId::ARRAY_DIMENSIONS_COUNT,
                  node->array_dimensions.size());

        Variable var;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
        var.is_const = node->is_const;
        var.is_array = true;
        var.is_assigned = false;

        // 多次元配列かどうかチェック
        if (node->array_dimensions.size() > 1) {
            debug_msg(DebugMsgId::MULTIDIM_ARRAY_PROCESSING);
            // 多次元配列の場合
            // 各次元のサイズを評価して整数配列に変換
            std::vector<ArrayDimension> dimensions;
            for (const auto &dim_expr : node->array_dimensions) {
                int dim_size = static_cast<int>(
                    expression_evaluator_->evaluate_expression(dim_expr.get()));
                var.array_dimensions.push_back(dim_size);
                dimensions.push_back(ArrayDimension(dim_size, false));
            }

            // ArrayTypeInfoを作成
            var.array_type_info = ArrayTypeInfo(node->type_info, dimensions);
            var.is_multidimensional = true;

            // 総要素数を計算
            int total_size = 1;
            for (int dim : var.array_dimensions) {
                total_size *= dim;
            }
            var.array_size = total_size;

            // 要素の型
            TypeInfo elem_type = node->type_info;
            if (elem_type == TYPE_STRING) {
                var.multidim_array_strings.resize(total_size, "");
            } else {
                var.multidim_array_values.resize(total_size, 0);
            }

            // 多次元配列リテラル初期化
            if (node->init_expr &&
                node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                debug_msg(DebugMsgId::PRINTF_OFFSET_CALLED, 0);
                debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                          "processing initialization");

                // 多次元配列リテラルの処理を改善
                if (node->init_expr->arguments.empty()) {
                    // 空の配列リテラル
                    debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                              "empty array literal");
                } else {
                    // 最初の要素が配列リテラルかチェック
                    bool is_multidim =
                        node->init_expr->arguments[0]->node_type ==
                        ASTNodeType::AST_ARRAY_LITERAL;

                    if (is_multidim) {
                        debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                                  "processing N-dimensional literal");

                        // N次元配列リテラルの再帰的処理を使用
                        int flat_index = 0;
                        process_ndim_array_literal(node->init_expr.get(), var,
                                                   elem_type, flat_index,
                                                   total_size);

                        if (debug_mode) {
                            debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                                      ("N-dimensional array initialization "
                                       "complete. Total elements: " +
                                       std::to_string(flat_index))
                                          .c_str());
                        }
                    } else {
                        if (debug_mode) {
                            debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                                      "Processing 1D array literal in multidim "
                                      "context");
                        }
                        // 1次元配列リテラルの処理
                        for (size_t i = 0;
                             i < node->init_expr->arguments.size() &&
                             i < static_cast<size_t>(total_size);
                             ++i) {
                            const auto &element = node->init_expr->arguments[i];
                            if (elem_type == TYPE_STRING) {
                                if (element->node_type ==
                                    ASTNodeType::AST_STRING_LITERAL) {
                                    var.multidim_array_strings[i] =
                                        element->str_value;
                                }
                            } else {
                                int64_t val =
                                    expression_evaluator_->evaluate_expression(
                                        element.get());
                                var.multidim_array_values[i] = val;
                            }
                        }
                    }
                }
            }
        } else {
            debug_msg(DebugMsgId::SINGLE_DIM_ARRAY_PROCESSING);
            // 単一次元配列の処理（既存のロジック）
            // 配列サイズ決定
            if (node->array_size_expr) {
                var.array_size =
                    static_cast<int>(expression_evaluator_->evaluate_expression(
                        node->array_size_expr.get()));
            } else if (!node->array_dimensions.empty() &&
                       node->array_dimensions[0]) {
                // 配列次元から評価（変数サイズ対応）
                var.array_size =
                    static_cast<int>(expression_evaluator_->evaluate_expression(
                        node->array_dimensions[0].get()));
            } else if (node->init_expr && node->init_expr->node_type ==
                                              ASTNodeType::AST_ARRAY_LITERAL) {
                // 配列リテラルからサイズを推測
                var.array_size =
                    static_cast<int>(node->init_expr->arguments.size());
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

            // 1次元配列の場合、array_dimensionsも設定
            var.array_dimensions.clear();
            var.array_dimensions.push_back(var.array_size);
        }

        // 初期化式がある場合の処理
        // 多次元配列の場合は既に初期化済みなのでスキップ
        if (node->init_expr && node->array_dimensions.size() <= 1) {
            if (node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                debug_msg(DebugMsgId::PRINTF_OFFSET_CALLED,
                          0); // デバッグ用メッセージ
                debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING);

                // 事前に型の検証を行う
                TypeInfo elem_type = node->type_info;
                for (size_t i = 0; i < node->init_expr->arguments.size() &&
                                   i < static_cast<size_t>(var.array_size);
                     ++i) {
                    const auto &element = node->init_expr->arguments[i];

                    // 型チェック（配列への書き込み前に実行）
                    if (elem_type == TYPE_STRING) {
                        if (element->node_type !=
                            ASTNodeType::AST_STRING_LITERAL) {
                            if (debug_mode) {
                                std::cerr
                                    << "[DEBUG] Type mismatch: expected string "
                                       "literal in string array"
                                    << std::endl;
                            }
                            fprintf(stderr,
                                    "Type mismatch: string array cannot "
                                    "contain non-string elements\n");
                            throw std::runtime_error(
                                "Type mismatch: string array cannot contain "
                                "non-string elements");
                        }
                    } else {
                        if (element->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            if (debug_mode) {
                                std::cerr
                                    << "[DEBUG] Type mismatch: found string "
                                       "literal in integer array"
                                    << std::endl;
                            }
                            fprintf(stderr,
                                    "Type mismatch: integer array cannot "
                                    "contain string elements\n");
                            throw std::runtime_error(
                                "Type mismatch: integer array cannot contain "
                                "string elements");
                        }
                    }
                }

                // 型検証が通った場合のみ実際の初期化を行う
                for (size_t i = 0; i < node->init_expr->arguments.size() &&
                                   i < static_cast<size_t>(var.array_size);
                     ++i) {
                    const auto &element = node->init_expr->arguments[i];
                    debug_msg(DebugMsgId::ARRAY_ELEMENT_PROCESSING_DEBUG,
                              (int)i, (int)element->node_type);

                    if (elem_type == TYPE_STRING) {
                        var.array_strings[i] = element->str_value;
                    } else {
                        debug_msg(DebugMsgId::ARRAY_ELEMENT_EVAL_START, (int)i);
                        int64_t val =
                            expression_evaluator_->evaluate_expression(
                                element.get());
                        debug_msg(DebugMsgId::ARRAY_ELEMENT_EVAL_VALUE, val);
                        check_type_range(elem_type, val, node->name);
                        var.array_values[i] = val;
                    }
                }
            } else if (node->init_expr->node_type ==
                       ASTNodeType::AST_FUNC_CALL) {
                // 関数呼び出しによる配列初期化
                if (debug_mode) {
                    std::cerr << "[DEBUG] Processing array initialization from "
                                 "function call: "
                              << node->init_expr->name << std::endl;
                }

                try {
                    // 関数を実行してReturnExceptionを取得
                    if (debug_mode) {
                        std::cerr << "[DEBUG] About to call function: "
                                  << node->init_expr->name << std::endl;
                    }
                    expression_evaluator_->evaluate_expression(
                        node->init_expr.get());
                    // 通常の関数呼び出しは値のみを返すので、配列戻り値の場合は例外的に処理される
                    if (debug_mode) {
                        std::cerr << "[DEBUG] Function call completed without "
                                     "exception"
                                  << std::endl;
                    }
                } catch (const ReturnException &ret) {
                    if (debug_mode) {
                        std::cerr
                            << "[DEBUG] Caught ReturnException, is_array: "
                            << ret.is_array << std::endl;
                    }
                    if (ret.is_array) {
                        if (debug_mode) {
                            std::cerr << "[DEBUG] Received array return value"
                                      << std::endl;
                        }

                        // 戻り値の配列を現在の配列変数にコピー
                        TypeInfo elem_type = node->type_info;
                        if (elem_type == TYPE_STRING ||
                            elem_type == TYPE_CHAR) {
                            // 文字列配列
                            if (!ret.str_array_3d.empty() &&
                                !ret.str_array_3d[0].empty()) {
                                const auto &src_row = ret.str_array_3d[0][0];
                                if (debug_mode) {
                                    std::cerr << "[DEBUG] Copying string "
                                                 "array, size: "
                                              << src_row.size() << std::endl;
                                }
                                for (size_t i = 0;
                                     i < src_row.size() &&
                                     i < static_cast<size_t>(var.array_size);
                                     ++i) {
                                    var.array_strings[i] = src_row[i];
                                }
                            }
                        } else {
                            // 整数配列
                            if (!ret.int_array_3d.empty() &&
                                !ret.int_array_3d[0].empty()) {
                                const auto &src_row = ret.int_array_3d[0][0];
                                if (debug_mode) {
                                    std::cerr << "[DEBUG] Copying integer "
                                                 "array, size: "
                                              << src_row.size() << std::endl;
                                }
                                for (size_t i = 0;
                                     i < src_row.size() &&
                                     i < static_cast<size_t>(var.array_size);
                                     ++i) {
                                    var.array_values[i] = src_row[i];
                                    if (debug_mode) {
                                        std::cerr
                                            << "[DEBUG] Copied array element ["
                                            << i << "] = " << src_row[i]
                                            << std::endl;
                                    }
                                }
                            }
                        }
                    } else {
                        // 通常の戻り値の場合はエラー
                        if (debug_mode) {
                            std::cerr
                                << "[DEBUG] Function does not return an array"
                                << std::endl;
                        }
                        throw std::runtime_error(
                            "Function does not return an array");
                    }
                } catch (const std::exception &e) {
                    if (debug_mode) {
                        std::cerr << "[DEBUG] Exception during function call: "
                                  << e.what() << std::endl;
                    }
                    throw;
                }
            }
        }
        // 初期化リストがある場合（既存の形式）
        else {
            for (size_t i = 0; i < node->children.size() &&
                               i < static_cast<size_t>(var.array_size);
                 ++i) {
                const auto &child = node->children[i];
                if (child->node_type == ASTNodeType::AST_STMT_LIST) {
                    // 配列リテラル [1,2,3,...] の場合
                    TypeInfo elem_type = node->type_info; // ここで再定義
                    size_t j = 0;
                    for (const auto &element : child->children) {
                        if (j >= static_cast<size_t>(var.array_size))
                            break;
                        if (elem_type == TYPE_STRING) {
                            var.array_strings[j] = element->str_value;
                        } else {
                            int64_t val =
                                expression_evaluator_->evaluate_expression(
                                    element.get());
                            check_type_range(elem_type, val, node->name);
                            var.array_values[j] = val;
                        }
                        j++;
                    }
                    break; // 配列リテラルは一つだけ
                } else {
                    // 単一要素の初期化
                    TypeInfo elem_type = node->type_info; // ここで再定義
                    if (elem_type == TYPE_STRING) {
                        var.array_strings[i] = child->str_value;
                    } else {
                        int64_t val =
                            expression_evaluator_->evaluate_expression(
                                child.get());
                        check_type_range(elem_type, val, node->name);
                        var.array_values[i] = val;
                    }
                }
            }
        }

        current_scope().variables[node->name] = var;
    } break;

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
            if (node->left->node_type == ASTNodeType::AST_STRING_LITERAL) {
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
    if (!format_str ||
        format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        std::cout << "(invalid format)" << std::endl;
        return;
    }

    std::string format = format_str->str_value;
    std::vector<int64_t> int_args;
    std::vector<std::string> str_args;

    // 引数リストを評価
    if (arg_list && arg_list->node_type == ASTNodeType::AST_STMT_LIST) {
        for (const auto &arg : arg_list->arguments) {
            if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                str_args.push_back(arg->str_value);
                int_args.push_back(0); // プレースホルダー
            } else if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                Variable *var = find_variable(arg->name);
                if (var && var->type == TYPE_STRING) {
                    str_args.push_back(var->str_value);
                    int_args.push_back(0); // プレースホルダー
                } else {
                    int64_t value =
                        expression_evaluator_->evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                }
            } else {
                int64_t value =
                    expression_evaluator_->evaluate_expression(arg.get());
                int_args.push_back(value);
                str_args.push_back(""); // プレースホルダー
            }
        }
    }

    // フォーマット文字列を処理
    std::string result;
    size_t arg_index = 0;
    for (size_t i = 0; i < format.length(); i++) {
        if (format[i] == '%' && i + 1 < format.length()) {
            char specifier = format[i + 1];

            if (specifier == '%') {
                // %% の場合は常に % を追加（引数不要）
                result += '%';
                i++; // %% をスキップ
            } else if (arg_index < int_args.size()) {
                switch (specifier) {
                case 'd':
                case 'i':
                    result += std::to_string(int_args[arg_index]);
                    break;
                case 'l':
                    // %lld の処理
                    if (i + 3 < format.length() && format[i + 2] == 'l' &&
                        format[i + 3] == 'd') {
                        result += std::to_string(int_args[arg_index]);
                        i += 2; // 追加の 'll' をスキップ
                    } else {
                        result += std::to_string(int_args[arg_index]);
                    }
                    break;
                case 's':
                    if (arg_index < str_args.size() &&
                        !str_args[arg_index].empty()) {
                        result += str_args[arg_index];
                    } else {
                        result += std::to_string(int_args[arg_index]);
                    }
                    break;
                case 'c':
                    result += static_cast<char>(int_args[arg_index]);
                    break;
                default:
                    result += '%';
                    result += specifier;
                    break;
                }
                arg_index++;
                i++; // specifier をスキップ
            } else {
                result += format[i];
            }
        } else {
            result += format[i];
        }
    }

    std::cout << result << std::endl;
}

void Interpreter::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &name) {
    type_manager_->check_type_range(type, value, name);
}

// エラー表示ヘルパー関数の実装
void Interpreter::throw_runtime_error_with_location(const std::string &message,
                                                    const ASTNode *node) {
    if (node && !node->location.filename.empty()) {
        std::string source_line =
            get_source_line(node->location.filename, node->location.line);
        print_error_with_location(message, node->location.filename,
                                  node->location.line, node->location.column,
                                  source_line);
    } else {
        std::string error_prefix = (debug_language == DebugLanguage::JAPANESE)
                                       ? "エラー: "
                                       : "Error: ";
        std::cerr << error_prefix << message << std::endl;
    }
    throw std::runtime_error(message);
}

void Interpreter::print_error_at_node(const std::string &message,
                                      const ASTNode *node) {
    if (node && !node->location.filename.empty()) {
        std::string source_line =
            get_source_line(node->location.filename, node->location.line);
        print_error_with_location(message, node->location.filename,
                                  node->location.line, node->location.column,
                                  source_line);
    } else {
        std::string error_prefix = (debug_language == DebugLanguage::JAPANESE)
                                       ? "エラー: "
                                       : "Error: ";
        std::cerr << error_prefix << message << std::endl;
    }
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
