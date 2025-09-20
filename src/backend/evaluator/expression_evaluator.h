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
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
};
