#pragma once
#include "ast.h"
#include <memory>
#include <string>
#include <unordered_map>

// 型エイリアス（typedef）管理クラス
class TypeAliasRegistry {
  private:
    // エイリアス名 -> 実際の型情報のマッピング
    std::unordered_map<std::string, TypeInfo> aliases_;
    // エイリアス名 -> 配列型詳細情報のマッピング（配列型の場合）
    std::unordered_map<std::string, ArrayTypeInfo> array_aliases_;

  public:
    TypeAliasRegistry() = default;

    // 型エイリアスを登録
    bool register_alias(const std::string &alias_name, TypeInfo actual_type);

    // 配列型エイリアスを登録
    bool register_array_alias(const std::string &alias_name,
                              const ArrayTypeInfo &array_info);

    // 型エイリアスを解決（存在しない場合はTYPE_UNKNOWNを返す）
    TypeInfo resolve_alias(const std::string &alias_name) const;

    // 配列型エイリアスを解決
    ArrayTypeInfo resolve_array_alias(const std::string &alias_name) const;

    // 配列型エイリアスかどうか判定
    bool is_array_alias(const std::string &alias_name) const;

    // エイリアスが存在するかチェック
    bool has_alias(const std::string &alias_name) const;

    // 型情報を完全に解決（エイリアスのエイリアスも解決）
    TypeInfo resolve_complete(TypeInfo type_info,
                              const std::string &type_name = "") const;

    // 全エイリアスをクリア（テスト用）
    void clear();

    // デバッグ用：登録されたエイリアス一覧を取得
    const std::unordered_map<std::string, TypeInfo> &get_all_aliases() const;
    const std::unordered_map<std::string, ArrayTypeInfo> &
    get_all_array_aliases() const;
};

// グローバル型エイリアスレジストリ（シングルトン）
TypeAliasRegistry &get_global_type_alias_registry();

// 型名文字列から TypeInfo への変換（エイリアス解決含む）
TypeInfo parse_type_from_string(const std::string &type_name);

// TypeInfo から文字列への変換（エイリアス名優先）
std::string type_info_to_string_with_aliases(TypeInfo type_info);

// ランタイム配列リテラル解析
struct ArrayLiteralParseResult {
    bool success;
    std::vector<int64_t> values;
    std::string error_message;
};

// 配列リテラル文字列を解析（例: "[1, 2, 3]" -> {1, 2, 3}）
ArrayLiteralParseResult
parse_array_literal_runtime(const std::string &literal_str);

// typedef配列の初期化値を実行時に解析・設定
bool initialize_typedef_array_runtime(const std::string &var_name,
                                      const std::string &typedef_name,
                                      const std::string &init_expr);
