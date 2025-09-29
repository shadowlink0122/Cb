#pragma once
#include "../../../common/ast.h"
#include "../core/type_inference.h"
#include <string>

// 前方宣言
class Interpreter;
struct Variable;
class ReturnException;
class BreakException;

// 式評価エンジンクラス
class ExpressionEvaluator {
public:
    ExpressionEvaluator(Interpreter& interpreter);
    
    // 式評価の主要メソッド
    int64_t evaluate_expression(const ASTNode *node);
    
    // 型推論対応の式評価
    TypedValue evaluate_typed_expression(const ASTNode *node);
    
    // 修飾された関数呼び出し評価
    int64_t evaluate_qualified_function_call(const ASTNode *node);
    
    // 修飾された変数参照評価
    int64_t evaluate_qualified_variable_ref(const ASTNode *node);
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
    TypeInferenceEngine type_engine_;  // 型推論エンジン
    
    // 最後の型推論結果キャッシュ（文字列結果を保持するため）
    TypedValue last_typed_result_;
    
    // ヘルパー関数
    std::string type_info_to_string(TypeInfo type);
    void sync_self_changes_to_receiver(const std::string& receiver_name, Variable* receiver_var);
    
    // 型推論対応のヘルパー
    TypedValue evaluate_ternary_typed(const ASTNode* node);

public:
    // 最後の型推論結果にアクセス
    const TypedValue& get_last_typed_result() const { return last_typed_result_; }
};
