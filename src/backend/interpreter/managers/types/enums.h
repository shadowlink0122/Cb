#pragma once
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include <cstdint>
#include <string>
#include <unordered_map>

class EnumManager {
  public:
    EnumManager() = default;
    ~EnumManager() = default;

    // enum定義の登録
    void register_enum(const std::string &enum_name,
                       const EnumDefinition &definition);

    // enum定義の取得
    const EnumDefinition *
    get_enum_definition(const std::string &enum_name) const;

    // enum値の取得 (EnumName::member_name -> value)
    bool get_enum_value(const std::string &enum_name,
                        const std::string &member_name, int64_t &value) const;

    // enum値の検証（重複チェック）
    bool validate_enum_definition(const EnumDefinition &definition,
                                  std::string &error_message) const;

    // 全enum定義をクリア
    void clear_all_enums();

    // enum定義が存在するかチェック
    bool enum_exists(const std::string &enum_name) const;

  private:
    // enum名 -> EnumDefinition のマッピング
    std::unordered_map<std::string, EnumDefinition> enum_definitions_;

    // 重複値チェックのヘルパー関数
    bool
    has_duplicate_values_in_definition(const EnumDefinition &definition) const;
};
