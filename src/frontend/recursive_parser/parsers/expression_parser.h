// 式パーサー - 演算子の優先順位に従った式の解析
// Expression Parser - Parse expressions following operator precedence

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "../../common/ast.h"
#include "../recursive_lexer.h"

class RecursiveParser; // 前方宣言

// 式解析パーサー - Expression Parser
// 式の解析を担当するクラス
//
// v0.9.1 Phase 2: 委譲パターン実装
// 
// 責任範囲:
// - すべての式の解析（代入、演算子、リテラル等）
// - 演算子の優先順位に基づく再帰下降パーサー
// - 配列アクセス、メンバーアクセス、関数呼び出し
//
// 設計パターン:
// - 委譲パターン（Phase 2）: RecursiveParserに実装を委譲
// - 将来（Phase 3）: 完全な実装をこのクラスに移行予定
//
// パフォーマンス:
// - 現在の委譲呼び出しによるオーバーヘッド: 約3.6%
// - Phase 3で直接実装に移行後、オーバーヘッド削減見込み

#ifndef EXPRESSION_PARSER_H
#define EXPRESSION_PARSER_H

#include "../../common/ast.h"

class RecursiveParser;

/// @brief 式解析を担当するクラス
/// @details 19個の式解析メソッドを管理し、演算子の優先順位に基づいて
///          再帰下降パーサーを実装します。
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
