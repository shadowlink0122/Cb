#pragma once
#include "core/interpreter.h"
#include <map>
#include <string>

struct ASTNode;
class Interpreter;

/**
 * @brief Static変数とImpl Static変数の管理を担当するクラス
 * 
 * このクラスはInterpreterから分離され、以下の責務を持つ:
 * - 関数スコープのstatic変数の作成・検索
 * - implブロック内のstatic変数の作成・検索
 * - implコンテキストの管理
 */
class StaticVariableManager {
  public:
    explicit StaticVariableManager(Interpreter *interpreter);
    ~StaticVariableManager() = default;

    // Static変数管理
    Variable *find_static_variable(const std::string &name);
    void create_static_variable(const std::string &name, const ASTNode *node);

    // Impl Static変数管理
    Variable *find_impl_static_variable(const std::string &name);
    void create_impl_static_variable(const std::string &name,
                                     const ASTNode *node);

    // Implコンテキスト管理
    void enter_impl_context(const std::string &interface_name,
                            const std::string &struct_type_name);
    void exit_impl_context();
    std::string get_impl_static_namespace() const;

    // マップへのアクセス（読み取り専用、イテレーション用）
    const std::map<std::string, Variable> &get_static_variables() const {
        return static_variables_;
    }
    const std::map<std::string, Variable> &get_impl_static_variables() const {
        return impl_static_variables_;
    }

    // マップへの書き込みアクセス（特定のケースで必要）
    std::map<std::string, Variable> *get_static_variables_mutable() {
        return &static_variables_;
    }

  private:
    Interpreter *interpreter_; // 親Interpreterへの参照

    // Static変数ストレージ
    std::map<std::string, Variable> static_variables_;
    std::map<std::string, Variable> impl_static_variables_;

    // Implコンテキスト
    struct ImplContext {
        std::string interface_name;
        std::string struct_type_name;
        bool is_active = false;
    } current_impl_context_;
};
