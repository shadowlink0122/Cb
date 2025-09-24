#pragma once
#include "../../common/ast.h"
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
    
    // 修飾された関数呼び出し評価
    int64_t evaluate_qualified_function_call(const ASTNode *node);
    
    // 修飾された変数参照評価
    int64_t evaluate_qualified_variable_ref(const ASTNode *node);
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
};
