#include "handlers/interface_declaration_handler.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "core/interpreter.h"

InterfaceDeclarationHandler::InterfaceDeclarationHandler(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void InterfaceDeclarationHandler::handle_interface_declaration(const ASTNode *node) {
    std::string interface_name = node->name;
    debug_msg(DebugMsgId::INTERFACE_DECL_START, interface_name.c_str());

    InterfaceDefinition interface_def(interface_name);

    // ASTノードからinterface定義を復元
    for (const auto &method_node : node->arguments) {
        if (method_node->node_type == ASTNodeType::AST_FUNC_DECL) {
            InterfaceMember method(method_node->name,
                                   method_node->type_info,
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
            debug_msg(DebugMsgId::INTERFACE_METHOD_FOUND,
                      method_node->name.c_str());
        }
    }

    interpreter_->register_interface_definition(interface_name, interface_def);
    debug_msg(DebugMsgId::INTERFACE_DECL_COMPLETE, interface_name.c_str());
}
