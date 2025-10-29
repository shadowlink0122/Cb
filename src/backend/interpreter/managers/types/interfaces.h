#pragma once
#include "../../core/interpreter.h"
#include <map>
#include <string>
#include <vector>

struct ASTNode;
struct InterfaceDefinition;
struct ImplDefinition;
class Interpreter;

/**
 * @brief Interface/Impl定義と操作を管理するクラス
 *
 * このクラスはInterpreterから分離され、以下の責務を持つ:
 * - Interface定義の登録・検索
 * - Impl定義の登録・検索
 * - Interface型変数の管理
 * - Implコンテキストの処理
 * - 一時変数の管理（メソッドチェーン用）
 */
class InterfaceOperations {
  public:
    explicit InterfaceOperations(Interpreter *interpreter);
    ~InterfaceOperations() = default;

    // Interface定義管理
    void register_interface_definition(const std::string &interface_name,
                                       const InterfaceDefinition &definition);
    const InterfaceDefinition *
    find_interface_definition(const std::string &interface_name);

    // Impl定義管理
    void register_impl_definition(const ImplDefinition &impl_def);
    const ImplDefinition *
    find_impl_for_struct(const std::string &struct_name,
                         const std::string &interface_name);
    const std::vector<ImplDefinition> &get_impl_definitions() const;

    // Interface型変数管理
    void create_interface_variable(const std::string &var_name,
                                   const std::string &interface_name);
    Variable *get_interface_variable(const std::string &var_name);

    // Impl宣言処理
    void handle_impl_declaration(const ASTNode *node);

    // Self処理用ヘルパー
    std::string get_self_receiver_path();
    void sync_self_to_receiver(const std::string &receiver_path);

    // 一時変数管理（メソッドチェーン用）
    void add_temp_variable(const std::string &name, const Variable &var);
    void remove_temp_variable(const std::string &name);
    void clear_temp_variables();

    // マップへのアクセス（内部使用）
    std::map<std::string, InterfaceDefinition> *
    get_interface_definitions_mutable() {
        return &interface_definitions_;
    }
    std::vector<ImplDefinition> *get_impl_definitions_mutable() {
        return &impl_definitions_;
    }

    // v0.11.0 Phase 1a: インターフェース境界チェック（複数境界対応）
    bool check_interface_bound(const std::string &type_name,
                               const std::string &interface_name);
    void validate_interface_bounds(
        const std::string &struct_name,
        const std::vector<std::string> &type_parameters,
        const std::vector<std::string> &type_arguments,
        const std::unordered_map<std::string, std::vector<std::string>>
            &interface_bounds);

  private:
    Interpreter *interpreter_; // 親Interpreterへの参照

    // Interface/Impl定義ストレージ
    std::map<std::string, InterfaceDefinition> interface_definitions_;
    std::vector<ImplDefinition> impl_definitions_;

    // v0.12.0: インスタンス化されたimplノードのキャッシュ
    std::vector<std::unique_ptr<ASTNode>> instantiated_impl_nodes_;
};
