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
            default:
                return "unknown[]";
            }
        }
        return "unknown";
    }
}

// bool値を文字列に変換する関数
const char *bool_to_string(bool value) { return value ? "true" : "false"; }
