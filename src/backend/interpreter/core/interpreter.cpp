#include "core/interpreter.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../../../common/type_helpers.h"
#include "../../../common/utf8_utils.h"
#include "../../../frontend/recursive_parser/recursive_parser.h"
#include "core/error_handler.h"
#include "core/type_inference.h"
#include "evaluator/core/evaluator.h"
#include "executors/control_flow_executor.h" // 制御フロー実行サービス
#include "executors/statement_executor.h"    // ヘッダーから移動
#include "executors/statement_list_executor.h" // 文リスト・複合文実行サービス
#include "handlers/control/assertion.h" // アサーション文処理サービス
#include "handlers/control/break_continue.h" // break/continue文処理サービス
#include "handlers/control/return.h"         // return文処理サービス
#include "handlers/declarations/function.h" // 関数宣言処理サービス
#include "handlers/declarations/impl.h"     // impl宣言処理サービス
#include "handlers/declarations/interface.h" // インターフェース宣言処理サービス
#include "handlers/declarations/struct.h"   // 構造体宣言処理サービス
#include "handlers/statements/expression.h" // 式文処理サービス
#include "managers/arrays/manager.h"
#include "managers/common/global_init.h" // グローバル初期化管理サービス
#include "managers/common/operations.h"
#include "managers/structs/assignment.h"       // struct代入管理サービス
#include "managers/structs/member_variables.h" // struct変数管理サービス
#include "managers/structs/operations.h"       // struct操作管理サービス
#include "managers/structs/sync.h"             // struct同期管理サービス
#include "managers/types/enums.h"              // enum管理サービス
#include "managers/types/interfaces.h" // interface/impl管理サービス
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include "managers/variables/static.h" // static変数管理サービス
#include "output/output_manager.h"     // ヘッダーから移動
#include "services/array_processing_service.h" // DRY効率化: 統一配列処理サービス
#include "services/debug_service.h" // DRY効率化: 統一デバッグサービス
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
#include "services/variable_access_service.h" // DRY効率化: 統一変数アクセスサービス

// 分割されたファイル
#include "cleanup.h"
#include "initialization.h"
#include "utility.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// ========================================================================
// Core Functions & Infrastructure
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

// ========================================================================
// コンストラクタ、デストラクタ、スコープ管理、変数検索は
// initialization.cpp, cleanup.cpp, utility.cpp に移動しました
// ========================================================================

// デストラクタ（unique_ptrの完全な型定義が必要なため、ここに残す）
Interpreter::~Interpreter() = default;

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
// Initialization & Global Declarations
// ========================================================================
// グローバル宣言の登録とグローバル変数の初期化
// - register_global_declarations(): グローバル宣言の登録
// - initialize_global_variables(): グローバル変数の初期化
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
        case ASTNodeType::AST_CONSTRUCTOR_DECL:
            node_type_name = "AST_CONSTRUCTOR_DECL";
            break;
        case ASTNodeType::AST_DESTRUCTOR_DECL:
            node_type_name = "AST_DESTRUCTOR_DECL";
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
        // まずimport文を処理（他の宣言よりも先に実行）
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_IMPORT_STMT) {
                register_global_declarations(stmt.get());
            }
        }
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
            if (stmt->node_type == ASTNodeType::AST_ENUM_DECL ||
                stmt->node_type == ASTNodeType::AST_ENUM_TYPEDEF_DECL) {
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
                stmt->node_type != ASTNodeType::AST_ENUM_TYPEDEF_DECL &&
                stmt->node_type != ASTNodeType::AST_TYPEDEF_DECL &&
                stmt->node_type != ASTNodeType::AST_UNION_TYPEDEF_DECL &&
                stmt->node_type != ASTNodeType::AST_INTERFACE_DECL &&
                stmt->node_type != ASTNodeType::AST_IMPL_DECL &&
                stmt->node_type != ASTNodeType::AST_CONSTRUCTOR_DECL &&
                stmt->node_type != ASTNodeType::AST_DESTRUCTOR_DECL &&
                stmt->node_type != ASTNodeType::AST_IMPORT_STMT) {
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
                        array_member.is_default =
                            member_node->is_default_member;
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

                        // デフォルトメンバーの設定
                        if (member_node->is_default_member) {
                            StructMember &added_member =
                                struct_def.members.back();
                            added_member.is_default = true;
                        }

                        debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBER_ADDED,
                                  member_node->name.c_str(),
                                  (int)member_node->type_info);
                    }
                }
            }

            // デフォルトメンバー情報を設定
            for (const auto &member : struct_def.members) {
                if (member.is_default) {
                    struct_def.has_default_member = true;
                    struct_def.default_member_name = member.name;
                    break; // 1つだけのはず
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

    case ASTNodeType::AST_ENUM_TYPEDEF_DECL:
        // typedef enum定義を登録
        {
            debug_msg(DebugMsgId::INTERPRETER_ENUM_REGISTERING,
                      node->name.c_str());
            DEBUG_DEBUG(GENERAL, "Registering typedef enum definition: %s",
                        node->name.c_str());

            // ASTノードからenum定義情報を構築
            // arguments内に各メンバー情報が格納されている
            EnumDefinition enum_def;
            enum_def.name = node->name;

            for (const auto &member_node : node->arguments) {
                if (member_node->node_type == ASTNodeType::AST_VAR_DECL) {
                    enum_def.add_member(member_node->name,
                                        member_node->int_value, true);
                }
            }

            enum_manager_->register_enum(node->name, enum_def);

            if (debug_mode) {
                debug_print("Successfully registered typedef enum: %s with %zu "
                            "members\n",
                            node->name.c_str(), enum_def.members.size());
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
                    if (TypeHelpers::isString(var.type)) {
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

    case ASTNodeType::AST_IMPORT_STMT:
        // import文を処理
        handle_import_statement(node);
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
        // impl宣言を処理（メソッド登録）
        handle_impl_declaration(node);

        // v0.10.0: コンストラクタ/デストラクタを登録
        {
            std::string struct_name = node->struct_name;
            if (debug_mode) {
                debug_print("Processing impl for struct: %s\n",
                            struct_name.c_str());
                debug_print("Number of arguments: %zu\n",
                            node->arguments.size());
            }

            for (size_t i = 0; i < node->arguments.size(); ++i) {
                const auto &arg = node->arguments[i];
                if (!arg) {
                    if (debug_mode) {
                        debug_print(
                            "Warning: null argument %zu in impl block\n", i);
                    }
                    continue;
                }

                if (debug_mode) {
                    debug_print("Processing argument %zu, node_type: %d\n", i,
                                static_cast<int>(arg->node_type));
                }

                if (arg->node_type == ASTNodeType::AST_CONSTRUCTOR_DECL) {
                    struct_constructors_[struct_name].push_back(arg.get());
                    if (debug_mode) {
                        size_t param_count = arg->parameters.size();
                        debug_print(
                            "Registered constructor for %s (params: %zu)\n",
                            struct_name.c_str(), param_count);
                    }
                } else if (arg->node_type == ASTNodeType::AST_DESTRUCTOR_DECL) {
                    struct_destructors_[struct_name] = arg.get();
                    if (debug_mode) {
                        debug_print("Registered destructor for %s\n",
                                    struct_name.c_str());
                    }
                } else {
                    if (debug_mode) {
                        debug_print("Skipping non-constructor/destructor "
                                    "argument (type: %d)\n",
                                    static_cast<int>(arg->node_type));
                    }
                }
            }

            if (debug_mode) {
                debug_print("Finished processing impl for %s\n",
                            struct_name.c_str());
            }
        }
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

    // v0.11.0 Phase 1a: インスタンス化されたジェネリック構造体の型チェック
    // すべてのグローバル宣言(interface/impl含む)が登録された後に実行
    validate_all_interface_bounds();

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
        pop_scope(); // return時もスコープをクリーンアップ
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
    // 無名変数（DISCARD_VARIABLE）v0.10.0新機能
    // 初期化式を評価するが、値は保存しない
    // ========================================================================
    case ASTNodeType::AST_DISCARD_VARIABLE:
        if (node->init_expr) {
            // 初期化式を評価（副作用のため）
            evaluate(node->init_expr.get());
        }
        // 変数は登録しない（破棄される）
        break;

    // ========================================================================
    // 無名関数（LAMBDA_EXPR）v0.10.0新機能
    // 無名関数を内部的に通常の関数として登録
    // ========================================================================
    case ASTNodeType::AST_LAMBDA_EXPR:
        // 無名関数は評価器で処理される（関数ポインタとして扱う）
        // ここでは何もしない（式として評価される）
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

    case ASTNodeType::AST_SWITCH_STMT:
        control_flow_executor_->execute_switch_statement(node);
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
    // defer文（DEFER_STMT）
    // スコープ終了時に実行される文を登録
    // ========================================================================
    case ASTNodeType::AST_DEFER_STMT:
        if (node->body) {
            add_defer(node->body.get());
        }
        break;

    // ========================================================================
    // 関数宣言（FUNC_DECL）
    // 関数定義をグローバルスコープに登録
    // ========================================================================
    case ASTNodeType::AST_FUNC_DECL:
        function_declaration_handler_->handle_function_declaration(node);
        break;

    // ========================================================================
    // import文（IMPORT_STMT）
    // モジュールをロードして定義をインポート
    // ========================================================================
    case ASTNodeType::AST_IMPORT_STMT:
        handle_import_statement(node);
        break;

    // ========================================================================
    // 未対応の文型（式文として評価を試みる）
    // ========================================================================
    default:
        expression_statement_handler_->handle_expression_statement(node);
        break;
    }
}

// ========================================================================
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

// ========================================================================
// Variable Operations (managers/variables/へ完全委譲)
// ========================================================================
// 変数の代入、union変数、関数パラメータ、配列パラメータの処理
// 実装は variable_manager_ に完全委譲されています
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

void Interpreter::handle_import_statement(const ASTNode *node) {
    if (!node || node->import_path.empty()) {
        throw std::runtime_error(
            "Invalid import statement: no module path specified");
    }

    std::string module_path = node->import_path;

    // モジュールが既にロード済みかチェック
    if (loaded_modules.find(module_path) != loaded_modules.end()) {
        return; // 既にロード済み
    }

    // モジュールファイルのパスを解決
    // モジュールパスを相対ファイルパスに変換
    // 例: "stdlib.math.basic" -> "stdlib/math/basic.cb"
    //     "mymodule" -> "mymodule.cb"
    //     "../utils/helper.cb" -> "../utils/helper.cb" (そのまま)

    std::string file_path = module_path;

    // 既に.cbが含まれている場合はそのまま使用
    if (file_path.find(".cb") != std::string::npos) {
        // そのまま使用
    }
    // ドット記法の場合、パスに変換
    else if (module_path.find('.') != std::string::npos &&
             module_path.find('/') == std::string::npos &&
             module_path.find("..") == std::string::npos) {
        // ドット記法: stdlib.math.basic -> stdlib/math/basic.cb
        std::replace(file_path.begin(), file_path.end(), '.', '/');
        file_path += ".cb";
    } else {
        // 拡張子がない場合は追加
        file_path += ".cb";
    }

    // 検索パスの優先順位:
    // 1. 相対パス（カレントディレクトリから）
    // 2. modules/ ディレクトリ
    // 3. プロジェクトルート
    // 4. テストディレクトリ（テスト用）
    std::string resolved_path;
    std::ifstream file;
    std::vector<std::string> search_paths;

    // 相対パス（../ や ./）の場合、そのまま試す
    if (file_path.find("../") == 0 || file_path.find("./") == 0) {
        search_paths.push_back(file_path);
    } else {
        // 通常のモジュール検索パス
        search_paths = {
            file_path,                    // カレントディレクトリ
            "modules/" + file_path,       // modulesディレクトリ
            "../modules/" + file_path,    // 1つ上のmodulesディレクトリ
            "../../modules/" + file_path, // 2つ上のmodulesディレクトリ
            "../" + file_path,            // 1つ上のディレクトリ
            "../../" + file_path,         // 2つ上のディレクトリ
            "tests/cases/import_export/" + file_path, // テストディレクトリ
            "../../tests/cases/import_export/" +
                file_path // tests/integration/から
        };
    }

    for (const auto &path : search_paths) {
        file.open(path);
        if (file.is_open()) {
            resolved_path = path;
            break;
        }
    }

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open module file: " + module_path +
                                 " (searched: " + file_path + ")");
    }

    std::string source_code((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
    file.close();

    // パーサーを使ってモジュールをパース
    RecursiveParser parser(source_code, module_path);
    ASTNode *module_ast = nullptr;
    try {
        module_ast = parser.parse();
    } catch (const std::exception &e) {
        throw std::runtime_error("Failed to parse module '" + module_path +
                                 "': " + e.what());
    }

    if (!module_ast) {
        throw std::runtime_error("Failed to parse module: " + module_path);
    }

    // import項目の指定があるかチェック
    bool has_specific_items = !node->import_items.empty();
    std::unordered_set<std::string> import_items_set(node->import_items.begin(),
                                                     node->import_items.end());

    // モジュールのステートメントを実行
    if (!module_ast->statements.empty()) {
        for (const auto &stmt_ptr : module_ast->statements) {
            const ASTNode *stmt = stmt_ptr.get();
            if (!stmt || !stmt->is_exported)
                continue;

            // 特定の項目のみをインポートする場合、それ以外はスキップ
            if (has_specific_items &&
                import_items_set.find(stmt->name) == import_items_set.end()) {
                continue;
            }

            // エイリアスを取得（あれば）
            std::string imported_name = stmt->name;
            auto alias_it = node->import_aliases.find(stmt->name);
            if (alias_it != node->import_aliases.end()) {
                imported_name = alias_it->second;
            }
            // モジュール全体のエイリアス（as構文）
            auto module_alias_it = node->import_aliases.find("*");
            if (module_alias_it != node->import_aliases.end()) {
                imported_name = module_alias_it->second + "." + stmt->name;
            }

            // export要素を適切なスコープに登録
            switch (stmt->node_type) {
            case ASTNodeType::AST_FUNC_DECL: {
                // 通常の名前で登録
                global_scope.functions[imported_name] = stmt;
                // 修飾名でも登録（module.function()で呼び出せるように）
                std::string qualified_name = module_path + "." + stmt->name;
                global_scope.functions[qualified_name] = stmt;
                if (debug_mode) {
                    std::cerr
                        << "[IMPORT] Function registered: " << imported_name
                        << " (also as " << qualified_name << ")" << std::endl;
                }
            } break;

            case ASTNodeType::AST_STRUCT_DECL:
                // 構造体定義を登録
                {
                    if (debug_mode) {
                        std::cerr
                            << "[IMPORT] Registering struct: " << imported_name
                            << " with " << stmt->arguments.size() << " members"
                            << std::endl;
                    }

                    StructDefinition struct_def(imported_name);
                    // メンバーをstmt->argumentsから登録
                    for (const auto &member : stmt->arguments) {
                        if (debug_mode) {
                            std::cerr
                                << "[IMPORT]   Member node_type: "
                                << static_cast<int>(member->node_type)
                                << " (AST_VAR_DECL="
                                << static_cast<int>(ASTNodeType::AST_VAR_DECL)
                                << ")" << std::endl;
                        }
                        if (member->node_type == ASTNodeType::AST_VAR_DECL) {
                            if (debug_mode) {
                                std::cerr << "[IMPORT]   Adding member: "
                                          << member->name << " type="
                                          << static_cast<int>(member->type_info)
                                          << std::endl;
                            }
                            struct_def.add_member(
                                member->name, member->type_info,
                                member->type_name, member->is_pointer,
                                member->pointer_depth,
                                member->pointer_base_type_name,
                                member->pointer_base_type,
                                member->is_private_member, member->is_reference,
                                member->is_unsigned, member->is_const);
                            if (member->is_default_member) {
                                struct_def.has_default_member = true;
                                struct_def.default_member_name = member->name;
                            }
                        }
                    }

                    if (debug_mode) {
                        std::cerr << "[IMPORT] Struct " << imported_name
                                  << " registered with "
                                  << struct_def.members.size() << " members"
                                  << std::endl;
                    }

                    struct_definitions_[imported_name] = struct_def;
                }
                break;

            case ASTNodeType::AST_INTERFACE_DECL:
                // インターフェース定義を登録（ASTノードを直接保存）
                // interfaceは実行時に処理されるため、ここでは何もしない
                // グローバルスコープにASTを保存しておく必要がある場合は追加
                break;

            case ASTNodeType::AST_IMPL_DECL:
                // impl定義を登録
                {
                    if (debug_mode) {
                        std::cerr << "[IMPORT] Registering impl for struct: "
                                  << stmt->struct_name
                                  << " interface: " << stmt->interface_name
                                  << std::endl;
                    }

                    // コンストラクタ・デストラクタの登録処理
                    std::string struct_name = stmt->struct_name;
                    for (const auto &arg : stmt->arguments) {
                        if (!arg) {
                            continue;
                        }

                        if (arg->node_type ==
                            ASTNodeType::AST_CONSTRUCTOR_DECL) {
                            struct_constructors_[struct_name].push_back(
                                arg.get());

                            // コンストラクタを関数としても登録（Rectangle()で呼び出せるように）
                            // コンストラクタASTノードを直接関数として登録
                            register_function_to_global(struct_name, arg.get());
                            // 修飾名でも登録
                            std::string qualified_name =
                                module_path + "." + struct_name;
                            register_function_to_global(qualified_name,
                                                        arg.get());

                            if (debug_mode) {
                                std::cerr
                                    << "[IMPORT] Registered constructor for "
                                    << struct_name
                                    << " (params: " << arg->parameters.size()
                                    << ")" << std::endl;
                                std::cerr
                                    << "[IMPORT] Also registered as function: "
                                    << struct_name << " and " << qualified_name
                                    << std::endl;
                            }
                        } else if (arg->node_type ==
                                   ASTNodeType::AST_DESTRUCTOR_DECL) {
                            struct_destructors_[struct_name] = arg.get();
                            if (debug_mode) {
                                std::cerr
                                    << "[IMPORT] Registered destructor for "
                                    << struct_name << std::endl;
                            }
                        }
                    }

                    // implメソッドをグローバルに登録
                    handle_impl_declaration(stmt);
                }
                break;

            case ASTNodeType::AST_TYPEDEF_DECL:
            case ASTNodeType::AST_UNION_TYPEDEF_DECL:
            case ASTNodeType::AST_ENUM_TYPEDEF_DECL:
                // typedef定義を登録
                if (type_manager_) {
                    // typedefマップに登録（型名 -> 基底型）
                    typedef_map[imported_name] = stmt->type_name;
                }
                break;

            case ASTNodeType::AST_VAR_DECL: {
                // グローバル変数を登録
                if (stmt->is_const) {
                    // const変数の値を評価して登録
                    if (stmt->init_expr) {
                        TypedValue typed_val =
                            expression_evaluator_->evaluate_typed_expression(
                                stmt->init_expr.get());
                        Variable var;
                        var.type = stmt->type_info;
                        var.is_const = true;
                        var.value = typed_val.value;
                        if (stmt->type_info == TYPE_FLOAT ||
                            stmt->type_info == TYPE_DOUBLE ||
                            stmt->type_info == TYPE_QUAD) {
                            var.float_value = typed_val.double_value;
                        } else if (stmt->type_info == TYPE_STRING) {
                            var.str_value = typed_val.string_value;
                        }
                        global_scope.variables[imported_name] = var;
                        // 修飾名でも登録
                        std::string qualified_name =
                            module_path + "." + stmt->name;
                        global_scope.variables[qualified_name] = var;
                    }
                } else {
                    // 通常のグローバル変数
                    Variable var;
                    var.type = stmt->type_info;
                    if (stmt->init_expr) {
                        TypedValue typed_val =
                            expression_evaluator_->evaluate_typed_expression(
                                stmt->init_expr.get());
                        var.value = typed_val.value;
                        if (stmt->type_info == TYPE_FLOAT ||
                            stmt->type_info == TYPE_DOUBLE ||
                            stmt->type_info == TYPE_QUAD) {
                            var.float_value = typed_val.double_value;
                        } else if (stmt->type_info == TYPE_STRING) {
                            var.str_value = typed_val.string_value;
                        }
                    }
                    global_scope.variables[imported_name] = var;
                    // 修飾名でも登録
                    std::string qualified_name = module_path + "." + stmt->name;
                    global_scope.variables[qualified_name] = var;
                }
            } break;

            case ASTNodeType::AST_ENUM_DECL:
                // enum定義を登録
                if (enum_manager_) {
                    enum_manager_->register_enum(imported_name,
                                                 stmt->enum_definition);
                }
                break;

            default:
                // その他の宣言はスキップ
                break;
            }
        }
    }

    // モジュールをロード済みとしてマーク
    loaded_modules.insert(module_path);

    // ASTノードは削除しない（定義として保持する必要があるため）
    // delete module_ast;
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

// ========================================================================
// Array Operations (managers/arrays/ と array_processing_service_ へ委譲)
// ========================================================================
// 配列要素の代入、多次元配列アクセス、配列リテラル処理
// ほとんどの実装は以下に委譲:
// - array_manager_: 基本的な配列操作
// - array_processing_service_: 統一されたアクセスインターフェース
// - common_operations_: 安全な配列要素代入
//
// 注意: assign_array_element, assign_string_element には
// エラーハンドリングと境界チェックのロジックが含まれているため、
// これらは現時点では interpreter.cpp に残しています
// ========================================================================

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

// print_value と print_formatted は薄いラッパーなので、
// 呼び出し側で直接 output_manager_->method() を使用するよう変更予定
// 現在は後方互換性のために残しています

void Interpreter::print_value(const ASTNode *expr) {
    output_manager_->print_value(expr);
}

void Interpreter::print_formatted(const ASTNode *format_str,
                                  const ASTNode *arg_list) {
    output_manager_->print_formatted(format_str, arg_list);
}

// ========================================================================
// Array Operations (managers/arrays/ と array_processing_service_ へ委譲)
// ========================================================================
// 配列要素の代入、多次元配列アクセス、配列リテラル処理
// ほとんどの実装は以下に委譲:
// - array_manager_: 基本的な配列操作
// - array_processing_service_: 統一されたアクセスインターフェース
// - common_operations_: 安全な配列要素代入
//
// 注意: assign_array_element, assign_string_element には
// エラーハンドリングと境界チェックのロジックが含まれているため、
// これらは現時点では interpreter.cpp に残しています
// ========================================================================

int64_t Interpreter::getMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices) {
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        return array_manager_->getMultidimensionalArrayElement(var, indices);
    }
    return array_processing_service_->getArrayElement(
        var_name, indices,
        ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

void Interpreter::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, int64_t value) {
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        array_manager_->setMultidimensionalArrayElement(var, indices, value);
        return;
    }
    array_processing_service_->setArrayElement(
        var_name, indices, value,
        ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

void Interpreter::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, double value) {
    array_manager_->setMultidimensionalArrayElement(var, indices, value);
}

std::string Interpreter::getMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices) {
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
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
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        array_manager_->setMultidimensionalStringArrayElement(var, indices,
                                                              value);
        return;
    }
    array_processing_service_->setStringArrayElement(
        var_name, indices, value,
        ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
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
        bool inferred_array = false;

        if (var->type >= TYPE_ARRAY_BASE) {
            inferred_array = true;
        } else if (!var->array_dimensions.empty() ||
                   var->array_type_info.is_array()) {
            inferred_array = true;
        } else {
            std::string alias_name;
            if (!var->struct_type_name.empty()) {
                alias_name = var->struct_type_name;
            } else if (!var->type_name.empty()) {
                alias_name = var->type_name;
            }

            if (!alias_name.empty()) {
                std::string resolved =
                    type_manager_->resolve_typedef(alias_name);
                size_t bracket_pos = resolved.find('[');
                if (bracket_pos != std::string::npos) {
                    inferred_array = true;
                    std::string base = resolved.substr(0, bracket_pos);
                    TypeInfo base_type =
                        type_manager_->string_to_type_info(base);
                    if (base_type != TYPE_UNKNOWN) {
                        var->type =
                            static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
                        if (!var->array_type_info.is_array()) {
                            var->array_type_info.base_type = base_type;
                        }
                    }

                    if (var->array_dimensions.empty()) {
                        std::vector<int> inferred_dimensions;
                        std::vector<ArrayDimension> inferred_array_dims;
                        size_t pos = bracket_pos;
                        while (pos < resolved.size() && resolved[pos] == '[') {
                            size_t end = resolved.find(']', pos);
                            if (end == std::string::npos)
                                break;
                            std::string dim_str =
                                resolved.substr(pos + 1, end - pos - 1);
                            if (!dim_str.empty()) {
                                try {
                                    int parsed =
                                        static_cast<int>(std::stoll(dim_str));
                                    inferred_dimensions.push_back(parsed);
                                    inferred_array_dims.emplace_back(parsed,
                                                                     false);
                                } catch (const std::exception &) {
                                    inferred_dimensions.clear();
                                    inferred_array_dims.clear();
                                    break;
                                }
                            } else {
                                inferred_dimensions.push_back(-1);
                                inferred_array_dims.emplace_back(-1, true);
                            }
                            pos = end + 1;
                        }
                        if (!inferred_dimensions.empty()) {
                            var->array_dimensions = inferred_dimensions;
                            var->array_type_info.dimensions =
                                inferred_array_dims;
                        }
                    }
                }
            }
        }

        if (inferred_array) {
            var->is_array = true;
        }
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
// Static Variable Management (StaticVariableManagerへ委譲)
// ========================================================================
// Static変数とimpl static変数の管理はStaticVariableManagerに委譲済み
// 以下のメソッドは薄いラッパーとして機能
// ========================================================================

// Moved to utility.cpp, cleanup.cpp, or initialization.cpp

// ========================================================================
// Struct Operations (managers/structs/へ完全委譲)
// ========================================================================
// これらは薄いラッパーとして機能。実装は以下に分散:
// - struct_operations_: 基本操作、定義管理
// - struct_variable_manager_: 変数作成、メンバー変数管理
// - struct_assignment_manager_: 値の代入
// - struct_sync_manager_: 同期処理
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
    struct_assignment_manager_->assign_struct_literal(var_name, literal_node);
}
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       int64_t value) {
    struct_assignment_manager_->assign_struct_member(var_name, member_name,
                                                     value);
}

// structメンバに値を代入（文字列）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       const std::string &value) {
    struct_assignment_manager_->assign_struct_member(var_name, member_name,
                                                     value);
}

// structメンバに値を代入（TypedValue）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       const TypedValue &typed_value) {
    Variable value_var;
    value_var.type = typed_value.numeric_type;
    value_var.value = typed_value.value;
    value_var.double_value = typed_value.double_value;
    value_var.float_value = static_cast<float>(typed_value.double_value);
    value_var.quad_value = typed_value.quad_value;
    value_var.str_value = typed_value.string_value; // 文字列の値もコピー

    // 型がUNKNOWNで文字列の場合、型をSTRINGに設定
    if (value_var.type == TYPE_UNKNOWN && !value_var.str_value.empty()) {
        value_var.type = TYPE_STRING;
    }

    struct_assignment_manager_->assign_struct_member(var_name, member_name,
                                                     value_var);
}

// structメンバに構造体を代入
void Interpreter::assign_struct_member_struct(const std::string &var_name,
                                              const std::string &member_name,
                                              const Variable &struct_value) {
    struct_assignment_manager_->assign_struct_member_struct(
        var_name, member_name, struct_value);
}

void Interpreter::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    int64_t value) {
    struct_assignment_manager_->assign_struct_member_array_element(
        var_name, member_name, index, value);
}

void Interpreter::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    const std::string &value) {
    Variable value_var;
    value_var.str_value = value;
    value_var.type = TYPE_STRING;
    struct_assignment_manager_->assign_struct_member_array_element(
        var_name, member_name, index, value_var);
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
    struct_assignment_manager_->assign_struct_member_array_literal(
        var_name, member_name, array_literal);
}

// Moved to utility.cpp, cleanup.cpp, or initialization.cpp

// Moved to utility.cpp, cleanup.cpp, or initialization.cpp

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
// v0.10.0: Constructor/Destructor Support
// ========================================================================

void Interpreter::call_default_constructor(
    const std::string &var_name, const std::string &struct_type_name) {
    // デフォルトコンストラクタ（パラメータなし）を探す
    auto it = struct_constructors_.find(struct_type_name);
    if (it == struct_constructors_.end() || it->second.empty()) {
        // コンストラクタが定義されていない場合は何もしない
        if (debug_mode) {
            debug_print("No constructor defined for struct: %s\n",
                        struct_type_name.c_str());
        }
        return;
    }

    // パラメータなしのコンストラクタを探す
    const ASTNode *default_ctor = nullptr;
    for (const auto *ctor : it->second) {
        if (ctor->parameters.empty()) {
            default_ctor = ctor;
            break;
        }
    }

    if (!default_ctor) {
        // デフォルトコンストラクタが見つからない場合は何もしない
        if (debug_mode) {
            debug_print("No default constructor (0 params) for struct: %s\n",
                        struct_type_name.c_str());
        }
        return;
    }

    if (debug_mode) {
        debug_print("Calling default constructor for %s.%s\n",
                    struct_type_name.c_str(), var_name.c_str());
    }

    // まず、スコープをプッシュする前に構造体変数を取得
    Variable *struct_var = find_variable(var_name);

    if (debug_mode) {
        debug_print("DEBUG: find_variable(%s) returned: %p\n", var_name.c_str(),
                    (void *)struct_var);
    }

    // コンストラクタ本体を実行
    push_scope(); // コンストラクタ用の新しいスコープ

    // selfを現在の変数のコピーとして設定
    if (struct_var) {
        // self変数を作成（構造体変数の完全なコピー）
        Variable self_var = *struct_var;
        current_scope().variables["self"] = self_var;

        if (debug_mode) {
            debug_print("Created self variable with %zu struct_members\n",
                        self_var.struct_members.size());
            for (const auto &[name, member] : self_var.struct_members) {
                debug_print("  self.%s (type: %d)\n", name.c_str(),
                            static_cast<int>(member.type));
            }
        }
    }

    // コンストラクタ本体を実行
    if (default_ctor->body) {
        execute_statement(default_ctor->body.get());
    }

    // selfへの変更を元の変数に反映
    Variable *self = find_variable("self");
    if (self && struct_var) {
        // selfのstruct_membersを元の変数にコピー
        struct_var->struct_members = self->struct_members;

        // メンバー変数の直接アクセス用変数も更新
        for (const auto &[member_name, member_value] : self->struct_members) {
            std::string member_path = var_name + "." + member_name;
            Variable *direct_member = find_variable(member_path);
            if (direct_member) {
                *direct_member = member_value;
            }
        }

        // ネストされた構造体メンバーの個別変数も同期
        sync_direct_access_from_struct_value(var_name, *struct_var);
    }

    pop_scope(); // コンストラクタスコープを終了
}

void Interpreter::call_constructor(const std::string &var_name,
                                   const std::string &struct_type_name,
                                   const std::vector<TypedValue> &args) {
    // 引数付きコンストラクタを探す
    auto it = struct_constructors_.find(struct_type_name);
    if (it == struct_constructors_.end() || it->second.empty()) {
        throw std::runtime_error("No constructor defined for struct: " +
                                 struct_type_name);
    }

    // パラメータ数が一致するコンストラクタを探す
    const ASTNode *matching_ctor = nullptr;
    for (const auto *ctor : it->second) {
        if (ctor->parameters.size() == args.size()) {
            // TODO: 型チェックを追加（より厳密なマッチング）
            matching_ctor = ctor;
            break;
        }
    }

    if (!matching_ctor) {
        throw std::runtime_error("No matching constructor found for struct " +
                                 struct_type_name + " with " +
                                 std::to_string(args.size()) + " arguments");
    }

    if (debug_mode) {
        debug_print("Calling constructor for %s.%s with %zu arguments\n",
                    struct_type_name.c_str(), var_name.c_str(), args.size());
    }

    // 構造体変数を取得
    Variable *struct_var = find_variable(var_name);
    if (!struct_var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }

    // コンストラクタ用の新しいスコープを作成
    push_scope();

    // selfを現在の変数のコピーとして設定
    Variable self_var = *struct_var;
    current_scope().variables["self"] = self_var;

    // パラメータを設定
    for (size_t i = 0; i < matching_ctor->parameters.size(); ++i) {
        const auto &param = matching_ctor->parameters[i];
        const auto &arg = args[i];

        Variable param_var;
        param_var.type = arg.type.type_info;
        param_var.value = arg.value;
        param_var.double_value = arg.double_value;
        param_var.str_value = arg.string_value;
        param_var.is_assigned = true;

        current_scope().variables[param->name] = param_var;

        if (debug_mode) {
            debug_print("  Parameter %s = ", param->name.c_str());
            if (arg.type.type_info == TYPE_STRING) {
                debug_print("\"%s\"\n", arg.string_value.c_str());
            } else {
                debug_print("%lld\n", (long long)arg.value);
            }
        }
    }

    // コンストラクタ本体を実行
    if (matching_ctor->body) {
        execute_statement(matching_ctor->body.get());
    }

    // selfへの変更を元の変数に反映
    Variable *self = find_variable("self");
    if (self && struct_var) {
        // selfのstruct_membersを元の変数にコピー
        struct_var->struct_members = self->struct_members;

        // メンバー変数の直接アクセス用変数も更新
        for (const auto &[member_name, member_value] : self->struct_members) {
            std::string member_path = var_name + "." + member_name;
            Variable *direct_member = find_variable(member_path);
            if (direct_member) {
                *direct_member = member_value;
            }
        }

        // ネストされた構造体メンバーの個別変数も同期
        sync_direct_access_from_struct_value(var_name, *struct_var);
    }

    pop_scope(); // コンストラクタスコープを終了
}

void Interpreter::call_copy_constructor(const std::string &var_name,
                                        const std::string &struct_type_name,
                                        const std::string &source_var_name) {
    // コピーコンストラクタを探す（パラメータが1つでconst参照型）
    auto it = struct_constructors_.find(struct_type_name);
    if (it == struct_constructors_.end() || it->second.empty()) {
        if (debug_mode) {
            debug_print("No constructor defined for struct: %s, using "
                        "memberwise copy\n",
                        struct_type_name.c_str());
        }
        // コピーコンストラクタがない場合は、メンバーワイズコピーを実行
        Variable *dest_var = find_variable(var_name);
        Variable *source_var = find_variable(source_var_name);
        if (dest_var && source_var) {
            dest_var->struct_members = source_var->struct_members;
            // 個別変数も更新
            for (const auto &[member_name, member_value] :
                 source_var->struct_members) {
                std::string dest_member_path = var_name + "." + member_name;
                std::string source_member_path =
                    source_var_name + "." + member_name;
                Variable *dest_member = find_variable(dest_member_path);
                Variable *source_member = find_variable(source_member_path);
                if (dest_member && source_member) {
                    *dest_member = *source_member;
                }
            }
        }
        return;
    }

    // const T& 型のパラメータを持つコンストラクタを探す
    const ASTNode *copy_ctor = nullptr;
    for (const auto *ctor : it->second) {
        if (ctor->parameters.size() == 1) {
            const auto &param = ctor->parameters[0];
            // const参照型で、型名が一致するかチェック
            if (param->is_reference && param->is_const) {
                // 型名をチェック（基底型名と一致するか）
                std::string param_type = param->type_name;
                // "const Type&" から "Type" を抽出
                size_t const_pos = param_type.find("const");
                size_t ref_pos = param_type.find("&");
                if (const_pos != std::string::npos) {
                    param_type =
                        param_type.substr(const_pos + 5); // "const " をスキップ
                }
                if (ref_pos != std::string::npos) {
                    param_type = param_type.substr(0, param_type.find("&"));
                }
                // 空白を削除
                param_type.erase(std::remove_if(param_type.begin(),
                                                param_type.end(), ::isspace),
                                 param_type.end());

                if (param_type == struct_type_name) {
                    copy_ctor = ctor;
                    break;
                }
            }
        }
    }

    if (!copy_ctor) {
        // コピーコンストラクタが見つからない場合は、メンバーワイズコピー
        if (debug_mode) {
            debug_print("No copy constructor found for struct: %s, using "
                        "memberwise copy\n",
                        struct_type_name.c_str());
        }
        Variable *dest_var = find_variable(var_name);
        Variable *source_var = find_variable(source_var_name);
        if (dest_var && source_var) {
            dest_var->struct_members = source_var->struct_members;
            // 個別変数も更新
            for (const auto &[member_name, member_value] :
                 source_var->struct_members) {
                std::string dest_member_path = var_name + "." + member_name;
                std::string source_member_path =
                    source_var_name + "." + member_name;
                Variable *dest_member = find_variable(dest_member_path);
                Variable *source_member = find_variable(source_member_path);
                if (dest_member && source_member) {
                    *dest_member = *source_member;
                }
            }
        }
        return;
    }

    if (debug_mode) {
        debug_print("Calling copy constructor for %s from %s\n",
                    var_name.c_str(), source_var_name.c_str());
    }

    // 構造体変数を取得
    Variable *dest_var = find_variable(var_name);
    Variable *source_var = find_variable(source_var_name);
    if (!dest_var || !source_var) {
        throw std::runtime_error("Variable not found in copy constructor");
    }

    // コピーコンストラクタ用の新しいスコープを作成
    push_scope();

    // selfを現在の変数のコピーとして設定
    Variable self_var = *dest_var;
    current_scope().variables["self"] = self_var;

    // パラメータ（ソース変数への参照）を設定
    const auto &param = copy_ctor->parameters[0];
    current_scope().variables[param->name] = *source_var;

    if (debug_mode) {
        debug_print("  Copy parameter %s set to source variable\n",
                    param->name.c_str());
    }

    // コピーコンストラクタ本体を実行
    if (copy_ctor->body) {
        execute_statement(copy_ctor->body.get());
    }

    // selfへの変更を元の変数に反映
    Variable *self = find_variable("self");
    if (self && dest_var) {
        // selfのstruct_membersを元の変数にコピー
        dest_var->struct_members = self->struct_members;

        // メンバー変数の直接アクセス用変数も更新
        for (const auto &[member_name, member_value] : self->struct_members) {
            std::string member_path = var_name + "." + member_name;
            Variable *direct_member = find_variable(member_path);
            if (direct_member) {
                *direct_member = member_value;
            }
        }
    }

    pop_scope(); // コピーコンストラクタスコープを終了
}

void Interpreter::call_destructor(const std::string &var_name,
                                  const std::string &struct_type_name) {
    // デストラクタを探す
    auto it = struct_destructors_.find(struct_type_name);
    if (it == struct_destructors_.end() || !it->second) {
        // デストラクタが定義されていない場合は何もしない
        if (debug_mode) {
            debug_print("No destructor defined for struct: %s\n",
                        struct_type_name.c_str());
        }
        return;
    }

    const ASTNode *destructor = it->second;

    if (debug_mode) {
        debug_print("Calling destructor for %s.%s\n", struct_type_name.c_str(),
                    var_name.c_str());
    }

    // v0.10.0: デストラクタ呼び出し中フラグを設定（無限再帰防止）
    bool prev_flag = is_calling_destructor_;
    is_calling_destructor_ = true;

    // デストラクタ本体を実行
    push_scope(); // デストラクタ用の新しいスコープ

    // selfを現在の変数のコピーとして設定
    Variable *struct_var = find_variable(var_name);
    if (struct_var) {
        Variable self_var = *struct_var;
        current_scope().variables["self"] = self_var;
    }

    // デストラクタ本体を実行
    if (destructor->body) {
        execute_statement(destructor->body.get());
    }

    pop_scope(); // デストラクタスコープを終了

    // フラグを元に戻す
    is_calling_destructor_ = prev_flag;
}

void Interpreter::register_destructor_call(
    const std::string &var_name, const std::string &struct_type_name) {
    if (destructor_stacks_.empty()) {
        if (debug_mode) {
            debug_print("WARNING: destructor_stacks_ is empty when registering "
                        "%s, ignoring\n",
                        var_name.c_str());
        }
        // スタックが空の場合は登録しない（グローバル変数など）
        return;
    }

    // v0.10.0: ネストした構造体の値メンバーのデストラクタを再帰的に登録
    // まず構造体定義を取得
    std::string resolved_type =
        type_manager_->resolve_typedef(struct_type_name);
    const StructDefinition *struct_def = find_struct_definition(resolved_type);

    if (struct_def) {
        // 構造体の各メンバーをチェック
        for (const auto &member : struct_def->members) {
            // 値メンバー（ポインタでも参照でもない）で構造体型の場合
            if (member.type == TYPE_STRUCT && !member.is_pointer &&
                !member.is_reference && !member.type_alias.empty()) {
                // メンバーの完全な変数名
                std::string member_var_name = var_name + "." + member.name;

                // メンバーの型名を解決
                std::string member_type =
                    type_manager_->resolve_typedef(member.type_alias);

                // デストラクタが定義されているかチェック
                // find_impl_for_structの第2引数は空文字列（デストラクタはimpl
                // Structブロックにある）
                const ImplDefinition *impl_def =
                    interface_operations_->find_impl_for_struct(member_type,
                                                                "");

                if (impl_def && impl_def->destructor) {
                    // 再帰的に登録（メンバーの値メンバーも処理される）
                    register_destructor_call(member_var_name, member_type);

                    if (debug_mode) {
                        debug_print("  Registered nested value member for "
                                    "destruction: %s (type: %s)\n",
                                    member_var_name.c_str(),
                                    member_type.c_str());
                    }
                }
            }
        }
    }

    // 最後に自分自身を登録（これにより、メンバーが先に破壊される）
    destructor_stacks_.back().push_back(
        std::make_pair(var_name, struct_type_name));

    if (debug_mode) {
        debug_print(
            "Registered for destruction: %s (type: %s), stack depth: %zu\n",
            var_name.c_str(), struct_type_name.c_str(),
            destructor_stacks_.size());
    }
}

// ========================================================================
// SECTION 3: Interface Operations (InterfaceOperationsへ委譲)
// ========================================================================
// ========================================================================
// Interface/Impl Operations (InterfaceOperationsへ完全委譲)
// ========================================================================
// これらは薄いラッパーとして機能。将来的には呼び出し側で
// 直接 interface_operations_->method() を使用することを検討
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

// Moved to utility.cpp, cleanup.cpp, or initialization.cpp

// Moved to utility.cpp, cleanup.cpp, or initialization.cpp

// Moved to utility.cpp, cleanup.cpp, or initialization.cpp

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

// v0.11.0 Phase 1a: すべてのインスタンス化されたジェネリック構造体の型チェック
void Interpreter::validate_all_interface_bounds() {
    // sync時に遅延させた型チェックをここで実行
    for (const auto &pair : struct_definitions_) {
        const std::string &struct_name = pair.first;
        const StructDefinition &struct_def = pair.second;
        
        // インターフェース境界がある構造体のみチェック
        if (!struct_def.interface_bounds.empty() && 
            !struct_def.type_parameters.empty() &&
            !struct_def.type_parameter_bindings.empty()) {
            
            if (is_debug_mode()) {
                std::cerr << "[VALIDATE_BOUNDS] Checking " << struct_name << std::endl;
            }
            
            // 型引数のリストを構築
            std::vector<std::string> type_arguments;
            for (const auto &param : struct_def.type_parameters) {
                auto it = struct_def.type_parameter_bindings.find(param);
                if (it != struct_def.type_parameter_bindings.end()) {
                    type_arguments.push_back(it->second);
                }
            }
            
            // インターフェース境界を検証
            if (type_arguments.size() == struct_def.type_parameters.size()) {
                interface_operations_->validate_interface_bounds(
                    struct_name, struct_def.type_parameters, type_arguments,
                    struct_def.interface_bounds);
            }
        }
    }
}
