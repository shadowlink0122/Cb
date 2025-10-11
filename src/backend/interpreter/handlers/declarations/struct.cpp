#include "struct.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../core/interpreter.h"

StructDeclarationHandler::StructDeclarationHandler(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StructDeclarationHandler::handle_struct_declaration(const ASTNode *node) {
    debug_msg(DebugMsgId::PARSE_STRUCT_DEF, node->name.c_str());

    std::cerr << "[HANDLE_STRUCT_ENTRY] Processing struct: " << node->name
              << std::endl;

    std::string struct_name = node->name;
    StructDefinition struct_def(struct_name);

    // ASTノードからstruct定義を復元
    for (const auto &member_node : node->arguments) {
        if (member_node->node_type == ASTNodeType::AST_VAR_DECL) {
            struct_def.add_member(
                member_node->name, member_node->type_info,
                member_node->type_name, member_node->is_pointer,
                member_node->pointer_depth, member_node->pointer_base_type_name,
                member_node->pointer_base_type, member_node->is_private_member);

            // デフォルトメンバーの復元
            if (member_node->is_default_member) {
                StructMember &added_member = struct_def.members.back();
                added_member.is_default = true;
            }

            debug_msg(DebugMsgId::PARSE_VAR_DECL, member_node->name.c_str(),
                      member_node->type_name.c_str());
        }
    }

    // デフォルトメンバー情報を設定
    for (const auto &member : struct_def.members) {
        if (interpreter_->is_debug_mode()) {
            std::cerr << "[HANDLE_STRUCT] Member " << member.name
                      << ": is_default=" << member.is_default << std::endl;
        }
        if (member.is_default) {
            struct_def.has_default_member = true;
            struct_def.default_member_name = member.name;
            if (interpreter_->is_debug_mode()) {
                std::cerr << "[HANDLE_STRUCT] Set default member: "
                          << member.name << std::endl;
            }
            break; // 1つだけのはず
        }
    }

    if (interpreter_->is_debug_mode()) {
        std::cerr << "[HANDLE_STRUCT] Final: has_default_member="
                  << struct_def.has_default_member
                  << ", default_member_name=" << struct_def.default_member_name
                  << std::endl;
    }

    interpreter_->register_struct_definition(struct_name, struct_def);
    debug_msg(DebugMsgId::PARSE_STRUCT_DEF, struct_name.c_str());
}
