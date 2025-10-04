#include "core/interpreter.h"
#include "core/type_inference.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../../../common/utf8_utils.h"
#include "../../../frontend/recursive_parser/recursive_parser.h"
#include "managers/array_manager.h"
#include "managers/common_operations.h"
#include "services/debug_service.h" // DRY効率化: 統一デバッグサービス
#include "services/array_processing_service.h" // DRY効率化: 統一配列処理サービス
#include "managers/enum_manager.h"     // enum管理サービス
#include "core/error_handler.h"
#include "evaluator/expression_evaluator.h"
#include "executor/statement_executor.h" // ヘッダーから移動
#include "services/expression_service.h"    // DRY効率化: 統一式評価サービス
#include "output/output_manager.h" // ヘッダーから移動
#include "managers/type_manager.h"
#include "services/variable_access_service.h" // DRY効率化: 統一変数アクセスサービス
#include "managers/variable_manager.h"
#include <algorithm>
#include <cctype>
#include <utility>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace {

std::string trim_copy(const std::string &text) {
    auto begin = std::find_if_not(text.begin(), text.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

std::string normalize_struct_type_name(const std::string &raw_name) {
    std::string normalized = trim_copy(raw_name);
    if (normalized.empty()) {
        return normalized;
    }

    if (normalized.rfind("struct ", 0) == 0) {
        normalized = trim_copy(normalized.substr(7));
    }

    while (!normalized.empty() && normalized.back() == '*') {
        normalized.pop_back();
    }
    normalized = trim_copy(normalized);

    auto bracket_pos = normalized.find('[');
    if (bracket_pos != std::string::npos) {
        normalized = trim_copy(normalized.substr(0, bracket_pos));
    }

    return normalized;
}

std::string strip_array_suffix(const std::string &name) {
    auto bracket_pos = name.find('[');
    if (bracket_pos == std::string::npos) {
        return name;
    }
    return name.substr(0, bracket_pos);
}

std::string build_cycle_path(const std::vector<std::string> &cycle) {
    std::ostringstream oss;
    for (size_t i = 0; i < cycle.size(); ++i) {
        if (i > 0) {
            oss << " -> ";
        }
        oss << cycle[i];
    }
    return oss.str();
}

} // namespace

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
    array_processing_service_ = std::make_unique<ArrayProcessingService>(this, common_operations_.get());

    // enum管理サービスを初期化
    enum_manager_ = std::make_unique<EnumManager>();

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
        debug_msg(DebugMsgId::PARSE_REGISTER_GLOBAL_DECL, node_type_name, node->name.c_str());
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
                        array_member.pointer_base_type_name = member_node->pointer_base_type_name;
                        array_member.pointer_base_type = member_node->pointer_base_type;
                        array_member.is_private = member_node->is_private_member;
                        array_member.is_reference = member_node->is_reference;
                        array_member.is_unsigned = member_node->is_unsigned;
                        array_member.is_const = member_node->is_const;
                        struct_def.members.push_back(array_member);

                        debug_msg(DebugMsgId::INTERPRETER_STRUCT_ARRAY_MEMBER_ADDED, 
                                 member_node->name.c_str(),
                                 (int)member_node->type_info,
                                 member_node->array_type_info.dimensions[0].size);

                        // Array dimension details
                        const auto &dim = member_node->array_type_info.dimensions[0];
                        debug_msg(DebugMsgId::INTERPRETER_ARRAY_DIMENSION_INFO,
                                 dim.size, dim.is_dynamic ? 1 : 0,
                                 dim.size_expr.c_str());
                    } else {
                        struct_def.add_member(member_node->name,
                                              member_node->type_info,
                                              member_node->type_name,
                                              member_node->is_pointer,
                                              member_node->pointer_depth,
                                              member_node->pointer_base_type_name,
                                              member_node->pointer_base_type,
                                              member_node->is_private_member,
                                              member_node->is_reference,
                                              member_node->is_unsigned,
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
            debug_msg(DebugMsgId::INTERPRETER_ENUM_REGISTERING, node->name.c_str());
            DEBUG_DEBUG(GENERAL, "Registering enum definition: %s",
                        node->name.c_str());

            // ASTノードからenum定義情報を取得
            const EnumDefinition& enum_def = node->enum_definition;
            
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
            debug_msg(DebugMsgId::INTERPRETER_MULTIPLE_VAR_DECL_START, node->children.size());
            for (const auto &child : node->children) {
                if (child->node_type == ASTNodeType::AST_VAR_DECL) {
                    register_global_declarations(child.get());
                }
            }
        } else if (node->node_type == ASTNodeType::AST_ASSIGN) {
            debug_msg(DebugMsgId::INTERPRETER_GLOBAL_VAR_INIT_START, node->name.c_str());
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
                debug_msg(DebugMsgId::INTERPRETER_ARRAY_LITERAL_INIT, node->name.c_str());
                assign_array_literal(node->name, node->right.get());
            } else {
                // 通常の初期化
                debug_msg(DebugMsgId::INTERPRETER_NORMAL_VAR_INIT, node->name.c_str());
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
                            DEBUG_WARN(VARIABLE,
                                       "Unsigned global variable %s initialized with negative value (%lld); clamping to 0",
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
                TypedValue typed_result = expression_evaluator_->evaluate_typed_expression(node->init_expr.get());
                variable_manager_->assign_variable(node->name, typed_result, TYPE_UNKNOWN, false);
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
        type_manager_->register_union_typedef(node->name, node->union_definition);
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
                    interface_def.add_method(method_node->name, method_node->type_info);
                    debug_msg(DebugMsgId::INTERFACE_METHOD_FOUND, method_node->name.c_str());
                }
            }
            
            register_interface_definition(interface_name, interface_def);
            debug_msg(DebugMsgId::INTERFACE_DECL_COMPLETE, interface_name.c_str());
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
    // NOTE: この実装はArrayManager::processMultidimensionalArrayLiteralと重複する機能
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

void Interpreter::execute_statement(const ASTNode *node) {
    if (!node)
        return;

    // ASTNodeTypeが異常な値でないことを確認  
    int node_type_int = static_cast<int>(node->node_type);
    if (node_type_int < 0 || node_type_int > 100) {
        debug_msg(DebugMsgId::INTERPRETER_EXEC_STMT, 
                  "Abnormal node_type detected in core interpreter: %d, skipping execution", node_type_int);
        if (debug_mode) {
            std::cerr << "[CRITICAL_CORE] Abnormal node_type detected: " << node_type_int << ", skipping" << std::endl;
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
    case ASTNodeType::AST_STMT_LIST:
        debug_msg(DebugMsgId::INTERPRETER_STMT_LIST_EXEC, node->statements.size());
        for (const auto &stmt : node->statements) {
            execute_statement(stmt.get());
        }
        break;

    case ASTNodeType::AST_COMPOUND_STMT:
        debug_msg(DebugMsgId::INTERPRETER_COMPOUND_STMT_EXEC, node->statements.size());
        for (const auto &stmt : node->statements) {
            execute_statement(stmt.get());
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
        debug_msg(DebugMsgId::INTERPRETER_VAR_DECL, node->name.c_str());
        debug_msg(DebugMsgId::INTERPRETER_VAR_DECL_TYPE, (int)node->type_info);
        // 変数宣言をVariableManagerに委譲
        try {
            variable_manager_->process_var_decl_or_assign(node);
            debug_msg(DebugMsgId::INTERPRETER_VAR_DECL_SUCCESS, node->name.c_str());
        } catch (const std::exception& e) {
            error_msg(DebugMsgId::INTERPRETER_VAR_PROCESS_EXCEPTION, e.what());
            throw;
        }
        break;

    case ASTNodeType::AST_ASSIGN:
        debug_msg(DebugMsgId::INTERPRETER_ASSIGNMENT, node->name.c_str());
        // 代入をStatementExecutorに委譲
        statement_executor_->execute(node);
        debug_msg(DebugMsgId::INTERPRETER_ASSIGNMENT_SUCCESS, node->name.c_str());
        break;

    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
        debug_msg(DebugMsgId::INTERPRETER_MULTIPLE_VAR_DECL_EXEC, "");
        statement_executor_->execute_multiple_var_decl(node);
        break;

    case ASTNodeType::AST_ARRAY_DECL:
        debug_msg(DebugMsgId::INTERPRETER_ARRAY_DECL_EXEC, node->name.c_str());
        statement_executor_->execute_array_decl(node);
        break;

    case ASTNodeType::AST_STRUCT_DECL:
    case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
        // struct定義を登録
        {
            debug_msg(DebugMsgId::PARSE_STRUCT_DEF, node->name.c_str());
            std::string struct_name = node->name;
            StructDefinition struct_def(struct_name);

            // ASTノードからstruct定義を復元
            for (const auto &member_node : node->arguments) {
                if (member_node->node_type == ASTNodeType::AST_VAR_DECL) {
                    struct_def.add_member(member_node->name,
                                          member_node->type_info,
                                          member_node->type_name,
                                          member_node->is_pointer,
                                          member_node->pointer_depth,
                                          member_node->pointer_base_type_name,
                                          member_node->pointer_base_type,
                                          member_node->is_private_member);
                    debug_msg(DebugMsgId::PARSE_VAR_DECL, member_node->name.c_str(), member_node->type_name.c_str());
                }
            }

            register_struct_definition(struct_name, struct_def);
            debug_msg(DebugMsgId::PARSE_STRUCT_DEF, struct_name.c_str());
        }
        break;

    case ASTNodeType::AST_INTERFACE_DECL:
        // interface定義を登録
        {
            std::string interface_name = node->name;
            debug_msg(DebugMsgId::INTERFACE_DECL_START, interface_name.c_str());
            
            InterfaceDefinition interface_def(interface_name);

            // ASTノードからinterface定義を復元
            for (const auto &method_node : node->arguments) {
                if (method_node->node_type == ASTNodeType::AST_FUNC_DECL) {
                    InterfaceMember method(method_node->name, method_node->type_info,
                                            method_node->is_unsigned);

                    // パラメータ情報を復元
                    size_t param_index = 0;
                    for (const auto &param_node : method_node->arguments) {
                        if (param_node->node_type == ASTNodeType::AST_PARAM_DECL) {
                            bool param_unsigned = false;
                            if (param_index < method_node->arguments.size() &&
                                param_node) {
                                param_unsigned = param_node->is_unsigned;
                            }
                            method.add_parameter(param_node->name,
                                                 param_node->type_info,
                                                 param_unsigned);
                            ++param_index;
                        }
                    }

                    interface_def.methods.push_back(method);
                    debug_msg(DebugMsgId::INTERFACE_METHOD_FOUND, method_node->name.c_str());
                }
            }

            register_interface_definition(interface_name, interface_def);
            debug_msg(DebugMsgId::INTERFACE_DECL_COMPLETE, interface_name.c_str());
        }
        break;

    case ASTNodeType::AST_IMPL_DECL:
        handle_impl_declaration(node);
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
        debug_msg(DebugMsgId::INTERPRETER_EXEC_STMT, "AST_PRINTLN_STMT");
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
        debug_msg(DebugMsgId::INTERPRETER_IF_STMT_START, "");
        int64_t cond =
            expression_evaluator_->evaluate_expression(node->condition.get());
        debug_msg(DebugMsgId::INTERPRETER_IF_CONDITION_RESULT, cond);
        if (cond) {
            debug_msg(DebugMsgId::INTERPRETER_IF_THEN_EXEC, "");
            execute_statement(node->left.get());
        } else if (node->right) {
            debug_msg(DebugMsgId::INTERPRETER_IF_ELSE_EXEC, "");
            execute_statement(node->right.get());
        }
        debug_msg(DebugMsgId::INTERPRETER_IF_STMT_END, "");
    } break;

    case ASTNodeType::AST_WHILE_STMT:
        debug_msg(DebugMsgId::INTERPRETER_WHILE_STMT_START, "");
        try {
            int iteration = 0;
            while (true) {
                debug_msg(DebugMsgId::INTERPRETER_WHILE_CONDITION_CHECK, iteration);
                int64_t cond = expression_evaluator_->evaluate_expression(
                    node->condition.get());
                debug_msg(DebugMsgId::INTERPRETER_WHILE_CONDITION_RESULT, cond);
                if (!cond)
                    break;
                try {
                    debug_msg(DebugMsgId::INTERPRETER_WHILE_BODY_EXEC, iteration);
                    execute_statement(node->body.get());
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
        break;

    case ASTNodeType::AST_FOR_STMT:
        debug_msg(DebugMsgId::INTERPRETER_FOR_STMT_START, "");
        try {
            if (node->init_expr) {
                debug_msg(DebugMsgId::INTERPRETER_FOR_INIT_EXEC, "");
                execute_statement(node->init_expr.get());
            }
            int iteration = 0;
            while (true) {
                if (node->condition) {
                    debug_msg(DebugMsgId::INTERPRETER_FOR_CONDITION_CHECK, iteration);
                    int64_t cond = expression_evaluator_->evaluate_expression(
                        node->condition.get());
                    debug_msg(DebugMsgId::INTERPRETER_FOR_CONDITION_RESULT, cond);
                    if (!cond)
                        break;
                }
                try {
                    debug_msg(DebugMsgId::INTERPRETER_FOR_BODY_EXEC, iteration);
                    execute_statement(node->body.get());
                } catch (const ContinueException &e) {
                    // continue文でループ継続、update部分だけ実行
                    debug_msg(DebugMsgId::INTERPRETER_FOR_CONTINUE, iteration);
                }
                if (node->update_expr) {
                    debug_msg(DebugMsgId::INTERPRETER_FOR_UPDATE_EXEC, iteration);
                    execute_statement(node->update_expr.get());
                }
                iteration++;
            }
        } catch (const BreakException &e) {
            // break文でループ脱出
        }
        break;

    case ASTNodeType::AST_ASSERT_STMT: {
        debug_msg(DebugMsgId::ASSERT_CHECK_START);
        if (!node->left) {
            error_msg(DebugMsgId::ASSERT_FAILURE, node->location.line, "Missing condition");
            std::exit(1);
        }
        
        // 条件を評価
        int64_t condition = evaluate(node->left.get());
        
        if (condition) {
            debug_msg(DebugMsgId::ASSERT_CONDITION_TRUE);
        } else {
            debug_msg(DebugMsgId::ASSERT_CONDITION_FALSE, node->location.line);
            error_msg(DebugMsgId::ASSERT_FAILURE, node->location.line, "Assertion failed");
            std::cerr << "Assertion failed at line " << node->location.line << std::endl;
            std::exit(1);
        }
        break;
    }

    case ASTNodeType::AST_RETURN_STMT:
        debug_msg(DebugMsgId::INTERPRETER_RETURN_STMT);
        if (node->left) {
            debug_msg(DebugMsgId::INTERPRETER_RETURN_STMT);
            // 配列リテラルの直接返却をサポート
            if (node->left->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                const std::vector<std::unique_ptr<ASTNode>> &elements =
                    node->left->arguments;
                debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY, elements.size());
                // 配列リテラルを処理
                std::vector<int64_t> array_values;
                std::vector<std::string> array_strings;
                bool is_string_array = false;

                // 最初の要素で型を判定
                if (!elements.empty()) {
                    if (elements[0]->node_type ==
                        ASTNodeType::AST_STRING_LITERAL) {
                        is_string_array = true;
                    } else if (elements[0]->node_type ==
                        ASTNodeType::AST_ARRAY_LITERAL) {
                        // 多次元配列リテラルの場合
                        // ネストした配列リテラルの最初の要素をチェック
                        const auto &nested_elements = elements[0]->arguments;
                        if (!nested_elements.empty() &&
                            nested_elements[0]->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            is_string_array = true;
                        }
                    }
                }

                // 多次元配列リテラルの場合の特別処理
                if (!elements.empty() && 
                    elements[0]->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                    
                    if (is_string_array) {
                        // 多次元文字列配列を3D形式に変換
                        std::vector<std::vector<std::vector<std::string>>> str_array_3d;
                        std::vector<std::vector<std::string>> str_array_2d;
                        
                        for (const auto &row_element : elements) {
                            if (row_element->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
                                throw std::runtime_error("Expected nested array literal");
                            }
                            
                            std::vector<std::string> row;
                            for (const auto &cell_element : row_element->arguments) {
                                if (cell_element->node_type != ASTNodeType::AST_STRING_LITERAL) {
                                    throw std::runtime_error("Expected string literal in multidim array");
                                }
                                row.push_back(cell_element->str_value);
                            }
                            str_array_2d.push_back(row);
                        }
                        str_array_3d.push_back(str_array_2d);
                        
                        throw ReturnException(str_array_3d, "string[][]", 
                                              static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING));
                    } else {
                        // 多次元整数配列を3D形式に変換
                        std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
                        std::vector<std::vector<int64_t>> int_array_2d;
                        
                        for (const auto &row_element : elements) {
                            if (row_element->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
                                throw std::runtime_error("Expected nested array literal");
                            }
                            
                            std::vector<int64_t> row;
                            for (const auto &cell_element : row_element->arguments) {
                                int64_t value = expression_evaluator_->evaluate_expression(
                                    cell_element.get());
                                row.push_back(value);
                            }
                            int_array_2d.push_back(row);
                        }
                        int_array_3d.push_back(int_array_2d);
                        
                        throw ReturnException(int_array_3d, "int[][]", TYPE_INT);
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
            } else if (node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
                // AST_IDENTIFIER の処理（特に self）
                if (node->left->name == "self") {
                    Variable *self_var = find_variable("self");
                    if (self_var && self_var->is_struct) {
                        debug_print("RETURN_SELF: Before sync - members=%zu\n", self_var->struct_members.size());
                        for (const auto& member : self_var->struct_members) {
                            debug_print("RETURN_SELF: Before sync member %s = %lld\n", 
                                       member.first.c_str(), member.second.value);
                        }
                        
                        sync_struct_members_from_direct_access("self");
                        
                        debug_print("RETURN_SELF: After sync - members=%zu\n", self_var->struct_members.size());
                        for (const auto& member : self_var->struct_members) {
                            debug_print("RETURN_SELF: After sync member %s = %lld\n", 
                                       member.first.c_str(), member.second.value);
                        }
                        
                        // 構造体のtype情報を正しく設定
                        if (self_var->type != TYPE_INTERFACE) {
                            self_var->type = TYPE_STRUCT;
                        }
                        debug_print("RETURN_SELF: struct_type=%s, type=%d, is_struct=%d, members=%zu\n",
                                   self_var->struct_type_name.c_str(), self_var->type, 
                                   self_var->is_struct, self_var->struct_members.size());
                        throw ReturnException(*self_var);
                    } else {
                        debug_print("RETURN_SELF: self not found or not struct (found=%d, is_struct=%d)\n", 
                                   self_var != nullptr, self_var ? self_var->is_struct : 0);
                    }
                } else {
                    // 他の識別子の場合、変数として扱う
                    Variable *var = find_variable(node->left->name);
                    if (var) {
                        // 現在の関数の戻り値型が参照型かチェック
                        bool return_as_reference = false;
                        if (!current_function_name.empty()) {
                            const ASTNode* func = global_scope.functions[current_function_name];
                            if (func && func->return_type_name.find('&') != std::string::npos) {
                                return_as_reference = true;
                            }
                        }
                        
                        if (return_as_reference) {
                            // 参照として返す
                            // varが既に参照変数の場合、その参照先を返す
                            if (var->is_reference) {
                                Variable* target_var = reinterpret_cast<Variable*>(var->value);
                                throw ReturnException(target_var);
                            } else {
                                throw ReturnException(var);
                            }
                        } else if (var->is_struct) {
                            sync_struct_members_from_direct_access(node->left->name);
                            if (var->type != TYPE_INTERFACE) {
                                var->type = TYPE_STRUCT;  // 構造体のtype情報を正しく設定
                            }
                            throw ReturnException(*var);
                        } else if (!var->interface_name.empty()) {
                            Variable interface_copy = *var;
                            interface_copy.type = TYPE_INTERFACE;
                            throw ReturnException(interface_copy);
                        } else if (var->type == TYPE_STRING) {
                            throw ReturnException(var->str_value);
                        } else {
                            throw ReturnException(var->value);
                        }
                    }
                }
            } else if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                debug_msg(DebugMsgId::INTERPRETER_RETURN_VAR, node->left->name.c_str());
                // 変数の場合、配列変数かチェック
                Variable *var = find_variable(node->left->name);
                
                // 現在の関数の戻り値型が参照型かチェック
                bool return_as_reference = false;
                if (!current_function_name.empty()) {
                    const ASTNode* func = global_scope.functions[current_function_name];
                    if (func && func->return_type_name.find('&') != std::string::npos) {
                        return_as_reference = true;
                    }
                }
                
                if (return_as_reference && var) {
                    // 参照として返す
                    // varが既に参照変数の場合、その参照先を返す
                    if (var->is_reference) {
                        Variable* target_var = reinterpret_cast<Variable*>(var->value);
                        throw ReturnException(target_var);
                    } else {
                        throw ReturnException(var);
                    }
                }
                
                if (var && var->is_struct) {
                    // struct変数を返す前に直接アクセス変数からstruct_membersに同期
                    if (debug_mode) {
                        // 同期前の状態を確認
                        auto scores_it = var->struct_members.find("scores");
                        if (scores_it != var->struct_members.end() && scores_it->second.is_array) {
                            if (scores_it->second.array_values.size() >= 3) {
                                debug_print("RETURN_BEFORE_SYNC: %s.scores.array_values = [%lld, %lld, %lld]\n",
                                           node->left->name.c_str(),
                                           scores_it->second.array_values[0],
                                           scores_it->second.array_values[1],
                                           scores_it->second.array_values[2]);
                            }
                        }
                    }
                    
                    sync_struct_members_from_direct_access(node->left->name);
                    
                    if (debug_mode) {
                        // 同期後の状態を確認
                        auto scores_it = var->struct_members.find("scores");
                        if (scores_it != var->struct_members.end() && scores_it->second.is_array) {
                            if (scores_it->second.array_values.size() >= 3) {
                                debug_print("RETURN_AFTER_SYNC: %s.scores.array_values = [%lld, %lld, %lld]\n",
                                           node->left->name.c_str(),
                                           scores_it->second.array_values[0],
                                           scores_it->second.array_values[1],
                                           scores_it->second.array_values[2]);
                            }
                        }
                    }
                    
                    if (var->type != TYPE_INTERFACE) {
                        if (var->type != TYPE_INTERFACE) {
                            var->type = TYPE_STRUCT;  // 構造体のtype情報を正しく設定
                        }
                    }

                    if (var->is_array) {
                        debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY_VAR);

                        std::vector<std::vector<std::vector<Variable>>> struct_array_3d;
                        std::vector<std::vector<Variable>> struct_array_2d;
                        std::vector<Variable> struct_array_1d;

                        for (int i = 0; i < var->array_size; ++i) {
                            std::string element_name = node->left->name + "[" + std::to_string(i) + "]";
                            Variable *element_var = find_variable(element_name);

                            if (element_var && element_var->is_struct) {
                                Variable struct_element = *element_var;
                                struct_element.is_struct = true;
                                struct_element.type = TYPE_STRUCT;

                                if (struct_element.struct_type_name.empty()) {
                                    struct_element.struct_type_name = element_var->struct_type_name;
                                }

                                struct_array_1d.push_back(struct_element);
                                debug_msg(DebugMsgId::INTERPRETER_ARRAY_ELEMENT, i,
                                          element_var->struct_type_name.c_str());
                            } else {
                                Variable empty_struct;
                                empty_struct.type = TYPE_STRUCT;
                                empty_struct.is_struct = true;
                                empty_struct.struct_type_name = element_name;
                                struct_array_1d.push_back(empty_struct);
                            }
                        }

                        struct_array_2d.push_back(struct_array_1d);
                        struct_array_3d.push_back(struct_array_2d);

                        std::string struct_type_name =
                            var->type_name.empty() ? node->left->name : var->type_name;
                        throw ReturnException(struct_array_3d, struct_type_name);
                    }

                    debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY_VAR);
                    throw ReturnException(*var);
                } else if (var && !var->interface_name.empty()) {
                    Variable interface_copy = *var;
                    interface_copy.type = TYPE_INTERFACE;
                    throw ReturnException(interface_copy);
                } else if (var && var->is_array) {
                    debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY_VAR);
                    if (var->is_multidimensional) {
                        debug_msg(DebugMsgId::INTERPRETER_MULTIDIM_ARRAY_SIZE, var->multidim_array_values.size());
                    } else {
                        debug_msg(DebugMsgId::INTERPRETER_REGULAR_ARRAY_SIZE, var->array_values.size());
                    }

                    // 多次元配列の場合
                    if (var->is_multidimensional) {
                        debug_msg(DebugMsgId::INTERPRETER_MULTIDIM_PROCESSING);

                        // 文字列配列の場合
                        if (var->type == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING) ||
                            var->type == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_CHAR)) {
                            
                            // 多次元文字列配列の値を正しい次元構造で3D配列に変換
                            std::vector<std::vector<std::vector<std::string>>> str_array_3d;
                            
                            if (var->array_dimensions.size() == 2) {
                                // 2次元配列の場合
                                int rows = var->array_dimensions[0];
                                int cols = var->array_dimensions[1];
                                
                                std::vector<std::vector<std::string>> str_array_2d;
                                for (int i = 0; i < rows; i++) {
                                    std::vector<std::string> row;
                                    for (int j = 0; j < cols; j++) {
                                        int flat_index = i * cols + j;
                                        if (flat_index < var->multidim_array_strings.size()) {
                                            row.push_back(var->multidim_array_strings[flat_index]);
                                        } else {
                                            row.push_back("");
                                        }
                                    }
                                    str_array_2d.push_back(row);
                                }
                                str_array_3d.push_back(str_array_2d);
                            } else {
                                // その他の多次元配列（従来の方法で1次元として処理）
                                std::vector<std::vector<std::string>> str_array_2d;
                                std::vector<std::string> str_array_1d;

                                for (size_t i = 0; i < var->multidim_array_strings.size(); ++i) {
                                    str_array_1d.push_back(var->multidim_array_strings[i]);
                                }
                                str_array_2d.push_back(str_array_1d);
                                str_array_3d.push_back(str_array_2d);
                            }
                            
                            throw ReturnException(str_array_3d, node->left->name, var->type);
                        }

                        // 多次元配列の値を正しい次元構造で3D配列に変換
                        // まず型を確認
                        TypeInfo type_info = var->type;
                        TypeInfo base_type = (type_info >= TYPE_ARRAY_BASE) 
                                            ? static_cast<TypeInfo>(type_info - TYPE_ARRAY_BASE) 
                                            : type_info;
                        
                        // float/double/quad配列の場合
                        if (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE || base_type == TYPE_QUAD) {
                            std::vector<std::vector<std::vector<double>>> double_array_3d;
                            
                            if (var->array_dimensions.size() == 2) {
                                // 2次元float/double配列の場合
                                int rows = var->array_dimensions[0];
                                int cols = var->array_dimensions[1];
                                
                                std::vector<std::vector<double>> double_array_2d;
                                for (int i = 0; i < rows; i++) {
                                    std::vector<double> row;
                                    for (int j = 0; j < cols; j++) {
                                        int flat_index = i * cols + j;
                                        if (base_type == TYPE_FLOAT && flat_index < static_cast<int>(var->multidim_array_float_values.size())) {
                                            row.push_back(static_cast<double>(var->multidim_array_float_values[flat_index]));
                                        } else if (base_type == TYPE_DOUBLE && flat_index < static_cast<int>(var->multidim_array_double_values.size())) {
                                            row.push_back(var->multidim_array_double_values[flat_index]);
                                        } else if (base_type == TYPE_QUAD && flat_index < static_cast<int>(var->multidim_array_quad_values.size())) {
                                            row.push_back(static_cast<double>(var->multidim_array_quad_values[flat_index]));
                                        } else {
                                            row.push_back(0.0);
                                        }
                                    }
                                    double_array_2d.push_back(row);
                                }
                                double_array_3d.push_back(double_array_2d);
                            } else {
                                // その他の多次元float/double配列（従来の方法で1次元として処理）
                                std::vector<std::vector<double>> double_array_2d;
                                std::vector<double> double_array_1d;

                                if (base_type == TYPE_FLOAT) {
                                    for (size_t i = 0; i < var->multidim_array_float_values.size(); ++i) {
                                        double_array_1d.push_back(static_cast<double>(var->multidim_array_float_values[i]));
                                    }
                                } else if (base_type == TYPE_DOUBLE) {
                                    for (size_t i = 0; i < var->multidim_array_double_values.size(); ++i) {
                                        double_array_1d.push_back(var->multidim_array_double_values[i]);
                                    }
                                } else { // TYPE_QUAD
                                    for (size_t i = 0; i < var->multidim_array_quad_values.size(); ++i) {
                                        double_array_1d.push_back(static_cast<double>(var->multidim_array_quad_values[i]));
                                    }
                                }
                                double_array_2d.push_back(double_array_1d);
                                double_array_3d.push_back(double_array_2d);
                            }

                                            throw ReturnException(double_array_3d, node->left->name, base_type);
                        }
                        
                        // 整数型配列の処理
                        std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
                        
                        if (var->array_dimensions.size() == 2) {
                            // 2次元配列の場合
                            int rows = var->array_dimensions[0];
                            int cols = var->array_dimensions[1];
                            
                            std::vector<std::vector<int64_t>> int_array_2d;
                            for (int i = 0; i < rows; i++) {
                                std::vector<int64_t> row;
                                for (int j = 0; j < cols; j++) {
                                    int flat_index = i * cols + j;
                                    if (flat_index < var->multidim_array_values.size()) {
                                        row.push_back(var->multidim_array_values[flat_index]);
                                        debug_msg(DebugMsgId::INTERPRETER_MULTIDIM_ELEMENT,
                                                  flat_index, 
                                                  (long long)var->multidim_array_values[flat_index]);
                                    } else {
                                        row.push_back(0);
                                    }
                                }
                                int_array_2d.push_back(row);
                            }
                            int_array_3d.push_back(int_array_2d);
                        } else {
                            // その他の多次元配列（従来の方法で1次元として処理）
                            std::vector<std::vector<int64_t>> int_array_2d;
                            std::vector<int64_t> int_array_1d;

                            for (size_t i = 0; i < var->multidim_array_values.size(); ++i) {
                                int_array_1d.push_back(var->multidim_array_values[i]);
                                debug_msg(DebugMsgId::INTERPRETER_MULTIDIM_ELEMENT,
                                          (int)i, 
                                          (long long)var->multidim_array_values[i]);
                            }
                            int_array_2d.push_back(int_array_1d);
                            int_array_3d.push_back(int_array_2d);
                        }

                        throw ReturnException(int_array_3d, node->left->name, var->type);
                    }

                    // 1次元配列の既存処理
                    TypeInfo type_info = var->type;
                    TypeInfo base_type = (type_info >= TYPE_ARRAY_BASE) 
                                        ? static_cast<TypeInfo>(type_info - TYPE_ARRAY_BASE) 
                                        : type_info;
                    std::string type_name =
                        node->left->name; // 配列名を仮の型名として使用

                    // float/double/quad配列の処理
                    if (base_type == TYPE_FLOAT || base_type == TYPE_DOUBLE || base_type == TYPE_QUAD) {
                        std::vector<std::vector<std::vector<double>>> double_array_3d;
                        
                        if (var->is_multidimensional && var->array_dimensions.size() == 2) {
                            // 2次元float/double配列
                            int rows = var->array_dimensions[0];
                            int cols = var->array_dimensions[1];
                            
                            std::vector<std::vector<double>> double_array_2d;
                            for (int i = 0; i < rows; i++) {
                                std::vector<double> row;
                                for (int j = 0; j < cols; j++) {
                                    int flat_index = i * cols + j;
                                    if (base_type == TYPE_FLOAT && flat_index < static_cast<int>(var->multidim_array_float_values.size())) {
                                        row.push_back(static_cast<double>(var->multidim_array_float_values[flat_index]));
                                    } else if (base_type == TYPE_DOUBLE && flat_index < static_cast<int>(var->multidim_array_double_values.size())) {
                                        row.push_back(var->multidim_array_double_values[flat_index]);
                                    } else if (base_type == TYPE_QUAD && flat_index < static_cast<int>(var->multidim_array_quad_values.size())) {
                                        row.push_back(static_cast<double>(var->multidim_array_quad_values[flat_index]));
                                    } else {
                                        row.push_back(0.0);
                                    }
                                }
                                double_array_2d.push_back(row);
                            }
                            double_array_3d.push_back(double_array_2d);
                        } else {
                            // 1次元float/double配列
                            std::vector<std::vector<double>> double_array_2d;
                            std::vector<double> double_array_1d;
                            
                            if (base_type == TYPE_FLOAT) {
                                for (size_t i = 0; i < var->array_float_values.size(); ++i) {
                                    double_array_1d.push_back(static_cast<double>(var->array_float_values[i]));
                                }
                            } else if (base_type == TYPE_DOUBLE) {
                                for (size_t i = 0; i < var->array_double_values.size(); ++i) {
                                    double_array_1d.push_back(var->array_double_values[i]);
                                }
                            } else { // TYPE_QUAD
                                for (size_t i = 0; i < var->array_quad_values.size(); ++i) {
                                    double_array_1d.push_back(static_cast<double>(var->array_quad_values[i]));
                                }
                            }
                            double_array_2d.push_back(double_array_1d);
                            double_array_3d.push_back(double_array_2d);
                        }
                        
                        throw ReturnException(double_array_3d, type_name, base_type);
                    }
                    else if (type_info ==
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
                            debug_msg(DebugMsgId::INTERPRETER_ARRAY_ELEMENT,
                                      (int)i, 
                                      (long long)var->array_values[i]);
                        }
                        int_array_2d.push_back(int_array_1d);
                        int_array_3d.push_back(int_array_2d);

                        debug_msg(DebugMsgId::INTERPRETER_RETURN_EXCEPTION);
                        // 配列戻り値をReturnException として投げる
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
                        
                        if (var->is_multidimensional && var->array_dimensions.size() == 2) {
                            // 2次元文字列配列の場合
                            int rows = var->array_dimensions[0];
                            int cols = var->array_dimensions[1];
                            
                            std::vector<std::vector<std::string>> str_array_2d;
                            for (int i = 0; i < rows; i++) {
                                std::vector<std::string> row;
                                for (int j = 0; j < cols; j++) {
                                    int flat_index = i * cols + j;
                                    if (flat_index < var->multidim_array_strings.size()) {
                                        row.push_back(var->multidim_array_strings[flat_index]);
                                    } else {
                                        row.push_back("");
                                    }
                                }
                                str_array_2d.push_back(row);
                            }
                            str_array_3d.push_back(str_array_2d);
                        } else {
                            // 1次元文字列配列の場合（従来処理）
                            std::vector<std::vector<std::string>> str_array_2d;
                            std::vector<std::string> str_array_1d;

                            if (var->is_multidimensional) {
                                // 多次元だが2次元以外の場合
                                for (size_t i = 0; i < var->multidim_array_strings.size(); ++i) {
                                    str_array_1d.push_back(var->multidim_array_strings[i]);
                                }
                            } else {
                                // 1次元の場合
                                for (size_t i = 0; i < var->array_strings.size(); ++i) {
                                    str_array_1d.push_back(var->array_strings[i]);
                                }
                            }
                            str_array_2d.push_back(str_array_1d);
                            str_array_3d.push_back(str_array_2d);
                        }

                        throw ReturnException(str_array_3d, type_name, type_info);
                    } else if (type_info == TYPE_STRUCT && var->is_array) {
                        // 構造体配列を戻り値として処理
                        std::vector<std::vector<std::vector<Variable>>> struct_array_3d;
                        std::vector<std::vector<Variable>> struct_array_2d;
                        std::vector<Variable> struct_array_1d;
                        
                        // 構造体配列の各要素を収集
                        for (int i = 0; i < var->array_size; ++i) {
                            std::string element_name = node->left->name + "[" + std::to_string(i) + "]";
                            Variable* element_var = find_variable(element_name);
                            
                            if (element_var && element_var->is_struct) {
                                // 構造体要素の深いコピーを作成
                                Variable struct_element;
                                struct_element.type = element_var->type;
                                struct_element.is_struct = true;
                                struct_element.struct_type_name = element_var->struct_type_name;
                                
                                // 構造体メンバーの深いコピー
                                for (const auto& member_pair : element_var->struct_members) {
                                    struct_element.struct_members[member_pair.first] = member_pair.second;
                                }
                                
                                struct_array_1d.push_back(struct_element);
                                
                                debug_msg(DebugMsgId::INTERPRETER_ARRAY_ELEMENT, i, 
                                         element_var->struct_type_name.c_str());
                            } else {
                                // 空の構造体要素を作成
                                Variable empty_struct;
                                empty_struct.type = TYPE_STRUCT;
                                empty_struct.is_struct = true;
                                empty_struct.struct_type_name = element_name;
                                struct_array_1d.push_back(empty_struct);
                            }
                        }
                        
                        struct_array_2d.push_back(struct_array_1d);
                        struct_array_3d.push_back(struct_array_2d);
                        
                        debug_msg(DebugMsgId::INTERPRETER_RETURN_EXCEPTION);
                        throw ReturnException(struct_array_3d, var->type_name.empty() ? type_name : var->type_name);
                    }
                } else {
                    // debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND);
                }

                // 非配列変数または通常の処理
                if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable *var = find_variable(node->left->name);
                    
                    // デバッグ出力を削除
                    if (var && var->is_struct) {
                        // 構造体変数を返す場合、直接アクセス変数からstruct_membersに同期
                        sync_struct_members_from_direct_access(node->left->name);
                        throw ReturnException(*var);
                    } else if (var && (var->type == TYPE_STRING || 
                                      (var->is_assigned && !var->str_value.empty()))) {
                        // 文字列変数を返す（typedef型を含む）
                        // 文字列変数を返す（typedef型を含む）
                        throw ReturnException(var->str_value);
                    } else if (var) {
                        // 数値変数を返す（float/double/quad対応）
                        TypedValue typed_result =
                            expression_evaluator_->evaluate_typed_expression(
                                node->left.get());
                        if (typed_result.numeric_type == TYPE_FLOAT) {
                            throw ReturnException(typed_result.double_value, TYPE_FLOAT);
                        } else if (typed_result.numeric_type == TYPE_DOUBLE) {
                            throw ReturnException(typed_result.double_value, TYPE_DOUBLE);
                        } else if (typed_result.numeric_type == TYPE_QUAD) {
                            throw ReturnException(typed_result.quad_value, TYPE_QUAD);
                        } else {
                            throw ReturnException(typed_result.value, typed_result.numeric_type);
                        }
                    } else {
                        // 変数が見つからない場合、式評価で処理を試す
                        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND, node->left->name.c_str());
                        TypedValue typed_result =
                            expression_evaluator_->evaluate_typed_expression(
                                node->left.get());
                        if (typed_result.numeric_type == TYPE_FLOAT) {
                            throw ReturnException(typed_result.double_value, TYPE_FLOAT);
                        } else if (typed_result.numeric_type == TYPE_DOUBLE) {
                            throw ReturnException(typed_result.double_value, TYPE_DOUBLE);
                        } else if (typed_result.numeric_type == TYPE_QUAD) {
                            throw ReturnException(typed_result.quad_value, TYPE_QUAD);
                        } else {
                            throw ReturnException(typed_result.value, typed_result.numeric_type);
                        }
                    }
                } else {
                    // その他の式（メンバーアクセスなど）を処理                    
                    if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                        // メンバーアクセス（self.name など）の場合
                        std::string struct_name = node->left->left->name;
                        std::string member_name = node->left->name;
                        
                        try {
                            Variable *member_var = get_struct_member(struct_name, member_name);
                            if (member_var && member_var->type == TYPE_STRING) {
                                // 文字列メンバーを返す
                                throw ReturnException(member_var->str_value);
                            } else if (member_var) {
                                // 数値メンバーを返す（float/double/quad対応）
                                if (member_var->type == TYPE_FLOAT) {
                                    throw ReturnException(static_cast<double>(member_var->float_value), TYPE_FLOAT);
                                } else if (member_var->type == TYPE_DOUBLE) {
                                    throw ReturnException(member_var->double_value, TYPE_DOUBLE);
                                } else if (member_var->type == TYPE_QUAD) {
                                    throw ReturnException(member_var->quad_value, TYPE_QUAD);
                                } else {
                                    throw ReturnException(member_var->value, member_var->type);
                                }
                            }
                        } catch (const std::exception& e) {
                            // メンバーアクセスエラーの場合、式評価にフォールバック
                        }
                    }
                    
                    // selfの特別処理
                    if (node->left->node_type == ASTNodeType::AST_VARIABLE && node->left->name == "self") {
                        debug_print("RETURN_SELF_DEBUG: Processing return self in expression context\n");
                        Variable *self_var = find_variable("self");
                        if (self_var && self_var->is_struct) {
                            sync_struct_members_from_direct_access("self");
                            throw ReturnException(*self_var);
                        }
                    }
                    
                    // デフォルトの式評価（型推論対応）
                    TypedValue typed_result = expression_evaluator_->evaluate_typed_expression(
                        node->left.get());
                    
                    std::cerr << "DEBUG: typed_result.is_struct_result = " << typed_result.is_struct_result << std::endl;
                    
                    if (typed_result.is_struct_result) {
                        std::cerr << "DEBUG: Struct result detected, re-evaluating expression" << std::endl;
                        // 構造体の場合、再度評価してReturnExceptionを取得
                        try {
                            expression_evaluator_->evaluate_expression(node->left.get());
                            throw std::runtime_error("Struct evaluation did not throw ReturnException");
                        } catch (const ReturnException& ret_ex) {
                            std::cerr << "DEBUG: Successfully caught ReturnException for struct" << std::endl;
                            throw ret_ex; // そのまま再投げ
                        }
                    } else if (typed_result.is_string()) {
                        throw ReturnException(typed_result.string_value);
                    } else {
                        // 数値の場合、型情報を保持してReturnExceptionを作成
                        if (typed_result.numeric_type == TYPE_FLOAT) {
                            throw ReturnException(typed_result.double_value, TYPE_FLOAT);
                        } else if (typed_result.numeric_type == TYPE_DOUBLE) {
                            throw ReturnException(typed_result.double_value, TYPE_DOUBLE);
                        } else if (typed_result.numeric_type == TYPE_QUAD) {
                            throw ReturnException(typed_result.quad_value, TYPE_QUAD);
                        } else {
                            throw ReturnException(typed_result.value, typed_result.numeric_type);
                        }
                    }
                }
            } else {
                if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable *var = find_variable(node->left->name);
                    if (var && var->is_struct) {
                        // 構造体変数を返す場合、直接アクセス変数からstruct_membersに同期
                        sync_struct_members_from_direct_access(node->left->name);
                        throw ReturnException(*var);
                    } else if (var && var->type == TYPE_STRING) {
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
                    // その他の式（メンバーアクセスなど）を処理
                    if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                        // メンバーアクセス（self.name など）の場合
                        std::string struct_name = node->left->left->name;
                        std::string member_name = node->left->name;
                        
                        try {
                            Variable *member_var = get_struct_member(struct_name, member_name);
                            if (member_var && member_var->type == TYPE_STRING) {
                                // 文字列メンバーを返す
                                throw ReturnException(member_var->str_value);
                            } else if (member_var) {
                                // 数値メンバーを返す
                                throw ReturnException(member_var->value);
                            }
                        } catch (const std::exception& e) {
                            // メンバーアクセスエラーの場合、式評価にフォールバック
                        }
                    }
                    
                    // デフォルトの式評価（TypedValueを使用）
                    TypedValue result = expression_evaluator_->evaluate_typed_expression(
                        node->left.get());
                    if (result.is_string()) {
                        throw ReturnException(result.string_value);
                    } else {
                        // float/double/quad型の場合は適切な値を使用
                        if (result.numeric_type == TYPE_FLOAT) {
                            throw ReturnException(result.double_value, TYPE_FLOAT);
                        } else if (result.numeric_type == TYPE_DOUBLE) {
                            throw ReturnException(result.double_value, TYPE_DOUBLE);
                        } else if (result.numeric_type == TYPE_QUAD) {
                            throw ReturnException(result.quad_value, TYPE_QUAD);
                        } else {
                            throw ReturnException(result.value, result.numeric_type);
                        }
                    }
                }
            }
        } else {
            throw ReturnException(static_cast<int64_t>(0), TYPE_INT);
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
        try {
            expression_evaluator_->evaluate_expression(node); // 式文として評価
        } catch (const ReturnException &e) {
            // 関数呼び出し文でのreturn值は無視する（void文として扱う）
            // struct、array、stringの戻り値も含めて例外を伝播させない
        }
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
                                  const TypedValue &value,
                                  TypeInfo type_hint, bool is_const) {
    variable_manager_->assign_variable(name, value, type_hint, is_const);
}

void Interpreter::assign_union_variable(const std::string &name, const ASTNode* value_node) {
    Variable* var = find_variable(name);
    if (!var) {
        throw std::runtime_error("Undefined variable: " + name);
    }
    
    if (var->type != TYPE_UNION) {
        throw std::runtime_error("Variable is not a union type: " + name);
    }
    if (debug_mode) {
        debug_print("UNION_ASSIGN_INTERPRETER_DEBUG: Variable '%s' type_name='%s'\n", 
                   name.c_str(), var->type_name.c_str());
    }
    
    variable_manager_->assign_union_value(*var, var->type_name, value_node);
}

void Interpreter::handle_impl_declaration(const ASTNode *node) {
    if (!node) {
        return;
    }

    auto trim = [](const std::string &text) {
        const char *whitespace = " \t\r\n";
        size_t start = text.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return std::string();
        }
        size_t end = text.find_last_not_of(whitespace);
        return text.substr(start, end - start + 1);
    };

    const std::string delimiter = "_for_";
    std::string combined_name = node->name;
    std::string interface_name = combined_name;
    std::string struct_name = node->type_name;

    size_t delim_pos = combined_name.find(delimiter);
    if (delim_pos != std::string::npos) {
        interface_name = combined_name.substr(0, delim_pos);
        if (struct_name.empty()) {
            struct_name = combined_name.substr(delim_pos + delimiter.size());
        }
    }

    interface_name = trim(interface_name);
    struct_name = trim(struct_name);

    if (interface_name.empty()) {
        debug_msg(DebugMsgId::PARSE_STRUCT_DEF,
                  ("Skipping impl registration due to missing interface name: " + node->name).c_str());
        return;
    }

    ImplDefinition impl_def(interface_name, struct_name);

    for (const auto &method_node : node->arguments) {
        if (!method_node || method_node->node_type != ASTNodeType::AST_FUNC_DECL) {
            continue;
        }

        if (method_node->type_name.empty()) {
            method_node->type_name = struct_name;
        }
        method_node->qualified_name = interface_name + "::" + struct_name + "::" + method_node->name;

        impl_def.add_method(method_node.get());
    }

    register_impl_definition(impl_def);
}

void Interpreter::assign_function_parameter(const std::string &name,
                                            int64_t value, TypeInfo type,
                                            bool is_unsigned) {
    variable_manager_->assign_function_parameter(name, value, type,
                                                 is_unsigned);
}

void Interpreter::assign_function_parameter(const std::string &name,
                                            const TypedValue &value,
                                            TypeInfo type,
                                            bool is_unsigned) {
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
    variable_manager_->assign_interface_view(dest_name, std::move(interface_var), source_var, source_var_name);
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

void Interpreter::assign_array_element_float(const std::string &name, int64_t index,
                                             double value) {
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
        debug_msg(DebugMsgId::ARRAY_INDEX_OUT_OF_BOUNDS, index, var->array_size);
        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, name.c_str());
        throw std::runtime_error("Array index out of bounds");
    }

    // 配列の基底型を取得
    TypeInfo base_type = (var->type >= TYPE_ARRAY_BASE) 
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
        throw std::runtime_error("assign_array_element_float called on non-float array");
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
                                   const std::string &name,
                                   bool is_unsigned) {
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

int64_t Interpreter::getMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices) {
    // Priority 3: ArrayProcessingServiceを使用した統一アクセス
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        // 名前が見つからない場合は従来の方法にフォールバック
        return array_manager_->getMultidimensionalArrayElement(var, indices);
    }
    return array_processing_service_->getArrayElement(var_name, indices, ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
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
    array_processing_service_->setArrayElement(var_name, indices, value, ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
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
        return array_manager_->getMultidimensionalStringArrayElement(var, indices);
    }
    return array_processing_service_->getStringArrayElement(var_name, indices, ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

void Interpreter::setMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices,
    const std::string &value) {
    // Priority 3: ArrayProcessingServiceを使用した統一アクセス
    std::string var_name = find_variable_name(&var);
    if (var_name.empty()) {
        // 名前が見つからない場合は従来の方法にフォールバック
        array_manager_->setMultidimensionalStringArrayElement(var, indices, value);
        return;
    }
    array_processing_service_->setStringArrayElement(var_name, indices, value, ArrayProcessingService::ArrayContext::MULTIDIMENSIONAL);
}

// Priority 3: 変数ポインターから名前を取得するヘルパー
std::string Interpreter::find_variable_name(const Variable* target_var) {
    if (!target_var) return "";
    
    // VariableManagerから変数名を取得
    return variable_manager_->find_variable_name(target_var);
}

void Interpreter::assign_array_literal(const std::string &name,
                                       const ASTNode *literal_node) {
    if (debug_mode) {
        debug_print("assign_array_literal called for variable: %s\n", name.c_str());
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
        throw std::runtime_error("Array assignment failed: " + result.error_message);
    }
    
    if (debug_mode) {
        debug_print("Successfully assigned array literal to '%s' using ArrayProcessingService\n", 
                   name.c_str());
    }
}

void Interpreter::assign_array_from_return(const std::string &name,
                                           const ReturnException &ret) {
    std::cerr << "[DEBUG_ASSIGN_RETURN] assign_array_from_return called for: " << name << std::endl;
    std::cerr << "[DEBUG_ASSIGN_RETURN] ret.is_array: " << ret.is_array << std::endl;
    std::cerr << "[DEBUG_ASSIGN_RETURN] ret.int_array_3d.empty(): " << ret.int_array_3d.empty() << std::endl;
    std::cerr << "[DEBUG_ASSIGN_RETURN] ret.str_array_3d.empty(): " << ret.str_array_3d.empty() << std::endl;
    
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

        if (declared_array_size > 0 && declared_array_size != actual_return_size) {
            error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                      ("Struct array size mismatch in assignment: declared " +
                       std::to_string(declared_array_size) +
                       " elements but function returned " +
                       std::to_string(actual_return_size) + " elements")
                          .c_str());
            throw std::runtime_error(
                "Struct array size mismatch in function return assignment");
        }

        // 構造体配列の各要素を更新
        for (int i = 0; i < actual_return_size; ++i) {
            const Variable &source_struct = flattened_structs[i];
            std::string element_name =
                name + "[" + std::to_string(i) + "]";

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
                std::string member_path = element_name + "." + member_pair.first;
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

// ArrayManagerへのアクセス
int64_t Interpreter::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    return array_manager_->getMultidimensionalArrayElement(var, indices);
}

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
    debug_msg(DebugMsgId::INTERPRETER_STRUCT_DEFINITION_STORED, struct_name.c_str());

    struct_definitions_[struct_name] = definition;

    validate_struct_recursion_rules();
}

void Interpreter::validate_struct_recursion_rules() {
    if (!type_manager_ || struct_definitions_.empty()) {
        return;
    }

    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    adjacency.reserve(struct_definitions_.size());
    for (const auto &entry : struct_definitions_) {
        adjacency.emplace(entry.first, std::vector<std::string>{});
    }

    auto gather_forms = [&](const std::string &raw,
                            std::vector<std::string> &collector) {
        if (raw.empty()) {
            return;
        }

        std::string trimmed = trim_copy(raw);
        if (trimmed.empty()) {
            return;
        }

        collector.push_back(trimmed);

        std::string normalized = normalize_struct_type_name(trimmed);
        if (!normalized.empty()) {
            collector.push_back(normalized);
        }

        std::string resolved = type_manager_->resolve_typedef(trimmed);
        if (!resolved.empty()) {
            collector.push_back(resolved);

            std::string normalized_resolved =
                normalize_struct_type_name(resolved);
            if (!normalized_resolved.empty()) {
                collector.push_back(normalized_resolved);
            }
        }

        if (!normalized.empty()) {
            std::string resolved_from_normalized =
                type_manager_->resolve_typedef(normalized);
            if (!resolved_from_normalized.empty()) {
                collector.push_back(resolved_from_normalized);

                std::string normalized_twice =
                    normalize_struct_type_name(resolved_from_normalized);
                if (!normalized_twice.empty()) {
                    collector.push_back(normalized_twice);
                }
            }
        }
    };

    auto resolve_member_target = [&](const StructMember &member) -> std::string {
        std::vector<std::string> candidates;
        candidates.reserve(8);
        gather_forms(member.pointer_base_type_name, candidates);
        gather_forms(member.type_alias, candidates);

        std::unordered_set<std::string> seen;
        for (const auto &candidate : candidates) {
            std::string normalized = normalize_struct_type_name(candidate);
            if (normalized.empty()) {
                continue;
            }

            if (!seen.insert(normalized).second) {
                continue;
            }

            if (struct_definitions_.count(normalized)) {
                return normalized;
            }

            std::string resolved =
                normalize_struct_type_name(type_manager_->resolve_typedef(normalized));
            if (!resolved.empty() && seen.insert(resolved).second &&
                struct_definitions_.count(resolved)) {
                return resolved;
            }
        }

        return "";
    };

    for (const auto &entry : struct_definitions_) {
        const std::string &struct_name = entry.first;
        const StructDefinition &definition = entry.second;

        for (const auto &member : definition.members) {
            bool is_struct_value_member = !member.is_pointer &&
                                          (member.type == TYPE_STRUCT ||
                                           member.pointer_base_type == TYPE_STRUCT);

            if (!is_struct_value_member) {
                continue;
            }

            std::string target = resolve_member_target(member);
            if (target.empty()) {
                continue;
            }

            adjacency[struct_name].push_back(target);
        }
    }

    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    std::vector<std::string> path;

    std::function<void(const std::string &)> dfs = [&](const std::string &node) {
        if (visiting.count(node)) {
            auto cycle_start = std::find(path.begin(), path.end(), node);
            std::vector<std::string> cycle;
            if (cycle_start != path.end()) {
                cycle.assign(cycle_start, path.end());
            }
            cycle.push_back(node);

            std::ostringstream oss;
            oss << "Recursive struct value member cycle detected: "
                << build_cycle_path(cycle)
                << ". Recursive struct relationships must use pointer members.";
            throw std::runtime_error(oss.str());
        }

        if (visited.count(node)) {
            return;
        }

        visiting.insert(node);
        path.push_back(node);

        auto it = adjacency.find(node);
        if (it != adjacency.end()) {
            for (const auto &next : it->second) {
                if (!struct_definitions_.count(next)) {
                    continue;
                }
                dfs(next);
            }
        }

        visiting.erase(node);
        visited.insert(node);
        path.pop_back();
    };

    for (const auto &entry : struct_definitions_) {
        dfs(entry.first);
    }
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

bool Interpreter::is_current_impl_context_for(const std::string &struct_type_name) {
    if (struct_type_name.empty()) {
        return false;
    }

    auto resolve_struct_name = [&](const std::string &name) -> std::string {
        if (name.empty()) {
            return std::string();
        }
        std::string resolved = type_manager_->resolve_typedef(name);
        if (!resolved.empty()) {
            return normalize_struct_type_name(resolved);
        }
        return normalize_struct_type_name(name);
    };

    Variable *self_var = find_variable("self");
    if (!self_var) {
        return false;
    }

    auto extract_struct_type = [&](const Variable *var) -> std::string {
        if (!var) {
            return std::string();
        }
        if (!var->struct_type_name.empty()) {
            auto resolved = resolve_struct_name(var->struct_type_name);
            if (!resolved.empty()) {
                return resolved;
            }
        }
        if (!var->implementing_struct.empty()) {
            auto resolved = resolve_struct_name(var->implementing_struct);
            if (!resolved.empty()) {
                return resolved;
            }
        }
        return std::string();
    };

    std::string self_struct = extract_struct_type(self_var);
    if (self_struct.empty()) {
        return false;
    }

    std::string target_struct = resolve_struct_name(struct_type_name);
    if (target_struct.empty()) {
        target_struct = normalize_struct_type_name(struct_type_name);
    }

    return !target_struct.empty() && target_struct == self_struct;
}

void Interpreter::ensure_struct_member_access_allowed(const std::string &accessor_name,
                                                      const std::string &member_name) {
    if (accessor_name.empty()) {
        return;
    }

    Variable *struct_var = find_variable(accessor_name);
    if (!struct_var) {
        return;
    }

    const bool is_struct_like = struct_var->is_struct || struct_var->type == TYPE_STRUCT ||
                                struct_var->type == TYPE_INTERFACE;
    if (!is_struct_like) {
        return;
    }

    auto member_is_private = [&]() -> bool {
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it != struct_var->struct_members.end()) {
            return member_it->second.is_private_member;
        }

        std::string full_member_name = accessor_name + "." + member_name;
        if (Variable *direct_member = find_variable(full_member_name)) {
            if (direct_member->is_private_member) {
                return true;
            }
        }

        std::string struct_type = struct_var->struct_type_name;
        if (struct_type.empty() && !struct_var->implementing_struct.empty()) {
            struct_type = struct_var->implementing_struct;
        }

        if (struct_type.empty()) {
            return false;
        }

        std::string resolved = type_manager_->resolve_typedef(struct_type);
        if (resolved.empty()) {
            resolved = struct_type;
        }

        const StructDefinition *struct_def = find_struct_definition(resolved);
        if (!struct_def) {
            std::string normalized = normalize_struct_type_name(resolved);
            if (normalized != resolved) {
                struct_def = find_struct_definition(normalized);
            }
        }

        if (struct_def) {
            for (const auto &member : struct_def->members) {
                if (member.name == member_name) {
                    return member.is_private;
                }
            }
        }
        return false;
    }();

    if (!member_is_private) {
        return;
    }

    std::string sanitized_accessor = strip_array_suffix(accessor_name);
    if (sanitized_accessor == "self") {
        return;
    }

    std::string struct_type = struct_var->struct_type_name;
    if (struct_type.empty() && !struct_var->implementing_struct.empty()) {
        struct_type = struct_var->implementing_struct;
    }

    if (!is_current_impl_context_for(struct_type)) {
        throw std::runtime_error("Cannot access private struct member: " + accessor_name + "." + member_name);
    }
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
        find_struct_definition(type_manager_->resolve_typedef(struct_type_name));
    if (!struct_def) {
        throw std::runtime_error("Struct type not found: " + struct_type_name);
    }

    // Debug output removed - use --debug option if needed

    Variable struct_var;
    struct_var.type = TYPE_STRUCT;
    struct_var.is_struct = true;
    struct_var.struct_type_name = struct_type_name;
    struct_var.is_assigned = false;
    struct_var.struct_members.clear();

    // メンバ変数を初期化
    for (const auto &member : struct_def->members) {
        if (debug_mode) {
            debug_print("Processing member: %s, is_array: %d\n", 
                       member.name.c_str(), member.array_info.is_array());
        }

        if (member.array_info.is_array()) {
            if (debug_mode) {
                debug_print("Member %s is an array with %zu dimensions\n", 
                           member.name.c_str(), member.array_info.dimensions.size());
            }
            
            // 多次元配列の処理
            if (member.array_info.dimensions.size() > 1) {
                // 多次元配列メンバ
                if (debug_mode) {
                    debug_print("Creating multidimensional array member: %s with %zu dimensions\n",
                               member.name.c_str(), member.array_info.dimensions.size());
                }
                
                Variable multidim_array_member;
                multidim_array_member.type = member.type;
                multidim_array_member.is_array = true;
                multidim_array_member.is_multidimensional = true;
                multidim_array_member.is_private_member = member.is_private;
                multidim_array_member.is_unsigned = member.is_unsigned;
                multidim_array_member.is_const = member.is_const;
                
                if (debug_mode) {
                    debug_print("Set is_multidimensional = true for %s\n", member.name.c_str());
                }
                
                // 次元情報をコピー
                multidim_array_member.array_dimensions.clear();
                int total_size = 1;
                
                for (const auto& dim : member.array_info.dimensions) {
                    int dim_size = dim.size;
                    
                    // 動的サイズの場合は解決を試みる
                    if (dim_size == -1 && dim.is_dynamic && !dim.size_expr.empty()) {
                        Variable *const_var = find_variable(dim.size_expr);
                        if (const_var && const_var->is_const && const_var->is_assigned) {
                            dim_size = static_cast<int>(const_var->value);
                        } else {
                            throw std::runtime_error("Cannot resolve constant '" + dim.size_expr + "'");
                        }
                    }
                    
                    if (dim_size <= 0) {
                        throw std::runtime_error("Invalid dimension size for struct member " + member.name);
                    }
                    
                    multidim_array_member.array_dimensions.push_back(dim_size);
                    total_size *= dim_size;
                }
                
                // フラットな配列として初期化
                multidim_array_member.multidim_array_values.resize(total_size, 0);
                if (member.type == TYPE_STRING) {
                    multidim_array_member.multidim_array_strings.resize(total_size, "");
                }
                multidim_array_member.is_assigned = false;
                
                struct_var.struct_members[member.name] = multidim_array_member;
                
                if (debug_mode) {
                    debug_print("Multidimensional array member created: %s, total_size=%d\n",
                                member.name.c_str(), total_size);
                }
            } else {
                // 1次元配列メンバの場合（既存コード）
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
            array_member.is_private_member = member.is_private;
            array_member.is_unsigned = member.is_unsigned;
            array_member.is_const = member.is_const;

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

            if (debug_mode) {
                debug_print("Added to struct_members: %s, final array_size=%d\n",
                            member.name.c_str(), 
                            struct_var.struct_members[member.name].array_size);
            }

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
                array_element.is_unsigned = member.is_unsigned;
                array_element.is_private_member = member.is_private;
                array_element.is_assigned = false;

                // 構造体型の配列の場合
                if (member.type == TYPE_STRUCT && !member.type_alias.empty()) {
                    array_element.is_struct = true;
                    array_element.struct_type_name = member.type_alias;
                    
                    if (debug_mode) {
                        debug_print("Creating struct array element: %s[%d] of type %s\n",
                                   member.name.c_str(), i, member.type_alias.c_str());
                    }
                    
                    // 構造体定義を取得してメンバーを初期化
                    std::string resolved_type = type_manager_->resolve_typedef(member.type_alias);
                    const StructDefinition* element_struct_def = find_struct_definition(resolved_type);
                    
                    if (element_struct_def) {
                        for (const auto& element_member : element_struct_def->members) {
                            Variable element_member_var;
                            element_member_var.type = element_member.type;
                            element_member_var.is_unsigned = element_member.is_unsigned;
                            element_member_var.is_private_member = element_member.is_private;
                            element_member_var.is_assigned = false;
                            
                            if (element_member_var.type == TYPE_STRING) {
                                element_member_var.str_value = "";
                            } else {
                                element_member_var.value = 0;
                            }
                            
                            array_element.struct_members[element_member.name] = element_member_var;
                        }
                        
                        if (debug_mode) {
                            debug_print("Initialized struct array element with %zu members\n",
                                       array_element.struct_members.size());
                        }
                    }
                } else {
                    // プリミティブ型の配列要素
                    if (array_element.type == TYPE_STRING) {
                        array_element.str_value = "";
                    } else {
                        array_element.value = 0;
                    }
                }

                std::string element_name = var_name + "." + member.name + "[" +
                                           std::to_string(i) + "]";
                current_scope().variables[element_name] = array_element;
                
                // 親のstruct_membersにも追加
                struct_var.struct_members[member.name + "[" + std::to_string(i) + "]"] = array_element;
            }
            } // 1次元配列処理の終了
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
            member_var.is_private_member = member.is_private;
            member_var.is_unsigned = member.is_unsigned;
            member_var.is_const = member.is_const;

            struct_var.struct_members[member.name] = member_var;
        }
    }

    // 変数を登録
    current_scope().variables[var_name] = struct_var;
    
    // 構造体配列メンバーの要素を再度追加（変数登録後に行う）
    Variable* registered_var = find_variable(var_name);
    if (registered_var && registered_var->is_struct) {
        if (debug_mode) {
            debug_print("Post-registration: adding array elements to struct_members\n");
        }
        
        for (const auto& member : struct_def->members) {
            if (member.array_info.is_array() && member.array_info.dimensions.size() == 1) {
                int array_size = member.array_info.dimensions[0].size;
                
                if (debug_mode) {
                    debug_print("Processing array member: %s, size=%d\n", member.name.c_str(), array_size);
                }
                
                // 配列要素を struct_members に追加
                for (int i = 0; i < array_size; i++) {
                    std::string element_key = member.name + "[" + std::to_string(i) + "]";
                    std::string full_element_name = var_name + "." + element_key;
                    
                    if (debug_mode) {
                        debug_print("Looking for element: %s\n", full_element_name.c_str());
                    }
                    
                    Variable* element_var = find_variable(full_element_name);
                    if (element_var) {
                        registered_var->struct_members[element_key] = *element_var;
                        
                        if (debug_mode) {
                            debug_print("Added array element to struct_members: %s\n", element_key.c_str());
                        }
                    } else {
                        if (debug_mode) {
                            debug_print("Element not found: %s\n", full_element_name.c_str());
                        }
                    }
                }
            }
        }
    }
}

// structメンバにアクセス
Variable *Interpreter::get_struct_member(const std::string &var_name,
                                         const std::string &member_name) {
    debug_msg(DebugMsgId::EXPR_EVAL_STRUCT_MEMBER, member_name.c_str());
    debug_msg(DebugMsgId::INTERPRETER_GET_STRUCT_MEMBER, var_name.c_str(), member_name.c_str());

    Variable *var = find_variable(var_name);
    if (!var || !var->is_struct) {
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_STRUCT, var_name.c_str());
        throw std::runtime_error("Variable is not a struct: " + var_name);
    }

    // 構造体メンバーアクセス前に最新状態を同期
    sync_struct_members_from_direct_access(var_name);

    ensure_struct_member_access_allowed(var_name, member_name);

    debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBERS_FOUND, var->struct_members.size());

    auto it = var->struct_members.find(member_name);
    if (it != var->struct_members.end()) {
        // 親変数がconstの場合、メンバーもconstにする
        if (var->is_const && !it->second.is_const) {
            it->second.is_const = true;
        }
        
        debug_msg(DebugMsgId::EXPR_EVAL_MULTIDIM_ACCESS, 
                  it->second.is_multidimensional ? 1 : 0,
                  it->second.array_dimensions.size(),
                  (size_t)2); // 固定で2インデックス（i,j）
        debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBER_FOUND, 
                 member_name.c_str(), it->second.is_array);
        return &it->second;
    }

    throw std::runtime_error("Struct member not found: " + var_name + "." +
                             member_name);
}

// struct literalから値を代入
// 構造体メンバの個別変数を再帰的に作成
void Interpreter::create_struct_member_variables_recursively(const std::string &base_path,
                                                              const std::string &struct_type_name,
                                                              Variable &parent_var) {
    // 構造体定義を取得
    std::string resolved_type = type_manager_->resolve_typedef(struct_type_name);
    const StructDefinition* struct_def = find_struct_definition(resolved_type);
    
    if (!struct_def) {
        return;
    }
    
    // 各メンバーに対して個別変数を作成
    for (const auto& member_def : struct_def->members) {
        std::string full_member_path = base_path + "." + member_def.name;
        
        Variable member_var;
        member_var.type = member_def.type;
        member_var.is_unsigned = member_def.is_unsigned;
        member_var.is_assigned = false;
        
        // 親変数がconstの場合、またはメンバー定義でconstの場合、メンバーもconstにする
        member_var.is_const = parent_var.is_const || member_def.is_const;
        
        // parent_varのstruct_membersに追加
        parent_var.struct_members[member_def.name] = member_var;
        
        // 構造体メンバの場合
        if (member_def.type == TYPE_STRUCT && !member_def.type_alias.empty()) {
            member_var.is_struct = true;
            member_var.struct_type_name = member_def.type_alias;
            
            // parent_varのstruct_membersも更新
            parent_var.struct_members[member_def.name].is_struct = true;
            parent_var.struct_members[member_def.name].struct_type_name = member_def.type_alias;
            
            // 再帰的にサブメンバーを作成
            create_struct_member_variables_recursively(full_member_path, member_def.type_alias, 
                                                       parent_var.struct_members[member_def.name]);
        }
        
        // 個別変数を登録（parent_var.struct_membersからコピー）
        current_scope().variables[full_member_path] = parent_var.struct_members[member_def.name];
    }
    
    // base_pathの個別変数も更新（parent_varの情報をコピー）
    Variable* base_var = find_variable(base_path);
    if (base_var) {
        base_var->struct_members = parent_var.struct_members;
    }
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
        
        Variable* parent_var = find_variable(parent_name);
        if (parent_var && parent_var->type == TYPE_STRUCT) {
            // 親の構造体定義からメンバー情報を取得
            std::string resolved_parent_type = type_manager_->resolve_typedef(parent_var->struct_type_name);
            const StructDefinition* parent_struct_def = find_struct_definition(resolved_parent_type);
            
            if (parent_struct_def) {
                for (const auto& member_def : parent_struct_def->members) {
                    if (member_def.name == member_name && member_def.type == TYPE_STRUCT) {
                        var->type = TYPE_STRUCT;
                        var->is_struct = true;
                        var->struct_type_name = member_def.type_alias;
                        
                        // メンバの構造体定義を取得して、struct_membersを初期化
                        std::string resolved_member_type = type_manager_->resolve_typedef(member_def.type_alias);
                        const StructDefinition* member_struct_def = find_struct_definition(resolved_member_type);
                        if (member_struct_def) {
                            for (const auto& sub_member : member_struct_def->members) {
                                Variable sub_member_var;
                                sub_member_var.type = sub_member.type;
                                sub_member_var.is_unsigned = sub_member.is_unsigned;
                                sub_member_var.is_assigned = false;
                                if (sub_member.type == TYPE_STRUCT) {
                                    sub_member_var.is_struct = true;
                                    sub_member_var.struct_type_name = sub_member.type_alias;
                                }
                                var->struct_members[sub_member.name] = sub_member_var;
                                
                                // 個別変数も作成
                                std::string full_sub_member_name = var_name + "." + sub_member.name;
                                current_scope().variables[full_sub_member_name] = sub_member_var;
                            }
                        }
                        
                        std::cerr << "[DEBUG ASSIGN_STRUCT_LITERAL] Updated: is_struct=" << var->is_struct 
                                  << ", struct_type_name=" << var->struct_type_name << std::endl;
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
                   "Unsigned struct member %s.%s %s negative value (%lld); clamping to 0",
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
            std::string resolved_struct_name = type_manager_->resolve_typedef(array_var->struct_type_name);
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

    if (!var || !var->is_struct) {
        throw std::runtime_error("Variable is not a struct: " + var_name);
    }

    if (var->is_const && var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, var_name.c_str());
        throw std::runtime_error("Cannot assign to const struct: " + var_name);
    }
    
    // struct定義を取得してメンバ順序を確認
    // まずtypedefを解決してから構造体定義を検索
    std::string resolved_struct_name = type_manager_->resolve_typedef(var->struct_type_name);
    const StructDefinition *struct_def = find_struct_definition(resolved_struct_name);
    if (!struct_def) {
        throw std::runtime_error("Struct definition not found: " +
                                 var->struct_type_name);
    }
    
    // 親変数がconstの場合、すべてのstruct_membersと個別変数をconstにする（再帰的）
    if (var->is_const) {
        std::function<void(const std::string&, Variable&)> make_all_members_const;
        make_all_members_const = [&](const std::string& base_path, Variable& v) {
            for (auto& member_pair : v.struct_members) {
                member_pair.second.is_const = true;
                
                // 個別変数も更新
                std::string full_path = base_path + "." + member_pair.first;
                Variable* individual_var = find_variable(full_path);
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
        debug_msg(DebugMsgId::INTERPRETER_NAMED_STRUCT_LITERAL_INIT, var_name.c_str());

        for (const auto &member_init : literal_node->arguments) {
            if (member_init->node_type != ASTNodeType::AST_ASSIGN) {
                continue;
            }

            std::string member_name = member_init->name;
            
            debug_msg(DebugMsgId::INTERPRETER_MEMBER_INIT_PROCESSING, member_name.c_str());

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
            const StructMember* member_def = struct_def->find_member(member_name);
            if (member_def) {
                struct_member_var.is_const = var->is_const || member_def->is_const;
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
                    debug_print("Array member initialization: %s, array_size=%d, elements_count=%zu\n",
                               member_name.c_str(), struct_member_var.array_size, 
                               member_init->right->arguments.size());
                }

                // 配列リテラルの各要素を個別変数に代入
                const auto &array_elements = member_init->right->arguments;
                         
                for (size_t i = 0; i < array_elements.size() &&
                                   i < static_cast<size_t>(struct_member_var.array_size);
                     i++) {
                    
                    std::string element_name = var_name + "." + member_name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    std::string element_path = member_name + "[" + std::to_string(i) + "]";
                    
                    // float/double配列の処理
                    if (struct_member_var.type == TYPE_FLOAT || struct_member_var.type == TYPE_DOUBLE) {
                        TypedValue typed_result = expression_evaluator_->evaluate_typed_expression(
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
                            struct_member_var.array_float_values[i] = float_value;
                            
                            if (debug_mode) {
                                debug_print("Updated struct_members array element: "
                                            "%s[%zu] = %f\n",
                                            member_name.c_str(), i, float_value);
                            }
                        }
                    } else {
                        // int/bool/その他の型の配列
                        int64_t value = expression_evaluator_->evaluate_expression(
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
                                            element_name.c_str(), (long long)value);
                            }
                        }

                        // struct_membersの配列要素にも代入
                        if (i < struct_member_var.array_values.size()) {
                            struct_member_var.array_values[i] = value;
                            
                            if (debug_mode) {
                                debug_print("Updated struct_members array element: "
                                            "%s[%zu] = %lld\n",
                                            member_name.c_str(), i,
                                            (long long)value);
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
                struct_member_var.type = TYPE_STRING; // Union型の場合は実際の型をセット
                struct_member_var.is_assigned = true;

                // 直接アクセス変数も更新
                if (member_var) {
                    member_var->str_value = member_init->right->str_value;
                    member_var->type = TYPE_STRING; // Union型の場合は実際の型をセット
                    member_var->is_assigned = true;
                }
            } else if (struct_member_var.type == TYPE_STRUCT && 
                       member_init->right->node_type == ASTNodeType::AST_VARIABLE) {
                // 構造体メンバに別の構造体変数を代入
                Variable* source_var = find_variable(member_init->right->name);
                if (!source_var || source_var->type != TYPE_STRUCT) {
                    throw std::runtime_error("Source variable is not a struct: " + member_init->right->name);
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
                for (const auto& sm : source_var->struct_members) {
                    std::string source_member_path = member_init->right->name + "." + sm.first;
                    std::string target_member_path = full_member_name + "." + sm.first;
                    Variable* target_member_var = find_variable(target_member_path);
                    if (target_member_var) {
                        Variable* source_member_var = find_variable(source_member_path);
                        if (source_member_var) {
                            *target_member_var = *source_member_var;
                        }
                    }
                }
            } else if (struct_member_var.type == TYPE_STRUCT && 
                       member_init->right->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
                // 構造体メンバにネストした構造体リテラルを代入
                // 再帰的にassign_struct_literalを呼び出す
                debug_msg(DebugMsgId::INTERPRETER_NESTED_STRUCT_LITERAL, full_member_name.c_str());
                
                // メンバ変数が存在することを確認
                if (!member_var) {
                    throw std::runtime_error("Struct member variable not found: " + full_member_name);
                }
                
                // 親変数がconstの場合、このメンバーもconstにする
                if (var->is_const) {
                    struct_member_var.is_const = true;
                    member_var->is_const = true;
                }
                
                // 再帰的に構造体リテラルを代入
                assign_struct_literal(full_member_name, member_init->right.get());
                
                // struct_membersも更新（個別変数から同期）
                struct_member_var = *member_var;
            } else {
                // float/double型の処理
                if (struct_member_var.type == TYPE_FLOAT || struct_member_var.type == TYPE_DOUBLE) {
                    TypedValue typed_result = expression_evaluator_->evaluate_typed_expression(
                        member_init->right.get());
                    double float_value = typed_result.as_double();
                    
                    // struct_membersの値を直接更新（TYPEに応じて適切なフィールドに）
                    if (struct_member_var.type == TYPE_FLOAT) {
                        struct_member_var.float_value = static_cast<float>(float_value);
                    } else if (struct_member_var.type == TYPE_DOUBLE) {
                        struct_member_var.double_value = float_value;
                    } else if (struct_member_var.type == TYPE_QUAD) {
                        struct_member_var.quad_value = static_cast<long double>(float_value);
                    }
                    struct_member_var.is_assigned = true;
                    
                    // 注意: operator[]は存在しない要素を作ってしまうので使わない
                    // struct_member_varはすでにリファレンスなので更新済み

                    // 直接アクセス変数も更新
                    if (member_var) {
                        if (member_var->type == TYPE_FLOAT) {
                            member_var->float_value = static_cast<float>(float_value);
                        } else if (member_var->type == TYPE_DOUBLE) {
                            member_var->double_value = float_value;
                        } else if (member_var->type == TYPE_QUAD) {
                            member_var->quad_value = static_cast<long double>(float_value);
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
                    
                    // 注意: operator[]は存在しない要素を作ってしまうので使わない
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
        debug_print("STRUCT_LITERAL_DEBUG: Position-based initialization with %zu arguments\n", literal_node->arguments.size());
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
            debug_print("STRUCT_LITERAL_DEBUG: Initializing member %s (index %zu, type %d)\n", 
                       member_def.name.c_str(), i, (int)member_def.type);

            // メンバ値を評価して代入
            if (it->second.type == TYPE_STRING &&
                init_value->node_type == ASTNodeType::AST_STRING_LITERAL) {
                debug_print("STRUCT_LITERAL_DEBUG: String literal initialization: %s = \"%s\"\n", 
                           member_def.name.c_str(), init_value->str_value.c_str());
                it->second.str_value = init_value->str_value;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
                Variable *direct_member_var = find_variable(full_member_name);
                if (direct_member_var) {
                    direct_member_var->str_value = init_value->str_value;
                    direct_member_var->is_assigned = true;
                    debug_print("STRUCT_LITERAL_DEBUG: Updated direct access variable: %s = \"%s\"\n", 
                               full_member_name.c_str(), init_value->str_value.c_str());
                }
            } else if (it->second.is_array && init_value->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                debug_print("STRUCT_LITERAL_DEBUG: Array literal initialization: %s\n", member_def.name.c_str());
                
                // 配列の要素型を確認（構造体配列かプリミティブ配列か）
                TypeInfo element_type = member_def.array_info.base_type;
                
                if (element_type == TYPE_STRUCT) {
                    // 構造体配列の場合：各要素は構造体リテラルとして処理
                    debug_print("STRUCT_LITERAL_DEBUG: Struct array initialization with %zu elements\n", 
                               init_value->arguments.size());
                    
                    for (size_t j = 0; j < init_value->arguments.size() && 
                                       j < static_cast<size_t>(it->second.array_size); ++j) {
                        const ASTNode* element_node = init_value->arguments[j].get();
                        
                        if (element_node->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
                            // 配列要素の完全な名前を構築
                            std::string element_name = var_name + "." + member_def.name + 
                                                      "[" + std::to_string(j) + "]";
                            
                            debug_print("STRUCT_LITERAL_DEBUG: Assigning struct literal to array element: %s\n", 
                                       element_name.c_str());
                            
                            // 再帰的に構造体リテラルを代入
                            assign_struct_literal(element_name, element_node);
                        } else {
                            throw std::runtime_error("Expected struct literal for struct array element");
                        }
                    }
                    it->second.is_assigned = true;
                } else {
                    // プリミティブ型配列の場合：従来の処理
                    it->second.array_values.clear();
                    
                    // 個別要素変数を一時マップに収集
                    std::map<std::string, Variable> element_vars;
                    
                    for (size_t j = 0; j < init_value->arguments.size(); ++j) {
                        int64_t element_value = expression_evaluator_->evaluate_expression(init_value->arguments[j].get());
                        std::string element_path = member_def.name + "[" + std::to_string(j) + "]";
                        clamp_unsigned_member(it->second, element_value,
                                               element_path,
                                               "initialized with array literal");
                        it->second.array_values.push_back(element_value);
                        
                        // 個別要素変数を作成
                        std::string full_element_name = var_name + "." + member_def.name + "[" + std::to_string(j) + "]";
                        Variable element_var;
                        element_var.type = element_type;
                        element_var.value = element_value;
                        element_var.is_assigned = true;
                        element_vars[full_element_name] = element_var;
                        
                        debug_print("STRUCT_LITERAL_DEBUG: Array element [%zu] = %lld\n", j, (long long)element_value);
                    }
                    it->second.array_size = init_value->arguments.size();
                    it->second.is_assigned = true;
                    
                    // 直接アクセス変数も更新
                    std::string full_member_name = var_name + "." + member_def.name;
                    Variable *direct_member_var = find_variable(full_member_name);
                    if (direct_member_var && direct_member_var->is_array) {
                        direct_member_var->array_values = it->second.array_values;
                        direct_member_var->array_size = it->second.array_size;
                        direct_member_var->is_assigned = true;
                        debug_print("STRUCT_LITERAL_DEBUG: Updated direct access array variable: %s\n", 
                                   full_member_name.c_str());
                    }
                    
                    // 個別要素変数を一括登録（マップの再ハッシュを防ぐため一度に追加）
                    for (const auto& ev_pair : element_vars) {
                        variable_manager_->current_scope().variables[ev_pair.first] = ev_pair.second;
                    }
                }
            } else {
                // float/double型の処理
                if (it->second.type == TYPE_FLOAT || it->second.type == TYPE_DOUBLE || it->second.type == TYPE_QUAD) {
                    TypedValue typed_result =
                        expression_evaluator_->evaluate_typed_expression(init_value);
                    double float_value = typed_result.as_double();
                    debug_print("STRUCT_LITERAL_DEBUG: Float/Double initialization: %s = %f\n", 
                               member_def.name.c_str(), float_value);
                    
                    // TYPEに応じて適切なフィールドに設定
                    if (it->second.type == TYPE_FLOAT) {
                        it->second.float_value = static_cast<float>(float_value);
                    } else if (it->second.type == TYPE_DOUBLE) {
                        it->second.double_value = float_value;
                    } else if (it->second.type == TYPE_QUAD) {
                        it->second.quad_value = static_cast<long double>(float_value);
                    }
                    it->second.is_assigned = true;

                    // 直接アクセス変数も更新
                    std::string full_member_name = var_name + "." + member_def.name;
                    Variable *direct_member_var = find_variable(full_member_name);
                    if (direct_member_var) {
                        if (direct_member_var->type == TYPE_FLOAT) {
                            direct_member_var->float_value = static_cast<float>(float_value);
                        } else if (direct_member_var->type == TYPE_DOUBLE) {
                            direct_member_var->double_value = float_value;
                        } else if (direct_member_var->type == TYPE_QUAD) {
                            direct_member_var->quad_value = static_cast<long double>(float_value);
                        }
                        direct_member_var->is_assigned = true;
                        debug_print("STRUCT_LITERAL_DEBUG: Updated direct access variable: %s = %f\n", 
                                   full_member_name.c_str(), float_value);
                    }
                } else {
                    // int/bool/その他の型の処理
                    int64_t value =
                        expression_evaluator_->evaluate_expression(init_value);
                    debug_print("STRUCT_LITERAL_DEBUG: Numeric initialization: %s = %lld\n", 
                               member_def.name.c_str(), (long long)value);
                    clamp_unsigned_member(it->second, value, member_def.name,
                                           "initialized with literal");
                    it->second.value = value;

                    // 直接アクセス変数も更新
                    std::string full_member_name = var_name + "." + member_def.name;
                    Variable *direct_member_var = find_variable(full_member_name);
                    if (direct_member_var) {
                        direct_member_var->value = value;
                        direct_member_var->is_assigned = true;
                        debug_print("STRUCT_LITERAL_DEBUG: Updated direct access variable: %s = %lld\n", 
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
            debug_print("INT: Nested member assignment: var_name=%s, root_var_name=%s\n",
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
        }
    }
    
    if (Variable *struct_var = find_variable(var_name)) {
        if (debug_mode) {
            debug_print("assign_struct_member (int): var=%s, member=%s, value=%lld, struct_is_const=%d\n",
                        var_name.c_str(), member_name.c_str(),
                        static_cast<long long>(value),
                        struct_var->is_const ? 1 : 0);
        }
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
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
        if (!type_manager_->is_value_allowed_for_union(member_var->type_name, value)) {
            throw std::runtime_error("Integer value " + std::to_string(value) + " is not allowed for union type " + member_var->type_name + " in struct member " + member_name);
        }
        // Union型の場合はcurrent_typeを整数型に設定し、文字列値をクリア
        member_var->current_type = TYPE_INT;
        member_var->str_value.clear(); // 文字列値をクリア
    }
    
    int64_t member_value = value;
    if (member_var->is_unsigned && member_value < 0) {
        DEBUG_WARN(VARIABLE,
                   "Unsigned struct member %s.%s assigned negative value (%lld); clamping to 0",
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        // Union型の場合は制約をチェック
        bool is_union_direct = type_manager_->is_union_type(*direct_var);
        if (is_union_direct) {
            if (!type_manager_->is_value_allowed_for_union(direct_var->type_name, value)) {
                throw std::runtime_error("Integer value " + std::to_string(value) + " is not allowed for union type " + direct_var->type_name + " in struct member " + member_name);
            }
            // Union型の場合はcurrent_typeを整数型に設定し、文字列値をクリア
            direct_var->current_type = TYPE_INT;
            direct_var->str_value.clear(); // 文字列値をクリア
        }

        int64_t direct_value = member_var->is_unsigned ? member_value : value;
        if (direct_var->is_unsigned && direct_value < 0) {
            DEBUG_WARN(VARIABLE,
                       "Unsigned struct member %s.%s assigned negative value (%lld); clamping to 0",
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
        debug_print("assign_struct_member (string): var=%s, member=%s, value='%s'\n",
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
        }
    }
    
    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
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
        if (!type_manager_->is_value_allowed_for_union(member_var->type_name, value)) {
            throw std::runtime_error("String value '" + value + "' is not allowed for union type " + member_var->type_name + " in struct member " + member_name);
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        // Union型の場合は制約をチェック
        bool is_union_direct = type_manager_->is_union_type(*direct_var);
        if (is_union_direct) {
            if (!type_manager_->is_value_allowed_for_union(direct_var->type_name, value)) {
                throw std::runtime_error("String value '" + value + "' is not allowed for union type " + direct_var->type_name + " in struct member " + member_name);
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
            debug_print("Direct access var %s not found\n", direct_var_name.c_str());
        }
    }
}

// structメンバに値を代入（TypedValue）
void Interpreter::assign_struct_member(const std::string &var_name,
                                       const std::string &member_name,
                                       const TypedValue &typed_value) {
    if (debug_mode) {
        debug_print("assign_struct_member (TypedValue): var=%s, member=%s, type=%d\n",
                    var_name.c_str(), member_name.c_str(), static_cast<int>(typed_value.numeric_type));
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
        }
    }
    
    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
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
            DEBUG_WARN(VARIABLE, "Unsigned struct member %s.%s assignment with negative value (%lld); clamping to 0",
                      var_name.c_str(), member_name.c_str(), static_cast<long long>(assign_value));
            assign_value = 0;
        }
        member_var->value = assign_value;
        if (is_union_member) {
            member_var->current_type = (typed_value.numeric_type != TYPE_UNKNOWN) 
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }

        bool is_union_direct = (direct_var->type == TYPE_UNION);
        
        if (typed_value.numeric_type == TYPE_FLOAT) {
            direct_var->float_value = static_cast<float>(typed_value.double_value);
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
                DEBUG_WARN(VARIABLE, "Unsigned struct member %s assignment with negative value (%lld); clamping to 0",
                          direct_var_name.c_str(), static_cast<long long>(assign_value));
                assign_value = 0;
            }
            direct_var->value = assign_value;
            if (is_union_direct) {
                direct_var->current_type = (typed_value.numeric_type != TYPE_UNKNOWN) 
                                          ? typed_value.numeric_type 
                                          : TYPE_INT;
            }
            // unsignedフラグはメンバ定義から引き継がれるため、ここでは設定しない
        }
        direct_var->is_assigned = true;
        
        if (debug_mode) {
            debug_print("Updated direct access var %s (type=%d)\n",
                        direct_var_name.c_str(), static_cast<int>(direct_var->type));
        }
    }
}

// structメンバに構造体を代入
void Interpreter::assign_struct_member_struct(const std::string &var_name,
                                             const std::string &member_name,
                                             const Variable &struct_value) {
    if (debug_mode) {
        debug_print("assign_struct_member_struct: var=%s, member=%s, struct_type=%s\n",
                    var_name.c_str(), member_name.c_str(), struct_value.struct_type_name.c_str());
    }

    std::string target_full_name = var_name + "." + member_name;
    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
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
    if (!member_var->struct_type_name.empty() && !struct_value.struct_type_name.empty() &&
        member_var->struct_type_name != struct_value.struct_type_name) {
        throw std::runtime_error("Struct type mismatch: expected " + member_var->struct_type_name + 
                                ", got " + struct_value.struct_type_name);
    }
    
    // 型名が空の場合は代入先の型名を設定
    if (member_var->struct_type_name.empty()) {
        member_var->struct_type_name = struct_value.struct_type_name;
        if (debug_mode) {
            debug_print("Setting member struct type to: %s\n", struct_value.struct_type_name.c_str());
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        *direct_var = struct_value;
        direct_var->is_assigned = true;
        if (debug_mode) {
            debug_print("Updated direct access struct var %s\n", direct_var_name.c_str());
        }
    }
    
    // 構造体のメンバー変数も個別に更新
    for (const auto& member : struct_value.struct_members) {
        std::string nested_var_name = direct_var_name + "." + member.first;
        Variable* nested_var = find_variable(nested_var_name);
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
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
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
                   "Unsigned struct member %s.%s[%d] assigned negative value (%lld); clamping to 0",
                   var_name.c_str(), member_name.c_str(), index,
                   static_cast<long long>(adjusted_value));
        adjusted_value = 0;
    }

    member_var->array_values[index] = adjusted_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス配列要素変数も更新
    std::string direct_element_name = var_name + "." + member_name + "[" + std::to_string(index) + "]";
    Variable *direct_element = find_variable(direct_element_name);
    if (direct_element) {
        if (direct_element->is_const && direct_element->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, direct_element_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_element_name);
        }
        int64_t direct_value = member_var->is_unsigned ? adjusted_value : value;
        if (direct_element->is_unsigned && direct_value < 0) {
            DEBUG_WARN(VARIABLE,
                       "Unsigned struct member %s.%s[%d] assigned negative value (%lld); clamping to 0",
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
        debug_print("assign_struct_member_array_element (string): var=%s, member=%s, index=%d, value=%s\n",
                    var_name.c_str(), member_name.c_str(), index, value.c_str());
    }

    std::string target_full_name = var_name + "." + member_name;
    if (Variable *struct_var = find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
            throw std::runtime_error("Cannot assign to member of const struct: " +
                                     target_full_name);
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
            throw std::runtime_error(
                "Cannot assign to const struct member: " +
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

// N次元配列アクセス対応版
int64_t Interpreter::get_struct_member_multidim_array_element(
    const std::string &var_name, const std::string &member_name, 
    const std::vector<int64_t> &indices) {
    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    if (debug_mode) {
        debug_print("get_struct_member_multidim_array_element: var=%s, member=%s\n",
                    var_name.c_str(), member_name.c_str());
        debug_print("Indices: ");
        for (size_t i = 0; i < indices.size(); i++) {
            debug_print("[%lld]", indices[i]);
        }
        debug_print("\n");
        debug_print("Array dimensions: ");
        for (size_t i = 0; i < member_var->array_dimensions.size(); i++) {
            debug_print("[%zu]", member_var->array_dimensions[i]);
        }
        debug_print("\n");
    }

    // 多次元配列の場合、インデックスをフラットインデックスに変換
    if (member_var->is_multidimensional && !member_var->array_dimensions.empty()) {
        // 次元数チェック
        if (indices.size() != member_var->array_dimensions.size()) {
            throw std::runtime_error("Dimension mismatch: expected " + 
                                     std::to_string(member_var->array_dimensions.size()) + 
                                     " dimensions, got " + std::to_string(indices.size()));
        }

        // 各次元の境界チェックとフラットインデックス計算
        size_t flat_index = 0;
        size_t multiplier = 1;
        
        // 逆順（最後の次元から）でフラットインデックスを計算
        for (int d = static_cast<int>(indices.size()) - 1; d >= 0; d--) {
            if (indices[d] < 0 || indices[d] >= static_cast<int64_t>(member_var->array_dimensions[d])) {
                throw std::runtime_error("Array index out of bounds in dimension " + std::to_string(d));
            }
            flat_index += static_cast<size_t>(indices[d]) * multiplier;
            multiplier *= member_var->array_dimensions[d];
        }

        if (debug_mode) {
            debug_print("Calculated flat_index: %zu\n", flat_index);
        }

        if (flat_index >= member_var->multidim_array_values.size()) {
            throw std::runtime_error("Calculated flat index out of bounds");
        }

        if (debug_mode) {
            debug_print("Reading from multidim_array_values[%zu] = %lld\n", 
                        flat_index, member_var->multidim_array_values[flat_index]);
        }

        return member_var->multidim_array_values[flat_index];
    } else {
        // 1次元配列の場合
        if (indices.size() != 1) {
            throw std::runtime_error("Array is 1-dimensional but multiple indices provided");
        }
        return get_struct_member_array_element(var_name, member_name, static_cast<int>(indices[0]));
    }
}

std::string Interpreter::get_struct_member_array_string_element(
    const std::string &var_name, const std::string &member_name, int index) {
    if (debug_mode) {
        debug_print("get_struct_member_array_string_element: var=%s, member=%s, index=%d\n",
                    var_name.c_str(), member_name.c_str(), index);
    }
    
    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    auto is_string_array_type = [&](const Variable* var) {
        if (!var) {
            return false;
        }

        if (var->type == TYPE_STRING) {
            return true;
        }

        if (var->type == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            return true;
        }

        if (var->array_type_info.base_type == TYPE_STRING) {
            return true;
        }

        return false;
    };

    if (!is_string_array_type(member_var)) {
        throw std::runtime_error("Member is not a string array: " + member_name);
    }

    if (debug_mode) {
        debug_print("Returning string: array_strings[%d]=%s\n",
                    index, member_var->array_strings[index].c_str());
    }

    return member_var->array_strings[index];
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
        debug_print("member_var->is_multidimensional: %d, array_dimensions.size(): %zu\n", 
                   member_var->is_multidimensional, member_var->array_dimensions.size());
        debug_print("Address of member_var: %p\n", (void*)member_var);
    }

    // 共通実装を使用して配列リテラルを解析・代入
    try {
        auto result = common_operations_->parse_array_literal(array_literal);
        
        if (debug_mode) {
            debug_print("Before assign_array_literal_to_variable: array_dimensions.size(): %zu\n", 
                       member_var->array_dimensions.size());
        }
        
        common_operations_->assign_array_literal_to_variable(
            member_var, result, var_name + "." + member_name);
        
        if (debug_mode) {
            debug_print("After assign_array_literal_to_variable: array_dimensions.size(): %zu\n", 
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
                debug_print("member_var->is_multidimensional: %d\n", member_var->is_multidimensional);
                debug_print("member_var->array_dimensions.size(): %zu\n", member_var->array_dimensions.size());
                if (member_var->array_dimensions.size() >= 2) {
                    for (size_t i = 0; i < member_var->array_dimensions.size(); i++) {
                        debug_print("dimension[%zu]: %zu\n", i, member_var->array_dimensions[i]);
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
                    debug_print("Assigning N-dimensional array literal to %s.%s\n", 
                               var_name.c_str(), member_name.c_str());
                    debug_print("Total array size: %zu, values to assign: %zu\n",
                               member_var->array_values.size(), assigned_count);
                }
                
                // フラット配列データを直接更新
                size_t max_elements = std::min(member_var->array_values.size(), assigned_count);
                
                // multidim_array_values も初期化
                if (member_var->multidim_array_values.size() != member_var->array_values.size()) {
                    member_var->multidim_array_values.resize(member_var->array_values.size());
                    if (debug_mode) {
                        debug_print("Resized multidim_array_values to %zu elements\n", member_var->array_values.size());
                    }
                }
                
                for (size_t i = 0; i < max_elements; i++) {
                    member_var->array_values[i] = assigned_values[i];
                    member_var->multidim_array_values[i] = assigned_values[i];  // multidim_array_values にも設定
                    if (debug_mode) {
                        debug_print("Set flat_index[%zu] = %lld (both array_values and multidim_array_values)\n", i, assigned_values[i]);
                    }
                }
                
                // N次元インデックス表示のためのデバッグ（2次元の場合の例）
                if (debug_mode && member_var->array_dimensions.size() == 2) {
                    size_t rows = member_var->array_dimensions[0];
                    size_t cols = member_var->array_dimensions[1];
                    for (size_t r = 0; r < rows && (r * cols) < assigned_count; r++) {
                        for (size_t c = 0; c < cols && (r * cols + c) < assigned_count; c++) {
                            size_t flat_index = r * cols + c;
                            debug_print("  [%zu][%zu] = %lld (flat_index: %zu)\n", 
                                       r, c, member_var->array_values[flat_index], flat_index);
                        }
                    }
                }
                
                // 多次元配列でも個別要素変数を更新
                for (size_t i = 0; i < max_elements; i++) {
                    std::string element_name = var_name + "." + member_name + "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    if (element_var) {
                        element_var->value = assigned_values[i];
                        element_var->is_assigned = true;
                        if (debug_mode) {
                            debug_print("Updated individual element variable %s = %lld\n", 
                                       element_name.c_str(), assigned_values[i]);
                        }
                    }
                }
            } else {
                // 1次元配列の場合（既存の処理）
                for (size_t i = 0; i < result.size && i < assigned_count; i++) {
                    std::string element_name = var_name + "." + member_name + "[" +
                                               std::to_string(i) + "]";
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
    if (!node)
        return;

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        if (node->statements.size() > 0) {
            debug_msg(DebugMsgId::INTERPRETER_PROCESSING_STMT_LIST, node->statements.size());
        }
        
        // 2パス初期化: まずconst変数を初期化し、その後他の変数を初期化
        // 第1パス: const変数のみ初期化（配列サイズ式で使用される可能性があるため）
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL && stmt->is_const && !stmt->is_array) {
                debug_msg(DebugMsgId::INTERPRETER_FOUND_VAR_DECL, stmt->name.c_str());
                initialize_global_variables(stmt.get());
            }
        }
        
        // 第2パス: 配列とその他の変数を初期化
        for (const auto &stmt : node->statements) {
            debug_msg(DebugMsgId::INTERPRETER_CHECKING_STATEMENT_TYPE, (int)stmt->node_type, stmt->name.c_str());
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL) {
                // 第1パスで既に初期化されたconst変数をスキップ
                if (stmt->is_const && !stmt->is_array) {
                    continue;
                }
                debug_msg(DebugMsgId::INTERPRETER_FOUND_VAR_DECL, stmt->name.c_str());
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


void Interpreter::sync_enum_definitions_from_parser(RecursiveParser* parser) {
    if (!parser) return;
    
    auto& parser_enums = parser->get_enum_definitions();
    for (const auto& pair : parser_enums) {
        const std::string& enum_name = pair.first;
        const EnumDefinition& enum_def = pair.second;
        
        // EnumManagerに登録
        enum_manager_->register_enum(enum_name, enum_def);
        
        if (debug_mode) {
            debug_print("Synced enum definition: %s with %zu members\\n",
                      enum_name.c_str(), enum_def.members.size());
        }
    }
}

void Interpreter::sync_struct_definitions_from_parser(RecursiveParser* parser) {
    if (!parser) return;
    
    auto& parser_structs = parser->get_struct_definitions();
    for (const auto& pair : parser_structs) {
        const std::string& struct_name = pair.first;
        const StructDefinition& struct_def = pair.second;
        
        // Interpreterのstruct_definitions_に登録
        struct_definitions_[struct_name] = struct_def;
        
        debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED, struct_name.c_str(), struct_def.members.size());
    }
}

void Interpreter::sync_struct_members_from_direct_access(const std::string &var_name) {
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START, var_name.c_str());
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START, var_name.c_str());
    
    // 空の変数名はスキップ
    if (var_name.empty()) {
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND, "empty variable name");
        return;
    }
    
    // デバッグ: sync開始前に特定の個別変数を確認
    if (debug_mode && var_name == "student1") {
        Variable* test_var = find_variable("student1.scores[0]");
        if (test_var) {
            debug_print("SYNC_DEBUG: Before sync, student1.scores[0] = %lld, is_assigned=%d\n",
                       (long long)test_var->value, test_var->is_assigned);
        }
    }
    
    // 変数を取得
    Variable *var = find_variable(var_name);
    if (!var) {
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND, var_name.c_str());
        return;
    }
    if (!var->is_struct) {
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_STRUCT, var_name.c_str());
        return;
    }
    
    // 構造体定義を取得（typedefを解決）
    std::string resolved_struct_name = type_manager_->resolve_typedef(var->struct_type_name);
    const StructDefinition *struct_def = find_struct_definition(resolved_struct_name);
    if (!struct_def) {
        debug_msg(DebugMsgId::INTERPRETER_STRUCT_DEFINITION_STORED, var->struct_type_name.c_str());
        return;
    }
    
    debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBERS_FOUND, struct_def->members.size());
    
    // 各メンバについてダイレクトアクセス変数から struct_members に同期
    for (const auto &member : struct_def->members) {
        std::string direct_var_name = var_name + "." + member.name;
        Variable *direct_var = find_variable(direct_var_name);
        
        if (direct_var) {
            debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBER_FOUND, member.name.c_str());
            auto resolve_base_type = [](TypeInfo type) {
                if (type >= TYPE_ARRAY_BASE) {
                    return static_cast<TypeInfo>(type - TYPE_ARRAY_BASE);
                }
                return type;
            };
            TypeInfo member_base_type = resolve_base_type(member.type);
            TypeInfo direct_base_type = resolve_base_type(direct_var->type);
            
            // struct_membersに保存（配列チェックを先に実行）
            if (member.type >= TYPE_ARRAY_BASE || member.array_info.base_type != TYPE_UNKNOWN || direct_var->is_array) {
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_ARRAY_MEMBER_ADDED, 
                         member.name.c_str(), (int)member.type, direct_var->array_size);
                
                // 既存のメンバーがあれば保持、なければ新規作成
                if (var->struct_members.find(member.name) == var->struct_members.end()) {
                    var->struct_members[member.name] = Variable();
                }
                
                var->struct_members[member.name].type = member.type;
                var->struct_members[member.name].is_array = true;
                var->struct_members[member.name].array_size = direct_var->array_size;
                
                // 多次元配列情報をコピー
                if (direct_var->is_multidimensional) {
                    var->struct_members[member.name].is_multidimensional = true;
                    var->struct_members[member.name].array_dimensions = direct_var->array_dimensions;
                    debug_print("SYNC_STRUCT: Preserved multidimensional info for %s.%s (dimensions: %zu)\n",
                              var_name.c_str(), member.name.c_str(), direct_var->array_dimensions.size());
                }
                
                // 配列要素を個別にチェックして同期
                // 既存の値を保持するため、サイズが変わる場合のみresize
                if (var->struct_members[member.name].array_values.size() != static_cast<size_t>(direct_var->array_size)) {
                    var->struct_members[member.name].array_values.resize(direct_var->array_size);
                }
                if (var->struct_members[member.name].array_strings.size() != static_cast<size_t>(direct_var->array_size)) {
                    var->struct_members[member.name].array_strings.resize(direct_var->array_size);
                }
                
                // 多次元配列の場合は multidim_array_values も初期化（元の値をコピー）
                if (var->struct_members[member.name].is_multidimensional) {
                    // 既存の multidim_array_values をバックアップしてからリサイズ
                    std::vector<int64_t> backup_values = direct_var->multidim_array_values;
                    var->struct_members[member.name].multidim_array_values.resize(direct_var->array_size);
                    
                    // バックアップした値を復元
                    size_t copy_size = std::min(backup_values.size(), static_cast<size_t>(direct_var->array_size));
                    for (size_t i = 0; i < copy_size; i++) {
                        var->struct_members[member.name].multidim_array_values[i] = backup_values[i];
                    }
                    
                    debug_print("SYNC_STRUCT: Initialized multidim_array_values for %s.%s (size: %d, copied: %zu values)\n",
                              var_name.c_str(), member.name.c_str(), direct_var->array_size, copy_size);
                }
                
                // 個別要素変数からデータをコピー
                for (int i = 0; i < direct_var->array_size; i++) {
                    std::string element_name = var_name + "." + member.name + "[" + std::to_string(i) + "]";
                    std::string element_key = member.name + "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    
                    // まず親構造体のstruct_membersから配列要素を探す（構造体配列の場合）
                    auto element_it = var->struct_members.find(element_key);
                    bool found_in_struct_members = (element_it != var->struct_members.end());
                    
                    if (element_var) {
                        if (debug_mode) {
                            debug_print("SYNC_STRUCT: Found element_var for %s: value=%lld, str_value='%s', is_assigned=%d\n",
                                       element_name.c_str(), (long long)element_var->value, 
                                       element_var->str_value.c_str(), element_var->is_assigned ? 1 : 0);
                        }
                        // 構造体配列の場合、配列要素も構造体として同期する
                        if (element_var->is_struct && !element_var->struct_members.empty()) {
                            // 配列要素の構造体を再帰的に同期
                            sync_struct_members_from_direct_access(element_name);
                            
                            // 親構造体のstruct_membersに配列要素を追加
                            var->struct_members[element_key] = *element_var;
                            
                            if (debug_mode) {
                                debug_print("SYNC_STRUCT: Synced struct array element %s (key: %s) with %zu members\n",
                                           element_name.c_str(), element_key.c_str(), element_var->struct_members.size());
                            }
                        } else {
                            // プリミティブ型配列の場合
                            TypeInfo element_base_type = resolve_base_type(element_var->type);
                            if (member_base_type == TYPE_STRING || direct_base_type == TYPE_STRING || element_base_type == TYPE_STRING) {
                                var->struct_members[member.name].array_strings[i] = element_var->str_value;
                                if (debug_mode) {
                                    debug_print("SYNC_STRUCT: Copied string element[%d] = \"%s\" for %s.%s\n",
                                               i, element_var->str_value.c_str(), var_name.c_str(), member.name.c_str());
                                }
                            } else {
                                var->struct_members[member.name].array_values[i] = element_var->value;
                                if (debug_mode) {
                                    debug_print("SYNC_STRUCT: Copied element[%d] = %lld for %s.%s (element_var found)\n",
                                               i, element_var->value, var_name.c_str(), member.name.c_str());
                                }
                                // 多次元配列の場合は multidim_array_values にも設定
                                if (var->struct_members[member.name].is_multidimensional) {
                                    var->struct_members[member.name].multidim_array_values[i] = element_var->value;
                                    if (debug_mode) {
                                        debug_print("SYNC_STRUCT: Copied element[%d] = %lld to multidim_array_values for %s.%s\n", 
                                                  i, element_var->value, var_name.c_str(), member.name.c_str());
                                    }
                                }
                            }
                        }
                    } else if (found_in_struct_members && element_it->second.is_struct) {
                        // 個別変数がないが、struct_membersに構造体配列要素がある場合
                        // (これは既に同期済みの可能性がある)
                        if (debug_mode) {
                            debug_print("SYNC_STRUCT: Struct array element %s already in struct_members\n", element_key.c_str());
                        }
                    } else {
                        // 要素変数が見つからない場合、direct_var自体の配列データを使用
                        if ((member_base_type == TYPE_STRING || direct_base_type == TYPE_STRING) &&
                            i < static_cast<int>(direct_var->array_strings.size())) {
                            var->struct_members[member.name].array_strings[i] = direct_var->array_strings[i];
                        } else if (member_base_type != TYPE_STRING && i < static_cast<int>(direct_var->array_values.size())) {
                            var->struct_members[member.name].array_values[i] = direct_var->array_values[i];
                            if (var->struct_members[member.name].is_multidimensional) {
                                var->struct_members[member.name].multidim_array_values[i] = direct_var->array_values[i];
                            }
                        }
                    }
                }
                
                var->struct_members[member.name].is_assigned = true;
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED, member.name.c_str(), direct_var->array_size);
            } else {
                std::string member_union_alias = member.is_pointer ? member.pointer_base_type_name : member.type_alias;
                bool direct_is_union = type_manager_->is_union_type(*direct_var);
                bool member_is_union = (!member_union_alias.empty() && type_manager_->is_union_type(member_union_alias));

                Variable member_value = *direct_var;

                // メンバー定義由来のメタ情報を優先度高く上書き
                member_value.is_pointer = member.is_pointer;
                member_value.pointer_depth = member.pointer_depth;
                member_value.pointer_base_type_name = member.pointer_base_type_name;
                member_value.pointer_base_type = member.pointer_base_type;
                member_value.is_private_member = member.is_private;
                member_value.is_reference = member.is_reference;
                member_value.is_unsigned = member.is_unsigned;
                member_value.is_const = member.is_const;

                // unionメンバーの場合、型名とcurrent_typeを確実に保持
                if (direct_is_union || member_is_union) {
                    member_value.type = TYPE_UNION;
                    if (!direct_var->type_name.empty()) {
                        member_value.type_name = direct_var->type_name;
                    } else if (!member_union_alias.empty()) {
                        member_value.type_name = member_union_alias;
                    }
                    member_value.current_type = direct_var->current_type;
                } else {
                    // 通常のメンバー型を構造体定義に合わせる
                    member_value.type = member.type;
                }

                // 数値/文字列/浮動小数などの基本値を direct_var に合わせて整合させる
                member_value.value = direct_var->value;
                member_value.str_value = direct_var->str_value;
                member_value.float_value = direct_var->float_value;
                member_value.double_value = direct_var->double_value;
                member_value.quad_value = direct_var->quad_value;
                member_value.big_value = direct_var->big_value;
                member_value.is_assigned = direct_var->is_assigned;

                // 構造体・配列などの複合型もコピー（= *direct_varでコピー済みだが念のため整合）
                member_value.is_array = direct_var->is_array;
                member_value.array_size = direct_var->array_size;
                member_value.array_values = direct_var->array_values;
                member_value.array_strings = direct_var->array_strings;
                member_value.array_dimensions = direct_var->array_dimensions;
                member_value.is_multidimensional = direct_var->is_multidimensional;
                member_value.multidim_array_values = direct_var->multidim_array_values;
                member_value.multidim_array_strings = direct_var->multidim_array_strings;
                member_value.is_struct = direct_var->is_struct;
                member_value.struct_type_name = direct_var->struct_type_name;
                member_value.struct_members = direct_var->struct_members;

                var->struct_members[member.name] = member_value;
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED, member.name.c_str(), member_value.struct_members.size());
            }
        } else {
            // ダイレクトアクセス変数が見つからない場合、配列メンバーかチェック
            if (member.array_info.base_type != TYPE_UNKNOWN) {
                int arr_size = member.array_info.dimensions.empty() ? 0 : member.array_info.dimensions[0].size;
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_ARRAY_MEMBER_ADDED, 
                         member.name.c_str(), (int)member.type, arr_size);
                
                var->struct_members[member.name] = Variable();
                var->struct_members[member.name].type = member.type;
                var->struct_members[member.name].is_array = true;
                auto resolve_base_type = [](TypeInfo type) {
                    if (type >= TYPE_ARRAY_BASE) {
                        return static_cast<TypeInfo>(type - TYPE_ARRAY_BASE);
                    }
                    return type;
                };
                TypeInfo member_base_type = resolve_base_type(member.type);
                
                int array_size = (!member.array_info.dimensions.empty()) ? 
                                member.array_info.dimensions[0].size : 1;
                var->struct_members[member.name].array_size = array_size;
                
                // 多次元配列情報を構造体定義から設定
                if (member.array_info.dimensions.size() > 1) {
                    var->struct_members[member.name].is_multidimensional = true;
                    for (const auto& dim : member.array_info.dimensions) {
                        var->struct_members[member.name].array_dimensions.push_back(dim.size);
                    }
                    debug_print("SYNC_STRUCT: Set multidimensional info for %s.%s from struct definition (dimensions: %zu)\n",
                              var_name.c_str(), member.name.c_str(), member.array_info.dimensions.size());
                }
                
                // 配列要素を個別にチェックして同期
                var->struct_members[member.name].array_values.resize(array_size);
                var->struct_members[member.name].array_strings.resize(array_size);
                
                // 多次元配列の場合は multidim_array_values も初期化（元の値を保持）
                if (var->struct_members[member.name].is_multidimensional) {
                    // 既存のサイズを確認してリサイズが必要な場合のみ実行
                    if (var->struct_members[member.name].multidim_array_values.size() != static_cast<size_t>(array_size)) {
                        var->struct_members[member.name].multidim_array_values.resize(array_size);
                        debug_print("SYNC_STRUCT: Resized multidim_array_values for %s.%s from definition (size: %d)\n",
                                  var_name.c_str(), member.name.c_str(), array_size);
                    }
                }
                
                bool found_elements = false;
                for (int i = 0; i < array_size; i++) {
                    std::string element_name = var_name + "." + member.name + "[" + std::to_string(i) + "]";
                    Variable *element_var = find_variable(element_name);
                    if (element_var) {
                        found_elements = true;
                        if (member_base_type == TYPE_STRING || resolve_base_type(element_var->type) == TYPE_STRING) {
                            var->struct_members[member.name].array_strings[i] = element_var->str_value;
                        } else {
                            var->struct_members[member.name].array_values[i] = element_var->value;
                            // 多次元配列の場合は multidim_array_values にも設定
                            if (var->struct_members[member.name].is_multidimensional) {
                                var->struct_members[member.name].multidim_array_values[i] = element_var->value;
                                if (debug_mode) {
                                    debug_print("SYNC_STRUCT: Copied element[%d] = %lld to multidim_array_values for %s.%s (from definition)\n", 
                                              i, element_var->value, var_name.c_str(), member.name.c_str());
                                }
                            }
                        }
                    }
                }
                
                if (found_elements) {
                    var->struct_members[member.name].is_assigned = true;
                    debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED, member.name.c_str(), (size_t)array_size);
                }
            }
        }
    }
    
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_END, var_name.c_str());
}

void Interpreter::sync_direct_access_from_struct_value(const std::string &var_name,
                                                       const Variable &struct_value) {
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START, var_name.c_str());

    std::map<std::string, Variable> *target_map = nullptr;
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
        if (it->variables.find(var_name) != it->variables.end()) {
            target_map = &it->variables;
            break;
        }
    }

    if (!target_map) {
        auto global_it = global_scope.variables.find(var_name);
        if (global_it != global_scope.variables.end()) {
            target_map = &global_scope.variables;
        }
    }

    if (!target_map) {
        auto static_it = static_variables.find(var_name);
        if (static_it != static_variables.end()) {
            target_map = &static_variables;
        }
    }

    if (!target_map) {
        target_map = &current_scope().variables;
    }

    // ルートの構造体変数を最新の値で更新
    (*target_map)[var_name] = struct_value;
    Variable &root_var = (*target_map)[var_name];
    root_var.is_assigned = true;
    root_var.is_struct = true;

    if (debug_mode) {
        debug_print("DIRECT_SYNC: updating %s with %zu members\n",
                    var_name.c_str(), struct_value.struct_members.size());
    }

    std::function<void(std::map<std::string, Variable> &, const std::string &, const Variable &)> copy_members;
    copy_members = [&](std::map<std::string, Variable> &vars, const std::string &base_name, const Variable &source) {
        for (const auto &member_pair : source.struct_members) {
            const std::string &member_name = member_pair.first;
            const Variable &member_value = member_pair.second;
            std::string qualified_name = base_name + "." + member_name;

            vars[qualified_name] = member_value;
            Variable &dest_member = vars[qualified_name];
            dest_member.is_assigned = true;

            if (debug_mode) {
                debug_print("DIRECT_SYNC_MEMBER: %s value=%lld str='%s' type=%d current_type=%d\n",
                            qualified_name.c_str(),
                            static_cast<long long>(member_value.value),
                            member_value.str_value.c_str(),
                            static_cast<int>(member_value.type),
                            static_cast<int>(member_value.current_type));
            }

            if (member_value.is_array || member_value.type >= TYPE_ARRAY_BASE || member_value.is_multidimensional) {
                dest_member.array_size = member_value.array_size;
                dest_member.array_dimensions = member_value.array_dimensions;
                dest_member.array_values = member_value.array_values;
                dest_member.array_strings = member_value.array_strings;
                dest_member.multidim_array_values = member_value.multidim_array_values;
                dest_member.multidim_array_strings = member_value.multidim_array_strings;
                dest_member.is_array = member_value.is_array;
                dest_member.is_multidimensional = member_value.is_multidimensional;

                int total_size = 0;
                if (!member_value.array_dimensions.empty()) {
                    total_size = 1;
                    for (int dim : member_value.array_dimensions) {
                        total_size *= dim;
                    }
                } else if (member_value.is_multidimensional) {
                    total_size = static_cast<int>(member_value.multidim_array_values.size());
                    if (total_size == 0) {
                        total_size = static_cast<int>(member_value.multidim_array_strings.size());
                    }
                } else if (!member_value.array_values.empty() || !member_value.array_strings.empty()) {
                    total_size = static_cast<int>(
                        std::max(member_value.array_values.size(), member_value.array_strings.size()));
                }

                if (total_size == 0) {
                    total_size = member_value.array_size;
                }

                if (total_size < 0) {
                    total_size = 0;
                }

                for (int i = 0; i < total_size; ++i) {
                    std::string element_name = qualified_name + "[" + std::to_string(i) + "]";
                    Variable element_var;
                    element_var.is_assigned = true;
                    element_var.is_const = dest_member.is_const;
                    element_var.is_unsigned = dest_member.is_unsigned;

                    bool treat_as_string = (member_value.type == TYPE_STRING ||
                                            member_value.current_type == TYPE_STRING ||
                                            !member_value.array_strings.empty() ||
                                            !member_value.multidim_array_strings.empty());

                    if (treat_as_string) {
                        element_var.type = TYPE_STRING;
                        if (i < static_cast<int>(member_value.array_strings.size())) {
                            element_var.str_value = member_value.array_strings[i];
                        } else if (i < static_cast<int>(member_value.multidim_array_strings.size())) {
                            element_var.str_value = member_value.multidim_array_strings[i];
                        } else {
                            element_var.str_value = "";
                        }
                    } else {
                        element_var.type = member_value.type;
                        if (element_var.type >= TYPE_ARRAY_BASE) {
                            element_var.type = static_cast<TypeInfo>(element_var.type - TYPE_ARRAY_BASE);
                        }
                        int64_t numeric_value = 0;
                        if (member_value.is_multidimensional &&
                            i < static_cast<int>(member_value.multidim_array_values.size())) {
                            numeric_value = member_value.multidim_array_values[i];
                        } else if (i < static_cast<int>(member_value.array_values.size())) {
                            numeric_value = member_value.array_values[i];
                        }
                        element_var.value = numeric_value;
                    }

                    vars[element_name] = element_var;
                    if (debug_mode) {
                        if (element_var.type == TYPE_STRING) {
                            debug_print("DIRECT_SYNC_ARRAY_ELEM: %s str='%s'\n",
                                        element_name.c_str(), element_var.str_value.c_str());
                        } else {
                            debug_print("DIRECT_SYNC_ARRAY_ELEM: %s value=%lld\n",
                                        element_name.c_str(),
                                        static_cast<long long>(element_var.value));
                        }
                    }
                }
            }

            if (member_value.is_struct && !member_value.struct_members.empty()) {
                copy_members(vars, qualified_name, member_value);
            }
        }
    };

    copy_members(*target_map, var_name, struct_value);

    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_END, var_name.c_str());
}

// interface管理メソッド
void Interpreter::register_interface_definition(const std::string &interface_name,
                                               const InterfaceDefinition &definition) {
    interface_definitions_[interface_name] = definition;
    debug_msg(DebugMsgId::PARSE_STRUCT_DEF, interface_name.c_str());
}

const InterfaceDefinition *
Interpreter::find_interface_definition(const std::string &interface_name) {
    auto it = interface_definitions_.find(interface_name);
    if (it != interface_definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

// impl管理メソッド
void Interpreter::register_impl_definition(const ImplDefinition &impl_def) {
    auto trim = [](const std::string &text) {
        const char *whitespace = " \t\r\n";
        size_t start = text.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return std::string();
        }
        size_t end = text.find_last_not_of(whitespace);
        return text.substr(start, end - start + 1);
    };

    auto normalize_struct = [](const std::string &name) {
        const std::string prefix = "struct ";
        if (name.rfind(prefix, 0) == 0) {
            return name.substr(prefix.size());
        }
        return name;
    };

    ImplDefinition stored_def(trim(impl_def.interface_name), trim(impl_def.struct_name));
    stored_def.methods = impl_def.methods;

    auto existing = std::find_if(impl_definitions_.begin(), impl_definitions_.end(),
        [&](const ImplDefinition &candidate) {
            return candidate.interface_name == stored_def.interface_name &&
                   candidate.struct_name == stored_def.struct_name;
        });

    if (existing != impl_definitions_.end()) {
        *existing = stored_def;
    } else {
        impl_definitions_.emplace_back(stored_def);
        existing = std::prev(impl_definitions_.end());
    }

    auto register_function = [&](const std::string &key, const ASTNode *method) {
        if (key.empty() || !method) {
            return;
        }
    global_scope.functions[key] = method;
    debug_print("IMPL_REGISTER: Registered method key '%s'\n", key.c_str());
    };

    std::string normalized_struct_name = normalize_struct(existing->struct_name);
    std::string original_struct_name = existing->struct_name;
    std::string interface_name = existing->interface_name;

    for (const auto *method : existing->methods) {
        if (!method) {
            continue;
        }

        std::string method_name = method->name;

        if (!normalized_struct_name.empty()) {
            register_function(normalized_struct_name + "::" + method_name, method);
        }

        if (!original_struct_name.empty() && original_struct_name != normalized_struct_name) {
            register_function(original_struct_name + "::" + method_name, method);
        }

        if (!interface_name.empty()) {
            std::string interface_key = interface_name + "_" + normalized_struct_name + "_" + method_name;
            register_function(interface_key, method);

            if (!original_struct_name.empty() && original_struct_name != normalized_struct_name) {
                register_function(interface_name + "_" + original_struct_name + "_" + method_name, method);
            }
        }
    }

    debug_msg(DebugMsgId::PARSE_STRUCT_DEF,
              (existing->interface_name + "_for_" + existing->struct_name).c_str());
}

const ImplDefinition *Interpreter::find_impl_for_struct(const std::string &struct_name, 
                                                       const std::string &interface_name) {
    for (const auto &impl_def : impl_definitions_) {
        if (impl_def.struct_name == struct_name && impl_def.interface_name == interface_name) {
            return &impl_def;
        }
    }
    return nullptr;
}

// interface型変数管理
void Interpreter::create_interface_variable(const std::string &var_name, 
                                          const std::string &interface_name) {
    Variable var(interface_name, true); // interface用コンストラクタを使用
    var.is_assigned = false;
    
    current_scope().variables[var_name] = var; // add_variableの代わりに直接設定
    debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(), interface_name.c_str());
}

Variable *Interpreter::get_interface_variable(const std::string &var_name) {
    Variable *var = find_variable(var_name);
    if (var && var->type == TYPE_INTERFACE) {
        return var;
    }
    return nullptr;
}

// self処理用のヘルパー関数実装
std::string Interpreter::get_self_receiver_path() {
    // デバッグモードの場合、self_receiver_pathを取得
    // 現在は簡単な実装として、最初に見つかったself以外の構造体変数を返す
    for (auto& scope : scope_stack) {
        for (auto& [name, var] : scope.variables) {
            if (name != "self" && var.is_struct && var.is_assigned) {
                debug_print("SELF_RECEIVER_DEBUG: Found receiver path: %s\n", name.c_str());
                return name;
            }
        }
    }
    
    // グローバルスコープもチェック
    for (auto& [name, var] : global_scope.variables) {
        if (name != "self" && var.is_struct && var.is_assigned) {
            debug_print("SELF_RECEIVER_DEBUG: Found global receiver path: %s\n", name.c_str());
            return name;
        }
    }
    
    debug_print("SELF_RECEIVER_DEBUG: No receiver path found\n");
    return "";
}

void Interpreter::sync_self_to_receiver(const std::string& receiver_path) {
    Variable* self_var = find_variable("self");
    Variable* receiver_var = find_variable(receiver_path);
    
    if (!self_var || !receiver_var) {
        debug_print("SYNC_SELF_DEBUG: Variables not found: self=%p, receiver=%p\n", 
                   (void*)self_var, (void*)receiver_var);
        return;
    }
    
    debug_print("SYNC_SELF_DEBUG: Syncing self to %s\n", receiver_path.c_str());
    
    // self.memberからreceiver.memberに値をコピー
    for (auto& [member_name, self_member] : self_var->struct_members) {
        std::string receiver_member_name = receiver_path + "." + member_name;
        Variable* receiver_member = find_variable(receiver_member_name);
        
        if (receiver_member) {
            if (self_member.type == TYPE_STRING) {
                receiver_member->str_value = self_member.str_value;
            } else {
                receiver_member->value = self_member.value;
            }
            receiver_member->is_assigned = self_member.is_assigned;
            
            // receiver構造体のstruct_membersも更新
            if (receiver_var->struct_members.find(member_name) != receiver_var->struct_members.end()) {
                receiver_var->struct_members[member_name] = self_member;
            }
            
            debug_print("SYNC_SELF_DEBUG: Synced %s to %s\n", 
                       ("self." + member_name).c_str(), receiver_member_name.c_str());
        }
    }
}

// 関数定義の検索
const ASTNode* Interpreter::find_function_definition(const std::string& func_name) {
    return find_function(func_name);
}

TypedValue Interpreter::evaluate_ternary_typed(const ASTNode* node) {
    return expression_evaluator_->evaluate_ternary_typed(node);
}

// 一時変数管理（メソッドチェーン用）
void Interpreter::add_temp_variable(const std::string &name, const Variable &var) {
    current_scope().variables[name] = var;
    debug_print("TEMP_VAR: Added temporary variable %s\n", name.c_str());
}

void Interpreter::remove_temp_variable(const std::string &name) {
    auto& vars = current_scope().variables;
    auto it = vars.find(name);
    if (it != vars.end()) {
        vars.erase(it);
        debug_print("TEMP_VAR: Removed temporary variable %s\n", name.c_str());
    }
}

void Interpreter::clear_temp_variables() {
    auto& vars = current_scope().variables;
    for (auto it = vars.begin(); it != vars.end();) {
        if (it->first.substr(0, 12) == "__temp_chain" || 
            it->first.substr(0, 12) == "__chain_self") {
            debug_print("TEMP_VAR: Clearing temporary variable %s\n", it->first.c_str());
            it = vars.erase(it);
        } else {
            ++it;
        }
    }
}

// 型定義検索メソッド
const ASTNode* Interpreter::find_union_definition(const std::string &union_name) {
    // ユニオン定義をマップから検索（簡易実装）
    // 実際の実装では union_definitions_ マップを使用
    return nullptr; // 簡易実装：ユニオンサポートは後で実装
}

const ASTNode* Interpreter::find_typedef_definition(const std::string &typedef_name) {
    // typedef定義をマップから検索（簡易実装）
    // 実際の実装では typedef_definitions_ マップを使用
    return nullptr; // 簡易実装：typedefサポートは後で実装
}
