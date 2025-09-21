#include "ast.h"
#include "type_alias.h"

// 型名を文字列に変換する関数
const char *type_info_to_string(TypeInfo type) {
    // まずエイリアス名がないかチェック
    std::string alias_name = type_info_to_string_with_aliases(type);
    static thread_local std::string result;
    result = alias_name;
    return result.c_str();
}

// 基本型名を文字列に変換する関数（エイリアスなし）
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
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "bool";
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
            case TYPE_STRING:
                return "string[]";
            case TYPE_BOOL:
                return "bool[]";
            default:
                return "unknown[]";
            }
        }
        return "unknown";
    }
}

// bool値を文字列に変換する関数
const char *bool_to_string(bool value) { return value ? "true" : "false"; }

// ArrayTypeInfo::to_string()の実装
std::string ArrayTypeInfo::to_string() const {
    if (!is_array()) {
        return type_info_to_string_basic(base_type);
    }

    std::string result = type_info_to_string_basic(base_type);
    for (const auto &dim : dimensions) {
        result += "[";
        if (!dim.is_dynamic) {
            result += std::to_string(dim.size);
        }
        result += "]";
    }
    return result;
}
