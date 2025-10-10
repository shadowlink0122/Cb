#include "eval.h"
#include "../../../../common/debug.h"
#include <stdexcept>

// デバッグ言語設定（外部変数）
extern DebugLanguage debug_language;
extern bool debug_mode;

namespace LiteralEvalHelpers {

// ヘルパー関数：型情報から文字列への変換（簡易版）
static const char *type_info_to_string_simple(TypeInfo type) {
    switch (type) {
    case TYPE_BOOL:
        return "bool";
    case TYPE_CHAR:
        return "char";
    case TYPE_TINY:
        return "tiny";
    case TYPE_SHORT:
        return "short";
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_BIG:
        return "big";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_QUAD:
        return "quad";
    case TYPE_STRING:
        return "string";
    case TYPE_VOID:
        return "void";
    case TYPE_POINTER:
        return "pointer";
    default:
        return "unknown";
    }
}

TypedValue evaluate_number_literal_typed(const ASTNode *node,
                                         const InferredType &inferred_type) {
    if (node->is_float_literal) {
        TypeInfo literal_type = inferred_type.type_info;
        if (literal_type == TYPE_UNKNOWN &&
            node->literal_type != TYPE_UNKNOWN) {
            literal_type = node->literal_type;
        }
        if (literal_type == TYPE_FLOAT) {
            InferredType float_type = inferred_type.type_info == TYPE_FLOAT
                                          ? inferred_type
                                          : InferredType(TYPE_FLOAT, "float");
            return TypedValue(static_cast<double>(node->double_value),
                              float_type);
        }
        if (literal_type == TYPE_QUAD) {
            InferredType quad_type = inferred_type.type_info == TYPE_QUAD
                                         ? inferred_type
                                         : InferredType(TYPE_QUAD, "quad");
            return TypedValue(node->quad_value, quad_type);
        }
        InferredType double_type = inferred_type.type_info == TYPE_DOUBLE
                                       ? inferred_type
                                       : InferredType(TYPE_DOUBLE, "double");
        return TypedValue(node->double_value, double_type);
    }
    // リテラルの値から適切な型を判定
    // 大きなリテラルの場合は推論された型を無視してlong型を使用
    InferredType int_type;
    int64_t value = node->int_value;

    // リテラルの値がint32_tの範囲を超える場合は、推論型に関係なくlong型を使用
    if (value < INT32_MIN || value > INT32_MAX) {
        int_type = InferredType(TYPE_LONG, "long");
    } else if (inferred_type.type_info != TYPE_UNKNOWN) {
        // 推論された型がある場合はそれを使用
        int_type = inferred_type;
    } else {
        // 推論型がない場合はintをデフォルトとして使用
        int_type = InferredType(TYPE_INT, "int");
    }
    return TypedValue(node->int_value, int_type);
}

TypedValue evaluate_string_literal_typed(const ASTNode *node,
                                         const InferredType &inferred_type) {
    InferredType string_type = inferred_type;
    if (string_type.type_info != TYPE_STRING) {
        string_type = InferredType(TYPE_STRING, "string");
    }
    return TypedValue(node->str_value, string_type);
}

TypedValue evaluate_nullptr_literal_typed() {
    // nullptr は TYPE_NULLPTR として評価
    InferredType nullptr_type(TYPE_NULLPTR, "nullptr");
    return TypedValue(static_cast<int64_t>(0), nullptr_type);
}

TypedValue evaluate_variable_typed(const ASTNode *node,
                                   Interpreter &interpreter,
                                   const InferredType &inferred_type) {
    // 変数参照の場合、変数の型に応じて適切なTypedValueを返す
    Variable *var = interpreter.find_variable(node->name);
    if (!var) {
        std::string error_message = (debug_language == DebugLanguage::JAPANESE)
                                        ? "未定義の変数です: " + node->name
                                        : "Undefined variable: " + node->name;
        interpreter.throw_runtime_error_with_location(error_message, node);
    }

    // 参照型変数の場合、参照先変数を取得
    if (var->is_reference) {
        var = reinterpret_cast<Variable *>(var->value);
        if (!var) {
            throw std::runtime_error("Invalid reference variable: " +
                                     node->name);
        }
    }

    // 関数ポインタの場合、関数ポインタ情報を含むTypedValueを返す
    if (var->is_function_pointer) {
        auto &fp_map = interpreter.current_scope().function_pointers;
        auto it = fp_map.find(node->name);
        if (it != fp_map.end()) {
            return TypedValue::function_pointer(
                var->value, it->second.function_name, it->second.function_node,
                inferred_type);
        }
    }

    auto make_numeric_value =
        [&](TypeInfo numeric_type,
            const InferredType &fallback_type) -> TypedValue {
        switch (numeric_type) {
        case TYPE_FLOAT:
            return TypedValue(static_cast<double>(var->float_value),
                              fallback_type);
        case TYPE_DOUBLE:
            return TypedValue(var->double_value, fallback_type);
        case TYPE_QUAD:
            return TypedValue(var->quad_value, fallback_type);
        default:
            return TypedValue(var->value, fallback_type);
        }
    };

    // 変数の型に基づいて適切なTypedValueを作成
    if (var->type == TYPE_STRING) {
        return TypedValue(var->str_value, InferredType(TYPE_STRING, "string"));
    } else if (var->type == TYPE_STRUCT) {
        return TypedValue(*var,
                          InferredType(TYPE_STRUCT, var->struct_type_name));
    } else if (var->type == TYPE_INTERFACE) {
        return TypedValue(*var,
                          InferredType(TYPE_INTERFACE, var->interface_name));
    } else if (var->type == TYPE_UNION) {
        // Union型の場合、current_typeに基づいて適切なTypedValueを返す
        if (var->current_type == TYPE_STRING) {
            return TypedValue(var->str_value,
                              InferredType(TYPE_STRING, "string"));
        }
        InferredType union_numeric_type(
            var->current_type, type_info_to_string_simple(var->current_type));
        return make_numeric_value(var->current_type, union_numeric_type);
    } else if (var->is_array || var->type >= TYPE_ARRAY_BASE) {
        // 配列の場合、型情報を保持したTypedValueを返す
        TypeInfo base_type =
            var->type >= TYPE_ARRAY_BASE
                ? static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE)
                : TYPE_INT;
        std::string type_name =
            std::string(type_info_to_string_simple(base_type)) + "[]";
        return TypedValue(*var, InferredType(var->type, type_name));
    } else if (var->type == TYPE_POINTER || var->is_pointer) {
        // ポインタ型の場合、numeric_typeにTYPE_POINTERを設定
        TypedValue ptr_value(
            var->value, InferredType(TYPE_POINTER,
                                     type_info_to_string_simple(TYPE_POINTER)));
        ptr_value.numeric_type = TYPE_POINTER;
        return ptr_value;
    } else {
        // 数値型の場合
        InferredType var_type(
            var->type, var->type_name.empty()
                           ? std::string(type_info_to_string_simple(var->type))
                           : var->type_name);
        return make_numeric_value(var->type, var_type);
    }
}

// ========================================================================
// 識別子の評価（AST_IDENTIFIER）
// ========================================================================
int64_t evaluate_identifier(const ASTNode *node, Interpreter &interpreter) {
    debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());

    // selfキーワードの処理
    if (node->name == "self") {
        // メソッドコンテキスト内で self を処理
        Variable *self_var = interpreter.find_variable("self");
        if (!self_var) {
            std::string error_message =
                (debug_language == DebugLanguage::JAPANESE)
                    ? "selfはメソッドコンテキスト外では使用できません"
                    : "self can only be used within method context";
            interpreter.throw_runtime_error_with_location(error_message, node);
        }

        // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
        if (self_var->type == TYPE_STRUCT || self_var->type == TYPE_INTERFACE) {
            interpreter.sync_struct_members_from_direct_access("self");
            throw ReturnException(*self_var);
        } else {
            // プリミティブ型の場合は値を返す
            return self_var->value;
        }
    }

    // 通常の識別子として処理
    Variable *var = interpreter.find_variable(node->name);
    if (!var) {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND, node->name.c_str());
        std::string error_message = (debug_language == DebugLanguage::JAPANESE)
                                        ? "未定義の変数です: " + node->name
                                        : "Undefined variable: " + node->name;
        interpreter.throw_runtime_error_with_location(error_message, node);
    }

    debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);

    if (debug_mode && var->type == TYPE_POINTER) {
        std::cerr << "[EXPR_EVAL] Variable " << node->name
                  << " value: " << var->value << " (0x" << std::hex
                  << var->value << std::dec << ")" << std::endl;
    }

    return var->value;
}

// ========================================================================
// 変数参照の評価（AST_VARIABLE）
// ========================================================================
int64_t evaluate_variable(const ASTNode *node, Interpreter &interpreter) {
    debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());

    // selfキーワードの特別処理（構造体戻り値用）
    if (node->name == "self") {
        Variable *self_var = interpreter.find_variable("self");
        if (!self_var) {
            std::string error_message =
                (debug_language == DebugLanguage::JAPANESE)
                    ? "selfはメソッドコンテキスト外では使用できません"
                    : "self can only be used within method context";
            interpreter.throw_runtime_error_with_location(error_message, node);
        }

        // デバッグ出力: self変数の詳細情報
        debug_print("SELF_DEBUG: self found - type=%d, is_struct=%d, "
                    "TYPE_STRUCT=%d, TYPE_INTERFACE=%d\n",
                    (int)self_var->type, self_var->is_struct, (int)TYPE_STRUCT,
                    (int)TYPE_INTERFACE);

        // selfが構造体またはインターフェース型の場合、ReturnExceptionで構造体を返す
        if (self_var->type == TYPE_STRUCT || self_var->type == TYPE_INTERFACE) {
            debug_print(
                "SELF_DEBUG: Throwing ReturnException for struct self\n");
            interpreter.sync_struct_members_from_direct_access("self");
            throw ReturnException(*self_var);
        } else {
            debug_print(
                "SELF_DEBUG: self is not struct, returning primitive value\n");
            return self_var->value;
        }
    }

    Variable *var = interpreter.find_variable(node->name);
    if (!var) {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND, node->name.c_str());
        std::string error_message = (debug_language == DebugLanguage::JAPANESE)
                                        ? "未定義の変数です: " + node->name
                                        : "Undefined variable: " + node->name;
        interpreter.throw_runtime_error_with_location(error_message, node);
    }

    // 参照型変数の場合、参照先変数の値を返す
    if (var->is_reference) {
        Variable *target_var = reinterpret_cast<Variable *>(var->value);
        if (!target_var) {
            throw std::runtime_error("Invalid reference variable: " +
                                     node->name);
        }

        if (debug_mode) {
            std::cerr << "[DEBUG] Reference access: " << node->name
                      << " -> target value: " << target_var->value << std::endl;
        }

        // 参照先が構造体の場合
        if (target_var->type == TYPE_STRUCT) {
            throw ReturnException(*target_var);
        }

        // 参照先の値を返す
        return target_var->value;
    }

    // ユニオン型変数の場合、current_typeに応じて適切な値を返す
    if (var->type == TYPE_UNION) {
        if (var->current_type == TYPE_STRING) {
            debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), 0);
            return 0;
        } else {
            debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(),
                      var->value);
            return var->value;
        }
    }

    // 構造体変数の場合、ReturnExceptionをスローして構造体データを返す
    if (var->type == TYPE_STRUCT) {
        throw ReturnException(*var);
    }

    debug_msg(DebugMsgId::EXPR_EVAL_VAR_VALUE, node->name.c_str(), var->value);
    return var->value;
}

} // namespace LiteralEvalHelpers
