#include "variable_manager.h"
#include "../interpreter.h"
#include "../../common/debug_messages.h"
#include "../../common/utf8_utils.h"
#include <stdexcept>
#include <iostream>
#include <cstdlib>

VariableManager::VariableManager(Interpreter& interpreter) 
    : interpreter_(interpreter) {}

void VariableManager::assign_variable(const std::string &name, int64_t value,
                                     TypeInfo type) {
    Variable *var = interpreter_.find_variable(name);
    if (!var) {
        // 新しい変数を作成
        Variable new_var;
        new_var.type = type;
        new_var.value = value;
        new_var.is_assigned = true;
        new_var.is_const = false; // デフォルトはnon-const
        check_type_range(type, value, name);
        interpreter_.current_scope().variables[name] = new_var;
    } else {
        if (var->is_const && var->is_assigned) {
            std::cerr << "再代入できません: " << name << std::endl;
            std::exit(1);
        }
        if (var->is_array) {
            error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR, name.c_str());
            throw std::runtime_error("Direct array assignment error");
        }
        check_type_range(var->type, value, name);
        var->value = value;
        var->is_assigned = true;
    }
}

void VariableManager::assign_variable(const std::string &name, int64_t value,
                                     TypeInfo type, bool is_const) {
    debug_msg(DebugMsgId::VAR_ASSIGN_READABLE, name.c_str(), value,
              type_info_to_string(type), bool_to_string(is_const));
    Variable *var = interpreter_.find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::VAR_CREATE_NEW);
        // 新しい変数を作成
        Variable new_var;
        new_var.type = type;
        new_var.value = value;
        new_var.is_assigned = true;
        new_var.is_const = is_const;
        check_type_range(type, value, name);
        interpreter_.current_scope().variables[name] = new_var;
    } else {
        debug_msg(DebugMsgId::EXISTING_VAR_ASSIGN_DEBUG);
        if (var->is_const && var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
            std::exit(1);
        }
        if (var->is_array) {
            error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR, name.c_str());
            throw std::runtime_error("Direct array assignment error");
        }
        check_type_range(var->type, value, name);
        var->value = value;
        var->is_assigned = true;
    }
}

void VariableManager::assign_variable(const std::string &name,
                                     const std::string &value) {
    Variable *var = interpreter_.find_variable(name);
    if (!var) {
        Variable new_var;
        new_var.type = TYPE_STRING;
        new_var.str_value = value;
        new_var.is_assigned = true;
        interpreter_.current_scope().variables[name] = new_var;
    } else {
        if (var->is_const && var->is_assigned) {
            std::cerr << "再代入できません: " << name << std::endl;
            std::exit(1);
        }
        var->str_value = value;
        var->is_assigned = true;
    }
}

void VariableManager::assign_variable(const std::string &name,
                                     const std::string &value, bool is_const) {
    debug_msg(DebugMsgId::STRING_ASSIGN_READABLE, name.c_str(), value.c_str(),
              bool_to_string(is_const));
    Variable *var = interpreter_.find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::STRING_VAR_CREATE_NEW);
        Variable new_var;
        new_var.type = TYPE_STRING;
        new_var.str_value = value;
        new_var.is_assigned = true;
        new_var.is_const = is_const;
        interpreter_.current_scope().variables[name] = new_var;
    } else {
        debug_msg(DebugMsgId::EXISTING_STRING_VAR_ASSIGN_DEBUG);
        if (var->is_const && var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
            std::exit(1);
        }
        var->str_value = value;
        var->is_assigned = true;
    }
}

void VariableManager::assign_array_element(const std::string &name, int64_t index,
                                          int64_t value) {
    Variable *var = interpreter_.find_variable(name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_ARRAY_ERROR, name.c_str());
        throw std::runtime_error("Undefined array");
    }
    if (!var->is_array) {
        error_msg(DebugMsgId::NON_ARRAY_REF_ERROR, name.c_str());
        throw std::runtime_error("Non-array reference");
    }
    if (var->is_const) {
        error_msg(DebugMsgId::CONST_ARRAY_ASSIGN_ERROR, name.c_str());
        throw std::runtime_error("Assignment to const array");
    }
    if (index < 0 || index >= var->array_size) {
        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, name.c_str());
        throw std::runtime_error("Array out of bounds");
    }

    TypeInfo elem_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
    check_type_range(elem_type, value, name);
    var->array_values[index] = value;
}

void VariableManager::assign_string_element(const std::string &name, int64_t index,
                                           const std::string &value) {
    debug_msg(DebugMsgId::STRING_ELEMENT_ASSIGN_DEBUG, name.c_str(), index,
              value.c_str());

    Variable *var = interpreter_.find_variable(name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("Undefined variable");
    }
    if (var->type != TYPE_STRING) {
        error_msg(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR);
        throw std::runtime_error("Non-string character assignment");
    }
    if (var->is_const) {
        error_msg(DebugMsgId::CONST_STRING_ELEMENT_ASSIGN_ERROR, name.c_str());
        std::exit(1);
    }

    // UTF-8文字数で範囲チェック
    size_t utf8_length = utf8_utils::utf8_char_count(var->str_value);
    debug_msg(DebugMsgId::STRING_LENGTH_UTF8_DEBUG, utf8_length);

    if (index < 0 || index >= static_cast<int64_t>(utf8_length)) {
        error_msg(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR, name.c_str(), index,
                  utf8_length);
        throw std::runtime_error("String out of bounds");
    }

    // UTF-8文字列の指定位置の文字を置換
    // 新しい文字列を構築
    std::string new_string;
    size_t current_index = 0;
    for (size_t i = 0; i < var->str_value.size();) {
        int len =
            utf8_utils::utf8_char_length(static_cast<unsigned char>(var->str_value[i]));

        if (current_index == static_cast<size_t>(index)) {
            // 置換対象の文字位置
            new_string += value;
            debug_msg(DebugMsgId::STRING_ELEMENT_REPLACE_DEBUG, index,
                      value.c_str());
        } else {
            // 既存の文字をコピー
            new_string += var->str_value.substr(i, len);
        }

        i += len;
        current_index++;
    }

    var->str_value = new_string;
    debug_msg(DebugMsgId::STRING_AFTER_REPLACE_DEBUG, var->str_value.c_str());
}

void VariableManager::check_type_range(TypeInfo type, int64_t value,
                                      const std::string &name) {
    switch (type) {
    case TYPE_TINY:
        if (value < -128 || value > 127) {
            error_msg(DebugMsgId::TYPE_RANGE_ERROR);
            throw std::runtime_error("Type range error");
        }
        break;
    case TYPE_SHORT:
        if (value < -32768 || value > 32767) {
            error_msg(DebugMsgId::TYPE_RANGE_ERROR);
            throw std::runtime_error("Type range error");
        }
        break;
    case TYPE_INT:
        if (value < -2147483648LL || value > 2147483647LL) {
            error_msg(DebugMsgId::TYPE_RANGE_ERROR);
            throw std::runtime_error("Type range error");
        }
        break;
    case TYPE_BOOL:
        // bool型は0/1に正規化
        break;
    default:
        break;
    }
}
