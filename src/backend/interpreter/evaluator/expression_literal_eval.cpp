#include "expression_literal_eval.h"
#include "../../../common/debug.h"
#include <stdexcept>

// デバッグ言語設定（外部変数）
extern DebugLanguage debug_language;

namespace LiteralEvalHelpers {

// ヘルパー関数：型の確保
static InferredType ensure_type(const InferredType& inferred, TypeInfo type, const std::string& name) {
    if (inferred.type_info != TYPE_UNKNOWN) {
        return inferred;
    }
    return InferredType(type, name);
}

// ヘルパー関数：型情報から文字列への変換（簡易版）
static const char* type_info_to_string_simple(TypeInfo type) {
    switch (type) {
        case TYPE_BOOL: return "bool";
        case TYPE_CHAR: return "char";
        case TYPE_TINY: return "tiny";
        case TYPE_SHORT: return "short";
        case TYPE_INT: return "int";
        case TYPE_LONG: return "long";
        case TYPE_BIG: return "big";
        case TYPE_FLOAT: return "float";
        case TYPE_DOUBLE: return "double";
        case TYPE_QUAD: return "quad";
        case TYPE_STRING: return "string";
        case TYPE_VOID: return "void";
        case TYPE_POINTER: return "pointer";
        default: return "unknown";
    }
}

TypedValue evaluate_number_literal_typed(
    const ASTNode* node,
    const InferredType& inferred_type
) {
    if (node->is_float_literal) {
        TypeInfo literal_type = inferred_type.type_info;
        if (literal_type == TYPE_UNKNOWN && node->literal_type != TYPE_UNKNOWN) {
            literal_type = node->literal_type;
        }
        if (literal_type == TYPE_FLOAT) {
            InferredType float_type = inferred_type.type_info == TYPE_FLOAT ? inferred_type : InferredType(TYPE_FLOAT, "float");
            return TypedValue(static_cast<double>(node->double_value), float_type);
        }
        if (literal_type == TYPE_QUAD) {
            InferredType quad_type = inferred_type.type_info == TYPE_QUAD ? inferred_type : InferredType(TYPE_QUAD, "quad");
            return TypedValue(node->quad_value, quad_type);
        }
        InferredType double_type = inferred_type.type_info == TYPE_DOUBLE ? inferred_type : InferredType(TYPE_DOUBLE, "double");
        return TypedValue(node->double_value, double_type);
    }
    InferredType int_type = inferred_type.type_info == TYPE_UNKNOWN ? InferredType(TYPE_INT, "int") : inferred_type;
    return TypedValue(node->int_value, int_type);
}

TypedValue evaluate_string_literal_typed(
    const ASTNode* node,
    const InferredType& inferred_type
) {
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

TypedValue evaluate_variable_typed(
    const ASTNode* node,
    Interpreter& interpreter,
    const InferredType& inferred_type
) {
    // 変数参照の場合、変数の型に応じて適切なTypedValueを返す
    Variable *var = interpreter.find_variable(node->name);
    if (!var) {
        std::string error_message = (debug_language == DebugLanguage::JAPANESE) ? 
            "未定義の変数です: " + node->name : "Undefined variable: " + node->name;
        interpreter.throw_runtime_error_with_location(error_message, node);
    }
    
    // 参照型変数の場合、参照先変数を取得
    if (var->is_reference) {
        var = reinterpret_cast<Variable*>(var->value);
        if (!var) {
            throw std::runtime_error("Invalid reference variable: " + node->name);
        }
    }
    
    // 関数ポインタの場合、関数ポインタ情報を含むTypedValueを返す
    if (var->is_function_pointer) {
        auto& fp_map = interpreter.current_scope().function_pointers;
        auto it = fp_map.find(node->name);
        if (it != fp_map.end()) {
            return TypedValue::function_pointer(
                var->value,
                it->second.function_name,
                it->second.function_node,
                inferred_type
            );
        }
    }
    
    auto make_numeric_value = [&](TypeInfo numeric_type, const InferredType& fallback_type) -> TypedValue {
        switch (numeric_type) {
            case TYPE_FLOAT:
                return TypedValue(static_cast<double>(var->float_value), fallback_type);
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
        return TypedValue(*var, InferredType(TYPE_STRUCT, var->struct_type_name));
    } else if (var->type == TYPE_INTERFACE) {
        return TypedValue(*var, InferredType(TYPE_INTERFACE, var->interface_name));
    } else if (var->is_array || var->type >= TYPE_ARRAY_BASE) {
        // 配列の場合、型情報を保持したTypedValueを返す
        TypeInfo base_type = var->type >= TYPE_ARRAY_BASE ? 
            static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE) : TYPE_INT;
        std::string type_name = std::string(type_info_to_string_simple(base_type)) + "[]";
        return TypedValue(*var, InferredType(var->type, type_name));
    } else {
        // 数値型の場合
        InferredType var_type(var->type, var->type_name.empty() ? 
            std::string(type_info_to_string_simple(var->type)) : var->type_name);
        return make_numeric_value(var->type, var_type);
    }
}

} // namespace LiteralEvalHelpers
