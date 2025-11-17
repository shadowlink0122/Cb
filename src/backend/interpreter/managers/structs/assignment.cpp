#include "assignment.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../core/interpreter.h"
#include "../../evaluator/core/evaluator.h"
#include "../../services/debug_service.h"
#include "../common/operations.h"
#include "../types/manager.h"
#include "../variables/manager.h"
#include <functional>
#include <inttypes.h>
#include <map>
#include <stdexcept>

void StructAssignmentManager::assign_struct_member(
    const std::string &var_name, const std::string &member_name,
    const Variable &value_var) {
    // Note: value_var contains the source value to assign
    if (interpreter_->debug_mode) {
        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "assign_struct_member (Variable): var=%s, member=%s, type=%d",
                var_name.c_str(), member_name.c_str(),
                static_cast<int>(value_var.type));
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
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

    Variable *member_var =
        interpreter_->get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーかどうかを事前にチェック
    bool is_union_member = (member_var->type == TYPE_UNION);

    // value_varの型に応じて値を代入
    if (value_var.type == TYPE_STRING || !value_var.str_value.empty()) {
        // 文字列型の場合
        // str_valueが空でvalueにポインタ値がある場合（mallocで確保したstring）
        if (value_var.str_value.empty() && value_var.value != 0) {
            // ポインタ値をそのままvalueフィールドにコピー
            member_var->value = value_var.value;
            member_var->str_value = ""; // str_valueは空のまま
        } else {
            // 通常のstring値
            member_var->str_value = value_var.str_value;
            member_var->value = 0;
        }
        if (!is_union_member) {
            member_var->type = TYPE_STRING;
        } else {
            member_var->current_type = TYPE_STRING;
        }
        // その他の数値フィールドをクリア
        member_var->float_value = 0.0f;
        member_var->double_value = 0.0;
        member_var->quad_value = 0.0L;
    } else if (value_var.type == TYPE_FLOAT) {
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
                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "Unsigned struct member %s.%s assignment with negative ");
            }
            assign_value = 0;
        }
        member_var->value = assign_value;
        if (is_union_member) {
            member_var->current_type =
                (value_var.type != TYPE_UNKNOWN) ? value_var.type : TYPE_INT;
        }
        // unsignedフラグはメンバ定義から引き継がれるため、ここでは設定しない
    }
    member_var->is_assigned = true;

    // v0.12.1: enum関連フィールドのコピー
    // structメンバーに代入する際、enum情報が失われないようにする
    if (value_var.is_enum || !value_var.enum_type_name.empty()) {
        member_var->is_enum = value_var.is_enum;
        member_var->enum_type_name = value_var.enum_type_name;
        member_var->enum_variant = value_var.enum_variant;
        member_var->has_associated_value = value_var.has_associated_value;
        member_var->associated_int_value = value_var.associated_int_value;
        member_var->associated_str_value = value_var.associated_str_value;

        if (interpreter_->debug_mode) {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[STRUCT_ASSIGN] Copied enum info: is_enum=%d, "
                     "enum_type=%s, variant=%s",
                     member_var->is_enum, member_var->enum_type_name.c_str(),
                     member_var->enum_variant.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

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

        if (value_var.type == TYPE_STRING || !value_var.str_value.empty()) {
            // 文字列型の場合
            direct_var->str_value = value_var.str_value;
            if (!is_union_direct) {
                direct_var->type = TYPE_STRING;
            } else {
                direct_var->current_type = TYPE_STRING;
            }
            // 数値フィールドをクリア
            direct_var->value = 0;
            direct_var->float_value = 0.0f;
            direct_var->double_value = 0.0;
            direct_var->quad_value = 0.0L;
        } else if (value_var.type == TYPE_FLOAT) {
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
                    debug_msg(
                        DebugMsgId::GENERIC_DEBUG,
                        "Unsigned struct member %s assignment with negative ");
                }
                assign_value = 0;
            }
            direct_var->value = assign_value;
            if (is_union_direct) {
                direct_var->current_type = (value_var.type != TYPE_UNKNOWN)
                                               ? value_var.type
                                               : TYPE_INT;
            }
            // unsignedフラグはメンバ定義から引き継がれるため、ここでは設定しない
        }
        direct_var->is_assigned = true;

        // v0.12.1: enum関連フィールドのコピー (direct_var側)
        if (value_var.is_enum || !value_var.enum_type_name.empty()) {
            direct_var->is_enum = value_var.is_enum;
            direct_var->enum_type_name = value_var.enum_type_name;
            direct_var->enum_variant = value_var.enum_variant;
            direct_var->has_associated_value = value_var.has_associated_value;
            direct_var->associated_int_value = value_var.associated_int_value;
            direct_var->associated_str_value = value_var.associated_str_value;

            if (interpreter_->debug_mode) {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "[STRUCT_ASSIGN_DIRECT] Copied enum info: is_enum=%d, "
                         "enum_type=%s, variant=%s",
                         direct_var->is_enum,
                         direct_var->enum_type_name.c_str(),
                         direct_var->enum_variant.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }

        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Updated direct access var %s (type=%d)",
                         direct_var_name.c_str(),
                         static_cast<int>(direct_var->type));
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
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
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "INT: Nested member assignment: var_name=%s, ");
        }
    }

    // 最上位の親変数がconstかチェック
    if (Variable *root_var = interpreter_->find_variable(root_var_name)) {
        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "INT: Root variable %s found, is_const=%d",
                         root_var_name.c_str(), root_var->is_const ? 1 : 0);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
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
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "assign_struct_member (int): var=%s, member=%s, ");
        }
        if (struct_var->is_const) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR,
                      target_full_name.c_str());
            throw std::runtime_error(
                "Cannot assign to member of const struct: " + target_full_name);
        }
    }

    Variable *member_var =
        interpreter_->get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーの場合は制約をチェック
    bool is_union_member =
        interpreter_->type_manager_->is_union_type(*member_var);
    if (is_union_member) {
        if (!interpreter_->type_manager_->is_value_allowed_for_union(
                member_var->type_name, static_cast<int64_t>(value))) {
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
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "Unsigned struct member %s.%s assigned negative value ");
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
        bool is_union_direct =
            interpreter_->type_manager_->is_union_type(*direct_var);
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
                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "Unsigned struct member %s.%s assigned negative value ");
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
        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "assign_struct_member (string): var=%s, member=%s, value='%s'",
                var_name.c_str(), member_name.c_str(), str_value.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
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

    Variable *member_var =
        interpreter_->get_struct_member(var_name, member_name);
    if (member_var->is_const && member_var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, target_full_name.c_str());
        throw std::runtime_error("Cannot assign to const struct member: " +
                                 target_full_name);
    }

    // Union型メンバーの場合は制約をチェック
    bool is_union_member =
        interpreter_->type_manager_->is_union_type(*member_var);
    if (is_union_member) {
        if (!interpreter_->type_manager_->is_value_allowed_for_union(
                member_var->type_name, str_value)) {
            throw std::runtime_error("String value '" + str_value +
                                     "' is not allowed for union type " +
                                     member_var->type_name +
                                     " in struct member " + member_name);
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
        bool is_union_direct =
            interpreter_->type_manager_->is_union_type(*direct_var);
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
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Updated direct access var %s with value '%s'",
                         direct_var_name.c_str(), str_value.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    } else {
        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Direct access var %s not found",
                         direct_var_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }
}

void StructAssignmentManager::assign_struct_member_struct(
    const std::string &var_name, const std::string &member_name,
    const Variable &struct_value) {
    if (interpreter_->debug_mode) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "assign_struct_member_struct: var=%s, member=%s, "
                     "struct_type=%s",
                     var_name.c_str(), member_name.c_str(),
                     struct_value.struct_type_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
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
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Setting member struct type to: %s",
                         struct_value.struct_type_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }

    // 構造体データをコピー（struct_membersも含む）
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
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Updated direct access struct var %s",
                         direct_var_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }

    // 構造体のメンバー変数を再帰的に更新
    // ネストした構造体メンバーも正しく展開する
    sync_nested_struct_members_recursive(direct_var_name,
                                         struct_value.struct_members);
}

void StructAssignmentManager::assign_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index,
    long value) {
    if (interpreter_->debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "assign_struct_member_array_element: var=%s, member=%s, ");
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
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "Found member_var, is_array=%d, array_size=%d, ");
    }

    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    int64_t adjusted_value = value;
    if (member_var->is_unsigned && adjusted_value < 0) {
        if (interpreter_->debug_mode) {
            debug_msg(
                DebugMsgId::GENERIC_DEBUG,
                "WARNING: Unsigned struct member %s.%s[%d] assigned negative ");
        }
        adjusted_value = 0;
    }

    member_var->array_values[index] = adjusted_value;
    member_var->is_assigned = true;

    // ダイレクトアクセス親配列変数も更新（find_variableで取得される変数）
    // 構造体メンバー配列は2つの変数として管理されている：
    // 1. struct_members内の変数（get_struct_memberで取得）
    // 2. スコープ内のダイレクトアクセス変数（find_variableで取得）
    // printf等のtyped評価では2番目の変数が使用されるため、両方を更新する必要がある
    std::string direct_array_name = var_name + "." + member_name;
    Variable *direct_array_var = interpreter_->find_variable(direct_array_name);
    if (direct_array_var && direct_array_var != member_var) {
        // 配列サイズを確認して更新
        if (static_cast<size_t>(index) <
            direct_array_var->array_values.size()) {
            direct_array_var->array_values[index] = adjusted_value;
            direct_array_var->is_assigned = true;
        }
    }

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
                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "WARNING: Unsigned struct member %s.%s[%d] assigned ");
            }
            direct_value = 0;
        }
        direct_element->value = direct_value;
        direct_element->is_assigned = true;
    }

    if (interpreter_->debug_mode) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "Assignment completed, array_values[%d] = %" PRId64, index,
                     member_var->array_values[index]);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
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
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "assign_struct_member_array_element (string): var=%s, ");
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
            {
                char dbg_buf[512];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "Before assignment: array_strings.size()=%zu, index=%d",
                    member_var->array_strings.size(), index);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
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
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "After assignment: array_strings[%d]=%s", index,
                         member_var->array_strings[index].c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
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
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "assign_struct_member_array_literal: var=%s, member=%s",
                     var_name.c_str(), member_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    Variable *member_var =
        interpreter_->get_struct_member(var_name, member_name);
    if (!member_var) {
        throw std::runtime_error("Member variable not found: " + member_name);
    }

    if (interpreter_->debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "member_var->is_multidimensional: %d, ");
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf), "Address of member_var: %p",
                     (void *)member_var);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    // 共通実装を使用して配列リテラルを解析・代入
    try {
        auto result = interpreter_->common_operations_->parse_array_literal(
            array_literal);

        if (interpreter_->debug_mode) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "Before assign_array_literal_to_variable: ");
        }

        interpreter_->common_operations_->assign_array_literal_to_variable(
            member_var, result, var_name + "." + member_name);

        if (interpreter_->debug_mode) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "After assign_array_literal_to_variable: ");
        }

        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "result.is_string_array: %d, result.size: %zu",
                         result.is_string_array, result.size);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }

        // 構造体メンバー配列の場合、個別要素変数も更新する必要がある
        if (!result.is_string_array) {
            if (interpreter_->debug_mode) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "Entering individual element update block");
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "member_var->is_multidimensional: %d",
                             member_var->is_multidimensional);
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "member_var->array_dimensions.size(): %zu",
                             member_var->array_dimensions.size());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                if (member_var->array_dimensions.size() >= 2) {
                    for (size_t i = 0; i < member_var->array_dimensions.size();
                         i++) {
                        {
                            char dbg_buf[512];
                            snprintf(dbg_buf, sizeof(dbg_buf),
                                     "dimension[%zu]: %d", i,
                                     member_var->array_dimensions[i]);
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
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
                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "Assigning N-dimensional array literal to %s.%s",
                            var_name.c_str(), member_name.c_str());
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf),
                                 "Total array size: %zu, values to assign: %zu",
                                 member_var->array_values.size(),
                                 assigned_count);
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
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
                        {
                            char dbg_buf[512];
                            snprintf(
                                dbg_buf, sizeof(dbg_buf),
                                "Resized multidim_array_values to %zu elements",
                                member_var->array_values.size());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                    }
                }

                for (size_t i = 0; i < max_elements; i++) {
                    member_var->array_values[i] = assigned_values[i];
                    member_var->multidim_array_values[i] =
                        assigned_values[i]; // multidim_array_values にも設定
                    if (interpreter_->debug_mode) {
                        debug_msg(DebugMsgId::GENERIC_DEBUG,
                                  "Set flat_index[%zu] = %lld (both ");
                    }
                }

                // N次元インデックス表示のためのデバッグ（2次元の場合の例）
                if (interpreter_->debug_mode &&
                    member_var->array_dimensions.size() == 2) {
                    size_t rows = member_var->array_dimensions[0];
                    size_t cols = member_var->array_dimensions[1];
                    for (size_t r = 0; r < rows && (r * cols) < assigned_count;
                         r++) {
                        for (size_t c = 0;
                             c < cols && (r * cols + c) < assigned_count; c++) {
                            size_t flat_index = r * cols + c;
                            {
                                char dbg_buf[512];
                                snprintf(dbg_buf, sizeof(dbg_buf),
                                         "  [%zu][%zu] = %" PRId64
                                         " (flat_index: %zu)",
                                         r, c,
                                         member_var->array_values[flat_index],
                                         flat_index);
                                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                            }
                        }
                    }
                }

                // 多次元配列でも個別要素変数を更新
                for (size_t i = 0; i < max_elements; i++) {
                    std::string element_name = var_name + "." + member_name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var =
                        interpreter_->find_variable(element_name);
                    if (element_var) {
                        element_var->value = assigned_values[i];
                        element_var->is_assigned = true;
                        if (interpreter_->debug_mode) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "Updated individual element variable ");
                        }
                    }
                }

                // Also update the direct variable (e.g., "matrix.data") so sync
                // can access the full array
                std::string direct_var_name = var_name + "." + member_name;
                Variable *direct_var =
                    interpreter_->find_variable(direct_var_name);
                if (direct_var) {
                    direct_var->multidim_array_values =
                        member_var->multidim_array_values;
                    direct_var->array_values = member_var->array_values;
                    direct_var->is_assigned = true;
                    if (interpreter_->debug_mode) {
                        debug_msg(DebugMsgId::GENERIC_DEBUG,
                                  "Updated direct variable %s with %zu ");
                    }
                }
            } else {
                // 1次元配列の場合（既存の処理）
                for (size_t i = 0; i < result.size && i < assigned_count; i++) {
                    std::string element_name = var_name + "." + member_name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var =
                        interpreter_->find_variable(element_name);
                    if (element_var) {
                        element_var->value = assigned_values[i];
                        element_var->is_assigned = true;
                    }
                }
            }
        }

        if (interpreter_->debug_mode) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "Successfully assigned array literal to struct member ");
        }
    } catch (const std::exception &e) {
        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "Failed to assign array literal to struct member %s.%s: %s",
                    var_name.c_str(), member_name.c_str(), e.what());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
        throw;
    }
}

void StructAssignmentManager::assign_struct_literal(
    const std::string &var_name, const ASTNode *literal_node) {
    // 変数の検証と準備
    Variable *var = prepare_struct_literal_assignment(var_name, literal_node);

    // struct定義を取得
    std::string resolved_struct_name =
        interpreter_->type_manager_->resolve_typedef(var->struct_type_name);
    const StructDefinition *struct_def =
        interpreter_->find_struct_definition(resolved_struct_name);
    if (!struct_def) {
        throw std::runtime_error("Struct definition not found: " +
                                 var->struct_type_name);
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
        process_positional_initialization(var, var_name, literal_node,
                                          struct_def);
    }
    var->is_assigned = true;
}

// ============================================================================
// Helper methods for assign_struct_literal
// ============================================================================

Variable *StructAssignmentManager::prepare_struct_literal_assignment(
    const std::string &var_name, const ASTNode *literal_node) {

    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_STRUCT_LITERAL) {
        throw std::runtime_error("Invalid struct literal");
    }

    Variable *var = interpreter_->find_variable(var_name);

    // 変数が見つからない、または構造体でない場合、親構造体のstruct_membersと構造体定義から確認
    // v0.11.0: enum型もチェック
    if (var && !var->is_struct && !var->is_enum &&
        var_name.find('.') != std::string::npos) {
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
                element_var.is_assigned = false;

                // 構造体メンバーを初期化
                for (const auto &member_def : struct_def->members) {
                    Variable member_var;
                    member_var.type = member_def.type;
                    member_var.is_assigned = false;
                    member_var.is_unsigned = member_def.is_unsigned;
                    if (member_def.array_info.is_array()) {
                        member_var.is_array = true;
                        int array_size =
                            member_def.array_info.dimensions.empty()
                                ? 0
                                : member_def.array_info.dimensions[0].size;
                        member_var.array_size = array_size;
                        member_var.array_values.resize(array_size, 0);

                        // 配列メンバーの個別要素も作成
                        for (int i = 0; i < array_size; i++) {
                            std::string element_name = var_name + "." +
                                                       member_def.name + "[" +
                                                       std::to_string(i) + "]";
                            Variable element_var;
                            element_var.type = member_def.array_info.base_type;
                            element_var.is_assigned = false;
                            element_var.is_unsigned = member_def.is_unsigned;
                            interpreter_->current_scope()
                                .variables[element_name] = element_var;
                        }
                    } else if (member_def.type == TYPE_STRING) {
                        member_var.str_value = "";
                    }
                    element_var.struct_members[member_def.name] = member_var;

                    // 個別メンバー変数も作成
                    std::string full_member_name =
                        var_name + "." + member_def.name;
                    interpreter_->current_scope().variables[full_member_name] =
                        member_var;
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
    // v0.11.0: enum型もメンバーアクセスをサポート
    if (!var->is_struct && !var->is_enum) {
        throw std::runtime_error("Variable is not a struct or enum: " +
                                 var_name);
    }

    // enum型の場合は、evaluator側で処理されるのでここには来ないはず
    if (var->is_enum) {
        throw std::runtime_error(
            "Enum member assignment should be handled in evaluator: " +
            var_name);
    }

    if (var->is_const && var->is_assigned) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, var_name.c_str());
        throw std::runtime_error("Cannot assign to const struct: " + var_name);
    }

    // 親変数がconstの場合、すべてのstruct_membersと個別変数をconstにする（再帰的）
    if (var->is_const) {
        std::function<void(const std::string &, Variable &)>
            make_all_members_const;
        make_all_members_const = [&](const std::string &base_path,
                                     Variable &v) {
            for (auto &member_pair : v.struct_members) {
                member_pair.second.is_const = true;

                // 個別変数も更新
                std::string full_path = base_path + "." + member_pair.first;
                Variable *individual_var =
                    interpreter_->find_variable(full_path);
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
    Variable *var, const std::string &var_name, const ASTNode *literal_node,
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
                throw std::runtime_error("Member is not an array: " +
                                         member_name);
            }

            if (interpreter_->debug_mode) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "Array member initialization: %s, array_size=%d, ");
            }

            const auto &array_elements = member_init->right->arguments;

            for (size_t i = 0;
                 i < array_elements.size() &&
                 i < static_cast<size_t>(struct_member_var.array_size);
                 i++) {
                std::string element_name = var_name + "." + member_name + "[" +
                                           std::to_string(i) + "]";
                Variable *element_var =
                    interpreter_->find_variable(element_name);
                std::string element_path =
                    member_name + "[" + std::to_string(i) + "]";

                // float/double配列の処理
                if (struct_member_var.type == TYPE_FLOAT ||
                    struct_member_var.type == TYPE_DOUBLE) {
                    TypedValue typed_result =
                        interpreter_->expression_evaluator_
                            ->evaluate_typed_expression(
                                array_elements[i].get());
                    double float_value = typed_result.as_double();

                    if (element_var) {
                        element_var->float_value = float_value;
                        element_var->is_assigned = true;

                        if (interpreter_->debug_mode) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "Initialized struct member array ");
                        }
                    }

                    if (i < struct_member_var.array_float_values.size()) {
                        struct_member_var.array_float_values[i] = float_value;

                        if (interpreter_->debug_mode) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "Updated struct_members array element: ");
                        }
                    }
                } else {
                    // int/bool/その他の型の配列
                    int64_t value =
                        interpreter_->expression_evaluator_
                            ->evaluate_expression(array_elements[i].get());
                    clamp_unsigned_member(struct_member_var, value,
                                          element_path,
                                          "initialized with array literal");

                    if (element_var) {
                        element_var->value = value;
                        element_var->is_assigned = true;

                        if (interpreter_->debug_mode) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "Initialized struct member array ");
                        }
                    }

                    if (i < struct_member_var.array_values.size()) {
                        struct_member_var.array_values[i] = value;

                        if (interpreter_->debug_mode) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "Updated struct_members array element: ");
                        }
                    }
                }
            }
            struct_member_var.is_assigned = true;

            // ダイレクトアクセス配列変数も更新（find_variableで取得される変数）
            // これは構造体メンバー配列の要素代入と同じ問題への対処
            std::string direct_array_name = var_name + "." + member_name;
            Variable *direct_array_var =
                interpreter_->find_variable(direct_array_name);
            if (direct_array_var && direct_array_var->is_array &&
                direct_array_var != &struct_member_var) {
                // 配列値を同期
                direct_array_var->array_values = struct_member_var.array_values;
                direct_array_var->array_float_values =
                    struct_member_var.array_float_values;
                direct_array_var->array_double_values =
                    struct_member_var.array_double_values;
                direct_array_var->array_size = struct_member_var.array_size;
                direct_array_var->is_assigned = true;

                if (interpreter_->debug_mode) {
                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "Synced direct access array variable: %s (size=%d)",
                            direct_array_name.c_str(),
                            direct_array_var->array_size);
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                }
            }
        } else if ((struct_member_var.type == TYPE_STRING ||
                    interpreter_->type_manager_->is_union_type(
                        struct_member_var)) &&
                   member_init->right->node_type ==
                       ASTNodeType::AST_STRING_LITERAL) {
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
            Variable *source_var =
                interpreter_->find_variable(member_init->right->name);
            // v0.11.0: enum型もチェック（将来的にenum to
            // struct代入もサポート可能）
            if (!source_var ||
                (source_var->type != TYPE_STRUCT && !source_var->is_enum)) {
                throw std::runtime_error(
                    "Source variable is not a struct or enum: " +
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
                std::string source_member_path =
                    member_init->right->name + "." + sm.first;
                std::string target_member_path =
                    full_member_name + "." + sm.first;
                Variable *target_member_var =
                    interpreter_->find_variable(target_member_path);
                if (target_member_var) {
                    Variable *source_member_var =
                        interpreter_->find_variable(source_member_path);
                    if (source_member_var) {
                        *target_member_var = *source_member_var;
                    }
                }
            }
        } else if (struct_member_var.type == TYPE_STRUCT &&
                   member_init->right->node_type ==
                       ASTNodeType::AST_STRUCT_LITERAL) {
            // ネストした構造体リテラル
            debug_msg(DebugMsgId::INTERPRETER_NESTED_STRUCT_LITERAL,
                      full_member_name.c_str());

            if (!member_var) {
                throw std::runtime_error("Struct member variable not found: " +
                                         full_member_name);
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
                    interpreter_->expression_evaluator_
                        ->evaluate_typed_expression(member_init->right.get());
                double float_value = typed_result.as_double();

                if (struct_member_var.type == TYPE_FLOAT) {
                    struct_member_var.float_value =
                        static_cast<float>(float_value);
                } else if (struct_member_var.type == TYPE_DOUBLE) {
                    struct_member_var.double_value = float_value;
                } else if (struct_member_var.type == TYPE_QUAD) {
                    struct_member_var.quad_value =
                        static_cast<long double>(float_value);
                }
                struct_member_var.is_assigned = true;

                if (member_var) {
                    if (member_var->type == TYPE_FLOAT) {
                        member_var->float_value =
                            static_cast<float>(float_value);
                    } else if (member_var->type == TYPE_DOUBLE) {
                        member_var->double_value = float_value;
                    } else if (member_var->type == TYPE_QUAD) {
                        member_var->quad_value =
                            static_cast<long double>(float_value);
                    }
                    member_var->is_assigned = true;
                }
            } else {
                // int/bool/その他の型
                int64_t value =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        member_init->right.get());
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
    Variable *var, const std::string &var_name, const ASTNode *literal_node,
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
    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "STRUCT_LITERAL_DEBUG: Position-based initialization with ");
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
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "STRUCT_LITERAL_DEBUG: Initializing member %s (index ");

        // メンバ値を評価して代入
        if (it->second.type == TYPE_STRING &&
            init_value->node_type == ASTNodeType::AST_STRING_LITERAL) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "STRUCT_LITERAL_DEBUG: String literal ");
            it->second.str_value = init_value->str_value;

            // 直接アクセス変数も更新
            std::string full_member_name = var_name + "." + member_def.name;
            Variable *direct_member_var =
                interpreter_->find_variable(full_member_name);
            if (direct_member_var) {
                direct_member_var->str_value = init_value->str_value;
                direct_member_var->is_assigned = true;
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "STRUCT_LITERAL_DEBUG: Updated direct access ");
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
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "STRUCT_LITERAL_DEBUG: String variable ");
            it->second.str_value = str_var->str_value;

            // 直接アクセス変数も更新
            std::string full_member_name = var_name + "." + member_def.name;
            Variable *direct_member_var =
                interpreter_->find_variable(full_member_name);
            if (direct_member_var) {
                direct_member_var->str_value = str_var->str_value;
                direct_member_var->is_assigned = true;
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "STRUCT_LITERAL_DEBUG: Updated direct access ");
            }
        } else if (it->second.is_array &&
                   init_value->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            {
                char dbg_buf[512];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "STRUCT_LITERAL_DEBUG: Array literal initialization: %s",
                    member_def.name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }

            // 配列の要素型を確認（構造体配列かプリミティブ配列か）
            TypeInfo element_type = member_def.array_info.base_type;

            if (element_type == TYPE_STRUCT) {
                // 構造体配列の場合：各要素は構造体リテラルとして処理
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "STRUCT_LITERAL_DEBUG: Struct array ");

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

                        debug_msg(DebugMsgId::GENERIC_DEBUG,
                                  "STRUCT_LITERAL_DEBUG: Assigning ");

                        // 要素変数が存在し、構造体として初期化されているか確認
                        Variable *element_var =
                            interpreter_->find_variable(element_name);
                        if (!element_var) {
                            throw std::runtime_error(
                                "Element variable not found: " + element_name);
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
                        interpreter_->expression_evaluator_
                            ->evaluate_expression(
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

                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "STRUCT_LITERAL_DEBUG: Array element [%zu] ");
                }
                it->second.array_size = init_value->arguments.size();
                it->second.is_assigned = true;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
                Variable *direct_member_var =
                    interpreter_->find_variable(full_member_name);
                if (direct_member_var && direct_member_var->is_array) {
                    direct_member_var->array_values = it->second.array_values;
                    direct_member_var->array_size = it->second.array_size;
                    direct_member_var->is_assigned = true;
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "STRUCT_LITERAL_DEBUG: Updated direct ");
                }

                // 個別要素変数を一括登録（マップの再ハッシュを防ぐため一度に追加）
                for (const auto &ev_pair : element_vars) {
                    interpreter_->variable_manager_->current_scope()
                        .variables[ev_pair.first] = ev_pair.second;
                }
            }
        } else if (it->second.type == TYPE_STRUCT &&
                   init_value->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
            // ネストされた構造体リテラルの初期化
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "STRUCT_LITERAL_DEBUG: Nested struct literal ");

            std::string nested_var_name = var_name + "." + member_def.name;

            // ネストされた構造体変数を作成（既に変数が存在するはず）
            Variable *nested_var = interpreter_->find_variable(nested_var_name);
            if (!nested_var) {
                throw std::runtime_error("Nested struct variable not found: " +
                                         nested_var_name);
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
                    interpreter_->expression_evaluator_
                        ->evaluate_typed_expression(init_value);
                double float_value = typed_result.as_double();
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "STRUCT_LITERAL_DEBUG: Float/Double ");

                // TYPEに応じて適切なフィールドに設定
                if (it->second.type == TYPE_FLOAT) {
                    it->second.float_value = static_cast<float>(float_value);
                } else if (it->second.type == TYPE_DOUBLE) {
                    it->second.double_value = float_value;
                } else if (it->second.type == TYPE_QUAD) {
                    it->second.quad_value =
                        static_cast<long double>(float_value);
                }
                it->second.is_assigned = true;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
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
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "STRUCT_LITERAL_DEBUG: Updated direct ");
                }
            } else {
                // int/bool/その他の型の処理
                int64_t value =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        init_value);
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "STRUCT_LITERAL_DEBUG: Numeric initialization: ");
                clamp_unsigned_member(it->second, value, member_def.name,
                                      "initialized with literal");
                it->second.value = value;

                // 直接アクセス変数も更新
                std::string full_member_name = var_name + "." + member_def.name;
                Variable *direct_member_var =
                    interpreter_->find_variable(full_member_name);
                if (direct_member_var) {
                    direct_member_var->value = value;
                    direct_member_var->is_assigned = true;
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "STRUCT_LITERAL_DEBUG: Updated direct ");
                }
            }
        }
        it->second.is_assigned = true;
    }
}

// ネストした構造体メンバーを再帰的に同期する
void StructAssignmentManager::sync_nested_struct_members_recursive(
    const std::string &base_path,
    const std::map<std::string, Variable> &members) {

    if (interpreter_->debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "sync_nested_struct_members_recursive: base_path=%s, ");
    }

    for (const auto &member : members) {
        const std::string &member_name = member.first;
        const Variable &member_var = member.second;
        std::string nested_var_name = base_path + "." + member_name;

        // ダイレクトアクセス変数を更新
        Variable *nested_var = interpreter_->find_variable(nested_var_name);
        if (nested_var) {
            // 既存の変数を更新
            *nested_var = member_var;
            nested_var->is_assigned = true;

            if (interpreter_->debug_mode) {
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "Updated nested member: %s (type=%d)",
                             nested_var_name.c_str(), member_var.type);
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
        } else {
            // 変数が存在しない場合は新規作成
            // これは深いネスト初期化で必要
            // Interpreterのvariable managerを通じて変数を作成
            // 注: この場合、親の構造体メンバーに既に値が入っているので
            // ダイレクトアクセス変数が未作成でも問題ない
            // スキップして、struct_membersマップだけを信頼する

            if (interpreter_->debug_mode) {
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "Skipped creating nested member (not found): %s",
                             nested_var_name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
        }

        // もしこのメンバーが構造体型なら、さらに再帰的に展開
        if ((member_var.type == TYPE_STRUCT || member_var.is_struct) &&
            !member_var.struct_members.empty()) {

            if (interpreter_->debug_mode) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "Recursing into struct member: %s ");
            }

            sync_nested_struct_members_recursive(nested_var_name,
                                                 member_var.struct_members);
        }
    }
}
