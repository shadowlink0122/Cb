#pragma once
#include "../../../common/ast.h"
#include "../core/type_inference.h"
#include "expression_receiver_resolution.h" // Phase 12: Method Receiver Resolution
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// 前方宣言
class Interpreter;
struct Variable;
class ReturnException;
class BreakException;

// Phase 12: Method Receiver Resolutionの型エイリアス
using MethodReceiverResolution =
    ReceiverResolutionHelpers::MethodReceiverResolution;

// 式評価エンジンクラス
class ExpressionEvaluator {
  public:
    ExpressionEvaluator(Interpreter &interpreter);

    // 式評価の主要メソッド
    int64_t evaluate_expression(const ASTNode *node);

    // 型推論対応の式評価
    TypedValue evaluate_typed_expression(const ASTNode *node);

    // 型推論対応の三項演算子評価
    TypedValue evaluate_ternary_typed(const ASTNode *node);

    // 修飾された関数呼び出し評価
    int64_t evaluate_qualified_function_call(const ASTNode *node);

    // 修飾された変数参照評価
    int64_t evaluate_qualified_variable_ref(const ASTNode *node);

    // 関数戻り値からのメンバーアクセス処理
    TypedValue evaluate_function_member_access(const ASTNode *func_node,
                                               const std::string &member_name);

    // 関数戻り値からの配列アクセス処理
    TypedValue evaluate_function_array_access(const ASTNode *func_node,
                                              const ASTNode *index_node);

    // 関数戻り値からの複合アクセス処理（func()[index].member）
    TypedValue
    evaluate_function_compound_access(const ASTNode *func_node,
                                      const ASTNode *index_node,
                                      const std::string &member_name);

  private:
    Interpreter &interpreter_;        // インタープリターへの参照
    TypeInferenceEngine type_engine_; // 型推論エンジン

    // 最後の型推論結果キャッシュ（文字列結果を保持するため）
    TypedValue last_typed_result_;
    std::optional<std::pair<const ASTNode *, TypedValue>>
        last_captured_function_value_;

    // 構造体メンバー取得関数
    Variable get_struct_member_from_variable(const Variable &struct_var,
                                             const std::string &member_name);

    // 再帰的メンバーアクセス処理（将来のネスト構造体対応）
    TypedValue evaluate_recursive_member_access(
        const Variable &base_var, const std::vector<std::string> &member_path);

    // 遅延評価されたTypedValueを実際に評価する
    TypedValue resolve_deferred_evaluation(const TypedValue &deferred_value);

    // 型推論対応の式評価（内部実装）
    TypedValue evaluate_typed_expression_internal(const ASTNode *node);

    TypedValue consume_numeric_typed_value(const ASTNode *node,
                                           int64_t numeric_result,
                                           const InferredType &inferred_type);

    bool resolve_variable_name(const ASTNode *node, std::string &out_name,
                               Variable *&out_var);

    // ========================================================================
    // NOTE: Tier 2リファクタリングで抽出されたヘルパーメソッドは
    // expression_helpers.h/.cpp に移動しました
    // ========================================================================

  public:
    // 最後の型推論結果にアクセス
    const TypedValue &get_last_typed_result() const {
        return last_typed_result_;
    }
    void set_last_typed_result(const TypedValue &value) {
        last_typed_result_ = value;
    }

    // Interpreterへのアクセス（ヘルパーモジュール用）
    Interpreter &get_interpreter() { return interpreter_; }

    // TypeInferenceEngineへのアクセス（ヘルパーモジュール用）
    TypeInferenceEngine &get_type_engine() { return type_engine_; }

    // last_captured_function_value_へのアクセス（ヘルパーモジュール用）
    std::optional<std::pair<const ASTNode *, TypedValue>> &
    get_last_captured_function_value() {
        return last_captured_function_value_;
    }

    // Dispatcher用の実装メソッド（巨大なケースを分離）
    int64_t evaluate_function_call_impl(const ASTNode *node);
    int64_t evaluate_member_access_impl(const ASTNode *node);
};
