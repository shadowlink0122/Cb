#include "struct_assignment_manager.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include "../evaluator/expression_evaluator.h"
#include "../services/debug_service.h"
#include "common_operations.h"
#include "type_manager.h"
#include "variable_manager.h"
#include <functional>
#include <map>
#include <stdexcept>

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const Variable &value_var) {
    // Note: value_var contains the source value to assign
    if (interpreter_->debug_mode) {
        debug_print(
            "assign_struct_member (Variable): var=%s, member=%s, type=%d\n",
            var_name.c_str(), member_name.c_str(),
            static_cast<int>(value_var.type));
    }

    std::string target_full_name = var_name + "." + member_name;

    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_var_name = var_name;
    size_t dot_pos = var_name.find('.');
    if (dot_pos != std::string::npos) {
        root_var_name = var_name.substr(0, dot_pos);
    }

    // 最上位の親変数がconstかチェック
    if (Variable *root_var = interpreter_->find_variable(root_var_name)) {
        if (root_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    if (Variable *struct_var = interpreter_->find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = interpreter_->get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーかどうかを事前にチェック
    bool is_union_member = (member_var->type == TYPE_UNION);

    // value_varの型に応じて値を代入
    if (value_var.type == TYPE_FLOAT) {
        member_var->float_value = value_var.float_value;
        if (!is_union_member) {
            member_var->type = TYPE_FLOAT;
        } else {
            member_var->current_type = TYPE_FLOAT;
        }
    } else if (value_var.type == TYPE_DOUBLE) {
        member_var->double_value = value_var.double_value;
        if (!is_union_member) {
            member_var->type = TYPE_DOUBLE;
        } else {
            member_var->current_type = TYPE_DOUBLE;
        }
    } else if (value_var.type == TYPE_QUAD) {
        member_var->quad_value = value_var.quad_value;
        if (!is_union_member) {
            member_var->type = TYPE_QUAD;
        } else {
            member_var->current_type = TYPE_QUAD;
        }
    } else {
        // 整数型の場合
        int64_t assign_value = value_var.value;
        // unsignedの場合は負の値を0にクランプ
        if (member_var->is_unsigned && assign_value < 0) {
            if (interpreter_->debug_mode) {
                debug_print("Unsigned struct member %s.%s assignment with negative "
                           "value (%lld); clamping to 0\n",
                           var_name.c_str(), member_name.c_str(),
                           static_cast<long long>(assign_value));
            }
            assign_value = 0;
        }
        member_var->value = assign_value;
        if (is_union_member) {
            member_var->current_type =
                (value_var.type != TYPE_UNKNOWN)
                    ? value_var.type
                    : TYPE_INT;
        }
        // unsignedフラグはメンバ定義から引き継がれるため、ここでは設定しない
    }
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = interpreter_->find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }

        bool is_union_direct = (direct_var->type == TYPE_UNION);

        if (value_var.type == TYPE_FLOAT) {
            direct_var->float_value = value_var.float_value;
            if (!is_union_direct) {
                direct_var->type = TYPE_FLOAT;
            } else {
                direct_var->current_type = TYPE_FLOAT;
            }
        } else if (value_var.type == TYPE_DOUBLE) {
            direct_var->double_value = value_var.double_value;
            if (!is_union_direct) {
                direct_var->type = TYPE_DOUBLE;
            } else {
                direct_var->current_type = TYPE_DOUBLE;
            }
        } else if (value_var.type == TYPE_QUAD) {
            direct_var->quad_value = value_var.quad_value;
            if (!is_union_direct) {
                direct_var->type = TYPE_QUAD;
            } else {
                direct_var->current_type = TYPE_QUAD;
            }
        } else {
            int64_t assign_value = value_var.value;
            // unsignedの場合は負の値を0にクランプ
            if (direct_var->is_unsigned && assign_value < 0) {
                if (interpreter_->debug_mode) {
                    debug_print("Unsigned struct member %s assignment with negative "
                               "value (%lld); clamping to 0\n",
                               direct_var_name.c_str(),
                               static_cast<long long>(assign_value));
                }
                assign_value = 0;
            }
            direct_var->value = assign_value;
            if (is_union_direct) {
                direct_var->current_type =
                    (value_var.type != TYPE_UNKNOWN)
                        ? value_var.type
                        : TYPE_INT;
            }
            // unsignedフラグはメンバ定義から引き継がれるため、ここでは設定しない
        }
        direct_var->is_assigned = true;

        if (interpreter_->debug_mode) {
            debug_print("Updated direct access var %s (type=%d)\n",
                        direct_var_name.c_str(),
                        static_cast<int>(direct_var->type));
        }
    }
}

StructAssignmentManager::StructAssignmentManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}


void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name, long value) {
    std::string target_full_name = var_name + "." + member_name;

    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_var_name = var_name;
    size_t dot_pos = var_name.find('.');
    if (dot_pos != std::string::npos) {
        root_var_name = var_name.substr(0, dot_pos);
        if (interpreter_->debug_mode) {
            debug_print("INT: Nested member assignment: var_name=%s, "
                        "root_var_name=%s\n",
                        var_name.c_str(), root_var_name.c_str());
        }
    }

    // 最上位の親変数がconstかチェック
    if (Variable *root_var = interpreter_->find_variable(root_var_name)) {
        if (interpreter_->debug_mode) {
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

    if (Variable *struct_var = interpreter_->find_variable(var_name)) {
        if (interpreter_->debug_mode) {
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

    Variable *member_var = interpreter_->get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーの場合は制約をチェック
    bool is_union_member = interpreter_->type_manager_->is_union_type(*member_var);
    if (is_union_member) {
        if (!interpreter_->type_manager_->is_value_allowed_for_union(member_var->type_name,
                                                       static_cast<int64_t>(value))) {
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
        if (interpreter_->debug_mode) {
            debug_print("Unsigned struct member %s.%s assigned negative value "
                       "(%lld); clamping to 0\n",
                       var_name.c_str(), member_name.c_str(),
                       static_cast<long long>(member_value));
        }
        member_value = 0;
    }

    member_var->value = member_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = interpreter_->find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        // Union型の場合は制約をチェック
        bool is_union_direct = interpreter_->type_manager_->is_union_type(*direct_var);
        if (is_union_direct) {
            if (!interpreter_->type_manager_->is_value_allowed_for_union(
                    direct_var->type_name, static_cast<int64_t>(value))) {
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
            if (interpreter_->debug_mode) {
                debug_print("Unsigned struct member %s.%s assigned negative value "
                           "(%lld); clamping to 0\n",
                           var_name.c_str(), member_name.c_str(),
                           static_cast<long long>(direct_value));
            }
            direct_value = 0;
        }
        direct_var->value = direct_value;
        direct_var->is_assigned = true;
    }
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const std::string &str_value) {
    if (interpreter_->debug_mode) {
        debug_print(
            "assign_struct_member (string): var=%s, member=%s, value='%s'\n",
            var_name.c_str(), member_name.c_str(), str_value.c_str());
    }

    std::string target_full_name = var_name + "." + member_name;

    // ネストしたメンバーの場合、最上位の親変数のconstもチェック
    std::string root_var_name = var_name;
    size_t dot_pos = var_name.find('.');
    if (dot_pos != std::string::npos) {
        root_var_name = var_name.substr(0, dot_pos);
    }

    // 最上位の親変数がconstかチェック
    if (Variable *root_var = interpreter_->find_variable(root_var_name)) {
        if (root_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    if (Variable *struct_var = interpreter_->find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var = interpreter_->get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーの場合は制約をチェック
    bool is_union_member = interpreter_->type_manager_->is_union_type(*member_var);
    if (is_union_member) {
        if (!interpreter_->type_manager_->is_value_allowed_for_union(member_var->type_name,
                                                       str_value)) {
            throw std::runtime_error(
                "String value '" + str_value + "' is not allowed for union type " +
                member_var->type_name + " in struct member " + member_name);
        }
        // Union型の場合はcurrent_typeを文字列型に設定し、数値をクリア
        member_var->current_type = TYPE_STRING;
        member_var->value = 0; // 数値をクリア
    }

    member_var->str_value = str_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = interpreter_->find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        // Union型の場合は制約をチェック
        bool is_union_direct = interpreter_->type_manager_->is_union_type(*direct_var);
        if (is_union_direct) {
            if (!interpreter_->type_manager_->is_value_allowed_for_union(
                    direct_var->type_name, str_value)) {
                throw std::runtime_error("String value '" + str_value +
                                         "' is not allowed for union type " +
                                         direct_var->type_name +
                                         " in struct member " + member_name);
            }
            // Union型の場合はcurrent_typeを文字列型に設定し、数値をクリア
            direct_var->current_type = TYPE_STRING;
            direct_var->value = 0; // 数値をクリア
        }

        direct_var->str_value = str_value;
        direct_var->is_assigned = true;
        if (interpreter_->debug_mode) {
            debug_print("Updated direct access var %s with value '%s'\n",
                        direct_var_name.c_str(), str_value.c_str());
        }
    } else {
        if (interpreter_->debug_mode) {
            debug_print("Direct access var %s not found\n",
                        direct_var_name.c_str());
        }
    }
}

void StructAssignmentManager::assign_struct_member_struct(
    const std::string &var_name, const std::string &member_name,
    const Variable &struct_value) {
    if (interpreter_->debug_mode) {
        debug_print(
            "assign_struct_member_struct: var=%s, member=%s, struct_type=%s\n",
            var_name.c_str(), member_name.c_str(),
            struct_value.struct_type_name.c_str());
    }

    std::string target_full_name = var_name + "." + member_name;
    if (Variable *struct_var = interpreter_->find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var =
        interpreter_->get_struct_member(var_name, member_name);
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
        if (interpreter_->debug_mode) {
            debug_print("Setting member struct type to: %s\n",
                        struct_value.struct_type_name.c_str());
        }
    }

    // 構造体データをコピー
    *member_var = struct_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス変数も更新
    std::string direct_var_name = var_name + "." + member_name;
    Variable *direct_var = interpreter_->find_variable(direct_var_name);
    if (direct_var) {
        if (direct_var->is_const && direct_var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_var_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_var_name);
        }
        *direct_var = struct_value;
        direct_var->is_assigned = true;
        if (interpreter_->debug_mode) {
            debug_print("Updated direct access struct var %s\n",
                        direct_var_name.c_str());
        }
    }

    // 構造体のメンバー変数も個別に更新
    for (const auto &member : struct_value.struct_members) {
        std::string nested_var_name = direct_var_name + "." + member.first;
        Variable *nested_var = interpreter_->find_variable(nested_var_name);
        if (nested_var) {
            *nested_var = member.second;
            nested_var->is_assigned = true;
            if (interpreter_->debug_mode) {
                debug_print("Updated nested member: %s = %lld\n",
                            nested_var_name.c_str(), member.second.value);
            }
        }
    }
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    long value) {
    if (interpreter_->debug_mode) {
        debug_print("assign_struct_member_array_element: var=%s, member=%s, "
                    "index=%d, value=%lld\n",
                    var_name.c_str(), member_name.c_str(), index, value);
    }

    std::string target_full_name = var_name + "." + member_name;
    if (Variable *struct_var = interpreter_->find_variable(var_name)) {
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var =
        interpreter_->get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    if (member_var->is_const) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    if (interpreter_->debug_mode) {
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

    if (interpreter_->debug_mode) {
        debug_print("About to assign value to array_values[%d]\n", index);
    }

    int64_t adjusted_value = value;
    if (member_var->is_unsigned && adjusted_value < 0) {
        if (interpreter_->debug_mode) {
            debug_print(
                "WARNING: Unsigned struct member %s.%s[%d] assigned negative "
                "value (%lld); clamping to 0\n",
                var_name.c_str(), member_name.c_str(), index,
                static_cast<long long>(adjusted_value));
        }
        adjusted_value = 0;
    }

    member_var->array_values[index] = adjusted_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス配列要素変数も更新
    std::string direct_element_name =
        var_name + "." + member_name + "[" + std::to_string(index) + "]";
    Variable *direct_element = interpreter_->find_variable(direct_element_name);
    if (direct_element) {
        if (direct_element->is_const && direct_element->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      direct_element_name.c_str());
            throw std::runtime_error("Cannot assign to const struct member: " +
                                     direct_element_name);
        }
        int64_t direct_value = member_var->is_unsigned ? adjusted_value : value;
        if (direct_element->is_unsigned && direct_value < 0) {
            if (interpreter_->debug_mode) {
                debug_print(
                    "WARNING: Unsigned struct member %s.%s[%d] assigned "
                    "negative value (%lld); clamping to 0\n",
                    var_name.c_str(), member_name.c_str(), index,
                    static_cast<long long>(direct_value));
            }
            direct_value = 0;
        }
        direct_element->value = direct_value;
        direct_element->is_assigned = true;
    }

    if (interpreter_->debug_mode) {
        debug_print("Assignment completed, array_values[%d] = %lld\n", index,
                    member_var->array_values[index]);
    }
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    const Variable &value_var) {
    // Implementation: handle both string and numeric values from Variable
    if (value_var.type == TYPE_STRING || !value_var.str_value.empty()) {
        // String version
        const std::string &value = value_var.str_value;
        if (interpreter_->debug_mode) {
            debug_print(
                "assign_struct_member_array_element (string): var=%s, "
                "member=%s, index=%d, value=%s\n",
                var_name.c_str(), member_name.c_str(), index, value.c_str());
        }

        std::string target_full_name = var_name + "." + member_name;
        if (Variable *struct_var = interpreter_->find_variable(var_name)) {
            if (struct_var->is_const) {
                error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                          target_full_name.c_str());
                throw std::runtime_error(
                    "Cannot assign to member of const struct: " +
                    target_full_name);
            }
        }

        Variable *member_var =
            interpreter_->get_struct_member(var_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Member variable not found: " +
                                     member_name);
        }

        if (member_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
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

        if (interpreter_->debug_mode) {
            debug_print(
                "Before assignment: array_strings.size()=%zu, index=%d\n",
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
        Variable *direct_element =
            interpreter_->find_variable(direct_element_name);
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

        if (interpreter_->debug_mode) {
            debug_print("After assignment: array_strings[%d]=%s\n", index,
                        member_var->array_strings[index].c_str());
        }
    } else {
        // Numeric version - delegate to the int64_t version
        assign_struct_member_array_element(var_name, member_name, index,
                                           value_var.value);
    }
}

void StructAssignmentManager::assign_struct_member_array_literal(
    const std::string &var_name, const std::string &member_name,
    const ASTNode *array_literal) {
    if (interpreter_->debug_mode) {
        debug_print("assign_struct_member_array_literal: var=%s, member=%s\n",
                    var_name.c_str(), member_name.c_str());
    }

    Variable *member_var = interpreter_->get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    if (interpreter_->debug_mode) {
        debug_print("member_var->is_multidimensional: %d, "
                    "array_dimensions.size(): %zu\n",
                    member_var->is_multidimensional,
                    member_var->array_dimensions.size());
        debug_print("Address of member_var: %p\n", (void *)member_var);
    }

    // 共通実装を使用して配列リテラルを解析・代入
    try {
        auto result = interpreter_->common_operations_->parse_array_literal(array_literal);

        if (interpreter_->debug_mode) {
            debug_print("Before assign_array_literal_to_variable: "
                        "array_dimensions.size(): %zu\n",
                        member_var->array_dimensions.size());
        }

        interpreter_->common_operations_->assign_array_literal_to_variable(
            member_var, result, var_name + "." + member_name);

        if (interpreter_->debug_mode) {
            debug_print("After assign_array_literal_to_variable: "
                        "array_dimensions.size(): %zu\n",
                        member_var->array_dimensions.size());
        }

        if (interpreter_->debug_mode) {
            debug_print("result.is_string_array: %d, result.size: %zu\n",
                        result.is_string_array, result.size);
        }

        // 構造体メンバー配列の場合、個別要素変数も更新する必要がある
        if (!result.is_string_array) {
            if (interpreter_->debug_mode) {
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
                if (interpreter_->debug_mode) {
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
                    if (interpreter_->debug_mode) {
                        debug_print(
                            "Resized multidim_array_values to %zu elements\n",
                            member_var->array_values.size());
                    }
                }

                for (size_t i = 0; i < max_elements; i++) {
                    member_var->array_values[i] = assigned_values[i];
                    member_var->multidim_array_values[i] =
                        assigned_values[i]; // multidim_array_values にも設定
                    if (interpreter_->debug_mode) {
                        debug_print("Set flat_index[%zu] = %lld (both "
                                    "array_values and multidim_array_values)\n",
                                    i, assigned_values[i]);
                    }
                }

                // N次元インデックス表示のためのデバッグ（2次元の場合の例）
                if (interpreter_->debug_mode && member_var->array_dimensions.size() == 2) {
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
                    Variable *element_var = interpreter_->find_variable(element_name);
                    if (element_var) {
                        element_var->value = assigned_values[i];
                        element_var->is_assigned = true;
                        if (interpreter_->debug_mode) {
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
                    Variable *element_var = interpreter_->find_variable(element_name);
                    if (element_var) {
                        element_var->value = assigned_values[i];
                        element_var->is_assigned = true;
                    }
                }
            }
        }

        if (interpreter_->debug_mode) {
            debug_print("Successfully assigned array literal to struct member "
                        "%s.%s using common operations\n",
                        var_name.c_str(), member_name.c_str());
        }
    } catch (const std::exception &e) {
        if (interpreter_->debug_mode) {
            debug_print(
                "Failed to assign array literal to struct member %s.%s: %s\n",
                var_name.c_str(), member_name.c_str(), e.what());
        }
        throw;
    }
}

void StructAssignmentManager::assign_struct_literal(const std::string &var_name,
                                        const ASTNode *literal_node) {
    // 変数の検証と準備
    Variable *var = prepare_struct_literal_assignment(var_name, literal_node);

    // struct定義を取得
    std::string resolved_struct_name =
        interpreter_->type_manager_->resolve_typedef(var->struct_type_name);
    const StructDefinition *struct_def =
        interpreter_->find_struct_definition(resolved_struct_name);
    if (!struct_def) {
        throw std::runtime_error("Struct definition not found: " + var->struct_type_name);
    }

    // 名前付き初期化かどうかをチェック
    bool is_named_init = false;
    if (!literal_node->arguments.empty() &&
        literal_node->arguments[0]->node_type == ASTNodeType::AST_ASSIGN) {
        is_named_init = true;
    }

    if (is_named_init) {
        process_named_initialization(var, var_name, literal_node, struct_def);
    } else {
        process_positional_initialization(var, var_name, literal_node, struct_def);
    }
    var->is_assigned = true;
}

// ============================================================================
// Helper methods for assign_struct_literal
// ============================================================================

Variable* StructAssignmentManager::prepare_struct_literal_assignment(
    const std::string &var_name, const ASTNode *literal_node) {
    
    if (!literal_node || literal_node->node_type != ASTNodeType::AST_STRUCT_LITERAL) {
        throw std::runtime_error("Invalid struct literal");
    }

    Variable *var = interpreter_->find_variable(var_name);

    // 変数が見つからない、または構造体でない場合、親構造体のstruct_membersと構造体定義から確認
    if (var && !var->is_struct && var_name.find('.') != std::string::npos) {
        // "o.inner" -> "o" and "inner"
        size_t dot_pos = var_name.rfind('.');
        std::string parent_name = var_name.substr(0, dot_pos);
        std::string member_name = var_name.substr(dot_pos + 1);

        Variable *parent_var = interpreter_->find_variable(parent_name);
        if (parent_var && parent_var->type == TYPE_STRUCT) {
            // 親の構造体定義からメンバー情報を取得
            std::string resolved_parent_type =
                interpreter_->type_manager_->resolve_typedef(parent_var->struct_type_name);
            const StructDefinition *parent_struct_def =
                interpreter_->find_struct_definition(resolved_parent_type);

            if (parent_struct_def) {
                for (const auto &member_def : parent_struct_def->members) {
                    if (member_def.name == member_name && member_def.type == TYPE_STRUCT) {
                        var->type = TYPE_STRUCT;
                        var->is_struct = true;
                        var->struct_type_name = member_def.type_alias;

                        // メンバの構造体定義を取得して、struct_membersを初期化
                        std::string resolved_member_type =
                            interpreter_->type_manager_->resolve_typedef(member_def.type_alias);
                        const StructDefinition *member_struct_def =
                            interpreter_->find_struct_definition(resolved_member_type);
                        if (member_struct_def) {
                            for (const auto &sub_member : member_struct_def->members) {
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
                                interpreter_->current_scope().variables[full_sub_member_name] =
                                    sub_member_var;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    // 配列要素への構造体リテラル代入の場合、まだ構造体変数が作成されていない可能性がある
    if (!var && var_name.find('[') != std::string::npos) {
        // 配列要素名から配列名を抽出
        size_t bracket_pos = var_name.find('[');
        std::string array_name = var_name.substr(0, bracket_pos);

        // 配列変数を取得して構造体型を確認
        Variable *array_var = interpreter_->find_variable(array_name);
        if (array_var && array_var->is_array && !array_var->struct_type_name.empty()) {
            // 構造体配列の要素として新しい構造体変数を作成
            std::string resolved_struct_name =
                interpreter_->type_manager_->resolve_typedef(array_var->struct_type_name);
            const StructDefinition *struct_def =
                interpreter_->find_struct_definition(resolved_struct_name);
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
                        int array_size = member_def.array_info.dimensions.empty()
                                             ? 0
                                             : member_def.array_info.dimensions[0].size;
                        member_var.array_size = array_size;
                        member_var.array_values.resize(array_size, 0);

                        // 配列メンバーの個別要素も作成
                        for (int i = 0; i < array_size; i++) {
                            std::string element_name =
                                var_name + "." + member_def.name + "[" + std::to_string(i) + "]";
                            Variable element_var;
                            element_var.type = member_def.array_info.base_type;
                            element_var.is_assigned = false;
                            element_var.is_unsigned = member_def.is_unsigned;
                            interpreter_->current_scope().variables[element_name] = element_var;
                        }
                    } else if (member_def.type == TYPE_STRING) {
                        member_var.str_value = "";
                    }
                    element_var.struct_members[member_def.name] = member_var;

                    // 個別メンバー変数も作成
                    std::string full_member_name = var_name + "." + member_def.name;
                    interpreter_->current_scope().variables[full_member_name] = member_var;
                }

                // 要素変数を登録
                interpreter_->current_scope().variables[var_name] = element_var;
                var = interpreter_->find_variable(var_name);
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

    // 親変数がconstの場合、すべてのstruct_membersと個別変数をconstにする（再帰的）
    if (var->is_const) {
        std::function<void(const std::string &, Variable &)> make_all_members_const;
        make_all_members_const = [&](const std::string &base_path, Variable &v) {
            for (auto &member_pair : v.struct_members) {
                member_pair.second.is_const = true;

                // 個別変数も更新
                std::string full_path = base_path + "." + member_pair.first;
                Variable *individual_var = interpreter_->find_variable(full_path);
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

    return var;
}

void StructAssignmentManager::process_named_initialization(
    Variable *var,
    const std::string &var_name,
    const ASTNode *literal_node,
    const StructDefinition *struct_def) {
    
    // unsigned型のメンバーに負の値が代入される場合のクランプ用ラムダ
    auto clamp_unsigned_member = [&var_name](Variable &target, int64_t &value,
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
        Variable *member_var = interpreter_->find_variable(full_member_name);

        auto member_it = var->struct_members.find(member_name);
        if (member_it == var->struct_members.end()) {
            throw std::runtime_error("Unknown struct member: " + member_name);
        }

        // struct_membersの実際の要素への参照を取得
        Variable &struct_member_var = member_it->second;

        // 構造体定義からメンバの is_const フラグを取得して設定
        const StructMember *member_def = struct_def->find_member(member_name);
        if (member_def) {
            struct_member_var.is_const = var->is_const || member_def->is_const;
        }

        // 個別変数にもis_constを設定
        if (member_var) {
            member_var->is_const = struct_member_var.is_const;
        }

        // メンバ値を評価して代入
        if (member_init->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 配列メンバーの場合
            if (!struct_member_var.is_array) {
                throw std::runtime_error("Member is not an array: " + member_name);
            }

            if (interpreter_->debug_mode) {
                debug_print("Array member initialization: %s, array_size=%d, elements_count=%zu\n",
                            member_name.c_str(), struct_member_var.array_size,
                            member_init->right->arguments.size());
            }

            const auto &array_elements = member_init->right->arguments;

            for (size_t i = 0; i < array_elements.size() &&
                               i < static_cast<size_t>(struct_member_var.array_size); i++) {
                std::string element_name =
                    var_name + "." + member_name + "[" + std::to_string(i) + "]";
                Variable *element_var = interpreter_->find_variable(element_name);
                std::string element_path = member_name + "[" + std::to_string(i) + "]";

                // float/double配列の処理
                if (struct_member_var.type == TYPE_FLOAT ||
                    struct_member_var.type == TYPE_DOUBLE) {
                    TypedValue typed_result =
                        interpreter_->expression_evaluator_->evaluate_typed_expression(
                            array_elements[i].get());
                    double float_value = typed_result.as_double();

                    if (element_var) {
                        element_var->float_value = float_value;
                        element_var->is_assigned = true;

                        if (interpreter_->debug_mode) {
                            debug_print("Initialized struct member array element: %s = %f\n",
                                        element_name.c_str(), float_value);
                        }
                    }

                    if (i < struct_member_var.array_float_values.size()) {
                        struct_member_var.array_float_values[i] = float_value;

                        if (interpreter_->debug_mode) {
                            debug_print("Updated struct_members array element: %s[%zu] = %f\n",
                                        member_name.c_str(), i, float_value);
                        }
                    }
                } else {
                    // int/bool/その他の型の配列
                    int64_t value = interpreter_->expression_evaluator_->evaluate_expression(
                        array_elements[i].get());
                    clamp_unsigned_member(struct_member_var, value, element_path,
                                          "initialized with array literal");

                    if (element_var) {
                        element_var->value = value;
                        element_var->is_assigned = true;

                        if (interpreter_->debug_mode) {
                            debug_print("Initialized struct member array element: %s = %lld\n",
                                        element_name.c_str(), (long long)value);
                        }
                    }

                    if (i < struct_member_var.array_values.size()) {
                        struct_member_var.array_values[i] = value;

                        if (interpreter_->debug_mode) {
                            debug_print("Updated struct_members array element: %s[%zu] = %lld\n",
                                        member_name.c_str(), i, (long long)value);
                        }
                    }
                }
            }
            struct_member_var.is_assigned = true;
        } else if ((struct_member_var.type == TYPE_STRING ||
                    interpreter_->type_manager_->is_union_type(struct_member_var)) &&
                   member_init->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            // 文字列メンバー
            struct_member_var.str_value = member_init->right->str_value;
            struct_member_var.type = TYPE_STRING;
            struct_member_var.is_assigned = true;

            if (member_var) {
                member_var->str_value = member_init->right->str_value;
                member_var->type = TYPE_STRING;
                member_var->is_assigned = true;
            }
        } else if (struct_member_var.type == TYPE_STRUCT &&
                   member_init->right->node_type == ASTNodeType::AST_VARIABLE) {
            // 構造体メンバに別の構造体変数を代入
            Variable *source_var = interpreter_->find_variable(member_init->right->name);
            if (!source_var || source_var->type != TYPE_STRUCT) {
                throw std::runtime_error("Source variable is not a struct: " +
                                         member_init->right->name);
            }

            struct_member_var = *source_var;
            struct_member_var.is_assigned = true;

            if (member_var) {
                *member_var = *source_var;
                member_var->is_assigned = true;
            }

            // メンバの個別変数もコピー
            for (const auto &sm : source_var->struct_members) {
                std::string source_member_path = member_init->right->name + "." + sm.first;
                std::string target_member_path = full_member_name + "." + sm.first;
                Variable *target_member_var = interpreter_->find_variable(target_member_path);
                if (target_member_var) {
                    Variable *source_member_var = interpreter_->find_variable(source_member_path);
                    if (source_member_var) {
                        *target_member_var = *source_member_var;
                    }
                }
            }
        } else if (struct_member_var.type == TYPE_STRUCT &&
                   member_init->right->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
            // ネストした構造体リテラル
            debug_msg(DebugMsgId::INTERPRETER_NESTED_STRUCT_LITERAL, full_member_name.c_str());

            if (!member_var) {
                throw std::runtime_error("Struct member variable not found: " + full_member_name);
            }

            if (var->is_const) {
                struct_member_var.is_const = true;
                member_var->is_const = true;
            }

            // 再帰的に構造体リテラルを代入
            assign_struct_literal(full_member_name, member_init->right.get());

            // struct_membersも更新
            struct_member_var = *member_var;
        } else {
            // float/double型またはint/bool型の処理
            if (struct_member_var.type == TYPE_FLOAT ||
                struct_member_var.type == TYPE_DOUBLE ||
                struct_member_var.type == TYPE_QUAD) {
                TypedValue typed_result =
                    interpreter_->expression_evaluator_->evaluate_typed_expression(
                        member_init->right.get());
                double float_value = typed_result.as_double();

                if (struct_member_var.type == TYPE_FLOAT) {
                    struct_member_var.float_value = static_cast<float>(float_value);
                } else if (struct_member_var.type == TYPE_DOUBLE) {
                    struct_member_var.double_value = float_value;
                } else if (struct_member_var.type == TYPE_QUAD) {
                    struct_member_var.quad_value = static_cast<long double>(float_value);
                }
                struct_member_var.is_assigned = true;

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
                // int/bool/その他の型
                int64_t value =
                    interpreter_->expression_evaluator_->evaluate_expression(member_init->right.get());
                clamp_unsigned_member(struct_member_var, value, member_name,
                                      "initialized with literal");
                struct_member_var.value = value;
                struct_member_var.is_assigned = true;

                if (member_var) {
                    member_var->value = value;
                    member_var->is_assigned = true;
                }
            }
        }
    }
}

void StructAssignmentManager::process_positional_initialization(
    Variable *var,
    const std::string &var_name,
    const ASTNode *literal_node,
    const StructDefinition *struct_def) {
    
    // unsigned型のメンバーに負の値が代入される場合のクランプ用ラムダ
    auto clamp_unsigned_member = [&var_name](Variable &target, int64_t &value,
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
            Variable *direct_member_var = interpreter_->find_variable(full_member_name);
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
            Variable *str_var = interpreter_->find_variable(init_value->name);
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
            Variable *direct_member_var = interpreter_->find_variable(full_member_name);
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
                        Variable *element_var = interpreter_->find_variable(element_name);
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
                        interpreter_->expression_evaluator_->evaluate_expression(
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
                    interpreter_->find_variable(full_member_name);
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
                    interpreter_->variable_manager_->current_scope()
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
            Variable *nested_var = interpreter_->find_variable(nested_var_name);
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
                    interpreter_->expression_evaluator_->evaluate_typed_expression(
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
                    interpreter_->find_variable(full_member_name);
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
                    interpreter_->expression_evaluator_->evaluate_expression(init_value);
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
                    interpreter_->find_variable(full_member_name);
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
