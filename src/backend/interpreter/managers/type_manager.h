#pragma once
#include "../../../common/ast.h"
#include <map>
#include <string>

struct Variable;

// 前方宣言
class Interpreter;

// 型管理クラス
class TypeManager {
  private:
    Interpreter *interpreter_;
    std::map<std::string, UnionDefinition> union_definitions_;  // union typedef definitions

  public:
    TypeManager(Interpreter *interp) : interpreter_(interp) {}
    ~TypeManager() = default;

    // typedef処理
    void register_typedef(const std::string &name,
                          const std::string &type_name);
    std::string resolve_typedef(const std::string &type_name);
    std::string resolve_typedef_one_level(const std::string &type_name);
    TypeInfo string_to_type_info(const std::string &type_str);

    // union typedef処理
    void register_union_typedef(const std::string &name, const UnionDefinition &union_def);
    bool is_union_type(const std::string &type_name);
    bool is_union_type(const Variable &variable);
    std::string get_union_lookup_name(const Variable &variable) const;
    bool is_value_allowed_for_union(const std::string &type_name, const std::string &str_value);
    bool is_value_allowed_for_union(const std::string &type_name, int64_t int_value);
    bool is_value_allowed_for_union(const std::string &type_name, bool bool_value);
    bool is_custom_type_allowed_for_union(const std::string &union_type_name, const std::string &custom_type_name);
    bool is_array_type_allowed_for_union(const std::string &union_type_name, const std::string &array_type);

    // 型チェック
  void check_type_range(TypeInfo type, int64_t value,
              const std::string &var_name,
              bool is_unsigned = false);
};
