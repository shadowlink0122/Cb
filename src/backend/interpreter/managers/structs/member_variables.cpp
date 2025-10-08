#include "managers/structs/member_variables.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../core/interpreter.h"
#include "managers/structs/operations.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include <stdexcept>
#include <string>

StructVariableManager::StructVariableManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StructVariableManager::create_struct_variable(
    const std::string &var_name, const std::string &struct_type_name) {
    if (interpreter_->is_debug_mode()) {
        debug_print(
            "create_struct_variable called: var_name=%s, struct_type=%s\n",
            var_name.c_str(), struct_type_name.c_str());
    }

    const StructDefinition *struct_def = interpreter_->find_struct_definition(
        interpreter_->get_type_manager()->resolve_typedef(struct_type_name));
    if (!struct_def) {
        throw std::runtime_error("Struct type not found: " + struct_type_name);
    }

    Variable struct_var;
    struct_var.type = TYPE_STRUCT;
    struct_var.is_struct = true;
    struct_var.struct_type_name = struct_type_name;
    struct_var.is_assigned = false;
    struct_var.struct_members.clear();

    // メンバ変数を初期化
    for (const auto &member : struct_def->members) {
        if (interpreter_->is_debug_mode()) {
            debug_print("Processing member: %s, is_array: %d\n",
                        member.name.c_str(), member.array_info.is_array());
        }

        if (member.array_info.is_array()) {
            if (interpreter_->is_debug_mode()) {
                debug_print("Member %s is an array with %zu dimensions\n",
                            member.name.c_str(),
                            member.array_info.dimensions.size());
            }

            // 多次元配列の処理
            if (member.array_info.dimensions.size() > 1) {
                process_multidimensional_array_member(var_name, member,
                                                      struct_var);
            } else {
                process_1d_array_member(var_name, member, struct_var);
            }
        } else {
            process_regular_member(var_name, member, struct_var);
        }
    }

    // 変数を登録
    interpreter_->current_scope().variables[var_name] = struct_var;

    // 構造体配列メンバーの要素を再度追加（変数登録後に行う）
    post_process_array_elements(var_name, struct_def);
}

void StructVariableManager::create_struct_member_variables_recursively(
    const std::string &base_path, const std::string &struct_type_name,
    Variable &parent_var) {
    // 構造体定義を取得
    std::string resolved_type =
        interpreter_->get_type_manager()->resolve_typedef(struct_type_name);
    const StructDefinition *struct_def =
        interpreter_->find_struct_definition(resolved_type);

    if (!struct_def) {
        return;
    }

    // 各メンバーに対して個別変数を作成
    for (const auto &member_def : struct_def->members) {
        std::string full_member_path = base_path + "." + member_def.name;

        Variable member_var;
        member_var.type = member_def.type;
        member_var.is_unsigned = member_def.is_unsigned;
        member_var.is_assigned = false;
        member_var.is_const = parent_var.is_const || member_def.is_const;

        // 配列メンバーの場合
        if (member_def.array_info.is_array()) {
            process_array_member_recursively(full_member_path, member_def,
                                             member_var);
        }

        // parent_varのstruct_membersに追加
        parent_var.struct_members[member_def.name] = member_var;

        // 構造体メンバの場合
        if (member_def.type == TYPE_STRUCT && !member_def.type_alias.empty()) {
            member_var.is_struct = true;
            member_var.struct_type_name = member_def.type_alias;

            parent_var.struct_members[member_def.name].is_struct = true;
            parent_var.struct_members[member_def.name].struct_type_name =
                member_def.type_alias;

            // 再帰的にサブメンバーを作成
            create_struct_member_variables_recursively(
                full_member_path, member_def.type_alias,
                parent_var.struct_members[member_def.name]);
        }

        // 個別変数を登録
        interpreter_->current_scope().variables[full_member_path] =
            parent_var.struct_members[member_def.name];
    }

    // base_pathの個別変数も更新
    Variable *base_var = interpreter_->find_variable(base_path);
    if (base_var) {
        base_var->struct_members = parent_var.struct_members;
    }
}

// Private helper methods

void StructVariableManager::process_multidimensional_array_member(
    const std::string &var_name, const StructMember &member,
    Variable &struct_var) {
    Variable multidim_array_member;
    multidim_array_member.type = member.type;
    multidim_array_member.is_array = true;
    multidim_array_member.is_multidimensional = true;
    multidim_array_member.is_private_member = member.is_private;
    multidim_array_member.is_unsigned = member.is_unsigned;
    multidim_array_member.is_const = member.is_const;

    if (interpreter_->is_debug_mode()) {
        debug_print("Set is_multidimensional = true for %s\n",
                    member.name.c_str());
    }

    // 次元情報をコピー
    multidim_array_member.array_dimensions.clear();
    int total_size = 1;

    for (const auto &dim : member.array_info.dimensions) {
        int dim_size = dim.size;

        // 動的サイズの場合は解決を試みる
        if (dim_size == -1 && dim.is_dynamic && !dim.size_expr.empty()) {
            Variable *const_var = interpreter_->find_variable(dim.size_expr);
            if (const_var && const_var->is_const && const_var->is_assigned) {
                dim_size = static_cast<int>(const_var->value);
            } else {
                throw std::runtime_error("Cannot resolve constant '" +
                                         dim.size_expr + "'");
            }
        }

        if (dim_size <= 0) {
            throw std::runtime_error(
                "Invalid dimension size for struct member " + member.name);
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

    if (interpreter_->is_debug_mode()) {
        debug_print("Multidimensional array member created: %s, "
                    "total_size=%d\n",
                    member.name.c_str(), total_size);
    }
}

void StructVariableManager::process_1d_array_member(const std::string &var_name,
                                                    const StructMember &member,
                                                    Variable &struct_var) {
    int array_size = member.array_info.dimensions[0].size;

    // 動的サイズ（定数識別子）の場合は解決を試みる
    if (array_size == -1 && member.array_info.dimensions[0].is_dynamic &&
        !member.array_info.dimensions[0].size_expr.empty()) {
        array_size = resolve_array_size(member.array_info.dimensions[0]);
    }

    if (array_size <= 0) {
        throw std::runtime_error("Invalid array size for struct member " +
                                 member.name);
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

    // 配列の値を初期化
    array_member.array_values.resize(array_size, 0);
    if (member.type == TYPE_STRING) {
        array_member.array_strings.resize(array_size, "");
    }

    struct_var.struct_members[member.name] = array_member;

    // マップにコピーされた後、再度配列を初期化
    struct_var.struct_members[member.name].array_values.resize(array_size, 0);
    if (member.type == TYPE_STRING) {
        struct_var.struct_members[member.name].array_strings.resize(array_size,
                                                                    "");
    }

    // 各要素を個別の変数としても作成
    create_array_element_variables(var_name, member, array_size, struct_var);
}

void StructVariableManager::process_regular_member(const std::string &var_name,
                                                   const StructMember &member,
                                                   Variable &struct_var) {
    Variable member_var;
    member_var.type = member.type;

    // 構造体型メンバの特別処理
    if (member.type == TYPE_STRUCT && !member.type_alias.empty()) {
        process_struct_member(var_name, member, member_var);
    } else {
        // プリミティブ型メンバー
        if (member_var.type == TYPE_STRING) {
            member_var.str_value = "";
        } else {
            member_var.value = 0;
        }
        member_var.is_assigned = false;
        member_var.is_private_member = member.is_private;
        member_var.is_unsigned = member.is_unsigned;
        member_var.is_const = member.is_const;
    }

    struct_var.struct_members[member.name] = member_var;

    // 個別変数としても登録
    std::string member_path = var_name + "." + member.name;
    interpreter_->current_scope().variables[member_path] = member_var;
}

void StructVariableManager::process_struct_member(const std::string &var_name,
                                                  const StructMember &member,
                                                  Variable &member_var) {
    member_var.is_struct = true;
    member_var.struct_type_name = member.type_alias;
    member_var.is_assigned = false;
    member_var.is_private_member = member.is_private;
    member_var.is_const = member.is_const;

    // ネストした構造体の定義を取得
    std::string resolved_type =
        interpreter_->get_type_manager()->resolve_typedef(member.type_alias);
    const StructDefinition *nested_struct_def =
        interpreter_->find_struct_definition(resolved_type);

    if (nested_struct_def) {
        // ネストした構造体のメンバーを再帰的に初期化
        for (const auto &nested_member : nested_struct_def->members) {
            Variable nested_member_var;
            nested_member_var.type = nested_member.type;
            nested_member_var.is_unsigned = nested_member.is_unsigned;
            nested_member_var.is_private_member = nested_member.is_private;
            nested_member_var.is_const = nested_member.is_const;
            nested_member_var.is_assigned = false;

            if (nested_member_var.type == TYPE_STRING) {
                nested_member_var.str_value = "";
            } else {
                nested_member_var.value = 0;
            }

            member_var.struct_members[nested_member.name] = nested_member_var;

            // 個別変数としても登録
            std::string nested_member_path =
                var_name + "." + member.name + "." + nested_member.name;
            interpreter_->current_scope().variables[nested_member_path] =
                nested_member_var;
        }
    }
}

void StructVariableManager::create_array_element_variables(
    const std::string &var_name, const StructMember &member, int array_size,
    Variable &struct_var) {
    for (int i = 0; i < array_size; i++) {
        Variable array_element;
        array_element.type = member.type;
        array_element.is_unsigned = member.is_unsigned;
        array_element.is_private_member = member.is_private;
        array_element.is_assigned = false;

        // 構造体型の配列の場合
        if (member.type == TYPE_STRUCT && !member.type_alias.empty()) {
            initialize_struct_array_element(member, array_element);
        } else {
            // プリミティブ型の配列要素
            if (array_element.type == TYPE_STRING) {
                array_element.str_value = "";
            } else {
                array_element.value = 0;
            }
        }

        std::string element_name =
            var_name + "." + member.name + "[" + std::to_string(i) + "]";
        interpreter_->current_scope().variables[element_name] = array_element;

        // 親のstruct_membersにも追加
        struct_var.struct_members[member.name + "[" + std::to_string(i) + "]"] =
            array_element;
    }
}

void StructVariableManager::initialize_struct_array_element(
    const StructMember &member, Variable &array_element) {
    array_element.is_struct = true;
    array_element.struct_type_name = member.type_alias;

    // 構造体定義を取得してメンバーを初期化
    std::string resolved_type =
        interpreter_->get_type_manager()->resolve_typedef(member.type_alias);
    const StructDefinition *element_struct_def =
        interpreter_->find_struct_definition(resolved_type);

    if (element_struct_def) {
        for (const auto &element_member : element_struct_def->members) {
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

            array_element.struct_members[element_member.name] =
                element_member_var;
        }
    }
}

void StructVariableManager::post_process_array_elements(
    const std::string &var_name, const StructDefinition *struct_def) {
    Variable *registered_var = interpreter_->find_variable(var_name);
    if (!registered_var || !registered_var->is_struct) {
        return;
    }

    for (const auto &member : struct_def->members) {
        if (member.array_info.is_array() &&
            member.array_info.dimensions.size() == 1) {
            int array_size = member.array_info.dimensions[0].size;

            // 配列要素を struct_members に追加
            for (int i = 0; i < array_size; i++) {
                std::string element_key =
                    member.name + "[" + std::to_string(i) + "]";
                std::string full_element_name = var_name + "." + element_key;

                Variable *element_var =
                    interpreter_->find_variable(full_element_name);
                if (element_var) {
                    registered_var->struct_members[element_key] = *element_var;
                }
            }
        }
    }
}

int StructVariableManager::resolve_array_size(const ArrayDimension &dim_info) {
    if (interpreter_->is_debug_mode()) {
        debug_print("Attempting to resolve constant: %s\n",
                    dim_info.size_expr.c_str());
    }

    Variable *const_var = interpreter_->find_variable(dim_info.size_expr);
    if (!const_var) {
        throw std::runtime_error("Cannot resolve constant '" +
                                 dim_info.size_expr +
                                 "' for struct member array size");
    }

    if (!const_var->is_const || !const_var->is_assigned) {
        throw std::runtime_error("Constant '" + dim_info.size_expr +
                                 "' is not a valid const variable");
    }

    return static_cast<int>(const_var->value);
}

void StructVariableManager::process_array_member_recursively(
    const std::string &full_member_path, const StructMember &member_def,
    Variable &member_var) {
    member_var.is_array = true;
    member_var.array_size = member_def.array_info.dimensions.empty()
                                ? 0
                                : member_def.array_info.dimensions[0].size;
    member_var.array_type_info = member_def.array_info;

    if (member_def.array_info.dimensions.size() > 1) {
        member_var.is_multidimensional = true;
        for (const auto &dim : member_def.array_info.dimensions) {
            member_var.array_dimensions.push_back(dim.size);
        }
    }

    // 構造体配列の場合、各要素変数も作成
    if (member_def.array_info.base_type == TYPE_STRUCT &&
        !member_def.type_alias.empty()) {
        std::string element_type_name = member_def.type_alias;
        size_t bracket_pos = element_type_name.find('[');
        if (bracket_pos != std::string::npos) {
            element_type_name = element_type_name.substr(0, bracket_pos);
        }

        for (int i = 0; i < member_var.array_size; ++i) {
            std::string element_name =
                full_member_path + "[" + std::to_string(i) + "]";
            Variable element_var;
            element_var.type = TYPE_STRUCT;
            element_var.is_struct = true;
            element_var.struct_type_name = element_type_name;
            element_var.is_assigned = false;
            element_var.is_const = member_var.is_const;

            create_struct_member_variables_recursively(
                element_name, element_type_name, element_var);

            interpreter_->current_scope().variables[element_name] = element_var;
        }
    }
}
