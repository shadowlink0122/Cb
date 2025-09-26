#pragma once

#include "../../../common/ast.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

// 前方宣言
struct Variable;
class Interpreter;
class VariableManager;

/**
 * 統一変数アクセスサービス - DRY原則に基づく変数操作の統合
 * 全てのクラスが共通して使用する変数検索・操作機能を提供
 */
class VariableAccessService {
  public:
    VariableAccessService(Interpreter *interpreter);

    /**
     * 安全な変数検索（統一版）
     * @param name 変数名
     * @param context エラー時のコンテキスト情報
     * @param allow_null trueの場合、見つからない場合もnullptrを返す（例外なし）
     * @return 変数ポインタ（見つからない場合はnullptrまたは例外）
     */
    Variable *find_variable_safe(const std::string &name,
                                 const std::string &context = "",
                                 bool allow_null = false);

    /**
     * 構造体メンバーの安全なアクセス
     * @param struct_name 構造体名
     * @param member_name メンバー名
     * @param context エラー時のコンテキスト
     * @return メンバー変数ポインタ
     */
    Variable *find_struct_member_safe(const std::string &struct_name,
                                      const std::string &member_name,
                                      const std::string &context = "");

    /**
     * 配列要素の安全なアクセス
     * @param array_name 配列名
     * @param index インデックス
     * @param context エラー時のコンテキスト
     * @return 配列要素変数ポインタ
     */
    Variable *find_array_element_safe(const std::string &array_name,
                                      int64_t index,
                                      const std::string &context = "");

    /**
     * キャッシュ機能付き変数検索（パフォーマンス向上）
     * @param name 変数名
     * @param use_cache キャッシュを使用するかどうか
     * @return 変数ポインタ
     */
    Variable *find_variable_cached(const std::string &name,
                                   bool use_cache = true);

    /**
     * 変数存在チェック
     * @param name 変数名
     * @return 存在するかどうか
     */
    bool variable_exists(const std::string &name);

    /**
     * 変数アクセス統計の取得
     */
    struct AccessStats {
        size_t total_accesses = 0;
        size_t cache_hits = 0;
        size_t cache_misses = 0;
        size_t failed_accesses = 0;
        size_t struct_member_accesses = 0;
        size_t array_element_accesses = 0;
    };

    const AccessStats &get_stats() const { return stats_; }
    void reset_stats() { stats_ = AccessStats{}; }

    /**
     * キャッシュ管理
     */
    void clear_cache();
    void enable_cache(bool enable) { cache_enabled_ = enable; }
    bool is_cache_enabled() const { return cache_enabled_; }

  private:
    Interpreter *interpreter_;
    AccessStats stats_;

    // パフォーマンス向上のためのキャッシュ
    bool cache_enabled_ = true;
    std::unordered_map<std::string, Variable *> variable_cache_;

    // 内部ヘルパー
    void handle_access_error(const std::string &error_msg,
                             const std::string &context);
    void increment_stats(const std::string &access_type, bool success = true);
    void update_cache(const std::string &name, Variable *var);
};
