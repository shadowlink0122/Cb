// Expression Parser - 式解析を担当
// Phase 2: RecursiveParserへの委譲実装（v0.9.1）
// 
// このファイルは、式解析に関連する19個のメソッドを管理します。
// 現在は委譲パターンを使用しており、実際の実装はRecursiveParserに残されています。
//
// 管理するメソッド:
// - parseExpression: エントリーポイント
// - parseAssignment: 代入演算子（=, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=）
// - parseTernary: 三項演算子（? :）
// - parseLogicalOr: 論理OR（||）
// - parseLogicalAnd: 論理AND（&&）
// - parseBitwiseOr: ビットOR（|）
// - parseBitwiseXor: ビットXOR（^）
// - parseBitwiseAnd: ビットAND（&）
// - parseComparison: 比較演算子（==, !=, <, >, <=, >=）
// - parseShift: ビットシフト（<<, >>）
// - parseAdditive: 加算・減算（+, -）
// - parseMultiplicative: 乗算・除算・剰余（*, /, %）
// - parseUnary: 単項演算子（!, -, ~, &, *, ++, --）
// - parsePostfix: 後置演算子（++, --）、配列アクセス、メンバーアクセス
// - parsePrimary: リテラル、識別子、関数呼び出し、括弧式
// - parseMemberAccess: ドット演算子（obj.member）
// - parseArrowAccess: アロー演算子（ptr->member）
// - parseStructLiteral: 構造体リテラル（{a: 1, b: 2}）
// - parseArrayLiteral: 配列リテラル（[1, 2, 3]）
//
// Phase 3での実装移行予定:
// - 各メソッドの実装をこのファイルに移行
// - parser_->current_token_ などの内部状態アクセスを整理
// - RecursiveParserの式解析メソッドを削除またはスタブ化
// - 推定行数: 800-1000行
#include "expression_parser.h"
#include "../recursive_parser.h"

ExpressionParser::ExpressionParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// Phase 2: 委譲パターンの実装
// 各メソッドは親パーサーの対応するメソッドを呼び出す
// Phase 3で完全な実装に置き換え予定

ASTNode* ExpressionParser::parseExpression() {
    // 現時点では親パーサーに委譲
    // TODO: Phase 3で完全な実装をここに移行
    return parser_->parseExpression();
}

ASTNode* ExpressionParser::parseAssignment() {
    return parser_->parseAssignment();
}

ASTNode* ExpressionParser::parseTernary() {
    return parser_->parseTernary();
}

ASTNode* ExpressionParser::parseLogicalOr() {
    return parser_->parseLogicalOr();
}

ASTNode* ExpressionParser::parseLogicalAnd() {
    return parser_->parseLogicalAnd();
}

ASTNode* ExpressionParser::parseBitwiseOr() {
    return parser_->parseBitwiseOr();
}

ASTNode* ExpressionParser::parseBitwiseXor() {
    return parser_->parseBitwiseXor();
}

ASTNode* ExpressionParser::parseBitwiseAnd() {
    return parser_->parseBitwiseAnd();
}

ASTNode* ExpressionParser::parseComparison() {
    return parser_->parseComparison();
}

ASTNode* ExpressionParser::parseShift() {
    return parser_->parseShift();
}

ASTNode* ExpressionParser::parseAdditive() {
    return parser_->parseAdditive();
}

ASTNode* ExpressionParser::parseMultiplicative() {
    return parser_->parseMultiplicative();
}

ASTNode* ExpressionParser::parseUnary() {
    return parser_->parseUnary();
}

ASTNode* ExpressionParser::parsePostfix() {
    return parser_->parsePostfix();
}

ASTNode* ExpressionParser::parsePrimary() {
    return parser_->parsePrimary();
}

ASTNode* ExpressionParser::parseMemberAccess(ASTNode* object) {
    return parser_->parseMemberAccess(object);
}

ASTNode* ExpressionParser::parseArrowAccess(ASTNode* object) {
    return parser_->parseArrowAccess(object);
}

ASTNode* ExpressionParser::parseStructLiteral() {
    return parser_->parseStructLiteral();
}

ASTNode* ExpressionParser::parseArrayLiteral() {
    return parser_->parseArrayLiteral();
}
