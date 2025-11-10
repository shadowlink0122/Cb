#include "member_helpers.h"
#include "../../../../common/debug.h"
#include "../../core/interpreter.h"
#include "../../core/type_inference.h"
#include "../core/evaluator.h"
#include <iostream>
#include <stdexcept>

namespace MemberAccessHelpers {

// 型情報から文字列への変換（簡易版）
static const char *type_info_to_string_simple(TypeInfo type) {
    switch (type) {
    case TYPE_INT:
        return "int";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_QUAD:
        return "quad";
    case TYPE_STRING:
        return "string";
    default:
        return "unknown";
    }
}

TypedValue consume_numeric_typed_value(
    const ASTNode *node, int64_t numeric_result,
    const InferredType &inferred_type,
    std::optional<std::pair<const ASTNode *, TypedValue>>
        &last_captured_function_value,
    const TypedValue *last_typed_result) {
    if (last_captured_function_value.has_value()) {
        if (last_captured_function_value->first == node) {
            TypedValue captured =
                std::move(last_captured_function_value->second);
            last_captured_function_value = std::nullopt;
            return captured;
        }
        last_captured_function_value = std::nullopt;
    }

    // AST_ARROW_ACCESSの特別な結果の場合、last_typed_resultを参照
    // (evaluate_arrow_accessがset_last_typed_resultを呼び出している)
    if (node && node->node_type == ASTNodeType::AST_ARROW_ACCESS &&
        last_typed_result) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[consume_numeric] inferred=%d, last_result=%d",
                     static_cast<int>(inferred_type.type_info),
                     static_cast<int>(last_typed_result->type.type_info));
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        // 文字列、浮動小数点数の場合は常に last_typed_result を使用
        // （型推論が不完全な場合があるため）
        if (last_typed_result->type.type_info == TYPE_STRING ||
            last_typed_result->type.type_info == TYPE_FLOAT ||
            last_typed_result->type.type_info == TYPE_DOUBLE ||
            last_typed_result->type.type_info == TYPE_QUAD) {
            return *last_typed_result;
        }
    }

    InferredType resolved_type = inferred_type;
    if (resolved_type.type_info == TYPE_UNKNOWN) {
        resolved_type.type_info = TYPE_INT;
    }
    if (resolved_type.type_name.empty()) {
        resolved_type.type_name =
            type_info_to_string_simple(resolved_type.type_info);
    }

    // ポインタ値かどうかを確認
    // 注意: 単純に最上位ビットをチェックすると負の整数と誤判定される
    // ポインタは通常、ユーザー空間アドレスの範囲内（0x0000000100000000〜0x00007fffffffffff）
    // またはカーネル空間（0xffff800000000000以上）にある
    // 負の小さな整数（-128など）は 0xffffffffffffff80 となり、
    // これは有効なポインタアドレスとは考えにくい
    bool is_pointer_value = false;
    uint64_t unsigned_val = static_cast<uint64_t>(numeric_result);

    // ユーザー空間アドレス範囲: 0x0000000100000000 〜 0x00007fffffffffff
    // (macOS/Linux典型的な範囲) カーネル空間アドレス: 0xffff800000000000
    // 以上（負の整数の小さな値を除外）
    if ((unsigned_val >= 0x0000000100000000ULL &&
         unsigned_val <= 0x00007fffffffFFFFULL) ||
        (unsigned_val >= 0xffff800000000000ULL)) {
        is_pointer_value = true;
    }

    // ポインタ値の場合は、型にかかわらずそのまま返す
    if (is_pointer_value) {
        // ポインタメタデータのタグが付いている場合、TYPE_POINTERとして扱う
        return TypedValue(numeric_result,
                          InferredType(TYPE_POINTER, "pointer"));
    }

    switch (resolved_type.type_info) {
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
        return TypedValue(static_cast<double>(numeric_result), resolved_type);
    case TYPE_QUAD:
        return TypedValue(static_cast<long double>(numeric_result),
                          resolved_type);
    default:
        return TypedValue(numeric_result, resolved_type);
    }
}

Variable get_struct_member_from_variable(const Variable &struct_var,
                                         const std::string &member_name,
                                         Interpreter &interpreter) {
    // 参照型の場合、参照先の変数を取得
    const Variable *actual_var = &struct_var;

    if (struct_var.is_reference) {
        // 参照先はvalueフィールドにポインタとして格納されている
        actual_var = reinterpret_cast<Variable *>(struct_var.value);
        if (!actual_var) {
            throw std::runtime_error("Invalid reference in member access");
        }
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[DEBUG] get_struct_member_from_variable: resolving ");
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[MEMBER_ACCESS_DEBUG] Reference resolved:");
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf), "  ref_var ptr=%p",
                     (void *)&struct_var);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf), "  actual_var ptr=%p",
                     (void *)actual_var);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "  actual_var->struct_type_name=%s",
                     actual_var->struct_type_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "  actual_var->struct_members.size()=%zu",
                     actual_var->struct_members.size());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    // v0.11.0: enum型もサポート
    if (actual_var->type != TYPE_STRUCT && !actual_var->is_enum) {
        throw std::runtime_error("Variable is not a struct or enum");
    }

    // 参照変数の場合でも、常に参照先(actual_var)のstruct_membersを使用
    // 参照は常に最新の参照先のデータを反映すべき
    const std::map<std::string, Variable> *members_to_use =
        &actual_var->struct_members;

    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "[DEBUG] get_struct_member_from_variable: looking for '%s' in ");
    for (const auto &pair : *members_to_use) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[DEBUG]   - member: '%s' (type=%d, is_reference=%d)",
                     pair.first.c_str(), pair.second.type,
                     pair.second.is_reference);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    auto enforce_privacy = [&](const Variable &member_var) -> Variable {
        // メンバーが参照の場合、参照先を解決
        const Variable *final_member = &member_var;
        if (member_var.is_reference) {
            final_member = reinterpret_cast<Variable *>(member_var.value);
            if (!final_member) {
                throw std::runtime_error(
                    "Invalid reference in member variable");
            }
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[DEBUG] Member is a reference, resolving to target ");
        }

        if (!final_member->is_private_member) {
            return *final_member;
        }

        std::string struct_type = actual_var->struct_type_name;
        if (struct_type.empty() && !actual_var->implementing_struct.empty()) {
            struct_type = actual_var->implementing_struct;
        }

        if (!interpreter.is_current_impl_context_for(struct_type)) {
            std::string type_label =
                struct_type.empty() ? std::string("<anonymous>") : struct_type;
            std::cerr << "Error: Cannot access private member '" << member_name
                      << "' of '" << type_label
                      << "' from outside its impl block" << std::endl;
            std::exit(1);
        }

        return *final_member;
    };

    // まず struct_members から直接検索
    auto member_it = members_to_use->find(member_name);
    if (member_it != members_to_use->end()) {
        // ネストされた構造体メンバーの場合、そのstruct_membersを確認
        if (member_it->second.type == TYPE_STRUCT) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[DEBUG] Found struct member '%s' (type=%d, ");
        }

        // v0.13.0: デバッグ情報
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[MEMBER_DEBUG] member='%s', type=%d, is_enum=%d, "
                     "enum_type_name='%s', enum_variant='%s'",
                     member_name.c_str(), member_it->second.type,
                     member_it->second.is_enum,
                     member_it->second.enum_type_name.c_str(),
                     member_it->second.enum_variant.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        // v0.13.0: enum型メンバーの場合、is_enumフラグを修正
        // structメンバーに代入された時にenum情報が失われる問題の回避策
        Variable result = enforce_privacy(member_it->second);
        if (result.type == TYPE_ENUM && !result.is_enum) {
            // 型がTYPE_ENUMなのにis_enumがfalseの場合、修正
            result.is_enum = true;
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[MEMBER_FIX] Corrected is_enum flag for member");
        }
        // enum_type_nameが設定されていればis_enumをtrueに
        if (!result.enum_type_name.empty() && !result.is_enum) {
            result.is_enum = true;
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[MEMBER_FIX] Set is_enum based on enum_type_name");
        }

        return result;
    }

    // 構造体の識別子（struct_type_name）を使用してメンバーを検索
    std::string member_var_name =
        actual_var->struct_type_name + "." + member_name;
    Variable *member_var = interpreter.find_variable(member_var_name);

    if (member_var) {
        return enforce_privacy(*member_var);
    }

    // インタープリターの get_struct_member を使用
    try {
        std::string temp_struct_name =
            "temp_struct_" + struct_var.struct_type_name;
        member_var =
            interpreter.get_struct_member(temp_struct_name, member_name);
        if (member_var) {
            return enforce_privacy(*member_var);
        }
    } catch (...) {
        // 失敗した場合は続行
    }

    throw std::runtime_error("Struct member not found: " + member_name);
}

TypedValue evaluate_function_member_access(const ASTNode *func_node,
                                           const std::string &member_name,
                                           ExpressionEvaluator &evaluator) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_member_access");

    try {
        // 関数を実行してReturnExceptionを捕捉
        evaluator.evaluate_expression(func_node);
        throw std::runtime_error(
            "Function did not return a struct for member access");
    } catch (const ReturnException &ret_ex) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "FUNC_MEMBER_ACCESS: ReturnException caught - type=%d, ");
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "FUNC_MEMBER_ACCESS: struct_value type=%d, is_struct=%d, ");

        if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
            throw std::runtime_error(
                "Struct array function return member access requires index");
        } else {
            // 単一構造体の場合
            Variable struct_var = ret_ex.struct_value;
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "FUNC_MEMBER_ACCESS: Looking for member %s in struct",
                         member_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            Variable member_var = get_struct_member_from_variable(
                struct_var, member_name, evaluator.get_interpreter());

            if (member_var.type == TYPE_STRING) {
                // mallocで確保したstring型ポインタの場合
                std::string str_val;
                if (member_var.str_value.empty() && member_var.value != 0) {
                    const char *ptr =
                        reinterpret_cast<const char *>(member_var.value);
                    str_val = std::string(ptr);
                } else {
                    str_val = member_var.str_value;
                }
                TypedValue result(str_val, InferredType(TYPE_STRING, "string"));
                evaluator.set_last_typed_result(result);
                return result;
            } else {
                TypedValue result(member_var.value,
                                  InferredType(TYPE_INT, "int"));
                evaluator.set_last_typed_result(result);
                return result;
            }
        }
    }
}

TypedValue evaluate_function_array_access(const ASTNode *func_node,
                                          const ASTNode *index_node,
                                          ExpressionEvaluator &evaluator) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_array_access");

    // インデックスを評価
    int64_t index = evaluator.evaluate_expression(index_node);

    try {
        // 関数を実行して戻り値を取得
        evaluator.evaluate_expression(func_node);
        throw std::runtime_error(
            "Function did not return an array via exception");
    } catch (const ReturnException &ret) {
        if (!ret.is_array) {
            throw std::runtime_error("Function does not return an array");
        }

        if (ret.is_struct_array && !ret.struct_array_3d.empty() &&
            !ret.struct_array_3d[0].empty() &&
            !ret.struct_array_3d[0][0].empty()) {
            // 構造体配列の場合
            if (index >= 0 && index < static_cast<int64_t>(
                                          ret.struct_array_3d[0][0].size())) {
                Variable struct_element = ret.struct_array_3d[0][0][index];
                // 構造体として返す（後でメンバーアクセス可能）
                TypedValue result(
                    static_cast<int64_t>(0),
                    InferredType(TYPE_STRUCT, struct_element.struct_type_name));
                result.is_struct_result = true;
                result.struct_data = std::make_shared<Variable>(
                    struct_element); // 構造体データを保持
                evaluator.set_last_typed_result(result);
                return result;
            } else {
                throw std::runtime_error("Array index out of bounds");
            }
        } else if (!ret.int_array_3d.empty() && !ret.int_array_3d[0].empty() &&
                   !ret.int_array_3d[0][0].empty()) {
            // 数値配列の場合
            if (index >= 0 &&
                index < static_cast<int64_t>(ret.int_array_3d[0][0].size())) {
                return TypedValue(ret.int_array_3d[0][0][index],
                                  InferredType(TYPE_INT, "int"));
            } else {
                throw std::runtime_error("Array index out of bounds");
            }
        } else {
            throw std::runtime_error(
                "Unsupported array type in function return");
        }
    }
}

TypedValue evaluate_function_compound_access(const ASTNode *func_node,
                                             const ASTNode *index_node,
                                             const std::string &member_name,
                                             ExpressionEvaluator &evaluator) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_function_compound_access");

    // まず配列アクセスを実行
    TypedValue array_result =
        evaluate_function_array_access(func_node, index_node, evaluator);

    if (!array_result.is_struct_result || !array_result.struct_data) {
        throw std::runtime_error(
            "Array element is not a struct for member access");
    }

    // 構造体データからメンバーを取得
    Variable member_var = get_struct_member_from_variable(
        *array_result.struct_data, member_name, evaluator.get_interpreter());

    if (member_var.type == TYPE_STRING) {
        TypedValue result(member_var.str_value,
                          InferredType(TYPE_STRING, "string"));
        evaluator.set_last_typed_result(result);
        return result;
    } else {
        TypedValue result(member_var.value, InferredType(TYPE_INT, "int"));
        evaluator.set_last_typed_result(result);
        return result;
    }
}

TypedValue
evaluate_recursive_member_access(const Variable &base_var,
                                 const std::vector<std::string> &member_path,
                                 Interpreter &interpreter) {
    debug_msg(DebugMsgId::EXPR_EVAL_START, "evaluate_recursive_member_access");

    if (member_path.empty()) {
        throw std::runtime_error("Empty member path for recursive access");
    }

    debug_msg(DebugMsgId::MEMBER_ACCESS_RECURSIVE_START, member_path.size());

    Variable current_var = base_var;

    // 各レベルでのメンバーアクセスを再帰的に処理
    for (size_t i = 0; i < member_path.size(); ++i) {
        const std::string &member_name = member_path[i];
        debug_msg(DebugMsgId::MEMBER_ACCESS_LEVEL, i, member_name.c_str());

        // 現在の変数が構造体でない場合はエラー
        if (current_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Cannot access member '" + member_name +
                                     "' on non-struct type");
        }

        // メンバーを取得
        try {
            current_var = get_struct_member_from_variable(
                current_var, member_name, interpreter);
            debug_msg(DebugMsgId::MEMBER_ACCESS_SUCCESS,
                      static_cast<int>(current_var.type));
        } catch (const std::exception &e) {
            debug_msg(DebugMsgId::MEMBER_ACCESS_FAILED, member_name.c_str());
            throw;
        }

        // 最後のレベルでない場合、構造体である必要がある（将来の拡張用）
        if (i < member_path.size() - 1 && current_var.type != TYPE_STRUCT) {
            throw std::runtime_error("Intermediate member '" + member_name +
                                     "' is not a struct for further nesting");
        }
    }

    // 最終結果を TypedValue に変換
    debug_msg(DebugMsgId::MEMBER_ACCESS_FINAL_TYPE,
              static_cast<int>(current_var.type));

    if (current_var.type == TYPE_STRING) {
        TypedValue result(static_cast<int64_t>(0),
                          InferredType(TYPE_STRING, "string"));
        result.string_value = current_var.str_value;
        result.is_numeric_result = false;
        return result;
    } else if (current_var.type == TYPE_STRUCT) {
        // 構造体の場合、完全なデータを保持
        TypedValue result(
            current_var,
            InferredType(TYPE_STRUCT, current_var.struct_type_name));
        return result;
    } else {
        return TypedValue(current_var.value, InferredType(TYPE_INT, "int"));
    }
}

void sync_self_changes_to_receiver(const std::string &receiver_name,
                                   Variable *receiver_var,
                                   Interpreter &interpreter) {
    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "SELF_SYNC: Syncing self changes back to %s",
                 receiver_name.c_str());
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }

    // 構造体の各メンバーについて、selfから元の変数に同期
    for (const auto &member_pair : receiver_var->struct_members) {
        const std::string &member_name = member_pair.first;
        std::string self_member_path = "self." + member_name;
        std::string receiver_member_path = receiver_name + "." + member_name;

        // selfメンバーの変数を取得
        Variable *self_member = interpreter.find_variable(self_member_path);
        Variable *receiver_member =
            interpreter.find_variable(receiver_member_path);

        if (self_member && receiver_member) {
            // selfメンバーの値を元の変数に同期
            receiver_member->value = self_member->value;
            receiver_member->str_value = self_member->str_value;
            receiver_member->type = self_member->type;
            receiver_member->is_assigned = self_member->is_assigned;

            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "SELF_SYNC: %s.%s = %lld (\"%s\")",
                         receiver_name.c_str(), member_name.c_str(),
                         (long long)receiver_member->value,
                         receiver_member->str_value.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }
}

} // namespace MemberAccessHelpers
