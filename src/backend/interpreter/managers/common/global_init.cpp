#include "global_init.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../frontend/recursive_parser/recursive_parser.h"
#include "../../core/interpreter.h"
#include "managers/types/enums.h"
#include "managers/variables/manager.h"

GlobalInitializationManager::GlobalInitializationManager(
    Interpreter *interpreter)
    : interpreter_(interpreter) {}

void GlobalInitializationManager::initialize_global_variables(
    const ASTNode *node) {
    if (!node)
        return;

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        if (node->statements.size() > 0) {
            debug_msg(DebugMsgId::INTERPRETER_PROCESSING_STMT_LIST,
                      node->statements.size());
        }

        // 2パス初期化: まずconst変数を初期化し、その後他の変数を初期化
        // 第1パス:
        // const変数のみ初期化（配列サイズ式で使用される可能性があるため）
        for (const auto &stmt : node->statements) {
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL &&
                stmt->is_const && !stmt->is_array) {
                debug_msg(DebugMsgId::INTERPRETER_FOUND_VAR_DECL,
                          stmt->name.c_str());
                initialize_global_variables(stmt.get());
            }
        }

        // 第2パス: 配列とその他の変数を初期化
        for (const auto &stmt : node->statements) {
            debug_msg(DebugMsgId::INTERPRETER_CHECKING_STATEMENT_TYPE,
                      (int)stmt->node_type, stmt->name.c_str());
            if (stmt->node_type == ASTNodeType::AST_VAR_DECL) {
                // 第1パスで既に初期化されたconst変数をスキップ
                if (stmt->is_const && !stmt->is_array) {
                    continue;
                }
                debug_msg(DebugMsgId::INTERPRETER_FOUND_VAR_DECL,
                          stmt->name.c_str());
                initialize_global_variables(stmt.get());
            }
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Initializing global variable: %s",
                         node->name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }

        // グローバル変数を作成・初期化
        interpreter_->variable_manager_->process_var_decl_or_assign(node);

        // 変数が正しく作成されたか確認
        if (interpreter_->debug_mode) {
            Variable *created_var = interpreter_->find_variable(node->name);
            if (created_var) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "Global variable %s created successfully: ");
            } else {
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "ERROR: Global variable %s creation failed",
                             node->name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
        }
        break;

    default:
        break;
    }
}

void GlobalInitializationManager::sync_enum_definitions_from_parser(
    RecursiveParser *parser) {
    if (!parser)
        return;

    auto &parser_enums = parser->get_enum_definitions();
    for (const auto &pair : parser_enums) {
        const std::string &enum_name = pair.first;
        const EnumDefinition &enum_def = pair.second;

        // EnumManagerに登録
        interpreter_->enum_manager_->register_enum(enum_name, enum_def);

        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Synced enum definition: %s with %zu members",
                         enum_name.c_str(), enum_def.members.size());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }
}
