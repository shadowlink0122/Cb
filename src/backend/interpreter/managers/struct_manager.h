#ifndef STRUCT_MANAGER_H
#define STRUCT_MANAGER_H

#include <string>
#include <vector>
#include "common/ast.h"

class Interpreter;
class Variable;
class ASTNode;

/**
 * StructManager - 構造体の定義、初期化、メンバーアクセス管理
 * 
 * 責務:
 * - 構造体定義の登録・検索
 * - 構造体インスタンスの作成・初期化
 * - 構造体メンバーへのアクセス・代入
 * - 構造体リテラルの処理
 * - 構造体の再帰検証
 */
class StructManager {
public:
    explicit StructManager(Interpreter* interpreter);
    ~StructManager() = default;

    // 構造体定義の登録と検証
    void register_struct_definition(const std::string& struct_name,
                                   const std::vector<StructMember>& members,
                                   const ASTNode* definition_node);
    void validate_struct_recursion_rules();
    
    // パーサーとの同期
    void sync_struct_definitions_from_parser(class RecursiveParser* parser);
    
    // 構造体変数の作成と初期化
    void create_struct_variable(const std::string& var_name,
                               const std::string& struct_type,
                               bool is_global = false);
    
    void create_struct_member_variables_recursively(const std::string& base_path,
                                                   const std::string& struct_type,
                                                   bool is_global);
    
    // 構造体リテラルの代入
    void assign_struct_literal(const std::string& var_name,
                              const ASTNode* literal_node);
    
    // 構造体メンバーのアクセス
    Variable* get_struct_member(const std::string& var_name,
                               const std::string& member_path);
    
    // 構造体メンバーへの代入（複数のオーバーロード）
    void assign_struct_member(const std::string& var_name,
                            const std::string& member_path,
                            int64_t value);
    
    void assign_struct_member(const std::string& var_name,
                            const std::string& member_path,
                            const std::string& value);
    
    void assign_struct_member(const std::string& var_name,
                            const std::string& member_path,
                            double value);
    
    void assign_struct_member_struct(const std::string& var_name,
                                    const std::string& member_path,
                                    Variable* source_struct);
    
    // 構造体メンバー配列の操作
    void assign_struct_member_array_element(const std::string& var_name,
                                          const std::string& member_path,
                                          int64_t index,
                                          int64_t value);
    
    void assign_struct_member_array_element(const std::string& var_name,
                                          const std::string& member_path,
                                          int64_t index,
                                          const std::string& value);
    
    int64_t get_struct_member_array_element(const std::string& var_name,
                                           const std::string& member_path,
                                           int64_t index);
    
    int64_t get_struct_member_multidim_array_element(const std::string& var_name,
                                                     const std::string& member_path,
                                                     const std::vector<int64_t>& indices);
    
    std::string get_struct_member_array_string_element(const std::string& var_name,
                                                       const std::string& member_path,
                                                       int64_t index);
    
    void assign_struct_member_array_literal(const std::string& var_name,
                                          const std::string& member_path,
                                          const ASTNode* literal_node);
    
    // 構造体メンバーの同期
    void sync_struct_members_from_direct_access(const std::string& var_name);
    void sync_direct_access_from_struct_value(const std::string& var_name,
                                            const Variable* struct_var);
    void sync_individual_member_from_struct(Variable* struct_var,
                                          const std::string& member_name);
    
    // アクセス制御
    void ensure_struct_member_access_allowed(const std::string& accessor_name,
                                           const std::string& struct_var_name,
                                           const std::string& member_name);
    
    // impl コンテキストチェック
    bool is_current_impl_context_for(const std::string& struct_type_name);

private:
    Interpreter* interpreter_;
};

#endif // STRUCT_MANAGER_H
