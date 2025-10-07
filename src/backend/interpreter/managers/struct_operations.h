#pragma once
#include "../../../../common/ast.h"
#include "../core/interpreter.h"
#include <string>
#include <vector>

// Struct操作を管理するクラス
// interpreter.cppから抽出したstruct関連の機能を提供
class StructOperations {
  public:
    explicit StructOperations(Interpreter *interpreter);

    // Struct定義の登録と検証
    void register_struct_definition(const std::string &struct_name,
                                    const StructDefinition &definition);
    void validate_struct_recursion_rules();

    // Struct変数の作成と管理
    void create_struct_variable(const std::string &var_name,
                                const std::string &struct_type_name);
    void create_struct_member_variables_recursively(
        const std::string &base_path, const std::string &struct_type_name,
        Variable &parent_var);

    // Structリテラルの代入
    void assign_struct_literal(const std::string &var_name,
                               const ASTNode *literal_node);

    // Structメンバーの代入 (複数のオーバーロード)
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name, int64_t value);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const std::string &value);
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const TypedValue &typed_value);
    void assign_struct_member_struct(const std::string &var_name,
                                     const std::string &member_name,
                                     const Variable &struct_value);

    // Structメンバー配列の操作
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index, int64_t value);
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index,
                                            const std::string &value);
    int64_t get_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index);
    int64_t get_struct_member_multidim_array_element(
        const std::string &var_name, const std::string &member_name,
        const std::vector<int64_t> &indices);
    std::string get_struct_member_array_string_element(
        const std::string &var_name, const std::string &member_name, int index);
    void assign_struct_member_array_literal(const std::string &var_name,
                                            const std::string &member_name,
                                            const ASTNode *array_literal);

    // Struct同期操作
    void sync_struct_members_from_direct_access(const std::string &var_name);
    void sync_direct_access_from_struct_value(const std::string &var_name,
                                              const Variable &struct_value);
    void sync_individual_member_from_struct(Variable *struct_var,
                                            const std::string &member_name);

    // アクセス権限チェック
    void ensure_struct_member_access_allowed(const std::string &accessor_name,
                                             const std::string &member_name);

  private:
    Interpreter *interpreter_;
};
