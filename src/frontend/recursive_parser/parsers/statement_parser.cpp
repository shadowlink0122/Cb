// Statement Parser - 文解析を担当
// Phase 2: RecursiveParserへの委譲実装
#include "statement_parser.h"
#include "../recursive_parser.h"

StatementParser::StatementParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// Phase 2: 委譲パターンの実装

ASTNode* StatementParser::parseStatement() {
    return parser_->parseStatement();
}

ASTNode* StatementParser::parseCompoundStatement() {
    return parser_->parseCompoundStatement();
}

ASTNode* StatementParser::parseIfStatement() {
    return parser_->parseIfStatement();
}

ASTNode* StatementParser::parseForStatement() {
    return parser_->parseForStatement();
}

ASTNode* StatementParser::parseWhileStatement() {
    return parser_->parseWhileStatement();
}

ASTNode* StatementParser::parseReturnStatement() {
    return parser_->parseReturnStatement();
}

ASTNode* StatementParser::parseBreakStatement() {
    return parser_->parseBreakStatement();
}

ASTNode* StatementParser::parseContinueStatement() {
    return parser_->parseContinueStatement();
}

ASTNode* StatementParser::parseAssertStatement() {
    return parser_->parseAssertStatement();
}

ASTNode* StatementParser::parsePrintlnStatement() {
    return parser_->parsePrintlnStatement();
}

ASTNode* StatementParser::parsePrintStatement() {
    return parser_->parsePrintStatement();
}
