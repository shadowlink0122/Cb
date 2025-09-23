#include "type_manager.h"
#include "../common/debug_messages.h"
#include "interpreter.h"
#include <stdexcept>

void TypeManager::register_typedef(const std::string &name,
                                   const std::string &type_name) {
    // 重複チェック
    if (interpreter_->typedef_map.find(name) !=
        interpreter_->typedef_map.end()) {
        error_msg(DebugMsgId::VAR_REDECLARE_ERROR, name.c_str());
        throw std::runtime_error("Typedef redefinition error: " + name);
    }
    interpreter_->typedef_map[name] = type_name;
}

std::string TypeManager::resolve_typedef(const std::string &type_name) {
    auto it = interpreter_->typedef_map.find(type_name);
    if (it != interpreter_->typedef_map.end()) {
        // さらに別のtypedefの可能性があるので再帰的に解決
        return resolve_typedef(it->second);
    }
    return type_name; // typedef aliasでない場合はそのまま返す
}

TypeInfo TypeManager::string_to_type_info(const std::string &type_str) {
    std::string resolved = resolve_typedef(type_str);

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

    return TYPE_UNKNOWN;
}

void TypeManager::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &var_name) {
    bool out_of_range = false;

    switch (type) {
    case TYPE_TINY:
        out_of_range = (value < -128 || value > 127);
        break;
    case TYPE_SHORT:
        out_of_range = (value < -32768 || value > 32767);
        break;
    case TYPE_INT:
        out_of_range = (value < INT32_MIN || value > INT32_MAX);
        break;
    case TYPE_LONG:
        // int64_tの範囲内なので常にOK
        break;
    default:
        return; // 他の型はチェックしない
    }

    if (out_of_range) {
        error_msg(DebugMsgId::TYPE_RANGE_ERROR, var_name.c_str());
        throw std::runtime_error("Value out of range for type");
    }
}
