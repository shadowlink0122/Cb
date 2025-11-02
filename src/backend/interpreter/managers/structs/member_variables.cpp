#include "managers/structs/member_variables.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/type_helpers.h"
#include "../../core/interpreter.h"
#include "../../evaluator/functions/generic_instantiation.h"
#include "managers/structs/operations.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include <cctype>
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

    auto trim = [](std::string &text) {
        while (!text.empty() &&
               std::isspace(static_cast<unsigned char>(text.front()))) {
            text.erase(text.begin());
        }
        while (!text.empty() &&
               std::isspace(static_cast<unsigned char>(text.back()))) {
            text.pop_back();
        }
    };

    std::string normalized_type_name = struct_type_name;
    trim(normalized_type_name);

    size_t array_pos = normalized_type_name.find('[');
    if (array_pos != std::string::npos) {
        normalized_type_name = normalized_type_name.substr(0, array_pos);
        trim(normalized_type_name);
    }

    // Remove trailing pointer qualifiers for lookup (e.g., "Rect*" -> "Rect")
    while (!normalized_type_name.empty() &&
           normalized_type_name.back() == '*') {
        normalized_type_name.pop_back();
        trim(normalized_type_name);
    }

    std::string resolved_type_name =
        interpreter_->get_type_manager()->resolve_typedef(normalized_type_name);

    const StructDefinition *struct_def =
        interpreter_->find_struct_definition(resolved_type_name);
    if (!struct_def) {
        throw std::runtime_error("Struct type not found: " + struct_type_name);
    }

    Variable struct_var;
    struct_var.type = TYPE_STRUCT;
    struct_var.is_struct = true;
    struct_var.struct_type_name =
        normalized_type_name.empty() ? struct_type_name : normalized_type_name;
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
    // 配列要素の場合、親配列のスコープに登録する
    bool is_array_element = (var_name.find('[') != std::string::npos);
    if (is_array_element) {
        // 配列名を抽出
        std::string array_name = var_name.substr(0, var_name.find('['));

        // 親配列を検索（グローバルとローカルの両方を確認）
        Variable *parent_array = interpreter_->find_variable(array_name);

        if (parent_array) {
            // グローバルスコープで親配列を検索
            bool is_global = false;
            auto &global_vars = interpreter_->get_global_scope().variables;
            if (global_vars.find(array_name) != global_vars.end()) {
                is_global = true;
            }

            // 親配列がグローバルなら、要素もグローバルに登録
            if (is_global) {
                interpreter_->get_global_scope().variables[var_name] =
                    struct_var;
            } else {
                interpreter_->current_scope().variables[var_name] = struct_var;
            }
        } else {
            // 親配列が見つからない場合は現在のスコープに登録
            interpreter_->current_scope().variables[var_name] = struct_var;
        }
    } else {
        // 通常の変数は現在のスコープに登録
        interpreter_->current_scope().variables[var_name] = struct_var;
    }

    // 構造体配列メンバーの要素を再度追加（変数登録後に行う）
    post_process_array_elements(var_name, struct_def);

    // v0.13.0: ジェネリック構造体のimplブロックをインスタンス化
    // 例: Box<int>の場合、impl Box<T>のTをintに置換してインスタンス化
    debug_print(
        "[GENERIC_CTOR_DEBUG] resolved_type_name=%s, checking for '_'\n",
        resolved_type_name.c_str());

    if (resolved_type_name.find('_') != std::string::npos) {
        debug_print("[GENERIC_CTOR_DEBUG] Found underscore, processing...\n");

        // 正規化された型名（Box_int）からジェネリックimplをインスタンス化
        // 型引数を抽出: Box_int -> ["int"]
        std::vector<std::string> type_arguments;
        std::string base_name;

        size_t underscore_pos = resolved_type_name.find('_');
        if (underscore_pos != std::string::npos) {
            base_name = resolved_type_name.substr(0, underscore_pos);
            std::string type_args_str =
                resolved_type_name.substr(underscore_pos + 1);

            // 単一型引数のみサポート（Box_int）
            type_arguments.push_back(type_args_str);

            if (interpreter_->is_debug_mode()) {
                debug_print("[GENERIC_CTOR] Instantiating impl for %s (base: "
                            "%s, type_arg: %s)\n",
                            resolved_type_name.c_str(), base_name.c_str(),
                            type_args_str.c_str());
            }

            // ジェネリックimplを探してインスタンス化
            // find_impl_for_struct は interface_name="" でimpl構造体を探す
            const ImplDefinition *impl_def =
                interpreter_->find_impl_for_struct(base_name + "<T>", "");

            if (impl_def && impl_def->impl_node) {
                if (interpreter_->is_debug_mode()) {
                    debug_print("[GENERIC_CTOR] Found generic impl, "
                                "instantiating...\n");
                }

                // インスタンス化してコンストラクタ/デストラクタを登録
                try {
                    auto result =
                        GenericInstantiation::instantiate_generic_impl(
                            impl_def->impl_node, type_arguments, "",
                            base_name + "<T>");

                    // auto &inst_struct = std::get<1>(result);
                    auto &inst_node = std::get<2>(result);

                    // 意図的リーク（ダングリングポインタ防止）
                    ASTNode *inst_node_ptr = inst_node.release();

                    // コンストラクタ/デストラクタを登録
                    for (const auto &method_node : inst_node_ptr->arguments) {
                        if (!method_node)
                            continue;

                        if (method_node->node_type ==
                            ASTNodeType::AST_CONSTRUCTOR_DECL) {
                            interpreter_->register_constructor(
                                resolved_type_name, method_node.get());
                        } else if (method_node->node_type ==
                                   ASTNodeType::AST_DESTRUCTOR_DECL) {
                            interpreter_->register_destructor(
                                resolved_type_name, method_node.get());
                        }
                    }
                } catch (const std::exception &e) {
                    if (interpreter_->is_debug_mode()) {
                        debug_print(
                            "[GENERIC_CTOR] Failed to instantiate: %s\n",
                            e.what());
                    }
                }
            }
        }
    }

    // v0.10.0: デフォルトコンストラクタを自動呼び出し
    interpreter_->call_default_constructor(var_name, resolved_type_name);
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
        
        // ポインタ情報をコピー
        member_var.is_pointer = member_def.is_pointer;
        member_var.pointer_depth = member_def.pointer_depth;
        member_var.pointer_base_type_name = member_def.pointer_base_type_name;
        member_var.pointer_base_type = member_def.pointer_base_type;
        
        // プライベートメンバー情報をコピー
        member_var.is_private_member = member_def.is_private;

        // 配列メンバーの場合
        if (member_def.array_info.is_array()) {
            process_array_member_recursively(full_member_path, member_def,
                                             member_var);
        }

        // parent_varのstruct_membersに追加
        parent_var.struct_members[member_def.name] = member_var;

        // 配列が構造体配列の場合、struct_membersにも構造体情報を設定
        if (interpreter_->is_debug_mode()) {
            debug_print("Check struct array: name=%s, is_array=%d, "
                        "base_type=%d, TYPE_STRUCT=%d, type_alias='%s'\n",
                        member_def.name.c_str(),
                        member_def.array_info.is_array() ? 1 : 0,
                        static_cast<int>(member_def.array_info.base_type),
                        static_cast<int>(TYPE_STRUCT),
                        member_def.type_alias.c_str());
        }
        if (member_def.array_info.is_array() &&
            member_def.array_info.base_type == TYPE_STRUCT &&
            !member_def.type_alias.empty()) {
            std::string element_type_name = member_def.type_alias;
            size_t bracket_pos = element_type_name.find('[');
            if (bracket_pos != std::string::npos) {
                element_type_name = element_type_name.substr(0, bracket_pos);
            }
            parent_var.struct_members[member_def.name].is_struct = true;
            parent_var.struct_members[member_def.name].struct_type_name =
                element_type_name;

            if (interpreter_->is_debug_mode()) {
                debug_print("Set struct array info: %s.%s -> is_struct=true, "
                            "struct_type=%s\n",
                            base_path.c_str(), member_def.name.c_str(),
                            element_type_name.c_str());
            }
        }

        // 構造体メンバの場合
        if (TypeHelpers::isStruct(member_def.type) &&
            !member_def.type_alias.empty()) {
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
    if (TypeHelpers::isString(member.type)) {
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

    // 構造体配列の場合、構造体情報も設定
    TypeInfo elem_type =
        static_cast<TypeInfo>(static_cast<int>(member.type) - TYPE_ARRAY_BASE);
    if (elem_type == TYPE_STRUCT && !member.type_alias.empty()) {
        std::string elem_type_name = member.type_alias;
        size_t bracket_pos = elem_type_name.find('[');
        if (bracket_pos != std::string::npos) {
            elem_type_name = elem_type_name.substr(0, bracket_pos);
        }
        array_member.is_struct = true;
        array_member.struct_type_name = elem_type_name;
    }

    // 配列の値を初期化
    array_member.array_values.resize(array_size, 0);
    if (TypeHelpers::isString(member.type)) {
        array_member.array_strings.resize(array_size, "");
    }

    struct_var.struct_members[member.name] = array_member;

    // マップにコピーされた後、再度配列を初期化
    struct_var.struct_members[member.name].array_values.resize(array_size, 0);
    if (TypeHelpers::isString(member.type)) {
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
    if (TypeHelpers::isStruct(member.type) && !member.type_alias.empty()) {
        process_struct_member(var_name, member, member_var);
    } else {
        // プリミティブ型メンバー
        if (TypeHelpers::isString(member_var.type)) {
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

            if (TypeHelpers::isString(nested_member_var.type)) {
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
        if (TypeHelpers::isStruct(member.type) && !member.type_alias.empty()) {
            initialize_struct_array_element(member, array_element);
        } else {
            // プリミティブ型の配列要素
            if (TypeHelpers::isString(array_element.type)) {
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

            if (TypeHelpers::isString(element_member_var.type)) {
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

        // 配列変数自身にも構造体情報を設定
        member_var.is_struct = true;
        member_var.struct_type_name = element_type_name;

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
