#include "core/interpreter.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../../../common/utf8_utils.h"
#include "../../../frontend/recursive_parser/recursive_parser.h"
#include "core/error_handler.h"
#include "core/type_inference.h"
#include "evaluator/expression_evaluator.h"
#include "executors/control_flow_executor.h" // 制御フロー実行サービス
#include "executors/statement_executor.h"    // ヘッダーから移動
#include "executors/statement_list_executor.h" // 文リスト・複合文実行サービス
#include "handlers/assertion_handler.h" // アサーション文処理サービス
#include "handlers/break_continue_handler.h" // break/continue文処理サービス
#include "handlers/expression_statement_handler.h" // 式文処理サービス
#include "handlers/function_declaration_handler.h" // 関数宣言処理サービス
#include "handlers/impl_declaration_handler.h" // impl宣言処理サービス
#include "handlers/interface_declaration_handler.h" // インターフェース宣言処理サービス
#include "handlers/return_handler.h"             // return文処理サービス
#include "handlers/struct_declaration_handler.h" // 構造体宣言処理サービス
#include "managers/array_manager.h"
#include "managers/common_operations.h"
#include "managers/enum_manager.h"         // enum管理サービス
#include "managers/interface_operations.h"     // interface/impl管理サービス
#include "managers/static_variable_manager.h"   // static変数管理サービス
#include "managers/struct_operations.h"              // struct操作管理サービス
#include "managers/struct_variable_manager.h"        // struct変数管理サービス
#include "managers/struct_assignment_manager.h"      // struct代入管理サービス
#include "managers/struct_sync_manager.h"            // struct同期管理サービス
#include "managers/global_initialization_manager.h"  // グローバル初期化管理サービス
#include "managers/type_manager.h"
#include "managers/variable_manager.h"
#include "output/output_manager.h" // ヘッダーから移動
#include "services/array_processing_service.h" // DRY効率化: 統一配列処理サービス
#include "services/debug_service.h" // DRY効率化: 統一デバッグサービス
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
#include "services/variable_access_service.h" // DRY効率化: 統一変数アクセスサービス
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

std::string trim_copy(const std::string &text) {
    auto begin =
        std::find_if_not(text.begin(), text.end(),
                         [](unsigned char ch) { return std::isspace(ch); });
    auto end =
        std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
            return std::isspace(ch);
        }).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

} // namespace

// ========================================================================
// SECTION 0: Core Functions & Infrastructure (~1,000 lines)
// ========================================================================
// インタプリタのコア機能、初期化、スコープ管理
//
// 含まれる機能：
// - Constructor/Destructor
// - process() - メインエントリーポイント
// - evaluate(), evaluate_typed() - 式評価の委譲
// - execute_statement() - 文実行の委譲
// - Scope management: push_scope, pop_scope, push_interpreter_scope,
// pop_interpreter_scope
// - 検索機能: find_variable, find_function, find_struct_definition
// - アクセサメソッド: get_*_manager()
// - デバッグ機能: is_debug_mode, set_debug_mode
// - ヘルパー関数: find_variable_name, evaluate_ternary_typed
// - 一時変数管理: add_temp_variable, remove_temp_variable, clear_temp_variables
// ========================================================================

Interpreter::Interpreter(bool debug)
    : debug_mode(debug), output_manager_(std::make_unique<OutputManager>(this)),
      variable_manager_(std::make_unique<VariableManager>(this)),
      type_manager_(std::make_unique<TypeManager>(this)) {

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

    // DRY効率化: 統一配列処理サービスを初期化
    array_processing_service_ = std::make_unique<ArrayProcessingService>(
        this, common_operations_.get());

    // enum管理サービスを初期化
    enum_manager_ = std::make_unique<EnumManager>();

    // static変数管理サービスを初期化
    static_variable_manager_ = std::make_unique<StaticVariableManager>(this);

    // interface/impl管理サービスを初期化
    interface_operations_ = std::make_unique<InterfaceOperations>(this);

    // struct操作管理サービスを初期化
    struct_operations_ = std::make_unique<StructOperations>(this);

    // struct変数管理サービスを初期化
    struct_variable_manager_ = std::make_unique<StructVariableManager>(this);

    // struct代入管理サービスを初期化
    struct_assignment_manager_ =
        std::make_unique<StructAssignmentManager>(this);

    // struct同期管理サービスを初期化
    struct_sync_manager_ = std::make_unique<StructSyncManager>(this);

    // グローバル初期化管理サービスを初期化
    global_initialization_manager_ =
        std::make_unique<GlobalInitializationManager>(this);

    // 制御フロー実行サービスを初期化
    control_flow_executor_ = std::make_unique<ControlFlowExecutor>(this);

    // 文リスト・複合文実行サービスを初期化
    statement_list_executor_ = std::make_unique<StatementListExecutor>(this);

    // return文処理サービスを初期化
    return_handler_ = std::make_unique<ReturnHandler>(this);

    // アサーション文処理サービスを初期化
    assertion_handler_ = std::make_unique<AssertionHandler>(this);

    // break/continue文処理サービスを初期化
    break_continue_handler_ = std::make_unique<BreakContinueHandler>(this);

    // 関数宣言処理サービスを初期化
    function_declaration_handler_ =
        std::make_unique<FunctionDeclarationHandler>(this);

    // 構造体宣言処理サービスを初期化
    struct_declaration_handler_ =
        std::make_unique<StructDeclarationHandler>(this);

    // インターフェース宣言処理サービスを初期化
    interface_declaration_handler_ =
        std::make_unique<InterfaceDeclarationHandler>(this);

    // impl宣言処理サービスを初期化
    impl_declaration_handler_ = std::make_unique<ImplDeclarationHandler>(this);

    // 式文処理サービスを初期化
    expression_statement_handler_ =
        std::make_unique<ExpressionStatementHandler>(this);

    // グローバルスコープを初期化
    // ネストされた関数呼び出しに備えて容量を予約（再割り当てを防ぐ）
    scope_stack.reserve(64);
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

std::string
Interpreter::find_variable_name_by_address(const Variable *target_var) {
    if (!target_var) {
        return "";
    }

    // 現在のスコープスタックから検索
    // 全スコープを逆順に検索（最新のスコープから）
    if (!scope_stack.empty()) {
        for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
            for (const auto &[name, var] : it->variables) {
                if (&var == target_var) {
                    return name;
                }
            }
        }
    }

    // グローバルスコープも確認
    for (const auto &[name, var] : global_scope.variables) {
        if (&var == target_var) {
            return name;
        }
    }

    return "";
}

const ASTNode *Interpreter::find_function(const std::string &name) {
    // グローバルスコープの関数を検索
    if (debug_mode) {
        std::cerr << "[FIND_FUNCTION] Looking for: " << name << std::endl;
        std::cerr << "[FIND_FUNCTION] Available functions: ";
        for (const auto &pair : global_scope.functions) {
            std::cerr << pair.first << " ";
        }
        std::cerr << std::endl;
    }

    auto func_it = global_scope.functions.find(name);
    if (func_it != global_scope.functions.end()) {
        if (debug_mode) {
            std::cerr << "[FIND_FUNCTION] Found: " << name << std::endl;
        }
        return func_it->second;
    }

    if (debug_mode) {
        std::cerr << "[FIND_FUNCTION] Not found: " << name << std::endl;
    }
    return nullptr;
}

// ========================================================================
// SECTION 1: Initialization & Global Declarations (~400 lines)
// ========================================================================
// - register_global_declarations() - グローバル宣言の登録
// - initialize_global_variables() - グローバル変数の初期化
// - sync_enum_definitions_from_parser() - Enum定義の同期
// - sync_struct_definitions_from_parser() - Struct定義の同期
//
// このセクションは将来的に initialization_manager.cpp に抽出予定
// ========================================================================

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
        case ASTNodeType::AST_INTERFACE_DECL:
            node_type_name = "AST_INTERFACE_DECL";
            break;
        case ASTNodeType::AST_IMPL_DECL:
            node_type_name = "AST_IMPL_DECL";
            break;
        case ASTNodeType::AST_FUNC_DECL:
            node_type_name = "AST_FUNC_DECL";
            break;
        case ASTNodeType::AST_TYPEDEF_DECL:
            node_type_name = "AST_TYPEDEF_DECL";
            break;
        case ASTNodeType::AST_UNION_TYPEDEF_DECL:
            node_type_name = "AST_UNION_TYPEDEF_DECL";
            break;
        default:
            break;
        }
        debug_msg(DebugMsgId::PARSE_REGISTER_GLOBAL_DECL, node_type_name,
                  node->name.c_str());
        // DRY効率化: 統一デバッグサービスでの出力例（将来的に移行）
        DEBUG_DEBUG(GENERAL, "Processing global declaration: %s (name: %s)",
                    node_type_name, node->name.c_str());
    }

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        // 2パス変数宣言処理: 先にconst変数、次に配列
        // まずconst変数（配列以外）のみを処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL &&
                stmt->is_const && stmt->array_dimensions.empty()) {
                register_global_declarations(stmt.get());
            }
        }
        // 次に残りの変数宣言を処理（配列を含む）
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL ||
                stmt->node_type == ASTNodeType::AST_ARRAY_DECL) {
                // const変数は既に処理済みなのでスキップ
                if (stmt->node_type == ASTNodeType::AST_VAR_DECL &&
                    stmt->is_const && stmt->array_dimensions.empty()) {
                    continue;
                }
                register_global_declarations(stmt.get());
            }
        }
        // 次にstruct定義を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_STRUCT_DECL ||
                stmt->node_type == ASTNodeType::AST_STRUCT_TYPEDEF_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        // enumの定義を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_ENUM_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        // typedef宣言を処理（通常のtypedef）
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_TYPEDEF_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        // union typedef宣言を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_UNION_TYPEDEF_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        // interface定義を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_INTERFACE_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        // impl定義を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_IMPL_DECL) {
                register_global_declarations(stmt.get());
            }
        }

        // 最後にその他の宣言（関数など）を処理
        for (const auto &stmt : node->statements) {
            if (stmt->node_type != ASTNodeType::AST_VAR_DECL &&
                stmt->node_type != ASTNodeType::AST_ARRAY_DECL &&
                stmt->node_type != ASTNodeType::AST_STRUCT_DECL &&
                stmt->node_type != ASTNodeType::AST_STRUCT_TYPEDEF_DECL &&
                stmt->node_type != ASTNodeType::AST_ENUM_DECL &&
                stmt->node_type != ASTNodeType::AST_TYPEDEF_DECL &&
                stmt->node_type != ASTNodeType::AST_UNION_TYPEDEF_DECL &&
                stmt->node_type != ASTNodeType::AST_INTERFACE_DECL &&
                stmt->node_type != ASTNodeType::AST_IMPL_DECL) {
                register_global_declarations(stmt.get());
            }
        }
        break;

    case ASTNodeType::AST_STRUCT_DECL:
    case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
        // struct定義を登録
        {
            debug_msg(DebugMsgId::PARSE_STRUCT_REGISTER, node->name.c_str());
            // DRY効率化: 統一デバッグサービス（併用期間）
            DEBUG_DEBUG(STRUCT, "Registering struct definition: %s",
                        node->name.c_str());
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
                        array_member.is_pointer = member_node->is_pointer;
                        array_member.pointer_depth = member_node->pointer_depth;
                        array_member.pointer_base_type_name =
                            member_node->pointer_base_type_name;
                        array_member.pointer_base_type =
                            member_node->pointer_base_type;
                        array_member.is_private =
                            member_node->is_private_member;
                        array_member.is_reference = member_node->is_reference;
                        array_member.is_unsigned = member_node->is_unsigned;
                        array_member.is_const = member_node->is_const;
                        struct_def.members.push_back(array_member);

                        debug_msg(
                            DebugMsgId::INTERPRETER_STRUCT_ARRAY_MEMBER_ADDED,
                            member_node->name.c_str(),
                            (int)member_node->type_info,
                            member_node->array_type_info.dimensions[0].size);

                        // Array dimension details
                        const auto &dim =
                            member_node->array_type_info.dimensions[0];
                        debug_msg(DebugMsgId::INTERPRETER_ARRAY_DIMENSION_INFO,
                                  dim.size, dim.is_dynamic ? 1 : 0,
                                  dim.size_expr.c_str());
                    } else {
                        struct_def.add_member(
                            member_node->name, member_node->type_info,
                            member_node->type_name, member_node->is_pointer,
                            member_node->pointer_depth,
                            member_node->pointer_base_type_name,
                            member_node->pointer_base_type,
                            member_node->is_private_member,
                            member_node->is_reference, member_node->is_unsigned,
                            member_node->is_const);
                        debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBER_ADDED,
                                  member_node->name.c_str(),
                                  (int)member_node->type_info);
                    }
                }
            }

            register_struct_definition(struct_name, struct_def);
            debug_msg(DebugMsgId::INTERPRETER_STRUCT_REGISTERED,
                      struct_name.c_str(), struct_def.members.size());
        }
        break;

    case ASTNodeType::AST_ENUM_DECL:
        // enum定義を登録
        {
            debug_msg(DebugMsgId::INTERPRETER_ENUM_REGISTERING,
                      node->name.c_str());
            DEBUG_DEBUG(GENERAL, "Registering enum definition: %s",
                        node->name.c_str());

            // ASTノードからenum定義情報を取得
            const EnumDefinition &enum_def = node->enum_definition;

            enum_manager_->register_enum(node->name, enum_def);

            if (debug_mode) {
                debug_print("Successfully registered enum: %s\n",
                            node->name.c_str());
            }
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        if (node->node_type == ASTNodeType::AST_MULTIPLE_VAR_DECL) {
            // 複数変数宣言の場合、各子ノードを処理
            debug_msg(DebugMsgId::INTERPRETER_MULTIPLE_VAR_DECL_START,
                      node->children.size());
            for (const auto &child : node->children) {
                if (child->node_type == ASTNodeType::AST_VAR_DECL) {
                    register_global_declarations(child.get());
                }
            }
        } else if (node->node_type == ASTNodeType::AST_ASSIGN) {
            debug_msg(DebugMsgId::INTERPRETER_GLOBAL_VAR_INIT_START,
                      node->name.c_str());
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
                debug_msg(DebugMsgId::INTERPRETER_ARRAY_LITERAL_INIT,
                          node->name.c_str());
                assign_array_literal(node->name, node->right.get());
            } else {
                // 通常の初期化
                debug_msg(DebugMsgId::INTERPRETER_NORMAL_VAR_INIT,
                          node->name.c_str());
                Variable var;
                var.type =
                    node->type_info != TYPE_VOID ? node->type_info : TYPE_INT;
                var.is_const = node->is_const;
                var.is_unsigned = node->is_unsigned;
                var.is_assigned = false;

                if (node->right) {
                    int64_t value = expression_evaluator_->evaluate_expression(
                        node->right.get());
                    if (var.type == TYPE_STRING) {
                        var.str_value = node->right->str_value;
                    } else {
                        if (var.is_unsigned && value < 0) {
                            DEBUG_WARN(
                                VARIABLE,
                                "Unsigned global variable %s initialized with "
                                "negative value (%lld); clamping to 0",
                                node->name.c_str(),
                                static_cast<long long>(value));
                            value = 0;
                        }
                        var.value = value;
                        check_type_range(var.type, value, node->name,
                                         var.is_unsigned);
                    }
                    var.is_assigned = true;
                }

                global_scope.variables[node->name] = var;
            }
        } else if (node->node_type == ASTNodeType::AST_VAR_DECL) {
            // グローバル変数宣言をVariableManagerに委譲
            variable_manager_->declare_global_variable(node);

            // const変数の場合は即座に初期化（配列サイズ式で使用される可能性があるため）
            if (node->is_const && node->init_expr) {
                TypedValue typed_result =
                    expression_evaluator_->evaluate_typed_expression(
                        node->init_expr.get());
                variable_manager_->assign_variable(node->name, typed_result,
                                                   TYPE_UNKNOWN, false);
            }
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

    case ASTNodeType::AST_UNION_TYPEDEF_DECL:
        // union typedef宣言をTypeManagerに委譲
        type_manager_->register_union_typedef(node->name,
                                              node->union_definition);
        break;

    case ASTNodeType::AST_INTERFACE_DECL:
        // interface定義を登録
        {
            std::string interface_name = node->name;
            debug_msg(DebugMsgId::INTERFACE_DECL_START, interface_name.c_str());

            InterfaceDefinition interface_def(interface_name);

            // ASTノードからメソッドシグネチャを復元
            for (const auto &method_node : node->arguments) {
                if (method_node->node_type == ASTNodeType::AST_FUNC_DECL) {
                    interface_def.add_method(method_node->name,
                                             method_node->type_info);
                    debug_msg(DebugMsgId::INTERFACE_METHOD_FOUND,
                              method_node->name.c_str());
                }
            }

            register_interface_definition(interface_name, interface_def);
            debug_msg(DebugMsgId::INTERFACE_DECL_COMPLETE,
                      interface_name.c_str());
        }
        break;

    case ASTNodeType::AST_IMPL_DECL:
        handle_impl_declaration(node);
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
    debug_msg(DebugMsgId::INTERPRETER_GLOBAL_VAR_INIT);
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
        debug_msg(DebugMsgId::MAIN_FUNC_EXECUTE);

        if (main_func->body) {
            debug_msg(DebugMsgId::MAIN_FUNC_BODY_EXISTS);
        } else {
            debug_msg(DebugMsgId::MAIN_FUNC_BODY_NULL);
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

TypedValue Interpreter::evaluate_typed(const ASTNode *node) {
    return expression_evaluator_->evaluate_typed_expression(node);
}

TypedValue Interpreter::evaluate_typed_expression(const ASTNode *node) {
    return expression_evaluator_->evaluate_typed_expression(node);
}

// N次元配列リテラル処理の再帰関数
void Interpreter::process_ndim_array_literal(const ASTNode *literal_node,
                                             Variable &var, TypeInfo elem_type,
                                             int &flat_index, int max_size) {
    // NOTE:
    // この実装はArrayManager::processMultidimensionalArrayLiteralと重複する機能
    // 将来的にはArrayProcessingService経由での統一処理が推奨されます
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
                    debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                              ("Set string element[" +
                               std::to_string(flat_index) +
                               "] = " + element->str_value)
                                  .c_str());
                }
            } else {
                int64_t val =
                    expression_evaluator_->evaluate_expression(element.get());
                var.multidim_array_values[flat_index] = val;
                debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                          ("Set element[" + std::to_string(flat_index) +
                           "] = " + std::to_string(val))
                              .c_str());
            }
            flat_index++;
        }
    }
}

// ============================================================================
// execute_statement - 文実行のメインメソッド
// ============================================================================
// このメソッドは1,215行の巨大switch文です。
// 全ての文（宣言、代入、制御フロー、出力等）の実行を担当します。
//
// 【主なセクション】:
// - Line 697-709:   文リスト（STMT_LIST, COMPOUND_STMT）
// - Line 710-722:   変数宣言（VAR_DECL, MULTIPLE_VAR_DECL）
// - Line 723-858:   代入文（ASSIGN）
// - Line 859-910:   配列・構造体宣言（ARRAY_DECL, STRUCT_DECL,
// STRUCT_TYPEDEF_DECL）
// - Line 911-927:   Interface・impl宣言（INTERFACE_DECL, IMPL_DECL）
// - Line 928-964:   出力文（PRINT_STMT, PRINTLN_STMT, PRINTF_STMT,
// PRINTLNF_STMT）
// - Line 965-1036:  if文（IF_STMT）
// - Line 1037-1112: while文（WHILE_STMT）
// - Line 1113-1198: for文（FOR_STMT）
// - Line 1199-1309: assert文（ASSERT_STMT）
// - Line 1310-1443: return文（RETURN_STMT）
// - Line 1444-1457: break/continue文（BREAK_STMT, CONTINUE_STMT）
// - Line 1458-1842: 関数宣言（FUNC_DECL）
//
// 【TODO - 将来の改善】:
// このメソッドはStatementExecutorクラスに完全に移行すべき:
// 1. statement_executor_->execute(node) への単純な委譲にする
// 2. StatementExecutorで各statement typeごとのヘルパーメソッドを実装
// 3. パーサーリファクタリングの成功例（parseStatement:
// 1,452行→64行）を参考に進める
//
// 【注意】:
// 一部の文はStatementExecutorでも処理されるが、多くはまだこのメソッドで直接処理されている。
// StatementExecutorとの二重管理を避けるため、段階的な移行が必要。
// ============================================================================

void Interpreter::execute_statement(const ASTNode *node) {
    if (!node)
        return;

    // ASTNodeTypeが異常な値でないことを確認
    int node_type_int = static_cast<int>(node->node_type);
    if (node_type_int < 0 || node_type_int > 100) {
        if (debug_mode) {
            std::cerr << "[CRITICAL_CORE] Abnormal node_type detected in core "
                         "interpreter: "
                      << node_type_int << ", skipping execution" << std::endl;
        }
        return;
    }

    debug_msg(DebugMsgId::INTERPRETER_EXEC_STMT, node_type_int);

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
        case ASTNodeType::AST_FOR_STMT:
            node_type_name = "AST_FOR_STMT";
            break;
        case ASTNodeType::AST_COMPOUND_STMT:
            node_type_name = "AST_COMPOUND_STMT";
            break;
        default:
            break;
        }
        (void)node_type_name; // Suppress unused variable warning
    }

    switch (node->node_type) {
    // ========================================================================
    // 文リスト（STMT_LIST）- 再帰的な文実行
    // StatementListExecutorに委譲済み
    // ========================================================================
    case ASTNodeType::AST_STMT_LIST:
        statement_list_executor_->execute_statement_list(node);
        break;

    // ========================================================================
    // 複合文（COMPOUND_STMT）- ブロック内の文のリスト
    // StatementListExecutorに委譲済み
    // ========================================================================
    case ASTNodeType::AST_COMPOUND_STMT:
        statement_list_executor_->execute_compound_statement(node);
        break;

    // ========================================================================
    // 変数宣言（VAR_DECL, MULTIPLE_VAR_DECL）
    // VariableManagerに委譲済み
    // ========================================================================
    case ASTNodeType::AST_VAR_DECL:
        debug_msg(DebugMsgId::INTERPRETER_VAR_DECL, node->name.c_str());
        debug_msg(DebugMsgId::INTERPRETER_VAR_DECL_TYPE, (int)node->type_info);
        // 変数宣言をVariableManagerに委譲
        if (debug_mode) {
            std::cerr << "[INTERPRETER] About to call "
                         "process_var_decl_or_assign for: "
                      << node->name << std::endl;
        }
        try {
            variable_manager_->process_var_decl_or_assign(node);
            debug_msg(DebugMsgId::INTERPRETER_VAR_DECL_SUCCESS,
                      node->name.c_str());
        } catch (const std::exception &e) {
            error_msg(DebugMsgId::INTERPRETER_VAR_PROCESS_EXCEPTION, e.what());
            throw;
        }
        break;

    // ========================================================================
    // 代入文（ASSIGN）
    // StatementExecutorに委譲済み
    // ========================================================================
    case ASTNodeType::AST_ASSIGN:
        debug_msg(DebugMsgId::INTERPRETER_ASSIGNMENT, node->name.c_str());
        // 代入をStatementExecutorに委譲
        statement_executor_->execute(node);
        debug_msg(DebugMsgId::INTERPRETER_ASSIGNMENT_SUCCESS,
                  node->name.c_str());
        break;

    // ========================================================================
    // 複数変数宣言（MULTIPLE_VAR_DECL）
    // ========================================================================
    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
        debug_msg(DebugMsgId::INTERPRETER_MULTIPLE_VAR_DECL_EXEC, "");
        statement_executor_->execute_multiple_var_decl(node);
        break;

    // ========================================================================
    // 配列・構造体宣言（ARRAY_DECL, STRUCT_DECL, STRUCT_TYPEDEF_DECL）
    // ========================================================================
    case ASTNodeType::AST_ARRAY_DECL:
        debug_msg(DebugMsgId::INTERPRETER_ARRAY_DECL_EXEC, node->name.c_str());
        statement_executor_->execute_array_decl(node);
        break;

    case ASTNodeType::AST_STRUCT_DECL:
    case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
        struct_declaration_handler_->handle_struct_declaration(node);
        break;

    case ASTNodeType::AST_INTERFACE_DECL:
        interface_declaration_handler_->handle_interface_declaration(node);
        break;

    case ASTNodeType::AST_IMPL_DECL:
        impl_declaration_handler_->handle_impl_declaration(node);
        break;

    case ASTNodeType::AST_PRINT_STMT:
        debug_msg(DebugMsgId::PRINT_EXECUTING_STATEMENT);
        if (!node->arguments.empty()) {
            // 複数引数のprint文（再帰下降パーサー対応）
            debug_msg(DebugMsgId::PRINT_STATEMENT_HAS_ARGS);
            output_manager_->print_multiple(node);
        } else if (node->left) {
            // 単一引数のprint文
            debug_msg(DebugMsgId::PRINT_STATEMENT_HAS_ARGS);
            print_value(node->left.get());
        } else {
            debug_msg(DebugMsgId::PRINT_NO_ARGUMENTS);
        }
        break;

    case ASTNodeType::AST_PRINTLN_STMT:
        // debug_msg removed - already logged at function entry (line 639)
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

    // ========================================================================
    // 制御フロー文（IF_STMT, WHILE_STMT, FOR_STMT）
    // ControlFlowExecutorに委譲済み
    // ========================================================================
    case ASTNodeType::AST_IF_STMT:
        control_flow_executor_->execute_if_statement(node);
        break;

    case ASTNodeType::AST_WHILE_STMT:
        control_flow_executor_->execute_while_statement(node);
        break;

    case ASTNodeType::AST_FOR_STMT:
        control_flow_executor_->execute_for_statement(node);
        break;

    // ========================================================================
    // assert文（ASSERT_STMT）
    // AssertionHandlerに委譲済み
    // ========================================================================
    case ASTNodeType::AST_ASSERT_STMT:
        assertion_handler_->handle_assertion(node);
        break;

    // ========================================================================
    // return文（RETURN_STMT）
    // ReturnHandlerに完全委譲
    // ========================================================================
    // ========================================================================
    // return文（RETURN_STMT）
    // ReturnHandlerに完全委譲済み
    // ========================================================================
    case ASTNodeType::AST_RETURN_STMT:
        return_handler_->execute_return_statement(node);
        break;

    // ========================================================================
    // break/continue文（BREAK_STMT, CONTINUE_STMT）
    // BreakContinueHandlerに委譲済み
    // ========================================================================
    case ASTNodeType::AST_BREAK_STMT:
        break_continue_handler_->handle_break(node);
        break;

    case ASTNodeType::AST_CONTINUE_STMT:
        break_continue_handler_->handle_continue(node);
        break;

    // ========================================================================
    // 関数宣言（FUNC_DECL）
    // 関数定義をグローバルスコープに登録
    // ========================================================================
    case ASTNodeType::AST_FUNC_DECL:
        function_declaration_handler_->handle_function_declaration(node);
        break;

    // ========================================================================
    // 未対応の文型（式文として評価を試みる）
    // ========================================================================
    default:
        expression_statement_handler_->handle_expression_statement(node);
        break;
    }
}
// ============================================================================
// execute_statement メソッド終了
// ============================================================================

// ========================================================================
// SECTION 4: Variable Assignment & Parameters (~300 lines)
// ========================================================================
// 変数代入、関数パラメータ、配列要素代入などを管理
//
// これらのメソッドは主にVariableManagerに委譲しているため、
// 将来的により完全にManagerに移動することを検討
//
// 含まれる機能：
// - assign_variable (複数のオーバーロード)
// - assign_union_variable
// - assign_function_parameter
// - assign_array_parameter
// - assign_interface_view
// - assign_array_element, assign_array_element_float
// - assign_string_element
// - print_value, print_formatted
// ========================================================================

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
    InferredType inferred(TYPE_STRING, "string");
    TypedValue typed_value(value, inferred);
    variable_manager_->assign_variable(name, typed_value, TYPE_STRING, false);
}

void Interpreter::assign_variable(const std::string &name,
                                  const std::string &value, bool is_const) {
    InferredType inferred(TYPE_STRING, "string");
    TypedValue typed_value(value, inferred);
    variable_manager_->assign_variable(name, typed_value, TYPE_STRING,
                                       is_const);
}

void Interpreter::assign_variable(const std::string &name,
                                  const TypedValue &value, TypeInfo type_hint,
                                  bool is_const) {
    variable_manager_->assign_variable(name, value, type_hint, is_const);
}

void Interpreter::assign_union_variable(const std::string &name,
                                        const ASTNode *value_node) {
    Variable *var = find_variable(name);
    if (!var) {
        throw std::runtime_error("Undefined variable: " + name);
    }

    if (var->type != TYPE_UNION) {
        throw std::runtime_error("Variable is not a union type: " + name);
    }
    if (debug_mode) {
        debug_print(
            "UNION_ASSIGN_INTERPRETER_DEBUG: Variable '%s' type_name='%s'\n",
            name.c_str(), var->type_name.c_str());
    }

    variable_manager_->assign_union_value(*var, var->type_name, value_node);
}

void Interpreter::handle_impl_declaration(const ASTNode *node) {
    interface_operations_->handle_impl_declaration(node);
}

void Interpreter::assign_function_parameter(const std::string &name,
                                            int64_t value, TypeInfo type,
                                            bool is_unsigned) {
    variable_manager_->assign_function_parameter(name, value, type,
                                                 is_unsigned);
}

void Interpreter::assign_function_parameter(const std::string &name,
                                            const TypedValue &value,
                                            TypeInfo type, bool is_unsigned) {
    variable_manager_->assign_function_parameter(name, value, type,
                                                 is_unsigned);
}

void Interpreter::assign_array_parameter(const std::string &name,
                                         const Variable &source_array,
                                         TypeInfo type) {
    variable_manager_->assign_array_parameter(name, source_array, type);
}

void Interpreter::assign_interface_view(const std::string &dest_name,
                                        Variable interface_var,
                                        const Variable &source_var,
                                        const std::string &source_var_name) {
    variable_manager_->assign_interface_view(
        dest_name, std::move(interface_var), source_var, source_var_name);
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

void Interpreter::assign_array_element_float(const std::string &name,
                                             int64_t index, double value) {
    debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_DEBUG, name.c_str(), index,
              static_cast<int64_t>(value));

    Variable *var = find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::VARIABLE_NOT_FOUND, name.c_str());
        error_msg(DebugMsgId::UNDEFINED_ARRAY_ERROR, name.c_str());
        throw std::runtime_error("Undefined array");
    }

    // 境界チェック
    int idx = static_cast<int>(index);
    if (idx < 0 || idx >= var->array_size) {
        debug_msg(DebugMsgId::ARRAY_INDEX_OUT_OF_BOUNDS, index,
                  var->array_size);
        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, name.c_str());
        throw std::runtime_error("Array index out of bounds");
    }

    // 配列の基底型を取得
    TypeInfo base_type =
        (var->type >= TYPE_ARRAY_BASE)
            ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
            : var->type;

    // 型に応じた代入
    if (base_type == TYPE_FLOAT) {
        if (var->array_float_values.empty()) {
            var->array_float_values.resize(var->array_size, 0.0f);
        }
        var->array_float_values[idx] = static_cast<float>(value);
    } else if (base_type == TYPE_DOUBLE) {
        if (var->array_double_values.empty()) {
            var->array_double_values.resize(var->array_size, 0.0);
        }
        var->array_double_values[idx] = value;
    } else if (base_type == TYPE_QUAD) {
        if (var->array_quad_values.empty()) {
            var->array_quad_values.resize(var->array_size, 0.0L);
        }
        var->array_quad_values[idx] = static_cast<long double>(value);
    } else {
        throw std::runtime_error(
            "assign_array_element_float called on non-float array");
    }

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
                                   const std::string &name, bool is_unsigned) {
    type_manager_->check_type_range(type, value, name, is_unsigned);
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

// ========================================================================
// SECTION 5: Array Operations (~300 lines)
// ========================================================================
// 多次元配列のアクセス、設定、抽出を管理
//
// これらのメソッドは主にArrayManagerに委譲しているため、
// 将来的により完全にManagerに移動することを検討
//
// 含まれる機能：
// - getMultidimensionalArrayElement (2 overloads)
// - setMultidimensionalArrayElement (2 overloads)
// - getMultidimensionalStringArrayElement
// - setMultidimensionalStringArrayElement
// - extract_array_name, extract_array_indices, extract_array_element_name
// - assign_array_literal, assign_array_from_return
// - process_ndim_array_literal
// ========================================================================

int64_t Interpreter::getMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices) {
    // Priority 3: ArrayProcessingServiceを使用した統一アクセス
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        // 名前が見つからない場合は従来の方法にフォールバック
        return array_manager_->getMultidimensionalArrayElement(var, indices);
    }
    return array_processing_service_->getArrayElement(
        var_name, indices,
        ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

void Interpreter::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, int64_t value) {
    // Priority 3: ArrayProcessingServiceを使用した統一アクセス
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        // 名前が見つからない場合は従来の方法にフォールバック
        array_manager_->setMultidimensionalArrayElement(var, indices, value);
        return;
    }
    array_processing_service_->setArrayElement(
        var_name, indices, value,
        ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

// float/double値での多次元配列要素設定（オーバーロード）
void Interpreter::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, double value) {
    // 直接ArrayManagerを呼び出す
    array_manager_->setMultidimensionalArrayElement(var, indices, value);
}

std::string Interpreter::getMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices) {
    // Priority 3: ArrayProcessingServiceを使用した統一アクセス
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        // 名前が見つからない場合は従来の方法にフォールバック
        return array_manager_->getMultidimensionalStringArrayElement(var,
                                                                     indices);
    }
    return array_processing_service_->getStringArrayElement(
        var_name, indices,
        ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

void Interpreter::setMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices,
    const std::string &value) {
    // Priority 3: ArrayProcessingServiceを使用した統一アクセス
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        // 名前が見つからない場合は従来の方法にフォールバック
        array_manager_->setMultidimensionalStringArrayElement(var, indices,
                                                              value);
        return;
    }
    array_processing_service_->setStringArrayElement(
        var_name, indices, value,
        ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

// Priority 3: 変数ポインターから名前を取得するヘルパー
std::string Interpreter::find_variable_name(const Variable *target_var) {
    if (!target_var)
        return "";

    // VariableManagerから変数名を取得
    return variable_manager_->find_variable_name(target_var);
}

void Interpreter::assign_array_literal(const std::string &name,
                                       const ASTNode *literal_node) {
    if (debug_mode) {
        debug_print("assign_array_literal called for variable: %s\n",
                    name.c_str());
    }

    // 変数のコンテキストを判定
    ArrayProcessingService::ArrayContext context;
    if (variable_manager_->is_global_variable(name)) {
        context = ArrayProcessingService::ArrayContext::GLOBAL_VARIABLE;
    } else {
        context = ArrayProcessingService::ArrayContext::LOCAL_VARIABLE;
    }

    // ArrayProcessingServiceを使用して統一処理
    auto result = array_processing_service_->processArrayLiteral(
        name, literal_node, context);

    if (!result.success) {
        if (debug_mode) {
            debug_print("ArrayProcessingService failed for '%s': %s\n",
                        name.c_str(), result.error_message.c_str());
        }
        throw std::runtime_error("Array assignment failed: " +
                                 result.error_message);
    }

    if (debug_mode) {
        debug_print("Successfully assigned array literal to '%s' using "
                    "ArrayProcessingService\n",
                    name.c_str());
    }
}

void Interpreter::assign_array_from_return(const std::string &name,
                                           const ReturnException &ret) {
    std::cerr << "[DEBUG_ASSIGN_RETURN] assign_array_from_return called for: "
              << name << std::endl;
    std::cerr << "[DEBUG_ASSIGN_RETURN] ret.is_array: " << ret.is_array
              << std::endl;
    std::cerr << "[DEBUG_ASSIGN_RETURN] ret.int_array_3d.empty(): "
              << ret.int_array_3d.empty() << std::endl;
    std::cerr << "[DEBUG_ASSIGN_RETURN] ret.str_array_3d.empty(): "
              << ret.str_array_3d.empty() << std::endl;

    if (!ret.is_array) {
        throw std::runtime_error("Return value is not an array");
    }

    // 変数を検索
    Variable *var = find_variable(name);
    if (!var) {
        throw std::runtime_error("Variable '" + name + "' not found");
    }

    if (var->is_const && var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
        throw std::runtime_error("Cannot assign to const array: " + name);
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

    std::cerr << "[DEBUG_ASSIGN_RETURN] declared_array_size: "
              << declared_array_size << std::endl;
    std::cerr << "[DEBUG_ASSIGN_RETURN] var->array_dimensions.size(): "
              << var->array_dimensions.size() << std::endl;
    if (!var->array_dimensions.empty()) {
        std::cerr << "[DEBUG_ASSIGN_RETURN] var->array_dimensions[0]: "
                  << var->array_dimensions[0] << std::endl;
    }

    // ReturnExceptionから配列データを取得して変数に代入
    if (!ret.str_array_3d.empty()) {
        // 文字列配列の場合
        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                  "Processing string array return value");

        if (var->is_multidimensional) {
            // 多次元文字列配列の場合、multidim_array_stringsに設定
            var->multidim_array_strings.clear();

            // 3D配列から適切にデータを復元
            for (const auto &plane : ret.str_array_3d) {
                for (const auto &row : plane) {
                    for (const auto &element : row) {
                        var->multidim_array_strings.push_back(element);
                    }
                }
            }

            actual_return_size = var->multidim_array_strings.size();
            var->array_size = actual_return_size;
            var->array_strings.clear();
        } else {
            // 1次元文字列配列の場合（従来処理）
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
        }

        var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING);
        var->array_values.clear();

    } else if (ret.is_struct_array && !ret.struct_array_3d.empty()) {
        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                  "Processing struct array return value");

        // フラット化して要素数を取得
        std::vector<Variable> flattened_structs;
        for (const auto &plane : ret.struct_array_3d) {
            for (const auto &row : plane) {
                for (const auto &element : row) {
                    flattened_structs.push_back(element);
                }
            }
        }

        actual_return_size = static_cast<int>(flattened_structs.size());

        if (declared_array_size > 0 &&
            declared_array_size != actual_return_size) {
            std::cerr << "[WARN] Struct array size mismatch: declared "
                      << declared_array_size << " but got "
                      << actual_return_size << ", using returned size"
                      << std::endl;
            // エラーではなく警告として扱い、返された配列のサイズを使用
        }

        // 構造体配列の各要素を更新
        for (int i = 0; i < actual_return_size; ++i) {
            const Variable &source_struct = flattened_structs[i];
            std::string element_name = name + "[" + std::to_string(i) + "]";

            Variable *element_var = find_variable(element_name);
            if (!element_var) {
                // 同じスコープに要素変数が存在しない場合は作成
                current_scope().variables[element_name] = source_struct;
                element_var = &current_scope().variables[element_name];
            } else {
                *element_var = source_struct;
            }

            element_var->is_assigned = true;
            element_var->is_struct = true;
            element_var->type = TYPE_STRUCT;
            if (!source_struct.struct_type_name.empty()) {
                element_var->struct_type_name = source_struct.struct_type_name;
            }

            // 個別メンバー変数も同期
            for (const auto &member_pair : source_struct.struct_members) {
                std::string member_path =
                    element_name + "." + member_pair.first;
                Variable *member_var = find_variable(member_path);
                if (member_var) {
                    *member_var = member_pair.second;
                    member_var->is_assigned = member_pair.second.is_assigned;
                } else {
                    current_scope().variables[member_path] = member_pair.second;
                    current_scope().variables[member_path].is_assigned =
                        member_pair.second.is_assigned;
                }
            }
        }

        var->is_assigned = true;
        var->is_struct = true;
        var->type = TYPE_STRUCT;
        if (!ret.struct_type_name.empty()) {
            var->struct_type_name = ret.struct_type_name;
        }
        var->array_size = actual_return_size;
        if (!var->array_dimensions.empty()) {
            var->array_dimensions[0] = actual_return_size;
        }
        var->array_values.clear();
        var->array_strings.clear();
    } else if (!ret.int_array_3d.empty()) {
        // 整数配列の場合
        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                  "Processing integer array return value");

        if (var->is_multidimensional) {
            // 多次元配列の場合、multidim_array_valuesに設定
            var->multidim_array_values.clear();

            // 3D配列から適切にデータを復元
            for (const auto &plane : ret.int_array_3d) {
                for (const auto &row : plane) {
                    for (const auto &element : row) {
                        var->multidim_array_values.push_back(element);
                    }
                }
            }

            actual_return_size = var->multidim_array_values.size();
            var->array_size = actual_return_size;
            var->array_values.clear();
        } else {
            // 1次元配列の場合（従来処理）
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
        }

        var->type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
        var->array_strings.clear();
    } else {
        throw std::runtime_error(
            "Return exception contains no valid array data");
    }

    // 静的配列のサイズチェック:
    // 宣言されたサイズと返された配列のサイズが一致するか確認
    // declared_array_sizeが0の場合（パーサーがサイズを設定しなかった場合）は、
    // 動的配列として扱い、返された配列のサイズを使用する
    if (declared_array_size > 0 && declared_array_size != actual_return_size) {
        std::cerr << "[WARN] Array size mismatch: declared "
                  << declared_array_size << " but got " << actual_return_size
                  << ", using returned size" << std::endl;
        // エラーではなく警告として扱い、返された配列のサイズを使用
        var->array_size = actual_return_size;
        if (!var->array_dimensions.empty()) {
            var->array_dimensions[0] = actual_return_size;
        }
    }

    var->is_assigned = true;
    debug_msg(
        DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
        ("Array assignment completed, size: " + std::to_string(var->array_size))
            .c_str());
}

// ========================================================================
// SECTION 7: Type Resolution (~30 lines)
// ========================================================================
// 型解決関連のメソッド - TypeManagerへの薄いラッパー
// これらは完全にTypeManagerに委譲されており、将来的には削除可能
// ========================================================================

std::string Interpreter::resolve_typedef(const std::string &type_name) {
    return type_manager_->resolve_typedef(type_name);
}

TypeInfo Interpreter::resolve_type_alias(TypeInfo base_type,
                                         const std::string &type_name) {
    std::string resolved_type = type_manager_->resolve_typedef(type_name);
    if (resolved_type != type_name) {
        return type_manager_->string_to_type_info(resolved_type);
    }
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

// ArrayManagerへのアクセス
int64_t Interpreter::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    return array_manager_->getMultidimensionalArrayElement(var, indices);
}

// static変数の検索
// ========================================================================
// SECTION 6: Static Variable Management (StaticVariableManagerへ委譲)
// ========================================================================
// Static変数とimpl static変数の管理はStaticVariableManagerに移譲済み
// 以下のメソッドは薄いラッパーとして機能
// ========================================================================

Variable *Interpreter::find_static_variable(const std::string &name) {
    return static_variable_manager_->find_static_variable(name);
}

void Interpreter::create_static_variable(const std::string &name,
                                         const ASTNode *node) {
    static_variable_manager_->create_static_variable(name, node);
}

Variable *Interpreter::find_impl_static_variable(const std::string &name) {
    return static_variable_manager_->find_impl_static_variable(name);
}

void Interpreter::create_impl_static_variable(const std::string &name,
                                              const ASTNode *node) {
    static_variable_manager_->create_impl_static_variable(name, node);
}

void Interpreter::enter_impl_context(const std::string &interface_name,
                                     const std::string &struct_type_name) {
    static_variable_manager_->enter_impl_context(interface_name,
                                                 struct_type_name);
}

void Interpreter::exit_impl_context() {
    static_variable_manager_->exit_impl_context();
}

std::string Interpreter::get_impl_static_namespace() const {
    return static_variable_manager_->get_impl_static_namespace();
}

// ========================================================================
// ========================================================================
// SECTION 2: Struct Operations (Phase 3.4-3.5: 部分的に委譲)
// ========================================================================
// Phase 3.4a: register_struct_definition, validate_struct_recursion_rules
// Phase 3.4b: find_struct_definition, sync_struct_definitions_from_parser
// Phase 3.4c: is_current_impl_context_for, ensure_struct_member_access_allowed
// Phase 3.4d: get_struct_member
// Phase 3.5a: sync_individual_member_from_struct
// その他のメソッドは引き続きInterpreterに実装
// ========================================================================

void Interpreter::register_struct_definition(
    const std::string &struct_name, const StructDefinition &definition) {
    struct_operations_->register_struct_definition(struct_name, definition);
}

void Interpreter::validate_struct_recursion_rules() {
    struct_operations_->validate_struct_recursion_rules();
}

const StructDefinition *
Interpreter::find_struct_definition(const std::string &struct_name) {
    return struct_operations_->find_struct_definition(struct_name);
}

void Interpreter::sync_struct_definitions_from_parser(RecursiveParser *parser) {
    struct_operations_->sync_struct_definitions_from_parser(parser);
}

bool Interpreter::is_current_impl_context_for(
    const std::string &struct_type_name) {
    return struct_operations_->is_current_impl_context_for(struct_type_name);
}

void Interpreter::sync_individual_member_from_struct(
    Variable *struct_var, const std::string &member_name) {
    struct_operations_->sync_individual_member_from_struct(struct_var,
                                                           member_name);
}

void Interpreter::ensure_struct_member_access_allowed(
    const std::string &accessor_name, const std::string &member_name) {
    struct_operations_->ensure_struct_member_access_allowed(accessor_name,
                                                            member_name);
}

// struct変数を作成
void Interpreter::create_struct_variable(const std::string &var_name,
                                         const std::string &struct_type_name) {
    struct_variable_manager_->create_struct_variable(var_name,
                                                      struct_type_name);
}

// structメンバにアクセス
Variable *Interpreter::get_struct_member(const std::string &var_name,
                                         const std::string &member_name) {
    return struct_operations_->get_struct_member(var_name, member_name);
}

// struct literalから値を代入
// 構造体メンバの個別変数を再帰的に作成
void Interpreter::create_struct_member_variables_recursively(
    const std::string &base_path, const std::string &struct_type_name,
    Variable &parent_var) {
    struct_variable_manager_->create_struct_member_variables_recursively(
        base_path, struct_type_name, parent_var);
}

void Interpreter::assign_struct_literal(const std::string &var_name,
                                        const ASTNode *literal_node) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_STRUCT_LITERAL) {
        throw std::runtime_error("Invalid struct literal");
    }

    Variable *var = find_variable(var_name);

    // 変数が見つからない、または構造体でない場合、親構造体のstruct_membersと構造体定義から確認
    if (var && !var->is_struct && var_name.find('.') != std::string::npos) {
        // "o.inner" -> "o" and "inner"
        size_t dot_pos = var_name.rfind('.');
        std::string parent_name = var_name.substr(0, dot_pos);
        std::string member_name = var_name.substr(dot_pos + 1);

        Variable *parent_var = find_variable(parent_name);
        if (parent_var && parent_var->type == TYPE_STRUCT) {
            // 親の構造体定義からメンバー情報を取得
            std::string resolved_parent_type =
                type_manager_->resolve_typedef(parent_var->struct_type_name);
            const StructDefinition *parent_struct_def =
                find_struct_definition(resolved_parent_type);

            if (parent_struct_def) {
                for (const auto &member_def : parent_struct_def->members) {
                    if (member_def.name == member_name &&
                        member_def.type == TYPE_STRUCT) {
                        var->type = TYPE_STRUCT;
                        var->is_struct = true;
                        var->struct_type_name = member_def.type_alias;

                        // メンバの構造体定義を取得して、struct_membersを初期化
                        std::string resolved_member_type =
                            type_manager_->resolve_typedef(
                                member_def.type_alias);
                        const StructDefinition *member_struct_def =
                            find_struct_definition(resolved_member_type);
                        if (member_struct_def) {
                            for (const auto &sub_member :
                                 member_struct_def->members) {
                                Variable sub_member_var;
                                sub_member_var.type = sub_member.type;
                                sub_member_var.is_unsigned =
                                    sub_member.is_unsigned;
                                sub_member_var.is_assigned = false;
                                if (sub_member.type == TYPE_STRUCT) {
                                    sub_member_var.is_struct = true;
                                    sub_member_var.struct_type_name =
                                        sub_member.type_alias;
                                }
                                var->struct_members[sub_member.name] =
                                    sub_member_var;

                                // 個別変数も作成
                                std::string full_sub_member_name =
                                    var_name + "." + sub_member.name;
                                current_scope()
                                    .variables[full_sub_member_name] =
                                    sub_member_var;
                            }
                        }

                        break;
                    }
                }
            }
        }
    }
    auto clamp_unsigned_member = [&](Variable &target, int64_t &value,
                                     const std::string &member_name,
                                     const char *context) {
        if (!target.is_unsigned || value >= 0) {
            return;
        }
        DEBUG_WARN(VARIABLE,
                   "Unsigned struct member %s.%s %s negative value (%lld); "
                   "clamping to 0",
                   var_name.c_str(), member_name.c_str(), context,
                   static_cast<long long>(value));
        value = 0;
    };

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
            std::string resolved_struct_name =
                type_manager_->resolve_typedef(array_var->struct_type_name);
            const StructDefinition *struct_def =
                find_struct_definition(resolved_struct_name);
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
                    member_var.is_unsigned = member_def.is_unsigned;
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
                            element_var.is_unsigned = member_def.is_unsigned;
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

    if (!var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }
    if (!var->is_struct) {
        throw std::runtime_error("Variable is not a struct: " + var_name);
    }

    if (var->is_const && var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, var_name.c_str());
        throw std::runtime_error("Cannot assign to const struct: " + var_name);
    }

    // struct定義を取得してメンバ順序を確認
    // まずtypedefを解決してから構造体定義を検索
    std::string resolved_struct_name =
        type_manager_->resolve_typedef(var->struct_type_name);
    const StructDefinition *struct_def =
        find_struct_definition(resolved_struct_name);
    if (!struct_def) {
        throw std::runtime_error("Struct definition not found: " +
                                 var->struct_type_name);
    }

    // 親変数がconstの場合、すべてのstruct_membersと個別変数をconstにする（再帰的）
    if (var->is_const) {
        std::function<void(const std::string &, Variable &)>
            make_all_members_const;
        make_all_members_const = [&](const std::string &base_path,
                                     Variable &v) {
            for (auto &member_pair : v.struct_members) {
                member_pair.second.is_const = true;

                // 個別変数も更新
                std::string full_path = base_path + "." + member_pair.first;
                Variable *individual_var = find_variable(full_path);
                if (individual_var) {
                    individual_var->is_const = true;
                }

                // 再帰的にネストした構造体メンバー
                if (member_pair.second.is_struct) {
                    make_all_members_const(full_path, member_pair.second);
                }
            }
        };
        make_all_members_const(var_name, *var);
    }

    // 名前付き初期化かどうかをチェック
    bool is_named_init = false;
    if (!literal_node->arguments.empty() &&
        literal_node->arguments[0]->node_type == ASTNodeType::AST_ASSIGN) {
        is_named_init = true;
    }

    if (is_named_init) {
        // 名前付き初期化: {name: "Bob", age: 25}
        debug_msg(DebugMsgId::INTERPRETER_NAMED_STRUCT_LITERAL_INIT,
                  var_name.c_str());

        for (const auto &member_init : literal_node->arguments) {
            if (member_init->node_type != ASTNodeType::AST_ASSIGN) {
                continue;
            }

            std::string member_name = member_init->name;

            debug_msg(DebugMsgId::INTERPRETER_MEMBER_INIT_PROCESSING,
                      member_name.c_str());

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
            Variable &struct_member_var = member_it->second;

            // 構造体定義からメンバの is_const フラグを取得して設定
            // また、親変数がconstの場合もメンバーをconstにする
            const StructMember *member_def =
                struct_def->find_member(member_name);
            if (member_def) {
                struct_member_var.is_const =
                    var->is_const || member_def->is_const;
            }

            // 個別変数にもis_constを設定
            if (member_var) {
                member_var->is_const = struct_member_var.is_const;
            }

            // メンバ値を評価して代入
            if (member_init->right->node_type ==
                ASTNodeType::AST_ARRAY_LITERAL) {
                // 配列メンバーの場合: grades: [85, 92, 78]
                if (!struct_member_var.is_array) {
                    throw std::runtime_error("Member is not an array: " +
                                             member_name);
                }

                // デバッグ: 配列サイズを確認
                if (debug_mode) {
                    debug_print("Array member initialization: %s, "
                                "array_size=%d, elements_count=%zu\n",
                                member_name.c_str(),
                                struct_member_var.array_size,
                                member_init->right->arguments.size());
                }

                // 配列リテラルの各要素を個別変数に代入
                const auto &array_elements = member_init->right->arguments;

                for (size_t i = 0;
                     i < array_elements.size() &&
                     i < static_cast<size_t>(struct_member_var.array_size);
                     i++) {

                    std::string element_name = var_name + "." + member_name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    std::string element_path =
                        member_name + "[" + std::to_string(i) + "]";

                    // float/double配列の処理
                    if (struct_member_var.type == TYPE_FLOAT ||
                        struct_member_var.type == TYPE_DOUBLE) {
                        TypedValue typed_result =
                            expression_evaluator_->evaluate_typed_expression(
                                array_elements[i].get());
                        double float_value = typed_result.as_double();

                        // 個別変数に代入
                        if (element_var) {
                            element_var->float_value = float_value;
                            element_var->is_assigned = true;

                            if (debug_mode) {
                                debug_print("Initialized struct member array "
                                            "element: %s = %f\n",
                                            element_name.c_str(), float_value);
                            }
                        }

                        // struct_membersの配列要素にも代入
                        if (i < struct_member_var.array_float_values.size()) {
                            struct_member_var.array_float_values[i] =
                                float_value;

                            if (debug_mode) {
                                debug_print(
                                    "Updated struct_members array element: "
                                    "%s[%zu] = %f\n",
                                    member_name.c_str(), i, float_value);
                            }
                        }
                    } else {
                        // int/bool/その他の型の配列
                        int64_t value =
                            expression_evaluator_->evaluate_expression(
                                array_elements[i].get());
                        clamp_unsigned_member(struct_member_var, value,
                                              element_path,
                                              "initialized with array literal");

                        // 個別変数に代入
                        if (element_var) {
                            element_var->value = value;
                            element_var->is_assigned = true;

                            if (debug_mode) {
                                debug_print("Initialized struct member array "
                                            "element: %s = %lld\n",
                                            element_name.c_str(),
                                            (long long)value);
                            }
                        }

                        // struct_membersの配列要素にも代入
                        if (i < struct_member_var.array_values.size()) {
                            struct_member_var.array_values[i] = value;

                            if (debug_mode) {
                                debug_print(
                                    "Updated struct_members array element: "
                                    "%s[%zu] = %lld\n",
                                    member_name.c_str(), i, (long long)value);
                            }
                        }
                    }
                }
                struct_member_var.is_assigned = true;
            } else if ((struct_member_var.type == TYPE_STRING ||
                        type_manager_->is_union_type(struct_member_var)) &&
                       member_init->right->node_type ==
                           ASTNodeType::AST_STRING_LITERAL) {
                // struct_membersの値を直接更新（Union型文字列を含む）
                struct_member_var.str_value = member_init->right->str_value;
                struct_member_var.type =
                    TYPE_STRING; // Union型の場合は実際の型をセット
                struct_member_var.is_assigned = true;

                // 直接アクセス変数も更新
                if (member_var) {
                    member_var->str_value = member_init->right->str_value;
                    member_var->type =
                        TYPE_STRING; // Union型の場合は実際の型をセット
                    member_var->is_assigned = true;
                }
            } else if (struct_member_var.type == TYPE_STRUCT &&
                       member_init->right->node_type ==
                           ASTNodeType::AST_VARIABLE) {
                // 構造体メンバに別の構造体変数を代入
                Variable *source_var = find_variable(member_init->right->name);
                if (!source_var || source_var->type != TYPE_STRUCT) {
                    throw std::runtime_error(
                        "Source variable is not a struct: " +
                        member_init->right->name);
                }

                // 構造体全体をコピー
                struct_member_var = *source_var;
                struct_member_var.is_assigned = true;

                // 直接アクセス変数も更新
                if (member_var) {
                    *member_var = *source_var;
                    member_var->is_assigned = true;
                }

                // メンバの個別変数もコピー
                for (const auto &sm : source_var->struct_members) {
                    std::string source_member_path =
                        member_init->right->name + "." + sm.first;
                    std::string target_member_path =
                        full_member_name + "." + sm.first;
                    Variable *target_member_var =
                        find_variable(target_member_path);
                    if (target_member_var) {
                        Variable *source_member_var =
                            find_variable(source_member_path);
                        if (source_member_var) {
                            *target_member_var = *source_member_var;
                        }
                    }
                }
            } else if (struct_member_var.type == TYPE_STRUCT &&
                       member_init->right->node_type ==
                           ASTNodeType::AST_STRUCT_LITERAL) {
                // 構造体メンバにネストした構造体リテラルを代入
                // 再帰的にassign_struct_literalを呼び出す
                debug_msg(DebugMsgId::INTERPRETER_NESTED_STRUCT_LITERAL,
                          full_member_name.c_str());

                // メンバ変数が存在することを確認
                if (!member_var) {
                    throw std::runtime_error(
                        "Struct member variable not found: " +
                        full_member_name);
                }

                // 親変数がconstの場合、このメンバーもconstにする
                if (var->is_const) {
                    struct_member_var.is_const = true;
                    member_var->is_const = true;
                }

                // 再帰的に構造体リテラルを代入
                assign_struct_literal(full_member_name,
                                      member_init->right.get());

                // struct_membersも更新（個別変数から同期）
                struct_member_var = *member_var;
            } else {
                // float/double型の処理
                if (struct_member_var.type == TYPE_FLOAT ||
                    struct_member_var.type == TYPE_DOUBLE) {
                    TypedValue typed_result =
                        expression_evaluator_->evaluate_typed_expression(
                            member_init->right.get());
                    double float_value = typed_result.as_double();

                    // struct_membersの値を直接更新（TYPEに応じて適切なフィールドに）
                    if (struct_member_var.type == TYPE_FLOAT) {
                        struct_member_var.float_value =
                            static_cast<float>(float_value);
                    } else if (struct_member_var.type == TYPE_DOUBLE) {
                        struct_member_var.double_value = float_value;
                    } else if (struct_member_var.type == TYPE_QUAD) {
                        struct_member_var.quad_value =
                            static_cast<long double>(float_value);
                    }
                    struct_member_var.is_assigned = true;

                    // 注意:
                    // operator[]は存在しない要素を作ってしまうので使わない
                    // struct_member_varはすでにリファレンスなので更新済み

                    // 直接アクセス変数も更新
                    if (member_var) {
                        if (member_var->type == TYPE_FLOAT) {
                            member_var->float_value =
                                static_cast<float>(float_value);
                        } else if (member_var->type == TYPE_DOUBLE) {
                            member_var->double_value = float_value;
                        } else if (member_var->type == TYPE_QUAD) {
                            member_var->quad_value =
                                static_cast<long double>(float_value);
                        }
                        member_var->is_assigned = true;
                    }
                } else {
                    // int/bool/その他の型の処理
                    int64_t value = expression_evaluator_->evaluate_expression(
                        member_init->right.get());
                    clamp_unsigned_member(struct_member_var, value, member_name,
                                          "initialized with literal");
                    // struct_membersの値を直接更新
                    struct_member_var.value = value;
                    struct_member_var.is_assigned = true;

                    // 注意:
                    // operator[]は存在しない要素を作ってしまうので使わない
                    // struct_member_varはすでにリファレンスなので更新済み

                    // 直接アクセス変数も更新
                    if (member_var) {
                        member_var->value = value;
                        member_var->is_assigned = true;
                    }
                }
            }
        }
    } else {
        // 位置ベース初期化: {25, "Bob"}
        debug_print("STRUCT_LITERAL_DEBUG: Position-based initialization with "
                    "%zu arguments\n",
                    literal_node->arguments.size());
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
            debug_print("STRUCT_LITERAL_DEBUG: Initializing member %s (index "
                        "%zu, type %d, init_node_type=%d, is_array=%d)\n",
                        member_def.name.c_str(), i, (int)member_def.type,
                        (int)init_value->node_type,
                        it->second.is_array ? 1 : 0);

            // メンバ値を評価して代入
            if (it->second.type == TYPE_STRING &&
                init_value->node_type == ASTNodeType::AST_STRING_LITERAL) {
                debug_print("STRUCT_LITERAL_DEBUG: String literal "
                            "initialization: %s = \"%s\"\n",
                            member_def.name.c_str(),
                            init_value->str_value.c_str());
                it->second.str_value = init_value->str_value;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
                Variable *direct_member_var = find_variable(full_member_name);
                if (direct_member_var) {
                    direct_member_var->str_value = init_value->str_value;
                    direct_member_var->is_assigned = true;
                    debug_print("STRUCT_LITERAL_DEBUG: Updated direct access "
                                "variable: %s = \"%s\"\n",
                                full_member_name.c_str(),
                                init_value->str_value.c_str());
                }
            } else if (it->second.type == TYPE_STRING &&
                       (init_value->node_type == ASTNodeType::AST_VARIABLE ||
                        init_value->node_type == ASTNodeType::AST_IDENTIFIER)) {
                // 文字列変数の場合
                Variable *str_var = find_variable(init_value->name);
                if (!str_var || str_var->type != TYPE_STRING) {
                    throw std::runtime_error(
                        "Expected string variable for string member: " +
                        member_def.name);
                }
                debug_print("STRUCT_LITERAL_DEBUG: String variable "
                            "initialization: %s = \"%s\"\n",
                            member_def.name.c_str(),
                            str_var->str_value.c_str());
                it->second.str_value = str_var->str_value;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
                Variable *direct_member_var = find_variable(full_member_name);
                if (direct_member_var) {
                    direct_member_var->str_value = str_var->str_value;
                    direct_member_var->is_assigned = true;
                    debug_print("STRUCT_LITERAL_DEBUG: Updated direct access "
                                "variable: %s = \"%s\"\n",
                                full_member_name.c_str(),
                                str_var->str_value.c_str());
                }
            } else if (it->second.is_array &&
                       init_value->node_type ==
                           ASTNodeType::AST_ARRAY_LITERAL) {
                debug_print(
                    "STRUCT_LITERAL_DEBUG: Array literal initialization: %s\n",
                    member_def.name.c_str());

                // 配列の要素型を確認（構造体配列かプリミティブ配列か）
                TypeInfo element_type = member_def.array_info.base_type;

                if (element_type == TYPE_STRUCT) {
                    // 構造体配列の場合：各要素は構造体リテラルとして処理
                    debug_print("STRUCT_LITERAL_DEBUG: Struct array "
                                "initialization with %zu elements\n",
                                init_value->arguments.size());

                    for (size_t j = 0;
                         j < init_value->arguments.size() &&
                         j < static_cast<size_t>(it->second.array_size);
                         ++j) {
                        const ASTNode *element_node =
                            init_value->arguments[j].get();

                        if (element_node->node_type ==
                            ASTNodeType::AST_STRUCT_LITERAL) {
                            // 配列要素の完全な名前を構築
                            std::string element_name = var_name + "." +
                                                       member_def.name + "[" +
                                                       std::to_string(j) + "]";

                            debug_print("STRUCT_LITERAL_DEBUG: Assigning "
                                        "struct literal to array element: %s\n",
                                        element_name.c_str());

                            // 要素変数が存在し、構造体として初期化されているか確認
                            Variable *element_var = find_variable(element_name);
                            if (!element_var) {
                                throw std::runtime_error(
                                    "Element variable not found: " +
                                    element_name);
                            }
                            if (!element_var->is_struct) {
                                throw std::runtime_error(
                                    "Element is not a struct: " + element_name);
                            }

                            // 再帰的に構造体リテラルを代入
                            assign_struct_literal(element_name, element_node);
                        } else {
                            throw std::runtime_error(
                                "Expected struct literal for struct array "
                                "element");
                        }
                    }
                    it->second.is_assigned = true;
                } else {
                    // プリミティブ型配列の場合：従来の処理
                    it->second.array_values.clear();

                    // 個別要素変数を一時マップに収集
                    std::map<std::string, Variable> element_vars;

                    for (size_t j = 0; j < init_value->arguments.size(); ++j) {
                        int64_t element_value =
                            expression_evaluator_->evaluate_expression(
                                init_value->arguments[j].get());
                        std::string element_path =
                            member_def.name + "[" + std::to_string(j) + "]";
                        clamp_unsigned_member(it->second, element_value,
                                              element_path,
                                              "initialized with array literal");
                        it->second.array_values.push_back(element_value);

                        // 個別要素変数を作成
                        std::string full_element_name = var_name + "." +
                                                        member_def.name + "[" +
                                                        std::to_string(j) + "]";
                        Variable element_var;
                        element_var.type = element_type;
                        element_var.value = element_value;
                        element_var.is_assigned = true;
                        element_vars[full_element_name] = element_var;

                        debug_print("STRUCT_LITERAL_DEBUG: Array element [%zu] "
                                    "= %lld\n",
                                    j, (long long)element_value);
                    }
                    it->second.array_size = init_value->arguments.size();
                    it->second.is_assigned = true;

                    // 直接アクセス変数も更新
                    std::string full_member_name =
                        var_name + "." + member_def.name;
                    Variable *direct_member_var =
                        find_variable(full_member_name);
                    if (direct_member_var && direct_member_var->is_array) {
                        direct_member_var->array_values =
                            it->second.array_values;
                        direct_member_var->array_size = it->second.array_size;
                        direct_member_var->is_assigned = true;
                        debug_print("STRUCT_LITERAL_DEBUG: Updated direct "
                                    "access array variable: %s\n",
                                    full_member_name.c_str());
                    }

                    // 個別要素変数を一括登録（マップの再ハッシュを防ぐため一度に追加）
                    for (const auto &ev_pair : element_vars) {
                        variable_manager_->current_scope()
                            .variables[ev_pair.first] = ev_pair.second;
                    }
                }
            } else if (it->second.type == TYPE_STRUCT &&
                       init_value->node_type ==
                           ASTNodeType::AST_STRUCT_LITERAL) {
                // ネストされた構造体リテラルの初期化
                debug_print("STRUCT_LITERAL_DEBUG: Nested struct literal "
                            "initialization: %s (type=%s)\n",
                            member_def.name.c_str(),
                            it->second.struct_type_name.c_str());

                std::string nested_var_name = var_name + "." + member_def.name;

                // ネストされた構造体変数を作成（既に変数が存在するはず）
                Variable *nested_var = find_variable(nested_var_name);
                if (!nested_var) {
                    throw std::runtime_error(
                        "Nested struct variable not found: " + nested_var_name);
                }

                // 再帰的に構造体リテラルを代入
                assign_struct_literal(nested_var_name, init_value);

                // 親構造体のstruct_membersに結果を反映
                it->second = *nested_var;
                it->second.is_assigned = true;
            } else {
                // float/double型の処理
                if (it->second.type == TYPE_FLOAT ||
                    it->second.type == TYPE_DOUBLE ||
                    it->second.type == TYPE_QUAD) {
                    TypedValue typed_result =
                        expression_evaluator_->evaluate_typed_expression(
                            init_value);
                    double float_value = typed_result.as_double();
                    debug_print("STRUCT_LITERAL_DEBUG: Float/Double "
                                "initialization: %s = %f\n",
                                member_def.name.c_str(), float_value);

                    // TYPEに応じて適切なフィールドに設定
                    if (it->second.type == TYPE_FLOAT) {
                        it->second.float_value =
                            static_cast<float>(float_value);
                    } else if (it->second.type == TYPE_DOUBLE) {
                        it->second.double_value = float_value;
                    } else if (it->second.type == TYPE_QUAD) {
                        it->second.quad_value =
                            static_cast<long double>(float_value);
                    }
                    it->second.is_assigned = true;

                    // 直接アクセス変数も更新
                    std::string full_member_name =
                        var_name + "." + member_def.name;
                    Variable *direct_member_var =
                        find_variable(full_member_name);
                    if (direct_member_var) {
                        if (direct_member_var->type == TYPE_FLOAT) {
                            direct_member_var->float_value =
                                static_cast<float>(float_value);
                        } else if (direct_member_var->type == TYPE_DOUBLE) {
                            direct_member_var->double_value = float_value;
                        } else if (direct_member_var->type == TYPE_QUAD) {
                            direct_member_var->quad_value =
                                static_cast<long double>(float_value);
                        }
                        direct_member_var->is_assigned = true;
                        debug_print("STRUCT_LITERAL_DEBUG: Updated direct "
                                    "access variable: %s = %f\n",
                                    full_member_name.c_str(), float_value);
                    }
                } else {
                    // int/bool/その他の型の処理
                    int64_t value =
                        expression_evaluator_->evaluate_expression(init_value);
                    debug_print("STRUCT_LITERAL_DEBUG: Numeric initialization: "
                                "%s = %lld\n",
                                member_def.name.c_str(), (long long)value);
                    clamp_unsigned_member(it->second, value, member_def.name,
                                          "initialized with literal");
                    it->second.value = value;

                    // 直接アクセス変数も更新
                    std::string full_member_name =
                        var_name + "." + member_def.name;
                    Variable *direct_member_var =
                        find_variable(full_member_name);
                    if (direct_member_var) {
                        direct_member_var->value = value;
                        direct_member_var->is_assigned = true;
                        debug_print("STRUCT_LITERAL_DEBUG: Updated direct "
                                    "access variable: %s = %lld\n",
                                    full_member_name.c_str(), (long long)value);
                    }
                }
            }
            it->second.is_assigned = true;
        }
    }
    var->is_assigned = true;
}

// structメンバに値を代入（数値）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       int64_t value) {
    std::string target_full_name = var_name + "." + member_name;

    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_var_name = var_name;
    size_t dot_pos = var_name.find('.');
    if (dot_pos != std::string::npos) {
        root_var_name = var_name.substr(0, dot_pos);
        if (debug_mode) {
            debug_print("INT: Nested member assignment: var_name=%s, "
                        "root_var_name=%s\n",
                        var_name.c_str(), root_var_name.c_str());
        }
    }

    // 最上位の親変数がconstかチェック
    if (Variable *root_var = find_variable(root_var_name)) {
        if (debug_mode) {
            debug_print("INT: Root variable %s found, is_const=%d\n",
                        root_var_name.c_str(), root_var->is_const ? 1 : 0);
        }
        if (root_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    if (Variable *struct_var = find_variable(var_name)) {
        if (debug_mode) {
            debug_print("assign_struct_member (int): var=%s, member=%s, "
                        "value=%lld, struct_is_const=%d\n",
                        var_name.c_str(), member_name.c_str(),
                        static_cast<long long>(value),
                        struct_var->is_const ? 1 : 0);
        }
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーの場合は制約をチェック
    bool is_union_member = type_manager_->is_union_type(*member_var);
    if (is_union_member) {
        if (!type_manager_->is_value_allowed_for_union(member_var->type_name,
                                                       value)) {
            throw std::runtime_error("Integer value " + std::to_string(value) +
                                     " is not allowed for union type " +
                                     member_var->type_name +
                                     " in struct member " + member_name);
        }
        // Union型の場合はcurrent_typeを整数型に設定し、文字列値をクリア
        member_var->current_type = TYPE_INT;
        member_var->str_value.clear(); // 文字列値をクリア
    }

    int64_t member_value = value;
    if (member_var->is_unsigned && member_value < 0) {
        DEBUG_WARN(VARIABLE,
                   "Unsigned struct member %s.%s assigned negative value "
                   "(%lld); clamping to 0",
                   var_name.c_str(), member_name.c_str(),
                   static_cast<long long>(member_value));
        member_value = 0;
    }

    member_var->value = member_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        // Union型の場合は制約をチェック
        bool is_union_direct = type_manager_->is_union_type(*direct_var);
        if (is_union_direct) {
            if (!type_manager_->is_value_allowed_for_union(
                    direct_var->type_name, value)) {
                throw std::runtime_error(
                    "Integer value " + std::to_string(value) +
                    " is not allowed for union type " + direct_var->type_name +
                    " in struct member " + member_name);
            }
            // Union型の場合はcurrent_typeを整数型に設定し、文字列値をクリア
            direct_var->current_type = TYPE_INT;
            direct_var->str_value.clear(); // 文字列値をクリア
        }

        int64_t direct_value = member_var->is_unsigned ? member_value : value;
        if (direct_var->is_unsigned && direct_value < 0) {
            DEBUG_WARN(VARIABLE,
                       "Unsigned struct member %s.%s assigned negative value "
                       "(%lld); clamping to 0",
                       var_name.c_str(), member_name.c_str(),
                       static_cast<long long>(direct_value));
            direct_value = 0;
        }
        direct_var->value = direct_value;
        direct_var->is_assigned = true;
    }
}

// structメンバに値を代入（文字列）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       const std::string &value) {
    if (debug_mode) {
        debug_print(
            "assign_struct_member (string): var=%s, member=%s, value='%s'\n",
            var_name.c_str(), member_name.c_str(), value.c_str());
    }

    std::string target_full_name = var_name + "." + member_name;

    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_var_name = var_name;
    size_t dot_pos = var_name.find('.');
    if (dot_pos != std::string::npos) {
        root_var_name = var_name.substr(0, dot_pos);
    }

    // 最上位の親変数がconstかチェック
    if (Variable *root_var = find_variable(root_var_name)) {
        if (root_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーの場合は制約をチェック
    bool is_union_member = type_manager_->is_union_type(*member_var);
    if (is_union_member) {
        if (!type_manager_->is_value_allowed_for_union(member_var->type_name,
                                                       value)) {
            throw std::runtime_error(
                "String value '" + value + "' is not allowed for union type " +
                member_var->type_name + " in struct member " + member_name);
        }
        // Union型の場合はcurrent_typeを文字列型に設定し、数値をクリア
        member_var->current_type = TYPE_STRING;
        member_var->value = 0; // 数値をクリア
    }

    member_var->str_value = value;
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        // Union型の場合は制約をチェック
        bool is_union_direct = type_manager_->is_union_type(*direct_var);
        if (is_union_direct) {
            if (!type_manager_->is_value_allowed_for_union(
                    direct_var->type_name, value)) {
                throw std::runtime_error("String value '" + value +
                                         "' is not allowed for union type " +
                                         direct_var->type_name +
                                         " in struct member " + member_name);
            }
            // Union型の場合はcurrent_typeを文字列型に設定し、数値をクリア
            direct_var->current_type = TYPE_STRING;
            direct_var->value = 0; // 数値をクリア
        }

        direct_var->str_value = value;
        direct_var->is_assigned = true;
        if (debug_mode) {
            debug_print("Updated direct access var %s with value '%s'\n",
                        direct_var_name.c_str(), value.c_str());
        }
    } else {
        if (debug_mode) {
            debug_print("Direct access var %s not found\n",
                        direct_var_name.c_str());
        }
    }
}

// structメンバに値を代入（TypedValue）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       const TypedValue &typed_value) {
    if (debug_mode) {
        debug_print(
            "assign_struct_member (TypedValue): var=%s, member=%s, type=%d\n",
            var_name.c_str(), member_name.c_str(),
            static_cast<int>(typed_value.numeric_type));
    }

    std::string target_full_name = var_name + "." + member_name;

    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_var_name = var_name;
    size_t dot_pos = var_name.find('.');
    if (dot_pos != std::string::npos) {
        root_var_name = var_name.substr(0, dot_pos);
    }

    // 最上位の親変数がconstかチェック
    if (Variable *root_var = find_variable(root_var_name)) {
        if (root_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーかどうかを事前にチェック
    bool is_union_member = (member_var->type == TYPE_UNION);

    // TypedValueの型に応じて値を代入
    if (typed_value.numeric_type == TYPE_FLOAT) {
        member_var->float_value = static_cast<float>(typed_value.double_value);
        if (!is_union_member) {
            member_var->type = TYPE_FLOAT;
        } else {
            member_var->current_type = TYPE_FLOAT;
        }
    } else if (typed_value.numeric_type == TYPE_DOUBLE) {
        member_var->double_value = typed_value.double_value;
        if (!is_union_member) {
            member_var->type = TYPE_DOUBLE;
        } else {
            member_var->current_type = TYPE_DOUBLE;
        }
    } else if (typed_value.numeric_type == TYPE_QUAD) {
        member_var->quad_value = typed_value.quad_value;
        if (!is_union_member) {
            member_var->type = TYPE_QUAD;
        } else {
            member_var->current_type = TYPE_QUAD;
        }
    } else {
        // 整数型の場合
        int64_t assign_value = typed_value.value;
        // unsignedの場合は負の値を0にクランプ
        if (member_var->is_unsigned && assign_value < 0) {
            DEBUG_WARN(VARIABLE,
                       "Unsigned struct member %s.%s assignment with negative "
                       "value (%lld); clamping to 0",
                       var_name.c_str(), member_name.c_str(),
                       static_cast<long long>(assign_value));
            assign_value = 0;
        }
        member_var->value = assign_value;
        if (is_union_member) {
            member_var->current_type =
                (typed_value.numeric_type != TYPE_UNKNOWN)
                    ? typed_value.numeric_type
                    : TYPE_INT;
        }
        // unsignedフラグはメンバ定義から引き継がれるため、ここでは設定しない
    }
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }

        bool is_union_direct = (direct_var->type == TYPE_UNION);

        if (typed_value.numeric_type == TYPE_FLOAT) {
            direct_var->float_value =
                static_cast<float>(typed_value.double_value);
            if (!is_union_direct) {
                direct_var->type = TYPE_FLOAT;
            } else {
                direct_var->current_type = TYPE_FLOAT;
            }
        } else if (typed_value.numeric_type == TYPE_DOUBLE) {
            direct_var->double_value = typed_value.double_value;
            if (!is_union_direct) {
                direct_var->type = TYPE_DOUBLE;
            } else {
                direct_var->current_type = TYPE_DOUBLE;
            }
        } else if (typed_value.numeric_type == TYPE_QUAD) {
            direct_var->quad_value = typed_value.quad_value;
            if (!is_union_direct) {
                direct_var->type = TYPE_QUAD;
            } else {
                direct_var->current_type = TYPE_QUAD;
            }
        } else {
            int64_t assign_value = typed_value.value;
            // unsignedの場合は負の値を0にクランプ
            if (direct_var->is_unsigned && assign_value < 0) {
                DEBUG_WARN(VARIABLE,
                           "Unsigned struct member %s assignment with negative "
                           "value (%lld); clamping to 0",
                           direct_var_name.c_str(),
                           static_cast<long long>(assign_value));
                assign_value = 0;
            }
            direct_var->value = assign_value;
            if (is_union_direct) {
                direct_var->current_type =
                    (typed_value.numeric_type != TYPE_UNKNOWN)
                        ? typed_value.numeric_type
                        : TYPE_INT;
            }
            // unsignedフラグはメンバ定義から引き継がれるため、ここでは設定しない
        }
        direct_var->is_assigned = true;

        if (debug_mode) {
            debug_print("Updated direct access var %s (type=%d)\n",
                        direct_var_name.c_str(),
                        static_cast<int>(direct_var->type));
        }
    }
}

// structメンバに構造体を代入
void Interpreter::assign_struct_member_struct(const std::string &var_name,
                                              const std::string &member_name,
                                              const Variable &struct_value) {
    if (debug_mode) {
        debug_print(
            "assign_struct_member_struct: var=%s, member=%s, struct_type=%s\n",
            var_name.c_str(), member_name.c_str(),
            struct_value.struct_type_name.c_str());
    }

    std::string target_full_name = var_name + "." + member_name;
    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    if (member_var->type != TYPE_STRUCT) {
        throw std::runtime_error("Member is not a struct: " + member_name);
    }

    // 構造体の型が一致するかチェック（型名が空の場合はスキップ）
    if (!member_var->struct_type_name.empty() &&
        !struct_value.struct_type_name.empty() &&
        member_var->struct_type_name != struct_value.struct_type_name) {
        throw std::runtime_error("Struct type mismatch: expected " +
                                 member_var->struct_type_name + ", got " +
                                 struct_value.struct_type_name);
    }

    // 型名が空の場合は代入先の型名を設定
    if (member_var->struct_type_name.empty()) {
        member_var->struct_type_name = struct_value.struct_type_name;
        if (debug_mode) {
            debug_print("Setting member struct type to: %s\n",
                        struct_value.struct_type_name.c_str());
        }
    }

    // 構造体データをコピー
    *member_var = struct_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        *direct_var = struct_value;
        direct_var->is_assigned = true;
        if (debug_mode) {
            debug_print("Updated direct access struct var %s\n",
                        direct_var_name.c_str());
        }
    }

    // 構造体のメンバー変数も個別に更新
    for (const auto &member : struct_value.struct_members) {
        std::string nested_var_name = direct_var_name + "." + member.first;
        Variable *nested_var = find_variable(nested_var_name);
        if (nested_var) {
            *nested_var = member.second;
            nested_var->is_assigned = true;
            if (debug_mode) {
                debug_print("Updated nested member: %s = %lld\n",
                            nested_var_name.c_str(), member.second.value);
            }
        }
    }
}

void Interpreter::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    int64_t value) {
    if (debug_mode) {
        debug_print("assign_struct_member_array_element: var=%s, member=%s, "
                    "index=%d, value=%lld\n",
                    var_name.c_str(), member_name.c_str(), index, value);
    }

    std::string target_full_name = var_name + "." + member_name;
    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    if (member_var->is_const) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
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

    int64_t adjusted_value = value;
    if (member_var->is_unsigned && adjusted_value < 0) {
        DEBUG_WARN(VARIABLE,
                   "Unsigned struct member %s.%s[%d] assigned negative value "
                   "(%lld); clamping to 0",
                   var_name.c_str(), member_name.c_str(), index,
                   static_cast<long long>(adjusted_value));
        adjusted_value = 0;
    }

    member_var->array_values[index] = adjusted_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス配列要素変数も更新
    std::string direct_element_name =
        var_name + "." + member_name + "[" + std::to_string(index) + "]";
    Variable *direct_element = find_variable(direct_element_name);
    if (direct_element) {
        if (direct_element->is_const && direct_element->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_element_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_element_name);
        }
        int64_t direct_value = member_var->is_unsigned ? adjusted_value : value;
        if (direct_element->is_unsigned && direct_value < 0) {
            DEBUG_WARN(VARIABLE,
                       "Unsigned struct member %s.%s[%d] assigned negative "
                       "value (%lld); clamping to 0",
                       var_name.c_str(), member_name.c_str(), index,
                       static_cast<long long>(direct_value));
            direct_value = 0;
        }
        direct_element->value = direct_value;
        direct_element->is_assigned = true;
    }

    if (debug_mode) {
        debug_print("Assignment completed, array_values[%d] = %lld\n", index,
                    member_var->array_values[index]);
    }
}

void Interpreter::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    const std::string &value) {
    if (debug_mode) {
        debug_print("assign_struct_member_array_element (string): var=%s, "
                    "member=%s, index=%d, value=%s\n",
                    var_name.c_str(), member_name.c_str(), index,
                    value.c_str());
    }

    std::string target_full_name = var_name + "." + member_name;
    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    if (member_var->is_const) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    if (debug_mode) {
        debug_print("Before assignment: array_strings.size()=%zu, index=%d\n",
                    member_var->array_strings.size(), index);
    }

    if (index >= static_cast<int>(member_var->array_strings.size())) {
        member_var->array_strings.resize(index + 1);
    }
    member_var->array_strings[index] = value;
    member_var->is_assigned = true;

    // ダイレクトアクセス配列要素変数も更新
    std::string direct_element_name =
        var_name + "." + member_name + "[" + std::to_string(index) + "]";
    Variable *direct_element = find_variable(direct_element_name);
    if (direct_element) {
        if (direct_element->is_const && direct_element->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_element_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_element_name);
        }
        direct_element->str_value = value;
        direct_element->is_assigned = true;
    }

    if (debug_mode) {
        debug_print("After assignment: array_strings[%d]=%s\n", index,
                    member_var->array_strings[index].c_str());
    }
}

int64_t Interpreter::get_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index) {
    return struct_operations_->get_struct_member_array_element(
        var_name, member_name, index);
}

// N次元配列アクセス対応版
int64_t Interpreter::get_struct_member_multidim_array_element(
    const std::string &var_name, const std::string &member_name,
    const std::vector<int64_t> &indices) {
    return struct_operations_->get_struct_member_multidim_array_element(
        var_name, member_name, indices);
}

std::string Interpreter::get_struct_member_array_string_element(
    const std::string &var_name, const std::string &member_name, int index) {
    return struct_operations_->get_struct_member_array_string_element(
        var_name, member_name, index);
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

    if (debug_mode) {
        debug_print("member_var->is_multidimensional: %d, "
                    "array_dimensions.size(): %zu\n",
                    member_var->is_multidimensional,
                    member_var->array_dimensions.size());
        debug_print("Address of member_var: %p\n", (void *)member_var);
    }

    // 共通実装を使用して配列リテラルを解析・代入
    try {
        auto result = common_operations_->parse_array_literal(array_literal);

        if (debug_mode) {
            debug_print("Before assign_array_literal_to_variable: "
                        "array_dimensions.size(): %zu\n",
                        member_var->array_dimensions.size());
        }

        common_operations_->assign_array_literal_to_variable(
            member_var, result, var_name + "." + member_name);

        if (debug_mode) {
            debug_print("After assign_array_literal_to_variable: "
                        "array_dimensions.size(): %zu\n",
                        member_var->array_dimensions.size());
        }

        if (debug_mode) {
            debug_print("result.is_string_array: %d, result.size: %zu\n",
                        result.is_string_array, result.size);
        }

        // 構造体メンバー配列の場合、個別要素変数も更新する必要がある
        if (!result.is_string_array) {
            if (debug_mode) {
                debug_print("Entering individual element update block\n");
                debug_print("member_var->is_multidimensional: %d\n",
                            member_var->is_multidimensional);
                debug_print("member_var->array_dimensions.size(): %zu\n",
                            member_var->array_dimensions.size());
                if (member_var->array_dimensions.size() >= 2) {
                    for (size_t i = 0; i < member_var->array_dimensions.size();
                         i++) {
                        debug_print("dimension[%zu]: %zu\n", i,
                                    member_var->array_dimensions[i]);
                    }
                }
            }

            const auto &assigned_values = member_var->array_values;
            const size_t assigned_count = assigned_values.size();

            // member_varが多次元配列かチェック
            if (member_var->is_multidimensional &&
                member_var->array_dimensions.size() >= 2) {
                // N次元配列の場合 - フラット配列として直接更新
                if (debug_mode) {
                    debug_print(
                        "Assigning N-dimensional array literal to %s.%s\n",
                        var_name.c_str(), member_name.c_str());
                    debug_print(
                        "Total array size: %zu, values to assign: %zu\n",
                        member_var->array_values.size(), assigned_count);
                }

                // フラット配列データを直接更新
                size_t max_elements =
                    std::min(member_var->array_values.size(), assigned_count);

                // multidim_array_values も初期化
                if (member_var->multidim_array_values.size() !=
                    member_var->array_values.size()) {
                    member_var->multidim_array_values.resize(
                        member_var->array_values.size());
                    if (debug_mode) {
                        debug_print(
                            "Resized multidim_array_values to %zu elements\n",
                            member_var->array_values.size());
                    }
                }

                for (size_t i = 0; i < max_elements; i++) {
                    member_var->array_values[i] = assigned_values[i];
                    member_var->multidim_array_values[i] =
                        assigned_values[i]; // multidim_array_values にも設定
                    if (debug_mode) {
                        debug_print("Set flat_index[%zu] = %lld (both "
                                    "array_values and multidim_array_values)\n",
                                    i, assigned_values[i]);
                    }
                }

                // N次元インデックス表示のためのデバッグ（2次元の場合の例）
                if (debug_mode && member_var->array_dimensions.size() == 2) {
                    size_t rows = member_var->array_dimensions[0];
                    size_t cols = member_var->array_dimensions[1];
                    for (size_t r = 0; r < rows && (r * cols) < assigned_count;
                         r++) {
                        for (size_t c = 0;
                             c < cols && (r * cols + c) < assigned_count; c++) {
                            size_t flat_index = r * cols + c;
                            debug_print(
                                "  [%zu][%zu] = %lld (flat_index: %zu)\n", r, c,
                                member_var->array_values[flat_index],
                                flat_index);
                        }
                    }
                }

                // 多次元配列でも個別要素変数を更新
                for (size_t i = 0; i < max_elements; i++) {
                    std::string element_name = var_name + "." + member_name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    if (element_var) {
                        element_var->value = assigned_values[i];
                        element_var->is_assigned = true;
                        if (debug_mode) {
                            debug_print("Updated individual element variable "
                                        "%s = %lld\n",
                                        element_name.c_str(),
                                        assigned_values[i]);
                        }
                    }
                }
            } else {
                // 1次元配列の場合（既存の処理）
                for (size_t i = 0; i < result.size && i < assigned_count; i++) {
                    std::string element_name = var_name + "." + member_name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    if (element_var) {
                        element_var->value = assigned_values[i];
                        element_var->is_assigned = true;
                    }
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
    global_initialization_manager_->initialize_global_variables(node);
}

void Interpreter::sync_enum_definitions_from_parser(RecursiveParser *parser) {
    global_initialization_manager_->sync_enum_definitions_from_parser(parser);
}

void Interpreter::sync_struct_members_from_direct_access(
    const std::string &var_name) {
    struct_sync_manager_->sync_struct_members_from_direct_access(var_name);
}

void Interpreter::sync_direct_access_from_struct_value(
    const std::string &var_name, const Variable &struct_value) {
    struct_sync_manager_->sync_direct_access_from_struct_value(var_name,
                                                                struct_value);
}

// ========================================================================
// SECTION 3: Interface Operations (InterfaceOperationsへ委譲)
// ========================================================================
// Interface定義、Impl実装、メソッドディスパッチはInterfaceOperationsに移譲済み
// 以下のメソッドは薄いラッパーとして機能
// ========================================================================

void Interpreter::register_interface_definition(
    const std::string &interface_name, const InterfaceDefinition &definition) {
    interface_operations_->register_interface_definition(interface_name,
                                                         definition);
}

const InterfaceDefinition *
Interpreter::find_interface_definition(const std::string &interface_name) {
    return interface_operations_->find_interface_definition(interface_name);
}

void Interpreter::register_impl_definition(const ImplDefinition &impl_def) {
    interface_operations_->register_impl_definition(impl_def);
}

const std::vector<ImplDefinition> &Interpreter::get_impl_definitions() const {
    return interface_operations_->get_impl_definitions();
}

const ImplDefinition *
Interpreter::find_impl_for_struct(const std::string &struct_name,
                                  const std::string &interface_name) {
    return interface_operations_->find_impl_for_struct(struct_name,
                                                       interface_name);
}

void Interpreter::create_interface_variable(const std::string &var_name,
                                            const std::string &interface_name) {
    interface_operations_->create_interface_variable(var_name, interface_name);
}

Variable *Interpreter::get_interface_variable(const std::string &var_name) {
    return interface_operations_->get_interface_variable(var_name);
}

std::string Interpreter::get_self_receiver_path() {
    return interface_operations_->get_self_receiver_path();
}

void Interpreter::sync_self_to_receiver(const std::string &receiver_path) {
    interface_operations_->sync_self_to_receiver(receiver_path);
}

// 関数定義の検索
const ASTNode *
Interpreter::find_function_definition(const std::string &func_name) {
    return find_function(func_name);
}

TypedValue Interpreter::evaluate_ternary_typed(const ASTNode *node) {
    return expression_evaluator_->evaluate_ternary_typed(node);
}

void Interpreter::add_temp_variable(const std::string &name,
                                    const Variable &var) {
    interface_operations_->add_temp_variable(name, var);
}

void Interpreter::remove_temp_variable(const std::string &name) {
    interface_operations_->remove_temp_variable(name);
}

void Interpreter::clear_temp_variables() {
    interface_operations_->clear_temp_variables();
}

// 型定義検索メソッド
const ASTNode *
Interpreter::find_union_definition(const std::string &union_name) {
    // ユニオン定義をマップから検索（簡易実装）
    // 実際の実装では union_definitions_ マップを使用
    return nullptr; // 簡易実装：ユニオンサポートは後で実装
}

const ASTNode *
Interpreter::find_typedef_definition(const std::string &typedef_name) {
    // typedef定義をマップから検索（簡易実装）
    // 実際の実装では typedef_definitions_ マップを使用
    return nullptr; // 簡易実装：typedefサポートは後で実装
}
