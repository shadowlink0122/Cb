// 式パーサー - 演算子の優先順位に従った式の解析
// Expression Parser - Parse expressions following operator precedence

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "../../common/ast.h"
#include "../recursive_lexer.h"

class RecursiveParser; // 前方宣言

// 式解析を担当するクラス
class ExpressionParser {
public:
    explicit ExpressionParser(RecursiveParser* parser);
    
    // 式の解析エントリーポイント
    ASTNode* parseExpression();
    
    // 演算子の優先順位に従った解析
    ASTNode* parseAssignment();
    ASTNode* parseTernary();
    ASTNode* parseLogicalOr();
    ASTNode* parseLogicalAnd();
    ASTNode* parseBitwiseOr();
    ASTNode* parseBitwiseXor();
    ASTNode* parseBitwiseAnd();
    ASTNode* parseComparison();
    ASTNode* parseShift();
    ASTNode* parseAdditive();
    ASTNode* parseMultiplicative();
    ASTNode* parseUnary();
    ASTNode* parsePostfix();
    ASTNode* parsePrimary();
    
    // メンバーアクセス
    ASTNode* parseMemberAccess(ASTNode* object);
    ASTNode* parseArrowAccess(ASTNode* object);
    
    // リテラル
    ASTNode* parseStructLiteral();
    ASTNode* parseArrayLiteral();
    
private:
    RecursiveParser* parser_;  // 親パーサーへの参照
};

#endif // EXPRESSION_PARSER_H
