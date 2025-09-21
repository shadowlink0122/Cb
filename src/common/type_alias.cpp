#include "type_alias.h"
#include "ast.h"
#include "debug_messages.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

// グローバル型エイリアスレジストリのインスタンス
static TypeAliasRegistry global_type_alias_registry;

TypeAliasRegistry &get_global_type_alias_registry() {
    return global_type_alias_registry;
}

bool TypeAliasRegistry::register_alias(const std::string &alias_name,
                                       TypeInfo actual_type) {
    // 既存のエイリアスかチェック
    if (has_alias(alias_name)) {
        // 上書き警告（デバッグレベルで出力）
        debug_msg(DebugMsgId::TYPEDEF_REGISTER, alias_name.c_str(),
                  type_info_to_string_basic(actual_type));
    }

    // 基本型かチェック
    if (actual_type == TYPE_UNKNOWN) {
        return false;
    }

    debug_msg(DebugMsgId::TYPEDEF_REGISTER, alias_name.c_str(),
              type_info_to_string_basic(actual_type));

    aliases_[alias_name] = actual_type;
    return true;
}

// 配列型エイリアスを登録
bool TypeAliasRegistry::register_array_alias(const std::string &alias_name,
                                             const ArrayTypeInfo &array_info) {
    // 既存のエイリアスかチェック
    if (has_alias(alias_name) || is_array_alias(alias_name)) {
        debug_msg(DebugMsgId::TYPEDEF_REGISTER, alias_name.c_str(),
                  array_info.to_string().c_str());
    }

    // 配列型情報の妥当性チェック
    if (!array_info.is_array() || array_info.base_type == TYPE_UNKNOWN) {
        return false;
    }

    debug_msg(DebugMsgId::TYPEDEF_REGISTER, alias_name.c_str(),
              array_info.to_string().c_str());

    array_aliases_[alias_name] = array_info;
    // 下位互換性のため通常のaliasesにも登録
    aliases_[alias_name] =
        static_cast<TypeInfo>(TYPE_ARRAY_BASE + array_info.base_type);
    return true;
}

TypeInfo TypeAliasRegistry::resolve_alias(const std::string &alias_name) const {
    debug_msg(DebugMsgId::TYPE_ALIAS_RESOLVE, alias_name.c_str());

    auto it = aliases_.find(alias_name);
    if (it != aliases_.end()) {
        debug_msg(DebugMsgId::TYPE_ALIAS_RUNTIME_RESOLVE, alias_name.c_str(),
                  type_info_to_string_basic(it->second));
        return it->second;
    }
    return TYPE_UNKNOWN;
}

// 配列型エイリアスを解決
ArrayTypeInfo
TypeAliasRegistry::resolve_array_alias(const std::string &alias_name) const {
    auto it = array_aliases_.find(alias_name);
    if (it != array_aliases_.end()) {
        return it->second;
    }
    return ArrayTypeInfo(); // 空の配列型情報を返す
}

// 配列型エイリアスかどうか判定
bool TypeAliasRegistry::is_array_alias(const std::string &alias_name) const {
    return array_aliases_.find(alias_name) != array_aliases_.end();
}

bool TypeAliasRegistry::has_alias(const std::string &alias_name) const {
    return aliases_.find(alias_name) != aliases_.end() ||
           array_aliases_.find(alias_name) != array_aliases_.end();
}

TypeInfo
TypeAliasRegistry::resolve_complete(TypeInfo type_info,
                                    const std::string &type_name) const {
    // 既に基本型の場合はそのまま返す
    if (type_info != TYPE_UNKNOWN) {
        return type_info;
    }

    // 型名が指定されている場合、エイリアス解決を試す
    if (!type_name.empty()) {
        TypeInfo resolved = resolve_alias(type_name);
        if (resolved != TYPE_UNKNOWN) {
            return resolved;
        }
    }

    return TYPE_UNKNOWN;
}

void TypeAliasRegistry::clear() {
    aliases_.clear();
    array_aliases_.clear();
}

const std::unordered_map<std::string, TypeInfo> &
TypeAliasRegistry::get_all_aliases() const {
    return aliases_;
}

const std::unordered_map<std::string, ArrayTypeInfo> &
TypeAliasRegistry::get_all_array_aliases() const {
    return array_aliases_;
}

// 型名文字列から TypeInfo への変換
TypeInfo parse_type_from_string(const std::string &type_name) {
    // まず基本型かチェック
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
    if (type_name == "bool")
        return TYPE_BOOL;
    if (type_name == "string")
        return TYPE_STRING;

    // エイリアス解決を試す
    auto &registry = get_global_type_alias_registry();
    TypeInfo resolved = registry.resolve_alias(type_name);
    if (resolved != TYPE_UNKNOWN) {
        return resolved;
    }

    return TYPE_UNKNOWN;
}

// TypeInfo から文字列への変換（エイリアス名優先）
std::string type_info_to_string_with_aliases(TypeInfo type_info) {
    // まず、この型にエイリアスがあるかチェック
    auto &registry = get_global_type_alias_registry();
    for (const auto &pair : registry.get_all_aliases()) {
        if (pair.second == type_info) {
            return pair.first; // エイリアス名を返す
        }
    }

    // エイリアスがない場合は標準の型名を返す
    return type_info_to_string_basic(type_info);
}

// ランタイム配列リテラル解析の実装
ArrayLiteralParseResult
parse_array_literal_runtime(const std::string &literal_str) {
    ArrayLiteralParseResult result;
    result.success = false;

    // "[1, 2, 3]" 形式の文字列を解析
    if (literal_str.empty() || literal_str.front() != '[' ||
        literal_str.back() != ']') {
        result.error_message = "Invalid array literal format: must start with "
                               "'[' and end with ']'";
        return result;
    }

    // 括弧を除去
    std::string content = literal_str.substr(1, literal_str.length() - 2);

    // 空の配列の場合
    if (content.empty() ||
        (content.find_first_not_of(" \t\n") == std::string::npos)) {
        result.success = true;
        return result;
    }

    // カンマで分割して数値を解析
    size_t start = 0;
    size_t pos = 0;

    while (pos != std::string::npos) {
        pos = content.find(',', start);
        std::string token = (pos != std::string::npos)
                                ? content.substr(start, pos - start)
                                : content.substr(start);

        // 前後の空白を除去
        size_t first = token.find_first_not_of(" \t\n");
        size_t last = token.find_last_not_of(" \t\n");

        if (first == std::string::npos) {
            result.error_message = "Empty value in array literal";
            return result;
        }

        token = token.substr(first, last - first + 1);

        // 数値に変換
        try {
            int64_t value = std::stoll(token);
            result.values.push_back(value);
        } catch (const std::exception &e) {
            result.error_message = "Invalid number in array literal: " + token;
            return result;
        }

        start = pos + 1;
    }

    result.success = true;
    return result;
}

// typedef配列の初期化値を実行時に解析・設定
bool initialize_typedef_array_runtime(const std::string &var_name,
                                      const std::string &typedef_name,
                                      const std::string &init_expr) {
    // この実装は将来的に変数管理システムと連携
    // 現在はプレースホルダー
    return true;
}