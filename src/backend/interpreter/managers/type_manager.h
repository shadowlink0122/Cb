#pragma once
#include "../../../common/ast.h"
#include <map>
#include <string>

// 前方宣言
class Interpreter;

// 型管理クラス
class TypeManager {
  private:
    Interpreter *interpreter_;

  public:
    TypeManager(Interpreter *interp) : interpreter_(interp) {}
    ~TypeManager() = default;

    // typedef処理
    void register_typedef(const std::string &name,
                          const std::string &type_name);
    std::string resolve_typedef(const std::string &type_name);
    TypeInfo string_to_type_info(const std::string &type_str);

    // 型チェック
    void check_type_range(TypeInfo type, int64_t value,
                          const std::string &var_name);
};
