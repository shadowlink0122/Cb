#include "struct_assignment_manager.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include "evaluator/expression_evaluator.h"
#include "managers/type_manager.h"
#include "managers/variable_manager.h"
#include <functional>
#include <map>
#include <stdexcept>

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
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member(int) not implemented yet");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const std::string &str_value) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member(string) not implemented yet");
}

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const Variable &value_var) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member(var) not implemented yet");
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
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member_array_element(int) not implemented yet");
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    const Variable &value_var) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member_array_element(var) not implemented yet");
}

void StructAssignmentManager::assign_struct_member_array_literal(
    const std::string &var_name, const std::string &member_name,
    const std::vector<long> &values) {
    // TODO: 実装は次のセッションで移植
    throw std::runtime_error("StructAssignmentManager::assign_struct_member_array_literal not implemented yet");
}
