#include "interface_operations.h"
#include "../../../common/ast.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include <algorithm>

InterfaceOperations::InterfaceOperations(Interpreter *interpreter)
    : interpreter_(interpreter) {}

// ========================================================================
// Interface定義管理
// ========================================================================

void InterfaceOperations::register_interface_definition(
    const std::string &interface_name, const InterfaceDefinition &definition) {
    interface_definitions_[interface_name] = definition;
    debug_msg(DebugMsgId::PARSE_STRUCT_DEF, interface_name.c_str());
}

const InterfaceDefinition *InterfaceOperations::find_interface_definition(
    const std::string &interface_name) {
    auto it = interface_definitions_.find(interface_name);
    if (it != interface_definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ========================================================================
// Impl定義管理
// ========================================================================

void InterfaceOperations::register_impl_definition(
    const ImplDefinition &impl_def) {
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

    ImplDefinition stored_def(trim(impl_def.interface_name),
                              trim(impl_def.struct_name));
    stored_def.methods = impl_def.methods;

    auto existing = std::find_if(
        impl_definitions_.begin(), impl_definitions_.end(),
        [&](const ImplDefinition &candidate) {
            return candidate.interface_name == stored_def.interface_name &&
                   candidate.struct_name == stored_def.struct_name;
        });

    if (existing != impl_definitions_.end()) {
        *existing = stored_def;
        debug_print(
            "IMPL_DEF_STORAGE: Updated existing impl '%s' for '%s' (addr=%p)\n",
            stored_def.interface_name.c_str(), stored_def.struct_name.c_str(),
            (void *)&impl_definitions_);
    } else {
        impl_definitions_.emplace_back(stored_def);
        existing = std::prev(impl_definitions_.end());
        debug_print("IMPL_DEF_STORAGE: Added new impl '%s' for '%s' (total: "
                    "%zu, addr=%p)\n",
                    stored_def.interface_name.c_str(),
                    stored_def.struct_name.c_str(), impl_definitions_.size(),
                    (void *)&impl_definitions_);
    }

    auto register_function = [&](const std::string &key,
                                 const ASTNode *method) {
        if (key.empty() || !method) {
            return;
        }
        interpreter_->register_function_to_global(key, method);
        debug_print("IMPL_REGISTER: Registered method key '%s'\n", key.c_str());
    };

    std::string normalized_struct_name =
        normalize_struct(existing->struct_name);
    std::string original_struct_name = existing->struct_name;
    std::string interface_name = existing->interface_name;

    for (const auto *method : existing->methods) {
        if (!method) {
            continue;
        }

        std::string method_name = method->name;

        if (!normalized_struct_name.empty()) {
            register_function(normalized_struct_name + "::" + method_name,
                              method);
        }

        if (!original_struct_name.empty() &&
            original_struct_name != normalized_struct_name) {
            register_function(original_struct_name + "::" + method_name,
                              method);
        }

        if (!interface_name.empty()) {
            std::string interface_key = interface_name + "_" +
                                        normalized_struct_name + "_" +
                                        method_name;
            register_function(interface_key, method);

            if (!original_struct_name.empty() &&
                original_struct_name != normalized_struct_name) {
                register_function(interface_name + "_" + original_struct_name +
                                      "_" + method_name,
                                  method);
            }
        }
    }

    debug_msg(
        DebugMsgId::PARSE_STRUCT_DEF,
        (existing->interface_name + "_for_" + existing->struct_name).c_str());

    debug_print("IMPL_DEF_END: Finishing register_impl_definition, "
                "impl_definitions_.size()=%zu\n",
                impl_definitions_.size());
}

const std::vector<ImplDefinition> &
InterfaceOperations::get_impl_definitions() const {
    debug_print("GET_IMPL_DEFS: Called! size=%zu, addr=%p\n",
                impl_definitions_.size(), (void *)&impl_definitions_);
    return impl_definitions_;
}

const ImplDefinition *
InterfaceOperations::find_impl_for_struct(const std::string &struct_name,
                                          const std::string &interface_name) {
    for (const auto &impl_def : impl_definitions_) {
        if (impl_def.struct_name == struct_name &&
            impl_def.interface_name == interface_name) {
            return &impl_def;
        }
    }
    return nullptr;
}

// ========================================================================
// Interface型変数管理
// ========================================================================

void InterfaceOperations::create_interface_variable(
    const std::string &var_name, const std::string &interface_name) {
    Variable var(interface_name, true); // interface用コンストラクタを使用
    var.is_assigned = false;

    interpreter_->add_variable_to_current_scope(var_name, var);
    debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(),
              interface_name.c_str());
}

Variable *
InterfaceOperations::get_interface_variable(const std::string &var_name) {
    Variable *var = interpreter_->find_variable(var_name);
    if (var && var->type == TYPE_INTERFACE) {
        return var;
    }
    return nullptr;
}

// ========================================================================
// Impl宣言処理
// ========================================================================

void InterfaceOperations::handle_impl_declaration(const ASTNode *node) {
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
        debug_msg(
            DebugMsgId::PARSE_STRUCT_DEF,
            ("Skipping impl registration due to missing interface name: " +
             node->name)
                .c_str());
        return;
    }

    ImplDefinition impl_def(interface_name, struct_name);

    // impl static変数の登録（implコンテキストは一時的に設定）
    for (const auto &static_var_node : node->impl_static_variables) {
        if (!static_var_node ||
            static_var_node->node_type != ASTNodeType::AST_VAR_DECL) {
            continue;
        }
        // 各static変数登録時に一時的にコンテキストを設定
        interpreter_->enter_impl_context(interface_name, struct_name);
        interpreter_->create_impl_static_variable(static_var_node->name,
                                                  static_var_node.get());
        interpreter_->exit_impl_context();
    }

    // メソッドの登録
    for (const auto &method_node : node->arguments) {
        if (!method_node ||
            method_node->node_type != ASTNodeType::AST_FUNC_DECL) {
            continue;
        }

        if (method_node->type_name.empty()) {
            method_node->type_name = struct_name;
        }
        method_node->qualified_name =
            interface_name + "::" + struct_name + "::" + method_node->name;

        impl_def.add_method(method_node.get());
    }

    register_impl_definition(impl_def);
}

// ========================================================================
// Self処理用ヘルパー
// ========================================================================

std::string InterfaceOperations::get_self_receiver_path() {
    // デバッグモードの場合、self_receiver_pathを取得
    // 現在は簡単な実装として、最初に見つかったself以外の構造体変数を返す

    // ローカルスコープから検索
    auto &scope_stack = interpreter_->get_scope_stack();
    for (auto &scope : scope_stack) {
        for (auto &[name, var] : scope.variables) {
            if (name != "self" && var.is_struct && var.is_assigned) {
                debug_print("SELF_RECEIVER_DEBUG: Found receiver path: %s\n",
                            name.c_str());
                return name;
            }
        }
    }

    // グローバルスコープもチェック
    auto &global_scope = interpreter_->get_global_scope();
    for (auto &[name, var] : global_scope.variables) {
        if (name != "self" && var.is_struct && var.is_assigned) {
            debug_print("SELF_RECEIVER_DEBUG: Found global receiver path: %s\n",
                        name.c_str());
            return name;
        }
    }

    debug_print("SELF_RECEIVER_DEBUG: No receiver path found\n");
    return "";
}

void InterfaceOperations::sync_self_to_receiver(
    const std::string &receiver_path) {
    Variable *self_var = interpreter_->find_variable("self");
    Variable *receiver_var = interpreter_->find_variable(receiver_path);

    if (!self_var || !receiver_var) {
        debug_print(
            "SYNC_SELF_DEBUG: Variables not found: self=%p, receiver=%p\n",
            (void *)self_var, (void *)receiver_var);
        return;
    }

    debug_print("SYNC_SELF_DEBUG: Syncing self to %s\n", receiver_path.c_str());

    // self.memberからreceiver.memberに値をコピー
    for (auto &[member_name, self_member] : self_var->struct_members) {
        std::string receiver_member_name = receiver_path + "." + member_name;
        Variable *receiver_member =
            interpreter_->find_variable(receiver_member_name);

        if (receiver_member) {
            if (self_member.type == TYPE_STRING) {
                receiver_member->str_value = self_member.str_value;
            } else {
                receiver_member->value = self_member.value;
            }
            receiver_member->is_assigned = self_member.is_assigned;

            // receiver構造体のstruct_membersも更新
            if (receiver_var->struct_members.find(member_name) !=
                receiver_var->struct_members.end()) {
                receiver_var->struct_members[member_name] = self_member;
            }

            debug_print("SYNC_SELF_DEBUG: Synced %s to %s\n",
                        ("self." + member_name).c_str(),
                        receiver_member_name.c_str());
        }
    }
}

// ========================================================================
// 一時変数管理（メソッドチェーン用）
// ========================================================================

void InterfaceOperations::add_temp_variable(const std::string &name,
                                            const Variable &var) {
    interpreter_->add_variable_to_current_scope(name, var);
    debug_print("TEMP_VAR: Added temporary variable %s\n", name.c_str());
}

void InterfaceOperations::remove_temp_variable(const std::string &name) {
    auto &current_scope = interpreter_->current_scope();
    auto &vars = current_scope.variables;
    auto it = vars.find(name);
    if (it != vars.end()) {
        vars.erase(it);
        debug_print("TEMP_VAR: Removed temporary variable %s\n", name.c_str());
    }
}

void InterfaceOperations::clear_temp_variables() {
    auto &current_scope = interpreter_->current_scope();
    auto &vars = current_scope.variables;
    for (auto it = vars.begin(); it != vars.end();) {
        if (it->first.substr(0, 12) == "__temp_chain" ||
            it->first.substr(0, 12) == "__chain_self") {
            debug_print("TEMP_VAR: Clearing temporary variable %s\n",
                        it->first.c_str());
            it = vars.erase(it);
        } else {
            ++it;
        }
    }
}
