#pragma once

#include "../../../common/ast.h"
#include <cstdint>
#include <functional>
#include <string>

// 前方宣言
class Interpreter;
class ExpressionEvaluator;

/**
 * 統一式評価サービス - DRY原則に基づく式評価処理の統合
 * 全てのクラスが共通して使用する式評価機能を提供
 */
class ExpressionService {
  public:
    ExpressionService(Interpreter *interpreter);

    /**
     * エラーハンドリング付き式評価（統一版）
     * @param node 評価するASTノード
     * @param context エラー時のコンテキスト情報
     * @param error_handler カスタムエラーハンドラー（オプション）
     * @return 評価結果
     */
    int64_t evaluate_safe(
        const ASTNode *node, const std::string &context = "",
        std::function<void(const std::string &)> error_handler = nullptr);

    /**
     * 条件式評価の統一処理
     * @param condition_node 条件式のASTノード
     * @param context エラー時のコンテキスト
     * @return 条件の真偽値（0 = false, 非0 = true）
     */
    int64_t evaluate_condition(const ASTNode *condition_node,
                               const std::string &context = "条件式");

    /**
     * 配列インデックス評価の統一処理（境界チェック付き）
     * @param index_node インデックス式のASTノード
     * @param array_size 配列のサイズ
     * @param var_name 変数名（エラーメッセージ用）
     * @return 有効なインデックス値
     */
    int64_t evaluate_array_index(const ASTNode *index_node, size_t array_size,
                                 const std::string &var_name);

    /**
     * 式評価の統計情報を取得（パフォーマンス監視用）
     */
    struct EvaluationStats {
        size_t total_evaluations = 0;
        size_t failed_evaluations = 0;
        size_t condition_evaluations = 0;
        size_t array_index_evaluations = 0;
    };

    const EvaluationStats &get_stats() const { return stats_; }
    void reset_stats() { stats_ = EvaluationStats{}; }

    /**
     * デバッグモード状態の取得（Interpreter経由）
     * @return デバッグモードが有効かどうか
     */
    bool is_debug_mode() const;

  private:
    Interpreter *interpreter_;
    ExpressionEvaluator *expression_evaluator_;
    EvaluationStats stats_;

    // 内部ヘルパー
    void handle_evaluation_error(const std::string &error_msg,
                                 const std::string &context);
    void increment_stats(const std::string &evaluation_type,
                         bool success = true);
};
