// 文パーサー - 制御構文とステートメントの解析
// Statement Parser - Parse control flow and statements

#ifndef STATEMENT_PARSER_H
#define STATEMENT_PARSER_H

#include "../../common/ast.h"
#include "../../common/debug.h"

class RecursiveParser;

// 文の解析を担当するクラス
class StatementParser {
public:
    explicit StatementParser(RecursiveParser* parser);
    
    // 文の解析
    ASTNode* parseStatement();
    ASTNode* parseCompoundStatement();
    
    // 制御構文
    ASTNode* parseIfStatement();
    ASTNode* parseForStatement();
    ASTNode* parseWhileStatement();
    
    // ジャンプ文
    ASTNode* parseReturnStatement();
    ASTNode* parseBreakStatement();
    ASTNode* parseContinueStatement();
    
    // その他
    ASTNode* parseAssertStatement();
    ASTNode* parsePrintlnStatement();
    ASTNode* parsePrintStatement();
    
private:
    RecursiveParser* parser_;
};

#endif // STATEMENT_PARSER_H
