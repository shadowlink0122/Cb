#pragma once

#include "../../../common/ast.h"
#include <cstdint>
#include <string>
#include <vector>

// 前方宣言
struct Variable;
class Interpreter;
class ExpressionEvaluator;

/**
 * 共通操作クラス - DRY原則に基づく重複コードの統合
 * 配列操作、型チェック、式評価などの共通処理を提供
 */
class CommonOperations {
  public:
    CommonOperations(Interpreter *interpreter);

    // 配列リテラル処理の統合
    struct ArrayLiteralResult {
        std::vector<int64_t> int_values;
        std::vector<std::string> string_values;
        TypeInfo element_type;
        size_t size;
        bool is_string_array;
    };

    /**
     * 配列リテラルを解析して統一形式で返す
     */
    ArrayLiteralResult parse_array_literal(const ASTNode *literal_node);

    /**
     * 変数に配列リテラルを代入（統合版）
     */
    void assign_array_literal_to_variable(Variable *var,
                                          const ArrayLiteralResult &result);

    /**
     * 配列要素代入の統合処理（境界チェック、型チェック込み）
     */
    void assign_array_element_safe(Variable *var, int64_t index, int64_t value,
                                   const std::string &var_name);
    void assign_array_element_safe(Variable *var, int64_t index,
                                   const std::string &value,
                                   const std::string &var_name);

    /**
     * 式評価の統合処理（エラーハンドリング込み）
     */
    int64_t evaluate_expression_safe(const ASTNode *node,
                                     const std::string &context = "");

    /**
     * 型チェックの統合処理
     */
    void check_type_compatibility(TypeInfo expected, TypeInfo actual,
                                  const std::string &context);
    void check_array_bounds(const Variable *var, int64_t index,
                            const std::string &var_name);
    void check_const_assignment(const Variable *var,
                                const std::string &var_name);

    /**
     * 配列初期化の統合処理
     */
    void initialize_array_variable(Variable *var, TypeInfo base_type, int size,
                                   bool is_string_array = false);

    /**
     * デバッグ出力の統合処理
     */
    void debug_array_operation(const std::string &operation,
                               const std::string &var_name, int64_t index = -1,
                               int64_t value = 0);

  private:
    Interpreter *interpreter_;
    ExpressionEvaluator *expression_evaluator_;

    // 内部ヘルパー関数
    TypeInfo infer_array_element_type(const ASTNode *literal_node);
    void validate_array_literal_consistency(const ASTNode *literal_node);
};
