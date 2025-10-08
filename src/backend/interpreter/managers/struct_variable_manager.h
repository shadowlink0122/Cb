#ifndef CB_INTERPRETER_STRUCT_VARIABLE_MANAGER_H
#define CB_INTERPRETER_STRUCT_VARIABLE_MANAGER_H

#include <string>

// 前方宣言
struct ASTNode;
struct Variable;
class Interpreter;

/**
 * @brief 構造体変数の作成と初期化を管理するマネージャークラス
 *
 * このクラスは以下の責務を持ちます：
 * - 構造体変数の作成と初期化
 * - ネストした構造体メンバーの再帰的な作成
 * - 構造体メンバー配列の初期化
 *
 * Interpreter::create_struct_variable() などのメソッドをこのクラスに移動し、
 * Interpreterクラスのサイズを削減します。
 */
class StructVariableManager {
  public:
    explicit StructVariableManager(Interpreter *interpreter);

    /**
     * @brief 構造体変数を作成して初期化する
     *
     * @param var_name 変数名
     * @param struct_type_name 構造体型名
     *
     * 構造体定義を検索し、メンバー変数を初期化した構造体変数を作成します。
     * メンバーが配列や他の構造体の場合も適切に処理します。
     */
    void create_struct_variable(const std::string &var_name,
                                const std::string &struct_type_name);

    /**
     * @brief 構造体メンバー変数を再帰的に作成する
     *
     * @param base_path ベースパス（例: "var.member"）
     * @param struct_type_name 構造体型名
     * @param parent_var 親変数（構造体変数）への参照
     *
     * ネストした構造体のメンバーを再帰的に作成し、初期化します。
     */
    void create_struct_member_variables_recursively(
        const std::string &base_path, const std::string &struct_type_name,
        Variable &parent_var);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_STRUCT_VARIABLE_MANAGER_H
