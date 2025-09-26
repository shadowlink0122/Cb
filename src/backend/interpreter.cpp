#include "interpreter.h"
#include "../common/ast.h"
#include "../common/debug.h"
#include "../common/utf8_utils.h"
// DRY統合: 旧版ヘッダーから新版ヘッダーへ移行
#include "interpreter/core/error_handler.h"
#include "interpreter/evaluator/expression_evaluator.h"
#include "interpreter/executor/statement_executor.h"
#include "interpreter/managers/array_manager.h"     // 新版ArrayManager
#include "interpreter/managers/common_operations.h" // 新版CommonOperations
#include "interpreter/managers/enum_manager.h"      // 新版EnumManager
#include "interpreter/managers/type_manager.h"      // 新版TypeManager
#include "interpreter/managers/variable_manager.h"  // 新版VariableManager
#include "interpreter/output/output_manager.h"
#include "interpreter/services/debug_service.h" // 統一デバッグサービス
#include "interpreter/services/expression_service.h" // 統一式評価サービス
#include "interpreter/services/variable_access_service.h" // 統一変数アクセスサービス
#include <cstdlib>
#include <iostream>
#include <stdexcept>

Interpreter::Interpreter(bool debug)
    : debug_mode(debug), output_manager_(std::make_unique<OutputManager>(this)),
      variable_manager_(std::make_unique<VariableManager>(this)),
      type_manager_(std::make_unique<TypeManager>(this)),
      enum_manager_(std::make_unique<EnumManager>()),
      parser_(nullptr) { // enum定義アクセス用のparser初期化

    // ExpressionEvaluatorを最初に初期化
    expression_evaluator_ = std::make_unique<ExpressionEvaluator>(*this);

    // ArrayManagerはVariableManagerとExpressionEvaluatorが必要なので後で初期化
    array_manager_ = std::make_unique<ArrayManager>(
        variable_manager_.get(), expression_evaluator_.get(), this);

    // StatementExecutorを初期化
    statement_executor_ = std::make_unique<StatementExecutor>(*this);

    // CommonOperationsを初期化（他のManagerが必要なので最後に初期化）
    common_operations_ = std::make_unique<CommonOperations>(this);

    // DRY効率化: 統一変数アクセスサービスを初期化
    variable_access_service_ = std::make_unique<VariableAccessService>(this);

    // DRY効率化: 統一式評価サービスを初期化
    expression_service_ = std::make_unique<ExpressionService>(this);

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

    if (debug_mode) {
        const char *node_type_name = "UNKNOWN";
        switch (node->node_type) {
        case ASTNodeType::AST_STMT_LIST:
            node_type_name = "AST_STMT_LIST";
            break;
        case ASTNodeType::AST_VAR_DECL:
            node_type_name = "AST_VAR_DECL";
            break;
        case ASTNodeType::AST_STRUCT_DECL:
            node_type_name = "AST_STRUCT_DECL";
            break;
        case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
            node_type_name = "AST_STRUCT_TYPEDEF_DECL";
            break;
        case ASTNodeType::AST_ENUM_DECL:
            node_type_name = "AST_ENUM_DECL";
            break;
        case ASTNodeType::AST_ENUM_TYPEDEF_DECL:
            node_type_name = "AST_ENUM_TYPEDEF_DECL";
            break;
        case ASTNodeType::AST_FUNC_DECL:
            node_type_name = "AST_FUNC_DECL";
            break;
        default:
            break;
        }
        // DRY統合: DebugServiceを使用した統一デバッグ出力
        DEBUG_DEBUG(GENERAL,
                    "register_global_declarations processing: %s (name: %s)",
                    node_type_name, node->name.c_str());
    }

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        // まず変数宣言のみを処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        // 次にstruct定義とenum定義を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_STRUCT_DECL ||
                stmt->node_type == ASTNodeType::AST_STRUCT_TYPEDEF_DECL ||
                stmt->node_type == ASTNodeType::AST_ENUM_DECL ||
                stmt->node_type == ASTNodeType::AST_ENUM_TYPEDEF_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        // 最後にその他の宣言（関数など）を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type != ASTNodeType::AST_VAR_DECL &&
                stmt->node_type != ASTNodeType::AST_STRUCT_DECL &&
                stmt->node_type != ASTNodeType::AST_STRUCT_TYPEDEF_DECL &&
                stmt->node_type != ASTNodeType::AST_ENUM_DECL &&
                stmt->node_type != ASTNodeType::AST_ENUM_TYPEDEF_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        break;

    case ASTNodeType::AST_STRUCT_DECL:
    case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
        // struct定義を登録
        {
            if (debug_mode) {
                // DRY統合: DebugServiceを使用した統一デバッグ出力
                DEBUG_DEBUG(STRUCT, "Registering struct definition: %s",
                            node->name.c_str());
            }
            std::string struct_name = node->name;
            StructDefinition struct_def(struct_name);

            // ASTノードからstruct定義を復元
            for (const auto &member_node : node->arguments) {
                if (member_node->node_type == ASTNodeType::AST_VAR_DECL) {
                    // 配列メンバーの場合はArrayTypeInfoを保持
                    if (member_node->array_type_info.is_array()) {
                        StructMember array_member(member_node->name,
                                                  member_node->type_info,
                                                  member_node->type_name);
                        array_member.array_info = member_node->array_type_info;
                        struct_def.members.push_back(array_member);

                        if (debug_mode) {
                            debug_print(
                                "Adding array member: %s (type: %d, size: "
                                "%d)\n",
                                member_node->name.c_str(),
                                (int)member_node->type_info,
                                member_node->array_type_info.dimensions[0]
                                    .size);

                            // size_exprのデバッグ出力を追加
                            const auto &dim =
                                member_node->array_type_info.dimensions[0];
                            debug_print("  Array dimension: size=%d, "
                                        "is_dynamic=%d, size_expr='%s'\n",
                                        dim.size, dim.is_dynamic ? 1 : 0,
                                        dim.size_expr.c_str());
                        }
                    } else {
                        struct_def.add_member(member_node->name,
                                              member_node->type_info,
                                              member_node->type_name);
                        if (debug_mode) {
                            debug_print("Adding member: %s (type: %d)\n",
                                        member_node->name.c_str(),
                                        (int)member_node->type_info);
                        }
                    }
                }
            }

            register_struct_definition(struct_name, struct_def);
            if (debug_mode) {
                debug_print(
                    "Registered struct definition: %s with %zu members\n",
                    struct_name.c_str(), struct_def.members.size());
            }
        }
        break;

    case ASTNodeType::AST_ENUM_DECL:
    case ASTNodeType::AST_ENUM_TYPEDEF_DECL:
        // enum定義を登録
        {
            std::cerr << "[DEBUG] Processing enum definition node: "
                      << node->name << std::endl;
            if (debug_mode) {
                DEBUG_DEBUG(GENERAL, "Registering enum definition: %s",
                            node->name.c_str());
            }
            std::string enum_name = node->name;
            EnumDefinition enum_def(enum_name);

            // ASTノードからenum定義を復元
            for (const auto &member_node : node->arguments) {
                if (member_node->node_type == ASTNodeType::AST_VAR_DECL) {
                    enum_def.add_member(member_node->name,
                                        member_node->int_value, true);
                    if (debug_mode) {
                        debug_print("Adding enum member: %s = %lld\n",
                                    member_node->name.c_str(),
                                    member_node->int_value);
                    }
                }
            }

            enum_definitions_[enum_name] = enum_def;

            // EnumManagerにも登録
            std::cerr << "[DEBUG] Registering enum in EnumManager: "
                      << enum_name << " with " << enum_def.members.size()
                      << " members" << std::endl;
            enum_manager_->register_enum(enum_name, enum_def);

            if (debug_mode) {
                debug_print("Registered enum definition: %s with %zu members\n",
                            enum_name.c_str(),
                            enum_definitions_[enum_name].members.size());
            }
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
            if (node->right &&
                node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                // 配列リテラル初期化の場合 - 既に宣言済みの変数に代入
                assign_array_literal(node->name, node->right.get());
            } else {
                // 通常の初期化
                Variable var;
                var.type =
                    node->type_info != TYPE_VOID ? node->type_info : TYPE_INT;
                var.is_const = node->is_const;
                var.is_assigned = false;

                if (node->right) {
                    // DRY統合: ExpressionServiceを使用した統一式評価
                    int64_t value = expression_service_->evaluate_safe(
                        node->right.get(), "変数初期化");
                    if (var.type == TYPE_STRING) {
                        var.str_value = node->right->str_value;
                    } else {
                        var.value = value;
                        check_type_range(var.type, value, node->name);
                    }
                    var.is_assigned = true;
                }

                global_scope.variables[node->name] = var;
            }
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

    // グローバル変数の初期化を行う
    if (debug_mode) {
        debug_print("Initializing global variables\n");
    }
    initialize_global_variables(ast);

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
    // DRY統合: ExpressionServiceを使用した統一式評価
    return expression_service_->evaluate_safe(node, "evaluate()");
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
                // DRY統合: ExpressionServiceを使用した統一式評価
                int64_t val = expression_service_->evaluate_safe(
                    element.get(), "N次元配列リテラル要素");
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
        case ASTNodeType::AST_STRUCT_DECL:
            node_type_name = "AST_STRUCT_DECL";
            break;
        case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
            node_type_name = "AST_STRUCT_TYPEDEF_DECL";
            break;
        case ASTNodeType::AST_ENUM_DECL:
            node_type_name = "AST_ENUM_DECL";
            break;
        case ASTNodeType::AST_ENUM_TYPEDEF_DECL:
            node_type_name = "AST_ENUM_TYPEDEF_DECL";
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
        // 変数宣言をVariableManagerに委譲
        variable_manager_->process_var_decl_or_assign(node);
        break;

    case ASTNodeType::AST_ASSIGN:
        // 代入をStatementExecutorに委譲
        statement_executor_->execute(node);
        break;

    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
        statement_executor_->execute_multiple_var_decl(node);
        break;

    case ASTNodeType::AST_ARRAY_DECL:
        statement_executor_->execute_array_decl(node);
        break;

    case ASTNodeType::AST_STRUCT_DECL:
    case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
        // struct定義を登録
        {
            std::cerr << "[DEBUG] Processing struct definition: " << node->name
                      << std::endl;
            std::string struct_name = node->name;
            StructDefinition struct_def(struct_name);

            // ASTノードからstruct定義を復元
            for (const auto &member_node : node->arguments) {
                if (member_node->node_type == ASTNodeType::AST_VAR_DECL) {
                    struct_def.add_member(member_node->name,
                                          member_node->type_info,
                                          member_node->type_name);
                    std::cerr << "[DEBUG] Adding member: " << member_node->name
                              << " (type: " << (int)member_node->type_info
                              << ")" << std::endl;
                }
            }

            register_struct_definition(struct_name, struct_def);
            std::cerr << "[DEBUG] Registered struct definition: " << struct_name
                      << " with " << struct_def.members.size() << " members"
                      << std::endl;
        }
        break;

    case ASTNodeType::AST_ENUM_DECL:
    case ASTNodeType::AST_ENUM_TYPEDEF_DECL:
        // enum定義は既にregister_global_declarationsで処理済み
        // 実行時は何もしない
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
        // DRY統合: ExpressionServiceを使用した統一条件式評価
        int64_t cond = expression_service_->evaluate_condition(
            node->condition.get(), "IF文条件");
        if (cond) {
            execute_statement(node->left.get());
        } else if (node->right) {
            execute_statement(node->right.get());
        }
    } break;

    case ASTNodeType::AST_WHILE_STMT:
        try {
            while (true) {
                // DRY統合: ExpressionServiceを使用した統一条件式評価
                int64_t cond = expression_service_->evaluate_condition(
                    node->condition.get(), "WHILE文条件");
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
                    // DRY統合: ExpressionServiceを使用した統一条件式評価
                    int64_t cond = expression_service_->evaluate_condition(
                        node->condition.get(), "FOR文条件");
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
                        std::cerr << "[DEBUG] Returning array variable"
                                  << std::endl;
                        if (var->is_multidimensional) {
                            std::cerr
                                << "[DEBUG] Multidimensional array, size: "
                                << var->multidim_array_values.size()
                                << std::endl;
                        } else {
                            std::cerr << "[DEBUG] Regular array, size: "
                                      << var->array_values.size() << std::endl;
                        }
                    }

                    // 多次元配列の場合
                    if (var->is_multidimensional) {
                        if (debug_mode) {
                            std::cerr << "[DEBUG] Processing multidimensional "
                                         "array return"
                                      << std::endl;
                        }

                        // 多次元配列の値を3D配列に変換
                        std::vector<std::vector<std::vector<int64_t>>>
                            int_array_3d;
                        std::vector<std::vector<int64_t>> int_array_2d;
                        std::vector<int64_t> int_array_1d;

                        for (size_t i = 0;
                             i < var->multidim_array_values.size(); ++i) {
                            int_array_1d.push_back(
                                var->multidim_array_values[i]);
                            if (debug_mode) {
                                std::cerr
                                    << "[DEBUG] Multidim Array element[" << i
                                    << "] = " << var->multidim_array_values[i]
                                    << std::endl;
                            }
                        }
                        int_array_2d.push_back(int_array_1d);
                        int_array_3d.push_back(int_array_2d);

                        throw ReturnException(int_array_3d, node->left->name,
                                              var->type);
                    }

                    // 1次元配列の既存処理
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
                if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable *var = find_variable(node->left->name);
                    if (var && var->type == TYPE_STRING) {
                        // 文字列変数を返す
                        throw ReturnException(var->str_value);
                    } else {
                        // 数値変数を返す
                        int64_t value =
                            expression_evaluator_->evaluate_expression(
                                node->left.get());
                        throw ReturnException(value);
                    }
                } else {
                    int64_t value = expression_evaluator_->evaluate_expression(
                        node->left.get());
                    throw ReturnException(value);
                }
            } else {
                if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable *var = find_variable(node->left->name);
                    if (var && var->type == TYPE_STRING) {
                        // 文字列変数を返す
                        throw ReturnException(var->str_value);
                    } else {
                        // 数値変数を返す
                        int64_t value =
                            expression_evaluator_->evaluate_expression(
                                node->left.get());
                        throw ReturnException(value);
                    }
                } else {
                    int64_t value = expression_evaluator_->evaluate_expression(
                        node->left.get());
                    throw ReturnException(value);
                }
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

    // 共通実装を使用
    try {
        common_operations_->assign_array_element_safe(var, index, value, name);
        debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_SUCCESS);
    } catch (const std::exception &e) {
        debug_msg(DebugMsgId::ARRAY_INDEX_OUT_OF_BOUNDS, index,
                  var->array_size);
        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, name.c_str());
        throw;
    }
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

// Priority 3: 旧版重複メソッド - ArrayProcessingServiceに統合済み
/*
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
*/

void Interpreter::assign_array_literal(const std::string &name,
                                       const ASTNode *literal_node) {
    if (debug_mode) {
        std::cerr << "DEBUG: assign_array_literal called for variable: " << name
                  << std::endl;
    }

    // 変数を検索
    Variable *var = find_variable(name);
    if (!var) {
        if (debug_mode) {
            std::cerr << "DEBUG: Variable '" << name
                      << "' not found in assign_array_literal" << std::endl;
        }
        throw std::runtime_error("Variable '" + name + "' not found");
    }

    // 多次元配列の場合は従来の処理を使用
    if (var->is_multidimensional) {
        TypeInfo elem_type = var->array_type_info.base_type;
        array_manager_->processMultidimensionalArrayLiteral(*var, literal_node,
                                                            elem_type);
        var->is_assigned = true;
        return;
    }

    // 1次元配列の場合は共通実装を使用
    try {
        auto result = common_operations_->parse_array_literal(literal_node);
        common_operations_->assign_array_literal_to_variable(var, result);

        if (debug_mode) {
            std::cerr << "DEBUG: Successfully assigned array literal to '"
                      << name << "' using common operations" << std::endl;
        }
    } catch (const std::exception &e) {
        if (debug_mode) {
            std::cerr << "DEBUG: Failed to assign array literal to '" << name
                      << "': " << e.what() << std::endl;
        }
        throw;
    }
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

std::string Interpreter::extract_array_element_name(const ASTNode *node) {
    // 配列要素名を生成 (例: arr[0] -> "arr[0]")
    std::string array_name = extract_array_name(node);
    std::vector<int64_t> indices = extract_array_indices(node);

    std::string element_name = array_name;
    for (int64_t index : indices) {
        element_name += "[" + std::to_string(index) + "]";
    }

    return element_name;
}

// Priority 3: 旧版重複メソッド - ArrayProcessingServiceに統合済み
/*
int64_t Interpreter::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    return array_manager_->getMultidimensionalArrayElement(var, indices);
}
*/

// static変数の検索
Variable *Interpreter::find_static_variable(const std::string &name) {
    std::string static_key = current_function_name + "::" + name;
    auto it = static_variables.find(static_key);
    if (it != static_variables.end()) {
        return &it->second;
    }
    return nullptr;
}

// static変数の作成
void Interpreter::create_static_variable(const std::string &name,
                                         const ASTNode *node) {
    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_array = false;
    var.is_assigned = false;
    var.is_multidimensional = false;

    // デフォルト値を設定
    if (var.type == TYPE_STRING) {
        var.str_value = "";
    } else {
        var.value = 0;
    }

    // 初期化式があれば評価して設定
    if (node->init_expr) {
        if (var.type == TYPE_STRING &&
            node->init_expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
            var.str_value = node->init_expr->str_value;
        } else {
            var.value = evaluate(node->init_expr.get());
        }
        var.is_assigned = true;
    }

    // static変数をユニークな名前で保存（関数名+変数名）
    std::string static_key = current_function_name + "::" + name;
    static_variables[static_key] = var;
}

// struct定義を登録
void Interpreter::register_struct_definition(
    const std::string &struct_name, const StructDefinition &definition) {
    // 構造体定義は定数解決を行わずにそのまま保存
    // 定数解決は実際に構造体変数を作成する時に行う
    if (debug_mode) {
        debug_print(
            "Storing struct definition: %s (constant resolution deferred)\n",
            struct_name.c_str());
    }

    struct_definitions_[struct_name] = definition;
}

// struct定義を検索
const StructDefinition *
Interpreter::find_struct_definition(const std::string &struct_name) {
    auto it = struct_definitions_.find(struct_name);
    if (it != struct_definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

// struct変数を作成
void Interpreter::create_struct_variable(const std::string &var_name,
                                         const std::string &struct_type_name) {
    if (debug_mode) {
        debug_print(
            "create_struct_variable called: var_name=%s, struct_type=%s\n",
            var_name.c_str(), struct_type_name.c_str());
    }

    const StructDefinition *struct_def =
        find_struct_definition(struct_type_name);
    if (!struct_def) {
        throw std::runtime_error("Struct type not found: " + struct_type_name);
    }

    // Debug output removed - use --debug option if needed

    Variable struct_var(struct_type_name);

    // メンバ変数を初期化
    for (const auto &member : struct_def->members) {
        if (debug_mode) {
            debug_print("Processing member: %s, is_array: %d\n",
                        member.name.c_str(), member.array_info.is_array());
        }

        if (member.array_info.is_array()) {
            if (debug_mode) {
                debug_print("Member %s is an array\n", member.name.c_str());
            }
            // 配列メンバの場合
            int array_size = member.array_info.dimensions[0].size;

            // 動的サイズ（定数識別子）の場合は解決を試みる
            if (array_size == -1 &&
                member.array_info.dimensions[0].is_dynamic &&
                !member.array_info.dimensions[0].size_expr.empty()) {
                if (debug_mode) {
                    debug_print(
                        "Attempting to resolve constant: %s\n",
                        member.array_info.dimensions[0].size_expr.c_str());
                }
                Variable *const_var =
                    find_variable(member.array_info.dimensions[0].size_expr);
                if (const_var) {
                    if (debug_mode) {
                        debug_print(
                            "Found variable %s: is_const=%d, is_assigned=%d, "
                            "value=%ld\n",
                            member.array_info.dimensions[0].size_expr.c_str(),
                            const_var->is_const ? 1 : 0,
                            const_var->is_assigned ? 1 : 0, const_var->value);
                    }
                    if (const_var->is_const && const_var->is_assigned) {
                        array_size = static_cast<int>(const_var->value);
                        if (debug_mode) {
                            debug_print(
                                "Resolved constant %s to %d for member %s\n",
                                member.array_info.dimensions[0]
                                    .size_expr.c_str(),
                                array_size, member.name.c_str());
                        }
                    } else {
                        if (debug_mode) {
                            debug_print("Constant %s found but not const or "
                                        "not assigned\n",
                                        member.array_info.dimensions[0]
                                            .size_expr.c_str());
                        }
                        throw std::runtime_error(
                            "Constant '" +
                            member.array_info.dimensions[0].size_expr +
                            "' is not a valid const variable");
                    }
                } else {
                    if (debug_mode) {
                        debug_print(
                            "Constant %s not found in variable scope\n",
                            member.array_info.dimensions[0].size_expr.c_str());
                    }
                    throw std::runtime_error(
                        "Cannot resolve constant '" +
                        member.array_info.dimensions[0].size_expr +
                        "' for struct member array size");
                }
            }

            if (array_size <= 0) {
                throw std::runtime_error(
                    "Invalid array size for struct member " + member.name);
            }

            // struct_membersに配列メンバーを追加
            Variable array_member;
            array_member.type = member.type;
            array_member.is_array = true;
            array_member.array_size = array_size;
            array_member.is_assigned = false;

            if (debug_mode) {
                debug_print("Creating array member: %s with size %d\n",
                            member.name.c_str(), array_size);
            }

            if (array_size <= 0) {
                throw std::runtime_error("Invalid array size (" +
                                         std::to_string(array_size) +
                                         ") for struct member " + member.name +
                                         ". Array size must be positive.");
            }

            // 配列の値を初期化
            array_member.array_values.resize(array_size, 0);
            if (member.type == TYPE_STRING) {
                array_member.array_strings.resize(array_size, "");
            }

            if (debug_mode) {
                debug_print("Array member initialized: "
                            "array_values.size()=%zu, array_size=%d\n",
                            array_member.array_values.size(),
                            array_member.array_size);
            }

            struct_var.struct_members[member.name] = array_member;

            // マップにコピーされた後、再度配列を初期化
            struct_var.struct_members[member.name].array_values.resize(
                array_size, 0);
            if (member.type == TYPE_STRING) {
                struct_var.struct_members[member.name].array_strings.resize(
                    array_size, "");
            }

            if (debug_mode) {
                debug_print(
                    "Added array member to struct_members, final "
                    "array_values.size()=%zu\n",
                    struct_var.struct_members[member.name].array_values.size());
            }

            // 各要素を個別の変数としても作成（後方互換性のため）
            for (int i = 0; i < array_size; i++) {
                Variable array_element;
                array_element.type = member.type;

                // デフォルト値を設定
                if (array_element.type == TYPE_STRING) {
                    array_element.str_value = "";
                } else {
                    array_element.value = 0;
                }
                array_element.is_assigned = false;

                std::string element_name = var_name + "." + member.name + "[" +
                                           std::to_string(i) + "]";
                current_scope().variables[element_name] = array_element;
            }
        } else {
            if (debug_mode) {
                debug_print("Member %s is NOT an array\n", member.name.c_str());
            }
            // 通常のメンバ変数
            Variable member_var;
            member_var.type = member.type;

            // デフォルト値を設定
            if (member_var.type == TYPE_STRING) {
                member_var.str_value = "";
            } else {
                member_var.value = 0;
            }
            member_var.is_assigned = false;

            struct_var.struct_members[member.name] = member_var;
        }
    }

    current_scope().variables[var_name] = struct_var;
}

// structメンバにアクセス
Variable *Interpreter::get_struct_member(const std::string &var_name,
                                         const std::string &member_name) {
    if (debug_mode) {
        debug_print("get_struct_member: var=%s, member=%s\n", var_name.c_str(),
                    member_name.c_str());
    }

    Variable *var = find_variable(var_name);
    if (!var || !var->is_struct) {
        throw std::runtime_error("Variable is not a struct: " + var_name);
    }

    if (debug_mode) {
        debug_print("Found struct variable, struct_members.size()=%zu\n",
                    var->struct_members.size());
    }

    auto it = var->struct_members.find(member_name);
    if (it != var->struct_members.end()) {
        if (debug_mode) {
            debug_print("Found member in struct_members: %s, is_array=%d\n",
                        member_name.c_str(), it->second.is_array);
        }
        return &it->second;
    }

    throw std::runtime_error("Struct member not found: " + var_name + "." +
                             member_name);
}

// struct literalから値を代入
void Interpreter::assign_struct_literal(const std::string &var_name,
                                        const ASTNode *literal_node) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_STRUCT_LITERAL) {
        throw std::runtime_error("Invalid struct literal");
    }

    Variable *var = find_variable(var_name);

    // 配列要素への構造体リテラル代入の場合、まだ構造体変数が作成されていない可能性がある
    if (!var && var_name.find('[') != std::string::npos) {
        // 配列要素名から配列名を抽出
        size_t bracket_pos = var_name.find('[');
        std::string array_name = var_name.substr(0, bracket_pos);

        // 配列変数を取得して構造体型を確認
        Variable *array_var = find_variable(array_name);
        if (array_var && array_var->is_array &&
            !array_var->struct_type_name.empty()) {
            // 構造体配列の要素として新しい構造体変数を作成
            const StructDefinition *struct_def =
                find_struct_definition(array_var->struct_type_name);
            if (struct_def) {
                Variable element_var;
                element_var.type = TYPE_STRUCT;
                element_var.is_struct = true;
                element_var.struct_type_name = array_var->struct_type_name;
                element_var.is_assigned = false;

                // 構造体メンバーを初期化
                for (const auto &member_def : struct_def->members) {
                    Variable member_var;
                    member_var.type = member_def.type;
                    member_var.is_assigned = false;
                    if (member_def.array_info.is_array()) {
                        member_var.is_array = true;
                        int array_size =
                            member_def.array_info.dimensions.empty()
                                ? 0
                                : member_def.array_info.dimensions[0].size;
                        member_var.array_size = array_size;
                        member_var.array_values.resize(array_size, 0);

                        // 配列メンバーの個別要素も作成
                        for (int i = 0; i < array_size; i++) {
                            std::string element_name = var_name + "." +
                                                       member_def.name + "[" +
                                                       std::to_string(i) + "]";
                            Variable element_var;
                            element_var.type = member_def.array_info.base_type;
                            element_var.is_assigned = false;
                            current_scope().variables[element_name] =
                                element_var;
                        }
                    } else if (member_def.type == TYPE_STRING) {
                        member_var.str_value = "";
                    }
                    element_var.struct_members[member_def.name] = member_var;

                    // 個別メンバー変数も作成
                    std::string full_member_name =
                        var_name + "." + member_def.name;
                    current_scope().variables[full_member_name] = member_var;
                }

                // 要素変数を登録
                current_scope().variables[var_name] = element_var;
                var = find_variable(var_name);
            }
        }
    }

    if (!var || !var->is_struct) {
        throw std::runtime_error("Variable is not a struct: " + var_name);
    } // struct定義を取得してメンバ順序を確認
    const StructDefinition *struct_def =
        find_struct_definition(var->struct_type_name);
    if (!struct_def) {
        throw std::runtime_error("Struct definition not found: " +
                                 var->struct_type_name);
    }

    // 名前付き初期化かどうかをチェック
    bool is_named_init = false;
    if (!literal_node->arguments.empty() &&
        literal_node->arguments[0]->node_type == ASTNodeType::AST_ASSIGN) {
        is_named_init = true;
    }

    if (is_named_init) {
        // 名前付き初期化: {name: "Bob", age: 25}
        if (debug_mode) {
            debug_print("Processing named struct literal initialization\n");
        }

        for (const auto &member_init : literal_node->arguments) {
            if (member_init->node_type != ASTNodeType::AST_ASSIGN) {
                continue;
            }

            std::string member_name = member_init->name;
            if (debug_mode) {
                debug_print("Processing member initialization: %s\n",
                            member_name.c_str());
            }

            // 構造体配列要素の場合、個別変数にアクセス
            std::string full_member_name = var_name + "." + member_name;
            Variable *member_var = find_variable(full_member_name);

            // struct_membersから取得することを優先
            auto member_it = var->struct_members.find(member_name);
            if (member_it == var->struct_members.end()) {
                throw std::runtime_error("Unknown struct member: " +
                                         member_name);
            }

            // struct_membersの実際の要素への参照を取得
            Variable *struct_member_var = &member_it->second;

            // メンバ値を評価して代入
            if (member_init->right->node_type ==
                ASTNodeType::AST_ARRAY_LITERAL) {
                // 配列メンバーの場合: grades: [85, 92, 78]
                if (!struct_member_var->is_array) {
                    throw std::runtime_error("Member is not an array: " +
                                             member_name);
                }

                // 配列リテラルの各要素を個別変数に代入
                const auto &array_elements = member_init->right->arguments;
                for (size_t i = 0; i < array_elements.size() &&
                                   i < struct_member_var->array_size;
                     i++) {
                    std::string element_name = var_name + "." + member_name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    int64_t value = expression_evaluator_->evaluate_expression(
                        array_elements[i].get());

                    // 個別変数に代入
                    if (element_var) {
                        element_var->value = value;
                        element_var->is_assigned = true;

                        if (debug_mode) {
                            debug_print("Initialized struct member array "
                                        "element: %s = %lld\n",
                                        element_name.c_str(), (long long)value);
                        }
                    }

                    // struct_membersの配列要素にも代入
                    if (i < struct_member_var->array_values.size()) {
                        struct_member_var->array_values[i] = value;
                        if (debug_mode) {
                            debug_print("Updated struct_members array element: "
                                        "%s[%zu] = %lld\n",
                                        member_name.c_str(), i,
                                        (long long)value);
                        }
                    }
                }
                struct_member_var->is_assigned = true;
            } else if (struct_member_var->type == TYPE_STRING &&
                       member_init->right->node_type ==
                           ASTNodeType::AST_STRING_LITERAL) {
                // struct_membersの値を直接更新
                struct_member_var->str_value = member_init->right->str_value;
                struct_member_var->is_assigned = true;

                // 直接アクセス変数も更新
                if (member_var) {
                    member_var->str_value = member_init->right->str_value;
                    member_var->is_assigned = true;
                }
            } else {
                int64_t value = expression_evaluator_->evaluate_expression(
                    member_init->right.get());
                // struct_membersの値を直接更新
                struct_member_var->value = value;
                struct_member_var->is_assigned = true;

                // 直接アクセス変数も更新
                if (member_var) {
                    member_var->value = value;
                    member_var->is_assigned = true;
                }
            }
        }
    } else {
        // 位置ベース初期化: {25, "Bob"}
        if (literal_node->arguments.size() > struct_def->members.size()) {
            throw std::runtime_error("Too many initializers for struct");
        }

        for (size_t i = 0; i < literal_node->arguments.size(); ++i) {
            const StructMember &member_def = struct_def->members[i];
            auto it = var->struct_members.find(member_def.name);
            if (it == var->struct_members.end()) {
                continue;
            }

            const ASTNode *init_value = literal_node->arguments[i].get();

            // メンバ値を評価して代入
            if (it->second.type == TYPE_STRING &&
                init_value->node_type == ASTNodeType::AST_STRING_LITERAL) {
                it->second.str_value = init_value->str_value;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
                Variable *direct_member_var = find_variable(full_member_name);
                if (direct_member_var) {
                    direct_member_var->str_value = init_value->str_value;
                    direct_member_var->is_assigned = true;
                }
            } else {
                int64_t value =
                    expression_evaluator_->evaluate_expression(init_value);
                it->second.value = value;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
                Variable *direct_member_var = find_variable(full_member_name);
                if (direct_member_var) {
                    direct_member_var->value = value;
                    direct_member_var->is_assigned = true;
                }
            }
            it->second.is_assigned = true;
        }
    }
}

// structメンバに値を代入（数値）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       int64_t value) {
    Variable *member_var = get_struct_member(var_name, member_name);
    member_var->value = value;
    member_var->is_assigned = true;
}

// structメンバに値を代入（文字列）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       const std::string &value) {
    Variable *member_var = get_struct_member(var_name, member_name);
    member_var->str_value = value;
    member_var->is_assigned = true;
}

void Interpreter::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    int64_t value) {
    if (debug_mode) {
        debug_print("assign_struct_member_array_element: var=%s, member=%s, "
                    "index=%d, value=%lld\n",
                    var_name.c_str(), member_name.c_str(), index, value);
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    if (debug_mode) {
        debug_print("Found member_var, is_array=%d, array_size=%d, "
                    "array_values.size()=%zu\n",
                    member_var->is_array, member_var->array_size,
                    member_var->array_values.size());
    }

    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    if (debug_mode) {
        debug_print("About to assign value to array_values[%d]\n", index);
    }

    member_var->array_values[index] = value;
    member_var->is_assigned = true;

    if (debug_mode) {
        debug_print("Assignment completed, array_values[%d] = %lld\n", index,
                    member_var->array_values[index]);
    }
}

void Interpreter::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    const std::string &value) {
    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    member_var->array_strings[index] = value;
    member_var->is_assigned = true;
}

int64_t Interpreter::get_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index) {
    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    return member_var->array_values[index];
}

void Interpreter::assign_struct_member_array_literal(
    const std::string &var_name, const std::string &member_name,
    const ASTNode *array_literal) {
    if (debug_mode) {
        debug_print("assign_struct_member_array_literal: var=%s, member=%s\n",
                    var_name.c_str(), member_name.c_str());
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    // 共通実装を使用して配列リテラルを解析・代入
    try {
        auto result = common_operations_->parse_array_literal(array_literal);
        common_operations_->assign_array_literal_to_variable(member_var,
                                                             result);

        // 構造体メンバー配列の場合、個別要素変数も更新する必要がある
        if (!result.is_string_array) {
            for (size_t i = 0; i < result.size && i < result.int_values.size();
                 i++) {
                std::string element_name = var_name + "." + member_name + "[" +
                                           std::to_string(i) + "]";
                Variable *element_var = find_variable(element_name);
                if (element_var) {
                    element_var->value = result.int_values[i];
                    element_var->is_assigned = true;
                }
            }
        }

        if (debug_mode) {
            debug_print("Successfully assigned array literal to struct member "
                        "%s.%s using common operations\n",
                        var_name.c_str(), member_name.c_str());
        }
    } catch (const std::exception &e) {
        if (debug_mode) {
            debug_print(
                "Failed to assign array literal to struct member %s.%s: %s\n",
                var_name.c_str(), member_name.c_str(), e.what());
        }
        throw;
    }
}

void Interpreter::initialize_global_variables(const ASTNode *node) {
    if (!node)
        return;

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL) {
                initialize_global_variables(stmt.get());
            }
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
        if (debug_mode) {
            debug_print("Initializing global variable: %s\n",
                        node->name.c_str());
        }

        // グローバル変数を作成・初期化
        variable_manager_->process_var_decl_or_assign(node);

        // 変数が正しく作成されたか確認
        if (debug_mode) {
            Variable *created_var = find_variable(node->name);
            if (created_var) {
                debug_print("Global variable %s created successfully: "
                            "value=%ld, is_const=%d, is_assigned=%d\n",
                            node->name.c_str(), created_var->value,
                            created_var->is_const ? 1 : 0,
                            created_var->is_assigned ? 1 : 0);
            } else {
                debug_print("ERROR: Global variable %s creation failed\n",
                            node->name.c_str());
            }
        }
        break;

    default:
        break;
    }
}

void Interpreter::sync_enum_definitions_from_parser(RecursiveParser *parser) {
    if (!parser)
        return;

    auto &parser_enums = parser->enum_definitions_;
    for (const auto &pair : parser_enums) {
        const std::string &enum_name = pair.first;
        const EnumDefinition &enum_def = pair.second;

        std::cerr << "[DEBUG] Syncing enum definition: " << enum_name
                  << " with " << enum_def.members.size() << " members"
                  << std::endl;

        // Interpreterのenum定義に追加
        enum_definitions_[enum_name] = enum_def;

        // EnumManagerにも登録
        enum_manager_->register_enum(enum_name, enum_def);

        if (debug_mode) {
            debug_print("Synced enum definition: %s with %zu members
                        ",
                        enum_name.c_str(),
                        enum_def.members.size());
        }
    }
}
