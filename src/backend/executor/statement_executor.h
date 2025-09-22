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
    void execute(const ASTNode *node);
    
private:
    Interpreter& interpreter_;  // インタープリターへの参照
    
    // 個別の実行メソッド
    void execute_assignment(const ASTNode* node);
    void execute_variable_declaration(const ASTNode* node);
};
