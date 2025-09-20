#pragma once
#include "../../common/ast.h"

// 前方宣言
class Interpreter;

// Statement実行エンジンクラス
class StatementExecutor {
public:
    StatementExecutor(Interpreter& interpreter);
    
    // Statement実行の主要メソッド
    void execute_statement(const ASTNode *node);
    
    // 例外処理関連メソッド
    void execute_try_statement(const ASTNode *node);
    void execute_throw_statement(const ASTNode *node);
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
};
