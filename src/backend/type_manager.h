#pragma once
#include "../common/ast.h"
#include <map>
#include <string>

// 型管理クラス
class TypeManager {
  private:
    std::map<std::string, std::string> typedef_map;

  public:
    TypeManager() = default;
    ~TypeManager() = default;

    // typedef管理
    void registerTypedef(const std::string &alias_name,
                         const std::string &type_definition);
    std::string resolveTypedef(const std::string &type_name);
    bool isTypedefDefined(const std::string &alias_name) const;

    // 型変換とチェック
    TypeInfo stringToTypeInfo(const std::string &type_str);
    std::string typeInfoToString(TypeInfo type);

    // 型範囲チェック
    void checkTypeRange(TypeInfo type, int64_t value,
                        const std::string &var_name,
                        const ASTNode *location = nullptr);

    // 型互換性チェック
    bool isCompatibleType(TypeInfo from, TypeInfo to);
    TypeInfo getPromotedType(TypeInfo type1, TypeInfo type2);

    // 配列型処理
    bool isArrayType(TypeInfo type);
    TypeInfo getArrayBaseType(TypeInfo array_type);
    TypeInfo makeArrayType(TypeInfo base_type);

  private:
    // ヘルパー関数
    void throwRangeError(TypeInfo type, int64_t value,
                         const std::string &var_name, const ASTNode *location);
    int64_t getTypeMinValue(TypeInfo type);
    int64_t getTypeMaxValue(TypeInfo type);
};
