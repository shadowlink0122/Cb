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
    
    // 型範囲チェック
    void check_type_range(TypeInfo type, int64_t value, const std::string &name);
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
};
