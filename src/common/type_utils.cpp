#include "ast.h"

// 型名を文字列に変換する関数
const char *type_info_to_string(TypeInfo type) {
    switch (type) {
    case TYPE_VOID:
        return "void";
    case TYPE_TINY:
        return "tiny";
    case TYPE_SHORT:
        return "short";
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_CHAR:
        return "char";
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_BIG:
        return "big";
    case TYPE_QUAD:
        return "quad";
    case TYPE_STRUCT:
        return "struct";
    case TYPE_ENUM:
        return "enum";
    case TYPE_INTERFACE:
        return "interface";
    case TYPE_UNION:
        return "union";
    case TYPE_POINTER:
        return "pointer";
    case TYPE_NULLPTR:
        return "nullptr";
    case TYPE_FUNCTION_POINTER:
        return "function_pointer";
    default:
        // 配列型の場合
        if (type >= TYPE_ARRAY_BASE) {
            TypeInfo base_type = static_cast<TypeInfo>(type - TYPE_ARRAY_BASE);
            switch (base_type) {
            case TYPE_TINY:
                return "tiny[]";
            case TYPE_SHORT:
                return "short[]";
            case TYPE_INT:
                return "int[]";
            case TYPE_LONG:
                return "long[]";
            case TYPE_CHAR:
                return "char[]";
            case TYPE_STRING:
                return "string[]";
            case TYPE_BOOL:
                return "bool[]";
            case TYPE_FLOAT:
                return "float[]";
            case TYPE_DOUBLE:
                return "double[]";
            case TYPE_BIG:
                return "big[]";
            case TYPE_QUAD:
                return "quad[]";
            case TYPE_STRUCT:
                return "struct[]";
            case TYPE_ENUM:
                return "enum[]";
            case TYPE_INTERFACE:
                return "interface[]";
            case TYPE_UNION:
                return "union[]";
            default:
                return "unknown[]";
            }
        }
        return "unknown";
    }
}

// bool値を文字列に変換する関数
const char *bool_to_string(bool value) { return value ? "true" : "false"; }

// 基本型名を文字列に変換する関数（配列型は考慮しない）
const char *type_info_to_string_basic(TypeInfo type) {
    switch (type) {
    case TYPE_VOID:
        return "void";
    case TYPE_TINY:
        return "tiny";
    case TYPE_SHORT:
        return "short";
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_CHAR:
        return "char";
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_BIG:
        return "big";
    case TYPE_QUAD:
        return "quad";
    case TYPE_STRUCT:
        return "struct";
    case TYPE_ENUM:
        return "enum";
    case TYPE_INTERFACE:
        return "interface";
    case TYPE_UNION:
        return "union";
    case TYPE_POINTER:
        return "pointer";
    case TYPE_NULLPTR:
        return "nullptr";
    case TYPE_FUNCTION_POINTER:
        return "function_pointer";
    default:
        return "unknown";
    }
}

// FunctionPointerTypeInfo::to_string() の実装
std::string FunctionPointerTypeInfo::to_string() const {
    std::string result = type_info_to_string_basic(return_type);
    result += " (*)(";

    for (size_t i = 0; i < param_types.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }

        // カスタム型名があればそれを使用、なければ基本型名を使用
        if (i < param_type_names.size() && !param_type_names[i].empty()) {
            result += param_type_names[i];
        } else {
            result += type_info_to_string_basic(param_types[i]);
        }
    }

    result += ")";
    return result;
}
