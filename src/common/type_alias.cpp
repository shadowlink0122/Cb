#include "type_alias.h"
#include "ast.h"
#include <iostream>

// グローバル型エイリアスレジストリのインスタンス
static TypeAliasRegistry global_type_alias_registry;

TypeAliasRegistry &get_global_type_alias_registry() {
    return global_type_alias_registry;
}

bool TypeAliasRegistry::register_alias(const std::string &alias_name,
                                       TypeInfo actual_type) {
    // 基本型かチェック
    if (actual_type == TYPE_VOID) {
        return false;
    }

    aliases_[alias_name] = actual_type;
    return true;
}

TypeInfo TypeAliasRegistry::resolve_alias(const std::string &alias_name) const {
    auto it = aliases_.find(alias_name);
    if (it != aliases_.end()) {
        return it->second;
    }
    return TYPE_INT; // デフォルト
}

bool TypeAliasRegistry::has_alias(const std::string &alias_name) const {
    return aliases_.find(alias_name) != aliases_.end();
}

TypeInfo
TypeAliasRegistry::resolve_complete(TypeInfo type_info,
                                    const std::string &type_name) const {
    if (!type_name.empty()) {
        TypeInfo resolved = resolve_alias(type_name);
        if (resolved != TYPE_INT || has_alias(type_name)) {
            return resolved;
        }
    }
    return type_info;
}

void TypeAliasRegistry::clear() {
    aliases_.clear();
    array_aliases_.clear();
}

bool TypeAliasRegistry::is_array_alias(const std::string &alias_name) const {
    return array_aliases_.find(alias_name) != array_aliases_.end();
}

ArrayTypeInfo
TypeAliasRegistry::resolve_array_alias(const std::string &alias_name) const {
    auto it = array_aliases_.find(alias_name);
    if (it != array_aliases_.end()) {
        return it->second;
    }
    return ArrayTypeInfo(); // デフォルト
}

const std::unordered_map<std::string, TypeInfo> &
TypeAliasRegistry::get_all_aliases() const {
    return aliases_;
}

TypeInfo parse_type_from_string(const std::string &type_name) {
    // 基本型チェック
    if (type_name == "void")
        return TYPE_VOID;
    if (type_name == "tiny")
        return TYPE_TINY;
    if (type_name == "short")
        return TYPE_SHORT;
    if (type_name == "int")
        return TYPE_INT;
    if (type_name == "long")
        return TYPE_LONG;
    if (type_name == "string")
        return TYPE_STRING;
    if (type_name == "char")
        return TYPE_CHAR;
    if (type_name == "bool")
        return TYPE_BOOL;

    // エイリアス解決
    TypeInfo resolved =
        get_global_type_alias_registry().resolve_alias(type_name);
    return resolved;
}

std::string type_info_to_string_with_aliases(TypeInfo type_info) {
    return type_info_to_string(type_info);
}