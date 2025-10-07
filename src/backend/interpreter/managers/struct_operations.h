#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Forward declarations
class Interpreter;
class RecursiveParser;
struct ASTNode;
struct Variable;
struct TypedValue;
struct StructDefinition;

// Struct操作を管理するクラス (Phase 3.4a-b)
// interpreter.cppから抽出したstruct関連の機能を提供
// Phase 3.4a: 定義登録と検証のみ実装
// Phase 3.4b: 定義検索とパーサー同期
class StructOperations {
  public:
    explicit StructOperations(Interpreter *interpreter);

    // Phase 3.4a: Struct定義の登録と検証
    void register_struct_definition(const std::string &struct_name,
                                    const StructDefinition &definition);
    void validate_struct_recursion_rules();

    // Phase 3.4b: 定義検索とパーサー同期
    const StructDefinition *
    find_struct_definition(const std::string &struct_name);
    void sync_struct_definitions_from_parser(class RecursiveParser *parser);

    // Phase 3.4c: アクセス制御
    void ensure_struct_member_access_allowed(const std::string &accessor_name,
                                             const std::string &member_name);
    bool is_current_impl_context_for(const std::string &struct_type_name);

    // Phase 3.4d: メンバーアクセス
    Variable *get_struct_member(const std::string &var_name,
                                const std::string &member_name);

  private:
    Interpreter *interpreter_;
};
