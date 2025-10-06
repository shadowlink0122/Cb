// Statement Parser - 文解析を担当
// Phase 2: RecursiveParserへの委譲実装（v0.9.1）
//
// このファイルは、文（statement）解析に関連する11個のメソッドを管理します。
// 
// 管理するメソッド:
// - parseStatement: 文のエントリーポイント
// - parseCompoundStatement: 複合文（{...}）
// - parseIfStatement: if文（if, else if, else）
// - parseForStatement: for文
// - parseWhileStatement: while文
// - parseReturnStatement: return文
// - parseBreakStatement: break文
// - parseContinueStatement: continue文
// - parseAssertStatement: assert文
// - parsePrintlnStatement: println文
// - parsePrintStatement: print文（printf風フォーマット）
//
// Phase 3での実装移行予定:
// - 推定行数: 500-700行
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
