#include "ast.h"

// ASTNode staticメンバの初期化
int ASTNode::discard_counter = 0;
int ASTNode::lambda_counter = 0;

// 無名変数用の内部識別子を生成
std::string generate_discard_name() {
    return "__discard_" + std::to_string(++ASTNode::discard_counter);
}

// 無名関数用の内部識別子を生成
std::string generate_lambda_name() {
    return "__lambda_" + std::to_string(++ASTNode::lambda_counter);
}
