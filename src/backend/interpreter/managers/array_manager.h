#pragma once
#include "../../../common/ast.h"
#include "../core/type_inference.h"
#include <vector>

// 前方宣言
class VariableManager;
class ExpressionEvaluator;
class ExpressionService; // DRY効率化: 統一式評価サービス
class Interpreter;
struct Variable;

// 配列管理クラス
class ArrayManager {
  private:
    VariableManager *variable_manager_;
    ExpressionEvaluator *expression_evaluator_;
    Interpreter *interpreter_; // DRY効率化: 統一サービスアクセス用

    // 配列リテラルから次元を抽出する関数
    std::vector<int> extractArrayDimensions(const ASTNode *literal_node);

    // DRY効率化: 統一式評価メソッド
  int64_t evaluate_expression_safe(const ASTNode *node,
                   const std::string &context = "");
  TypedValue evaluate_expression_typed_safe(const ASTNode *node,
                        const std::string &context = "");

  public:
    ArrayManager(VariableManager *vm, ExpressionEvaluator *ee,
                 Interpreter *interpreter = nullptr)
        : variable_manager_(vm), expression_evaluator_(ee),
          interpreter_(interpreter) {}
    ~ArrayManager() = default;

    // 配列宣言処理
    void processArrayDeclaration(Variable &var, const ASTNode *node);

    // 多次元配列リテラル処理
    void processMultidimensionalArrayLiteral(Variable &var,
                                             const ASTNode *literal_node,
                                             TypeInfo base_type);
    void processNDimensionalArrayLiteral(Variable &var,
                                         const ASTNode *literal_node,
                                         TypeInfo base_type);

    // 多次元配列アクセス
  int64_t getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices);
  TypedValue getMultidimensionalArrayElementTyped(
    const Variable &var, const std::vector<int64_t> &indices);
  TypedValue getArrayElementTyped(const Variable &var,
                  const std::vector<int64_t> &indices);
    void setMultidimensionalArrayElement(Variable &var,
                                         const std::vector<int64_t> &indices,
                                         int64_t value);

    // 多次元文字列配列アクセス
    std::string
    getMultidimensionalStringArrayElement(const Variable &var,
                                          const std::vector<int64_t> &indices);
    void
    setMultidimensionalStringArrayElement(Variable &var,
                                          const std::vector<int64_t> &indices,
                                          const std::string &value);

    // 配列初期化
    void initializeArray(Variable &var, TypeInfo base_type,
                         const std::vector<int> &dimensions);
    void initializeMultidimensionalArray(Variable &var,
                                         const ArrayTypeInfo &array_info);

    // 配列サイズ計算
    int calculateTotalSize(const std::vector<int> &dimensions);
    std::vector<int>
    extractDimensionSizes(const std::vector<ArrayDimension> &dimensions);

    // 配列宣言処理
    void declare_array(const ASTNode *node);

    // 配列コピー機能
    void copyArray(Variable &dest, const Variable &src);
    void copyArraySlice(Variable &dest, const Variable &src,
                        const std::vector<int64_t> &slice_indices);
    bool isCompatibleArrayType(const Variable &dest, const Variable &src);

  private:
    // ヘルパー関数
    void processArrayLiteralRecursive(Variable &var, const ASTNode *node,
                                      TypeInfo base_type, int current_dim,
                                      std::vector<int> &current_indices);
    void validateArrayDimensions(const std::vector<int> &expected,
                                 const std::vector<int> &actual);
};
