// Expression Parser - 式解析を担当
// Phase 2: RecursiveParserへの委譲実装
#include "expression_parser.h"
#include "../recursive_parser.h"

ExpressionParser::ExpressionParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// Phase 2: 委譲パターンの実装
// 各メソッドは親パーサーの対応するメソッドを呼び出す
// 将来的に、ここで完全な実装に置き換える

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
