#ifndef STRUCT_SYNC_MANAGER_H
#define STRUCT_SYNC_MANAGER_H

#include <map>
#include <string>

class Interpreter;
struct Variable;

/**
 * @brief 構造体の同期操作を管理するクラス
 *
 * 構造体変数とそのメンバーの同期処理を担当します。
 * 主な機能：
 * - 構造体値から直接アクセスメンバーへの同期
 * - ネストした構造体の再帰的同期
 * - 配列メンバーの同期
 */
class StructSyncManager {
  public:
    /**
     * @brief コンストラクタ
     * @param interpreter Interpreterインスタンスへのポインタ
     */
    explicit StructSyncManager(Interpreter *interpreter);

    /**
     * @brief 直接アクセス変数から構造体メンバーへの同期
     *
     * 個別のメンバー変数（直接アクセス形式）の値を、
     * 構造体変数のstruct_membersマップに同期します。
     * 配列メンバーやネストした構造体も再帰的に処理します。
     *
     * @param var_name 同期する構造体変数の名前
     */
    void sync_struct_members_from_direct_access(const std::string &var_name);

    /**
     * @brief 構造体値から直接アクセス変数への同期
     *
     * 構造体変数の値を、直接アクセス可能な個別のメンバー変数に同期します。
     * ネストした構造体や配列メンバーも再帰的に処理します。
     *
     * @param var_name 同期する構造体変数の名前
     * @param struct_value 構造体の値
     */
    void sync_direct_access_from_struct_value(const std::string &var_name,
                                              const Variable &struct_value);

  private:
    Interpreter *interpreter_; ///< Interpreterインスタンスへのポインタ
};

#endif // STRUCT_SYNC_MANAGER_H
