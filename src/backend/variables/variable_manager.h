#pragma once
#include "../../common/ast.h"
#include <string>

// 前方宣言
class Interpreter;
struct Variable;

// 変数管理エンジンクラス
class VariableManager {
public:
    VariableManager(Interpreter& interpreter);
    
    // 変数代入の主要メソッド（全オーバーロード）
    void assign_variable(const std::string &name, int64_t value, TypeInfo type = TYPE_INT);
    void assign_variable(const std::string &name, int64_t value, TypeInfo type, bool is_const);
    void assign_variable(const std::string &name, const std::string &value);
    void assign_variable(const std::string &name, const std::string &value, bool is_const);
    
    // 配列・文字列要素代入
    void assign_array_element(const std::string &name, int64_t index, int64_t value);
    void assign_string_element(const std::string &name, int64_t index, const std::string &value);
    
    // 配列リテラル代入
    void assign_array_literal(const std::string &name, ASTNode* array_literal);
    
    // 型範囲チェック
    void check_type_range(TypeInfo type, int64_t value, const std::string &name);
    
    // 型エイリアス解決
    TypeInfo resolve_type_with_alias(TypeInfo type_info, const std::string& type_name);
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
};
