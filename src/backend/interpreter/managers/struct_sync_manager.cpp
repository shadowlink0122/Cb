#include "struct_sync_manager.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include "static_variable_manager.h"
#include <functional>

StructSyncManager::StructSyncManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StructSyncManager::sync_direct_access_from_struct_value(
    const std::string &var_name, const Variable &struct_value) {
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START,
              var_name.c_str());

    std::map<std::string, Variable> *target_map = nullptr;
    for (auto it = interpreter_->scope_stack.rbegin();
         it != interpreter_->scope_stack.rend(); ++it) {
        if (it->variables.find(var_name) != it->variables.end()) {
            target_map = &it->variables;
            break;
        }
    }

    if (!target_map) {
        auto global_it = interpreter_->global_scope.variables.find(var_name);
        if (global_it != interpreter_->global_scope.variables.end()) {
            target_map = &interpreter_->global_scope.variables;
        }
    }

    if (!target_map) {
        auto *static_vars =
            interpreter_->static_variable_manager_
                ->get_static_variables_mutable();
        auto static_it = static_vars->find(var_name);
        if (static_it != static_vars->end()) {
            target_map = static_vars;
        }
    }

    if (!target_map) {
        target_map = &interpreter_->current_scope().variables;
    }

    // ルートの構造体変数を最新の値で更新
    (*target_map)[var_name] = struct_value;
    Variable &root_var = (*target_map)[var_name];
    root_var.is_assigned = true;
    root_var.is_struct = true;

    if (interpreter_->debug_mode) {
        debug_print("DIRECT_SYNC: updating %s with %zu members\n",
                    var_name.c_str(), struct_value.struct_members.size());
    }

    std::function<void(std::map<std::string, Variable> &, const std::string &,
                       const Variable &)>
        copy_members;
    copy_members = [&](std::map<std::string, Variable> &vars,
                       const std::string &base_name, const Variable &source) {
        for (const auto &member_pair : source.struct_members) {
            const std::string &member_name = member_pair.first;
            const Variable &member_value = member_pair.second;
            std::string qualified_name = base_name + "." + member_name;

            vars[qualified_name] = member_value;
            Variable &dest_member = vars[qualified_name];
            dest_member.is_assigned = true;

            if (interpreter_->debug_mode) {
                debug_print("DIRECT_SYNC_MEMBER: %s value=%lld str='%s' "
                            "type=%d current_type=%d\n",
                            qualified_name.c_str(),
                            static_cast<long long>(member_value.value),
                            member_value.str_value.c_str(),
                            static_cast<int>(member_value.type),
                            static_cast<int>(member_value.current_type));
            }

            if (member_value.is_array || member_value.type >= TYPE_ARRAY_BASE ||
                member_value.is_multidimensional) {
                dest_member.array_size = member_value.array_size;
                dest_member.array_dimensions = member_value.array_dimensions;
                dest_member.array_values = member_value.array_values;
                dest_member.array_strings = member_value.array_strings;
                dest_member.multidim_array_values =
                    member_value.multidim_array_values;
                dest_member.multidim_array_strings =
                    member_value.multidim_array_strings;
                dest_member.is_array = member_value.is_array;
                dest_member.is_multidimensional =
                    member_value.is_multidimensional;

                int total_size = 0;
                if (!member_value.array_dimensions.empty()) {
                    total_size = 1;
                    for (int dim : member_value.array_dimensions) {
                        total_size *= dim;
                    }
                } else if (member_value.is_multidimensional) {
                    total_size = static_cast<int>(
                        member_value.multidim_array_values.size());
                    if (total_size == 0) {
                        total_size = static_cast<int>(
                            member_value.multidim_array_strings.size());
                    }
                } else if (!member_value.array_values.empty() ||
                           !member_value.array_strings.empty()) {
                    total_size = static_cast<int>(
                        std::max(member_value.array_values.size(),
                                 member_value.array_strings.size()));
                }

                if (total_size == 0) {
                    total_size = member_value.array_size;
                }

                if (total_size < 0) {
                    total_size = 0;
                }

                for (int i = 0; i < total_size; ++i) {
                    std::string element_name =
                        qualified_name + "[" + std::to_string(i) + "]";
                    Variable element_var;
                    element_var.is_assigned = true;
                    element_var.is_const = dest_member.is_const;
                    element_var.is_unsigned = dest_member.is_unsigned;

                    bool treat_as_string =
                        (member_value.type == TYPE_STRING ||
                         member_value.current_type == TYPE_STRING ||
                         !member_value.array_strings.empty() ||
                         !member_value.multidim_array_strings.empty());

                    if (treat_as_string) {
                        element_var.type = TYPE_STRING;
                        if (i < static_cast<int>(
                                    member_value.array_strings.size())) {
                            element_var.str_value =
                                member_value.array_strings[i];
                        } else if (i < static_cast<int>(
                                           member_value.multidim_array_strings
                                               .size())) {
                            element_var.str_value =
                                member_value.multidim_array_strings[i];
                        } else {
                            element_var.str_value = "";
                        }
                    } else {
                        element_var.type = member_value.type;
                        if (element_var.type >= TYPE_ARRAY_BASE) {
                            element_var.type = static_cast<TypeInfo>(
                                element_var.type - TYPE_ARRAY_BASE);
                        }
                        int64_t numeric_value = 0;
                        if (member_value.is_multidimensional &&
                            i < static_cast<int>(
                                    member_value.multidim_array_values
                                        .size())) {
                            numeric_value =
                                member_value.multidim_array_values[i];
                        } else if (i < static_cast<int>(
                                           member_value.array_values.size())) {
                            numeric_value = member_value.array_values[i];
                        }
                        element_var.value = numeric_value;
                    }

                    vars[element_name] = element_var;
                    if (interpreter_->debug_mode) {
                        if (element_var.type == TYPE_STRING) {
                            debug_print("DIRECT_SYNC_ARRAY_ELEM: %s str='%s'\n",
                                        element_name.c_str(),
                                        element_var.str_value.c_str());
                        } else {
                            debug_print(
                                "DIRECT_SYNC_ARRAY_ELEM: %s value=%lld\n",
                                element_name.c_str(),
                                static_cast<long long>(element_var.value));
                        }
                    }
                }
            }

            if (member_value.is_struct &&
                !member_value.struct_members.empty()) {
                copy_members(vars, qualified_name, member_value);
            }
        }
    };

    copy_members(*target_map, var_name, struct_value);

    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_END,
              var_name.c_str());
}
