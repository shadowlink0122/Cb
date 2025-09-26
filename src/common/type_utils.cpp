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
    case TYPE_STRUCT:
        return "struct";
    case TYPE_ENUM:
        return "enum";
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
            case TYPE_STRUCT:
                return "struct[]";
            case TYPE_ENUM:
                return "enum[]";
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
    case TYPE_STRUCT:
        return "struct";
    case TYPE_ENUM:
        return "enum";
    default:
        return "unknown";
    }
}
