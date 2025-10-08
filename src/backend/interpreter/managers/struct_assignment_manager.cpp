#include "struct_assignment_manager.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include "common_operations.h"
#include "type_manager.h"
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

void StructAssignmentManager::assign_struct_literal(
    const std::string &var_name, const ASTNode *literal_node) {
    // Implementation moved from interpreter.cpp
    // This is a large method (~772 lines), implementing it fully
    
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_STRUCT_LITERAL) {
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
                interpreter_->type_manager_->resolve_typedef(
                    parent_var->struct_type_name);
            const StructDefinition *parent_struct_def =
                interpreter_->find_struct_definition(resolved_parent_type);

            if (parent_struct_def) {
                for (const auto &member_def : parent_struct_def->members) {
                    if (member_def.name == member_name &&
                        member_def.type == TYPE_STRUCT) {
                        var->type = TYPE_STRUCT;
                        var->is_struct = true;
                        var->struct_type_name = member_def.type_alias;

                        // メンバの構造体定義を取得して、struct_membersを初期化
                        std::string resolved_member_type =
                            interpreter_->type_manager_->resolve_typedef(
                                member_def.type_alias);
                        const StructDefinition *member_struct_def =
                            interpreter_->find_struct_definition(
                                resolved_member_type);
                        if (member_struct_def) {
                            for (const auto &sub_member :
                                 member_struct_def->members) {
                                Variable sub_member_var;
                                sub_member_var.type = sub_member.type;
                                sub_member_var.is_unsigned =
                                    sub_member.is_unsigned;
                                sub_member_var.is_assigned = false;
                                if (sub_member.type == TYPE_STRUCT) {
                                    sub_member_var.is_struct = true;
                                    sub_member_var.struct_type_name =
                                        sub_member.type_alias;
                                }
                                var->struct_members[sub_member.name] =
                                    sub_member_var;

                                // 個別変数も作成
                                std::string full_sub_member_name =
                                    var_name + "." + sub_member.name;
                                interpreter_->current_scope()
                                    .variables[full_sub_member_name] =
                                    sub_member_var;
                            }
                        }

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
        if (interpreter_->debug_mode) {
            debug_print(
                "WARNING: Unsigned struct member %s.%s %s negative value "
                "(%lld); clamping to 0\n",
                var_name.c_str(), member_name.c_str(), context,
                static_cast<long long>(value));
        }
        value = 0;
    };

    // 配列要素への構造体リテラル代入の場合、まだ構造体変数が作成されていない可能性がある
    if (!var && var_name.find('[') != std::string::npos) {
        // 配列要素名から配列名を抽出
        size_t bracket_pos = var_name.find('[');
        std::string array_name = var_name.substr(0, bracket_pos);

        // 配列変数を取得して構造体型を確認
        Variable *array_var = interpreter_->find_variable(array_name);
        if (array_var && array_var->is_array &&
            !array_var->struct_type_name.empty()) {
            // 構造体配列の要素として新しい構造体変数を作成
            std::string resolved_struct_name =
                interpreter_->type_manager_->resolve_typedef(
                    array_var->struct_type_name);
            const StructDefinition *struct_def =
                interpreter_->find_struct_definition(resolved_struct_name);
            if (struct_def) {
                Variable element_var;
                element_var.type = TYPE_STRUCT;
                element_var.is_struct = true;
                element_var.struct_type_name = array_var->struct_type_name;
                element_var.is_assigned = true;

                // TODO: Continue implementation - this method is 772 lines total
                // Next section will be added in follow-up
                throw std::runtime_error(
                    "StructAssignmentManager::assign_struct_literal - "
                    "Partial implementation in progress (array element case)");
            }
        }
    }

    // TODO: Continue with remaining 600+ lines
    throw std::runtime_error(
        "StructAssignmentManager::assign_struct_literal - "
        "Partial implementation in progress");
}

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
