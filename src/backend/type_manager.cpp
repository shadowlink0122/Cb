#include "type_manager.h"
#include "../common/debug_messages.h"
#include "error_handler.h"
#include <climits>
#include <stdexcept>

void TypeManager::registerTypedef(const std::string &alias_name,
                                  const std::string &type_definition) {
    if (typedef_map.find(alias_name) != typedef_map.end()) {
        throw std::runtime_error("Typedef redefinition error: " + alias_name);
    }
    typedef_map[alias_name] = type_definition;
}

std::string TypeManager::resolveTypedef(const std::string &type_name) {
    auto it = typedef_map.find(type_name);
    if (it != typedef_map.end()) {
        // さらに別のtypedefの可能性があるので再帰的に解決
        return resolveTypedef(it->second);
    }
    return type_name; // typedef aliasでない場合はそのまま返す
}

bool TypeManager::isTypedefDefined(const std::string &alias_name) const {
    return typedef_map.find(alias_name) != typedef_map.end();
}

TypeInfo TypeManager::stringToTypeInfo(const std::string &type_str) {
    std::string resolved = resolveTypedef(type_str);

    if (resolved == "int")
        return TYPE_INT;
    if (resolved == "long")
        return TYPE_LONG;
    if (resolved == "short")
        return TYPE_SHORT;
    if (resolved == "tiny")
        return TYPE_TINY;
    if (resolved == "bool")
        return TYPE_BOOL;
    if (resolved == "string")
        return TYPE_STRING;
    if (resolved == "char")
        return TYPE_CHAR;
    if (resolved == "void")
        return TYPE_VOID;

    // 配列型の処理 (例: "int[5]")
    if (resolved.find("[") != std::string::npos) {
        std::string base_type_str = resolved.substr(0, resolved.find("["));
        TypeInfo base_type = stringToTypeInfo(base_type_str);
        return static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
    }

    return TYPE_UNKNOWN;
}

std::string TypeManager::typeInfoToString(TypeInfo type) {
    switch (type) {
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_SHORT:
        return "short";
    case TYPE_TINY:
        return "tiny";
    case TYPE_BOOL:
        return "bool";
    case TYPE_STRING:
        return "string";
    case TYPE_CHAR:
        return "char";
    case TYPE_VOID:
        return "void";
    default:
        if (isArrayType(type)) {
            TypeInfo base_type = getArrayBaseType(type);
            return typeInfoToString(base_type) + "[]";
        }
        return "unknown";
    }
}

void TypeManager::checkTypeRange(TypeInfo type, int64_t value,
                                 const std::string &var_name,
                                 const ASTNode *location) {
    int64_t min_val = getTypeMinValue(type);
    int64_t max_val = getTypeMaxValue(type);

    if (value < min_val || value > max_val) {
        throwRangeError(type, value, var_name, location);
    }

    // bool型は0/1に正規化
    if (type == TYPE_BOOL && (value != 0 && value != 1)) {
        // boolは通常0/1以外も受け入れるが、警告として記録
        debug_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                  ("Bool conversion: " + std::to_string(value) + " -> " +
                   (value ? "1" : "0"))
                      .c_str());
    }
}

bool TypeManager::isCompatibleType(TypeInfo from, TypeInfo to) {
    // 同じ型は互換
    if (from == to)
        return true;

    // 数値型間の変換
    if ((from >= TYPE_TINY && from <= TYPE_LONG) &&
        (to >= TYPE_TINY && to <= TYPE_LONG)) {
        return true;
    }

    // char と tiny は互換
    if ((from == TYPE_CHAR && to == TYPE_TINY) ||
        (from == TYPE_TINY && to == TYPE_CHAR)) {
        return true;
    }

    return false;
}

TypeInfo TypeManager::getPromotedType(TypeInfo type1, TypeInfo type2) {
    // より大きな型に昇格
    if (type1 == TYPE_LONG || type2 == TYPE_LONG)
        return TYPE_LONG;
    if (type1 == TYPE_INT || type2 == TYPE_INT)
        return TYPE_INT;
    if (type1 == TYPE_SHORT || type2 == TYPE_SHORT)
        return TYPE_SHORT;
    if (type1 == TYPE_TINY || type2 == TYPE_TINY)
        return TYPE_TINY;
    if (type1 == TYPE_CHAR || type2 == TYPE_CHAR)
        return TYPE_CHAR;
    if (type1 == TYPE_BOOL || type2 == TYPE_BOOL)
        return TYPE_BOOL;

    return TYPE_INT; // デフォルト
}

bool TypeManager::isArrayType(TypeInfo type) { return type >= TYPE_ARRAY_BASE; }

TypeInfo TypeManager::getArrayBaseType(TypeInfo array_type) {
    if (!isArrayType(array_type)) {
        throw std::runtime_error("Type is not an array type");
    }
    return static_cast<TypeInfo>(array_type - TYPE_ARRAY_BASE);
}

TypeInfo TypeManager::makeArrayType(TypeInfo base_type) {
    return static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
}

void TypeManager::throwRangeError(TypeInfo type, int64_t value,
                                  const std::string &var_name,
                                  const ASTNode *location) {
    std::string type_name = typeInfoToString(type);
    std::string error_msg = "Value " + std::to_string(value) +
                            " is out of range for type " + type_name +
                            " (variable: " + var_name + ")";

    if (location) {
        throw_detailed_runtime_error(error_msg, location);
    } else {
        throw std::runtime_error(error_msg);
    }
}

int64_t TypeManager::getTypeMinValue(TypeInfo type) {
    switch (type) {
    case TYPE_TINY:
        return -128;
    case TYPE_SHORT:
        return -32768;
    case TYPE_INT:
        return INT_MIN;
    case TYPE_LONG:
        return LLONG_MIN;
    case TYPE_CHAR:
        return 0;
    case TYPE_BOOL:
        return 0;
    default:
        return LLONG_MIN;
    }
}

int64_t TypeManager::getTypeMaxValue(TypeInfo type) {
    switch (type) {
    case TYPE_TINY:
        return 127;
    case TYPE_SHORT:
        return 32767;
    case TYPE_INT:
        return INT_MAX;
    case TYPE_LONG:
        return LLONG_MAX;
    case TYPE_CHAR:
        return 255;
    case TYPE_BOOL:
        return 1;
    default:
        return LLONG_MAX;
    }
}
