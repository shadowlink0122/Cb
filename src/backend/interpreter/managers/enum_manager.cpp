#include "enum_manager.h"
#include "../services/debug_service.h"
#include <sstream>

void EnumManager::register_enum(const std::string& enum_name, const EnumDefinition& definition) {
    DEBUG_DEBUG(GENERAL, "Registering enum: %s", enum_name.c_str());
    
    // 重複定義チェック
    if (enum_exists(enum_name)) {
        DEBUG_ERROR(GENERAL, "Enum %s already exists", enum_name.c_str());
        return;
    }
    
    // 値の重複チェック
    std::string error_message;
    if (!validate_enum_definition(definition, error_message)) {
        DEBUG_ERROR(GENERAL, "Enum validation failed for %s: %s", 
                   enum_name.c_str(), error_message.c_str());
        return;
    }
    
    // 定義を保存
    enum_definitions_[enum_name] = definition;
    
    DEBUG_INFO(GENERAL, "Successfully registered enum %s with %zu members", 
               enum_name.c_str(), definition.members.size());
}

const EnumDefinition* EnumManager::get_enum_definition(const std::string& enum_name) const {
    auto it = enum_definitions_.find(enum_name);
    if (it != enum_definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

bool EnumManager::get_enum_value(const std::string& enum_name, const std::string& member_name, int64_t& value) const {
    const EnumDefinition* definition = get_enum_definition(enum_name);
    if (!definition) {
        DEBUG_ERROR(GENERAL, "Enum %s not found", enum_name.c_str());
        return false;
    }
    
    const EnumMember* member = definition->find_member(member_name);
    if (!member) {
        DEBUG_ERROR(GENERAL, "Enum member %s::%s not found", 
                   enum_name.c_str(), member_name.c_str());
        return false;
    }
    
    value = member->value;
    DEBUG_DEBUG(GENERAL, "Found enum value %s::%s = %lld", 
               enum_name.c_str(), member_name.c_str(), value);
    return true;
}

bool EnumManager::validate_enum_definition(const EnumDefinition& definition, std::string& error_message) const {
    if (definition.members.empty()) {
        error_message = "Enum must have at least one member";
        return false;
    }
    
    // 値の重複チェック
    for (size_t i = 0; i < definition.members.size(); ++i) {
        for (size_t j = i + 1; j < definition.members.size(); ++j) {
            if (definition.members[i].value == definition.members[j].value) {
                std::ostringstream oss;
                oss << "Duplicate enum value " << definition.members[i].value 
                    << " found in members '" << definition.members[i].name 
                    << "' and '" << definition.members[j].name << "'";
                error_message = oss.str();
                return false;
            }
        }
    }
    
    // メンバー名の重複チェック
    for (size_t i = 0; i < definition.members.size(); ++i) {
        for (size_t j = i + 1; j < definition.members.size(); ++j) {
            if (definition.members[i].name == definition.members[j].name) {
                std::ostringstream oss;
                oss << "Duplicate enum member name '" << definition.members[i].name << "'";
                error_message = oss.str();
                return false;
            }
        }
    }
    
    return true;
}

void EnumManager::clear_all_enums() {
    DEBUG_INFO(GENERAL, "Clearing all enum definitions");
    enum_definitions_.clear();
}

bool EnumManager::enum_exists(const std::string& enum_name) const {
    return enum_definitions_.find(enum_name) != enum_definitions_.end();
}

bool EnumManager::has_duplicate_values_in_definition(const EnumDefinition& definition) const {
    std::string dummy_error;
    return !validate_enum_definition(definition, dummy_error);
}
