#include "interpreter.h"
#include "../common/ast.h"
#include "../common/debug.h"
#include "../common/debug_messages.h"
#include "../common/io_interface.h"
#include "../common/utf8_utils.h"
#include "evaluator/expression_evaluator.h"
#include "executor/statement_executor.h"
#include "output/output_manager.h"
#include "variables/variable_manager.h"
#include <cctype>
#include <cinttypes>
#include <codecvt>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <stdexcept>

Interpreter::Interpreter(bool debug) : debug_mode(debug) {
    // 環境変数からデバッグモード設定
    const char *env_debug = std::getenv("CB_DEBUG_MODE");
    if (env_debug && env_debug[0] == '1') {
        debug_mode = true;
    }

    // グローバルスコープを初期化
    scope_stack.emplace_back(std::move(global_scope));

    // OutputManagerを初期化
    output_manager_ = std::make_unique<OutputManager>(this);

    // ExpressionEvaluatorを初期化
    expression_evaluator_ = std::make_unique<ExpressionEvaluator>(*this);

    // StatementExecutorを初期化
    statement_executor_ = std::make_unique<StatementExecutor>(*this);

    // VariableManagerを初期化
    variable_manager_ = std::make_unique<VariableManager>(*this);

    // ModuleResolverを初期化
    module_resolver_ = std::make_unique<ModuleResolver>();

    // SemanticAnalyzerを初期化
    semantic_analyzer_ =
        std::make_unique<SemanticAnalyzer>(*this, *variable_manager_);
}

void Interpreter::push_scope() { scope_stack.emplace_back(); }

void Interpreter::pop_scope() {
    if (scope_stack.size() > 1) {
        scope_stack.pop_back();
    }
}

Scope &Interpreter::current_scope() { return scope_stack.back(); }

Variable *Interpreter::find_variable(const std::string &name) {
    // ローカルスコープから検索
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            return &var_it->second;
        }
    }

    // グローバルスコープから検索
    auto global_var_it = global_scope.variables.find(name);
    if (global_var_it != global_scope.variables.end()) {
        return &global_var_it->second;
    }

    return nullptr;
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
        // 最初にtypedef宣言をすべて処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_TYPEDEF_DECL) {
                statement_executor_->execute_typedef_statement(stmt.get());
            }
        }
        // その後に他の宣言を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type != ASTNodeType::AST_TYPEDEF_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        break;

    case ASTNodeType::AST_TYPEDEF_DECL:
        // typedef宣言は上記で既に処理済み
        break;

    case ASTNodeType::AST_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        if (node->node_type == ASTNodeType::AST_ASSIGN) {
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
                int64_t value = evaluate_expression(node->right.get());
                if (var.type == TYPE_STRING) {
                    var.string_value = node->right->str_value;
                } else {
                    var.int_value = value;
                    check_type_range(var.type, value, node->name);
                }
                var.is_assigned = true;
            }

            global_scope.variables.insert_or_assign(node->name, std::move(var));
        } else if (node->node_type == ASTNodeType::AST_VAR_DECL) {
            // グローバル変数の重複宣言チェック
            if (global_scope.variables.find(node->name) !=
                global_scope.variables.end()) {
                error_msg(DebugMsgId::VAR_REDECLARE_ERROR, node->name.c_str());
                throw std::runtime_error("Variable redeclaration error");
            }

            Variable var;
            var.type = node->type_info;
            var.is_const = node->is_const;
            var.is_assigned = false;

            // 初期化式がある場合は評価
            if (node->right) {
                int64_t value = evaluate_expression(node->right.get());
                if (var.type == TYPE_STRING) {
                    var.string_value = node->right->str_value;
                } else {
                    var.int_value = value;
                    check_type_range(var.type, value, node->name);
                }
                var.is_assigned = true;
            }

            // 配列typedef変数の場合、配列情報を設定
            if (node->array_type_info.is_array()) {
                var.is_array = true;
                // 1次元目のサイズを設定（多次元配列は将来拡張）
                if (!node->array_type_info.dimensions.empty()) {
                    auto &first_dim = node->array_type_info.dimensions[0];
                    var.array_size = first_dim.is_dynamic ? -1 : first_dim.size;
                } else {
                    var.array_size = -1; // 動的サイズ
                }
                // 配列typedef変数として認識されたことをデバッグ出力
                debug_msg(DebugMsgId::VAR_ASSIGN_READABLE, node->name.c_str());
            } else {
                var.is_array = false;
            }

            global_scope.variables.insert_or_assign(node->name, std::move(var));
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
                evaluate_expression(node->array_size_expr.get()));
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
                        if (element->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            var.array_strings()[j] = element->str_value;
                        } else {
                            var.array_strings()[j] = ""; // デフォルト値
                        }
                    } else {
                        int64_t val = evaluate_expression(element.get());
                        check_type_range(elem_type, val, node->name);
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
                    int64_t val = evaluate_expression(child.get());
                    check_type_range(elem_type, val, node->name);
                    var.array_values()[i] = val;
                }
            }
        }

        current_scope().variables.insert_or_assign(node->name, std::move(var));
    } break;

    case ASTNodeType::AST_FUNC_DECL:
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER, node->name.c_str());
        global_scope.functions[node->name] = node;
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE, node->name.c_str());
        break;

    case ASTNodeType::AST_IMPORT_STMT:
        // import文の処理
        if (debug_mode) {
            std::cout << "[DEBUG] Processing import during global declaration: "
                      << node->module_name << std::endl;
        }
        process_import(node);
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

    // 意味解析フェーズを実行
    if (!perform_semantic_analysis(ast)) {
        throw std::runtime_error("Semantic analysis failed");
    }

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
        execute_statement(main_func->body.get());
        pop_scope();
    } catch (const ReturnException &e) {
        debug_msg(DebugMsgId::MAIN_FUNC_EXIT, e.value);
    }
}

int64_t Interpreter::evaluate(const ASTNode *node) {
    return evaluate_expression(node);
}

void Interpreter::execute_statement(const ASTNode *node) {
    statement_executor_->execute_statement(node);
}

int64_t Interpreter::evaluate_expression(const ASTNode *node) {
    return expression_evaluator_->evaluate_expression(node);
}
void Interpreter::assign_variable(const std::string &name, int64_t value,
                                  TypeInfo type) {
    variable_manager_->assign_variable(name, value, type);
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

void Interpreter::assign_array_element(const std::string &name, int64_t index,
                                       int64_t value) {
    variable_manager_->assign_array_element(name, index, value);
}

void Interpreter::assign_string_element(const std::string &name, int64_t index,
                                        const std::string &value) {
    variable_manager_->assign_string_element(name, index, value);
}

void Interpreter::assign_array_literal(const std::string &name,
                                       ASTNode *array_literal) {
    variable_manager_->assign_array_literal(name, array_literal);
}

void Interpreter::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &name) {
    variable_manager_->check_type_range(type, value, name);
}

// 型エイリアス解決
TypeInfo Interpreter::resolve_type_alias(TypeInfo type_info,
                                         const std::string &type_name) {
    return variable_manager_->resolve_type_with_alias(type_info, type_name);
}

bool Interpreter::process_import(const ASTNode *import_node) {
    if (!import_node ||
        import_node->node_type != ASTNodeType::AST_IMPORT_STMT) {
        return false;
    }

    std::string module_name = import_node->module_name;
    if (module_name.empty()) {
        std::cerr << "Empty module name in import statement" << std::endl;
        return false;
    }

    if (debug_mode) {
        std::cout << "Processing import: " << module_name << std::endl;
    }

    // モジュール解決を試行
    bool success = module_resolver_->resolve_import(module_name);
    if (!success) {
        std::cerr << "Failed to resolve import: " << module_name << std::endl;
        return false;
    }

    // 組み込みモジュールの場合は特別な処理
    if (module_name == "stdio" || module_name == "std.io") {
        if (debug_mode) {
            std::cout << "Registered builtin module: " << module_name
                      << std::endl;
        }
        return true;
    }

    // ファイルベースのモジュールの場合、ASTを実行
    ModuleInfo *module_info = module_resolver_->get_module(module_name);
    if (module_info && module_info->ast) {
        if (debug_mode) {
            std::cout << "Executing module AST: " << module_name << std::endl;
        }

        // モジュールスコープで実行
        push_scope();
        try {
            statement_executor_->execute_statement(module_info->ast.get());
            pop_scope();
            return true;
        } catch (const std::exception &e) {
            pop_scope();
            std::cerr << "Error executing module " << module_name << ": "
                      << e.what() << std::endl;
            return false;
        }
    }

    return true;
}

bool Interpreter::is_module_loaded(const std::string &module_name) {
    return module_resolver_->is_module_loaded(module_name);
}

const ASTNode *
Interpreter::find_module_function(const std::string &module_name,
                                  const std::string &function_name) {
    return module_resolver_->find_module_function(module_name, function_name);
}

int64_t Interpreter::find_module_variable(const std::string &module_name,
                                          const std::string &variable_name) {
    return module_resolver_->find_module_variable(module_name, variable_name);
}

bool Interpreter::perform_semantic_analysis(const ASTNode *ast) {
    if (!ast || !semantic_analyzer_) {
        return false;
    }

    SemanticAnalysisResult result =
        semantic_analyzer_->analyze_declarations(const_cast<ASTNode *>(ast));

    if (!result.success) {
        std::cerr << "Semantic Analysis Error: " << result.error_message
                  << std::endl;
        if (!result.error_location.empty()) {
            std::cerr << "Location: " << result.error_location << std::endl;
        }
        return false;
    }

    return true;
}
