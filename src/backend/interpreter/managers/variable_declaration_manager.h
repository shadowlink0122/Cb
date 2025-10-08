#ifndef CB_INTERPRETER_VARIABLE_DECLARATION_MANAGER_H
#define CB_INTERPRETER_VARIABLE_DECLARATION_MANAGER_H

#include <string>

// 前方宣言
struct ASTNode;
struct Variable;
class Interpreter;

/**
 * @brief 変数宣言と代入を管理するマネージャークラス
 *
 * VariableManagerから変数宣言・初期化・代入の処理を分離
 * process_var_decl_or_assign()メソッドとその関連処理を担当
 */
class VariableDeclarationManager {
  public:
    explicit VariableDeclarationManager(Interpreter *interpreter);

    /**
     * @brief 変数宣言または代入を処理する
     * 
     * このメソッドは以下を処理します：
     * - AST_VAR_DECL: 変数宣言と初期化
     * - AST_ASSIGN: 変数への代入
     * 
     * @param node 変数宣言または代入のASTノード
     */
    void process_var_decl_or_assign(const ASTNode *node);

  private:
    Interpreter *interpreter_;

    // ========================================================================
    // Helper methods for process_var_decl_or_assign
    // ========================================================================

    /**
     * @brief 関数ポインタの処理
     * @return true if processed as function pointer, false otherwise
     */
    bool process_function_pointer(const ASTNode *node);

    /**
     * @brief 参照型変数の処理
     * @return true if processed as reference, false otherwise
     */
    bool process_reference_variable(const ASTNode *node);

    /**
     * @brief 変数宣言の処理 (AST_VAR_DECL)
     */
    void process_variable_declaration(const ASTNode *node);

    /**
     * @brief 変数代入の処理 (AST_ASSIGN)
     */
    void process_variable_assignment(const ASTNode *node);

    /**
     * @brief Union型値の代入
     */
    void assign_union_value(Variable &var, const std::string &union_type_name,
                            const ASTNode *value_node);

    /**
     * @brief 三項演算子による変数初期化
     */
    void handle_ternary_initialization(Variable &var, const ASTNode *ternary_node);

    /**
     * @brief 配列名抽出（N次元配列対応）
     */
    std::string extract_array_name(const ASTNode *node);
};

#endif // CB_INTERPRETER_VARIABLE_DECLARATION_MANAGER_H
