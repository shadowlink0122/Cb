#ifndef CB_INTERPRETER_STRUCT_ASSIGNMENT_MANAGER_H
#define CB_INTERPRETER_STRUCT_ASSIGNMENT_MANAGER_H

#include <string>
#include <vector>

// 前方宣言
struct ASTNode;
struct Variable;
class Interpreter;

/**
 * @brief 構造体への代入操作を管理するマネージャークラス
 *
 * このクラスは以下の責務を持ちます：
 * - 構造体リテラルの代入
 * - 構造体メンバーへの値の代入
 * - 構造体メンバー配列への代入
 * - 構造体メンバー配列リテラルの代入
 *
 * Interpreter::assign_struct_*() などのメソッドをこのクラスに移動し、
 * Interpreterクラスのサイズを削減します。
 */
class StructAssignmentManager {
  public:
    explicit StructAssignmentManager(Interpreter *interpreter);

    /**
     * @brief 構造体リテラルを変数に代入する
     *
     * @param var_name 変数名
     * @param literal_node 構造体リテラルのASTノード
     */
    void assign_struct_literal(const std::string &var_name,
                                const ASTNode *literal_node);

    /**
     * @brief 構造体メンバーに値を代入する (int/double/string)
     *
     * @param var_name 変数名
     * @param member_name メンバー名
     * @param value 代入する値
     */
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name, long value);

    /**
     * @brief 構造体メンバーに文字列を代入する
     *
     * @param var_name 変数名
     * @param member_name メンバー名
     * @param str_value 代入する文字列
     */
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const std::string &str_value);

    /**
     * @brief 構造体メンバーに変数値を代入する
     *
     * @param var_name 変数名
     * @param member_name メンバー名
     * @param value_var 代入する値を持つ変数
     */
    void assign_struct_member(const std::string &var_name,
                              const std::string &member_name,
                              const Variable &value_var);

    /**
     * @brief 構造体メンバー（構造体型）に構造体を代入する
     *
     * @param var_name 変数名
     * @param member_name メンバー名
     * @param src_struct 代入する構造体変数
     */
    void assign_struct_member_struct(const std::string &var_name,
                                     const std::string &member_name,
                                     const Variable &src_struct);

    /**
     * @brief 構造体メンバー配列の要素に値を代入する
     *
     * @param var_name 変数名
     * @param member_name メンバー名
     * @param index 配列インデックス
     * @param value 代入する値
     */
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index, long value);

    /**
     * @brief 構造体メンバー配列の要素に変数値を代入する
     *
     * @param var_name 変数名
     * @param member_name メンバー名
     * @param index 配列インデックス
     * @param value_var 代入する値を持つ変数
     */
    void assign_struct_member_array_element(const std::string &var_name,
                                            const std::string &member_name,
                                            int index,
                                            const Variable &value_var);

    /**
     * @brief 構造体メンバー配列に配列リテラルを代入する
     *
     * @param var_name 変数名
     * @param member_name メンバー名
     * @param values 代入する値の配列
     */
    void assign_struct_member_array_literal(const std::string &var_name,
                                            const std::string &member_name,
                                            const std::vector<long> &values);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_STRUCT_ASSIGNMENT_MANAGER_H
