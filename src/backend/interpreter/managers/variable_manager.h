#pragma once
#include "../../../common/ast.h"
#include "core/interpreter.h"
#include <map>
#include <string>
#include <vector>

// 前方宣言
class Interpreter;

// 変数管理クラス
class VariableManager {
  private:
    Interpreter *interpreter_;

  public:
    VariableManager(Interpreter *interp) : interpreter_(interp) {}
    ~VariableManager() = default;

    // スコープ管理
    void push_scope();
    void pop_scope();
    Scope &current_scope();

    // 変数検索
    Variable *find_variable(const std::string &name);
    bool is_global_variable(const std::string &name);

    // 変数宣言
    void declare_global_variable(const ASTNode *node);
    void declare_local_variable(const ASTNode *node);

    // 変数代入
    void assign_variable(const std::string &name, int64_t value, TypeInfo type,
                         bool is_const);
    void assign_variable(const std::string &name, const std::string &value);
    void assign_variable(const std::string &name, const std::string &value,
                         bool is_const);
    void assign_function_parameter(const std::string &name, int64_t value,
                                   TypeInfo type);
    void assign_array_parameter(const std::string &name,
                                const Variable &source_array, TypeInfo type);
    void assign_array_element(const std::string &name, int64_t index,
                              int64_t value);
    void assign_string_element(const std::string &name, int64_t index,
                               char value);

    // 複合処理 (AST_VAR_DECL/AST_ASSIGN ケース)
    void process_var_decl_or_assign(const ASTNode *node);

    // 配列名抽出関数（N次元配列対応）
    std::string extract_array_name(const ASTNode *node);
    std::vector<int64_t> extract_array_indices(const ASTNode *node);
    
    // Priority 3: 変数ポインターから名前を検索
    std::string find_variable_name(const Variable* target_var);

    // Interpreterアクセス
    Interpreter *getInterpreter() { return interpreter_; }
};
