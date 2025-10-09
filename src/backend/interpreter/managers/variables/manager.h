#pragma once
#include "../../../../common/ast.h"
#include "../../core/interpreter.h"
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
    void assign_variable(const std::string &name, const TypedValue &value,
                         TypeInfo type_hint, bool is_const);
    void assign_function_parameter(const std::string &name, int64_t value,
                                   TypeInfo type, bool is_unsigned);
    void assign_function_parameter(const std::string &name,
                                   const TypedValue &value, TypeInfo type,
                                   bool is_unsigned);
    void assign_array_parameter(const std::string &name,
                                const Variable &source_array, TypeInfo type);
    void assign_array_element(const std::string &name, int64_t index,
                              int64_t value);
    void assign_string_element(const std::string &name, int64_t index,
                               char value);

    // interface補助
    void assign_interface_view(const std::string &dest_name,
                               Variable interface_var,
                               const Variable &source_var,
                               const std::string &source_var_name);
    bool interface_impl_exists(const std::string &interface_name,
                               const std::string &struct_type_name) const;
    std::string resolve_interface_source_type(const Variable &source_var) const;

    // union型代入
    void assign_union_value(Variable &var, const std::string &union_type_name,
                            const ASTNode *value_node);

    // 複合処理 (AST_VAR_DECL/AST_ASSIGN ケース)
    void process_var_decl_or_assign(const ASTNode *node);

    // 三項演算子による変数初期化
    void handle_ternary_initialization(Variable &var,
                                       const ASTNode *ternary_node);

    // ========================================================================
    // Helper methods for process_var_decl_or_assign
    // ========================================================================

    /**
     * @brief 関数ポインタの処理
     * @return true if processed as function pointer, false otherwise
     */
    bool handle_function_pointer(const ASTNode *node);

    /**
     * @brief 参照型変数の処理
     * @return true if processed as reference, false otherwise
     */
    bool handle_reference_variable(const ASTNode *node);

    /**
     * @brief ArrayTypeInfo設定時の変数初期化処理
     * @return true if processed with ArrayTypeInfo, false otherwise
     */
    bool handle_array_type_info_declaration(const ASTNode *node, Variable &var);

    /**
     * @brief Union typedef変数の処理
     * @return true if processed as union typedef (処理完了), false otherwise
     */
    bool handle_union_typedef_declaration(const ASTNode *node, Variable &var);

    // 配列名抽出関数（N次元配列対応）
    std::string extract_array_name(const ASTNode *node);
    std::vector<int64_t> extract_array_indices(const ASTNode *node);

    // Priority 3: 変数ポインターから名前を検索
    std::string find_variable_name(const Variable *target_var);

    // Interpreterアクセス
    Interpreter *getInterpreter() { return interpreter_; }

  private:
    /**
     * @brief 変数宣言処理（AST_VAR_DECL）の内部実装
     */
    void process_variable_declaration(const ASTNode *node);

    /**
     * @brief 変数代入処理（AST_ASSIGN）の内部実装
     */
    void process_variable_assignment(const ASTNode *node);

    /**
     * @brief typedef解決と型設定を行う
     * @return true if typedef was resolved, false otherwise
     */
    bool handle_typedef_resolution(const ASTNode *node, Variable &var);

    int resolve_array_size_expression(const std::string &size_expr,
                                      const ASTNode *node);
    std::vector<ArrayDimension>
    parse_array_dimensions(const std::string &array_part, const ASTNode *node);
    void initialize_array_from_dimensions(
        Variable &var, TypeInfo base_type,
        const std::vector<ArrayDimension> &dimensions);

    /**
     * @brief 構造体変数のメンバー初期化を行う
     * @param node 変数宣言ノード
     * @param var 初期化する変数
     */
    void handle_struct_member_initialization(const ASTNode *node,
                                             Variable &var);

    /**
     * @brief Interface型変数の初期化処理
     * @param node 変数宣言ノード
     * @param var 初期化する変数
     * @return true if handled as interface initialization (early return), false
     * otherwise
     */
    bool handle_interface_initialization(const ASTNode *node, Variable &var);

    /**
     * @brief 配列リテラル初期化処理
     * @param node 変数宣言ノード
     * @param var 初期化する変数
     * @return true if handled as array literal initialization (early return),
     * false otherwise
     */
    bool handle_array_literal_initialization(const ASTNode *node,
                                             Variable &var);

    /**
     * @brief unsigned変数の負の値をクランプするヘルパー
     */
    void clamp_unsigned_value(Variable &target, int64_t &value,
                              const char *context, const ASTNode *node);
};
