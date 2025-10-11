#include "special.h"
#include "../../../../common/debug.h"
#include "../../managers/types/enums.h"
#include "../../managers/types/manager.h"
#include "../core/evaluator.h"
#include <functional>
#include <iostream>
#include <stdexcept>

namespace SpecialAccessHelpers {

int64_t evaluate_arrow_access(
    const ASTNode *node, Interpreter &interpreter,
    ExpressionEvaluator &evaluator,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<Variable(const Variable &, const std::string &)>
        get_struct_member_func) {
    // アロー演算子アクセス: ptr->member は (*ptr).member と等価
    // まず左側のポインタを評価
    debug_msg(DebugMsgId::EXPR_EVAL_START, "Arrow operator member access");

    std::string member_name = node->name;

    // ポインタを評価して値を取得
    int64_t ptr_value = evaluate_expression_func(node->left.get());

    if (interpreter.is_debug_mode()) {
        std::cerr << "[ARROW_DEBUG] ptr_value=" << ptr_value
                  << " has_meta=" << ((ptr_value & (1LL << 63)) ? "yes" : "no")
                  << std::endl;
    }

    if (ptr_value == 0) {
        throw std::runtime_error("Null pointer dereference in arrow operator");
    }

    // ポインタ値から構造体変数を取得
    Variable *struct_var = reinterpret_cast<Variable *>(ptr_value);

    if (!struct_var) {
        throw std::runtime_error("Invalid pointer in arrow operator");
    }

    if (interpreter.is_debug_mode()) {
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it != struct_var->struct_members.end()) {
            std::cerr << "[ARROW_DEBUG] struct_var=" << struct_var
                      << " member=" << member_name
                      << " value=" << member_it->second.value << " is_assigned="
                      << (member_it->second.is_assigned ? "true" : "false")
                      << std::endl;
        } else {
            std::cerr << "[ARROW_DEBUG] struct_var=" << struct_var
                      << " member=" << member_name << " not found" << std::endl;
        }
    }

    // 構造体型またはInterface型をチェック
    if (struct_var->type != TYPE_STRUCT && struct_var->type != TYPE_INTERFACE) {
        throw std::runtime_error(
            "Arrow operator requires struct or interface pointer");
    }

    // メンバーを取得
    Variable member_var = get_struct_member_func(*struct_var, member_name);

    if (member_var.type == TYPE_STRING) {
        if (interpreter.is_debug_mode()) {
            std::cerr << "[ARROW_DEBUG] STRING member found: str_value='"
                      << member_var.str_value << "'" << std::endl;
        }
        TypedValue typed_result(static_cast<int64_t>(0),
                                InferredType(TYPE_STRING, "string"));
        typed_result.string_value = member_var.str_value;
        typed_result.is_numeric_result = false;
        // last_typed_result_に設定
        evaluator.set_last_typed_result(typed_result);
        if (interpreter.is_debug_mode()) {
            std::cerr
                << "[ARROW_DEBUG] set_last_typed_result called with string: '"
                << typed_result.string_value << "'" << std::endl;
        }
        return 0;
    } else if (member_var.type == TYPE_POINTER) {
        // ポインタメンバの場合はそのまま値を返す
        return member_var.value;
    } else if (member_var.type == TYPE_STRUCT ||
               member_var.type == TYPE_INTERFACE) {
        // 構造体メンバの場合は、その構造体へのポインタを返す
        // struct_membersから実際の変数へのポインタを取得
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it != struct_var->struct_members.end()) {
            return reinterpret_cast<int64_t>(&member_it->second);
        }
        // fallback: member_varのアドレスを返す(コピーなので注意)
        throw std::runtime_error(
            "Cannot get address of temporary struct member");
    } else if (member_var.type == TYPE_FLOAT ||
               member_var.type == TYPE_DOUBLE || member_var.type == TYPE_QUAD) {
        // 浮動小数点数の場合
        // Note: last_typed_result_の設定は呼び出し側で処理
        return static_cast<int64_t>(member_var.float_value);
    } else {
        return member_var.value;
    }
}

int64_t evaluate_member_array_access(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<Variable(const Variable &, const std::string &)>
        get_struct_member_func) {
    // メンバの配列アクセス: obj.member[index] または func().member[index]
    std::string obj_name;
    Variable base_struct;
    bool is_function_call = false;

    if (node->left->node_type == ASTNodeType::AST_VARIABLE ||
        node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        obj_name = node->left->name;
    } else if (node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出し結果でのメンバー配列アクセス: func().member[index]
        is_function_call = true;
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Function call member array access");

        try {
            evaluate_expression_func(node->left.get());
            throw std::runtime_error(
                "Function did not return a struct for member array access");
        } catch (const ReturnException &ret_ex) {
            if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                throw std::runtime_error("Struct array function return member "
                                         "array access not yet supported");
            } else {
                base_struct = ret_ex.struct_value;
                obj_name = "func_result"; // 仮の名前
            }
        }
    } else {
        throw std::runtime_error(
            "Invalid object reference in member array access");
    }

    std::string member_name = node->name;

    // インデックスを評価（多次元対応）
    std::vector<int64_t> indices;
    if (node->right) {
        // 1次元の場合（従来通り）
        int64_t index = evaluate_expression_func(node->right.get());
        indices.push_back(index);
    } else if (!node->arguments.empty()) {
        // 多次元の場合
        for (const auto &arg : node->arguments) {
            int64_t index = evaluate_expression_func(arg.get());
            indices.push_back(index);
        }
    } else {
        throw std::runtime_error("No indices found for array access");
    }

    // 構造体メンバー変数を取得
    Variable member_var_copy; // 関数呼び出しの場合用のコピー
    Variable *member_var;

    if (is_function_call) {
        // 関数戻り値からメンバーを取得
        member_var_copy = get_struct_member_func(base_struct, member_name);
        member_var = &member_var_copy;
    } else {
        member_var = interpreter.get_struct_member(obj_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }
    }

    // 多次元配列の場合
    if (member_var->is_multidimensional && indices.size() > 1) {
        if (is_function_call) {
            // 関数戻り値の場合は直接配列要素を取得
            if (!member_var->is_array || member_var->array_values.empty()) {
                throw std::runtime_error(
                    "Member is not a valid array for multi-dimensional access");
            }
            // 多次元インデックス計算（簡易版）
            int64_t flat_index = indices[0];
            if (indices.size() > 1 && member_var->is_multidimensional) {
                // 簡易的な多次元計算（正確には別の実装が必要）
                flat_index = indices[0] * 10 + indices[1]; // 仮の計算
            }
            if (flat_index >= 0 &&
                flat_index < (int64_t)member_var->array_values.size()) {
                return member_var->array_values[flat_index];
            } else {
                throw std::runtime_error("Array index out of bounds in "
                                         "function member array access");
            }
        } else {
            return interpreter.getMultidimensionalArrayElement(*member_var,
                                                               indices);
        }
    }

    // 1次元配列の場合
    int64_t index = indices[0];
    if (is_function_call) {
        // 関数戻り値の場合
        if (!member_var->is_array || member_var->array_values.empty()) {
            throw std::runtime_error("Member is not a valid array");
        }
        if (index >= 0 && index < (int64_t)member_var->array_values.size()) {
            return member_var->array_values[index];
        } else {
            throw std::runtime_error(
                "Array index out of bounds in function member array access");
        }
    } else {
        return interpreter.get_struct_member_array_element(
            obj_name, member_name, static_cast<int>(index));
    }
}

int64_t evaluate_enum_access(const ASTNode *node, Interpreter &interpreter) {
    // enum値アクセス (EnumName::member)
    EnumManager *enum_manager = interpreter.get_enum_manager();
    int64_t enum_value;

    // typedef名を実際のenum名に解決
    std::string resolved_enum_name =
        interpreter.get_type_manager()->resolve_typedef(node->enum_name);

    if (enum_manager->get_enum_value(resolved_enum_name, node->enum_member,
                                     enum_value)) {
        debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, enum_value);
        return enum_value;
    } else {
        std::string error_message = "Undefined enum value: " + node->enum_name +
                                    "::" + node->enum_member;
        throw std::runtime_error(error_message);
    }
}

} // namespace SpecialAccessHelpers
