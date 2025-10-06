// Statement Parser - 文解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、文（Statement）の解析を担当します。
// 制御構造、ジャンプ文、出力文などを解析します。
//
// 【サポートする文の種類】:
// 1. 複合文（ブロック） { ... }
// 2. 条件分岐 if (...) { ... } else { ... }
// 3. ループ for (...) { ... }, while (...) { ... }
// 4. ジャンプ return, break, continue
// 5. 出力 println(...), print(...)
// 6. アサーション assert(...)
//
#include "statement_parser.h"
#include "../recursive_parser.h"

StatementParser::StatementParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// ========================================
// 文解析のエントリーポイント
// ========================================

/**
 * @brief 文解析のエントリーポイント
 * @return 解析されたAST文ノード
 * 
 * トークンの種類に応じて適切な解析メソッドを呼び出します
 */
ASTNode* StatementParser::parseStatement() {
    return parser_->parseStatement();
}

// ========================================
// 複合文
// ========================================

/**
 * @brief 複合文（ブロック）を解析
 * @return 解析されたAST複合文ノード
 * 
 * 構文: { statement1; statement2; ... }
 * 新しいスコープを作成します
 */
ASTNode* StatementParser::parseCompoundStatement() {
    return parser_->parseCompoundStatement();
}

// ========================================
// 制御構造
// ========================================

/**
 * @brief if文を解析
 * @return 解析されたASTif文ノード
 * 
 * 構文:
 * - if (condition) statement
 * - if (condition) statement else statement
 * - if (condition) statement else if (condition) statement else statement
 */
ASTNode* StatementParser::parseIfStatement() {
    return parser_->parseIfStatement();
}

/**
 * @brief for文を解析
 * @return 解析されたASTfor文ノード
 * 
 * 構文: for (init; condition; update) statement
 * 
 * サポートする形式:
 * - for (int i = 0; i < 10; i++) { ... }
 * - for (; condition; ) { ... }
 */
ASTNode* StatementParser::parseForStatement() {
    return parser_->parseForStatement();
}

/**
 * @brief while文を解析
 * @return 解析されたASTwhile文ノード
 * 
 * 構文: while (condition) statement
 */
ASTNode* StatementParser::parseWhileStatement() {
    return parser_->parseWhileStatement();
}

// ========================================
// ジャンプ文
// ========================================

/**
 * @brief return文を解析
 * @return 解析されたASTreturn文ノード
 * 
 * 構文:
 * - return;
 * - return expression;
 */
ASTNode* StatementParser::parseReturnStatement() {
    return parser_->parseReturnStatement();
}

/**
 * @brief break文を解析
 * @return 解析されたASTbreak文ノード
 * 
 * 構文: break;
 * ループまたはswitchから脱出（現在はループのみサポート）
 */
ASTNode* StatementParser::parseBreakStatement() {
    return parser_->parseBreakStatement();
}

/**
 * @brief continue文を解析
 * @return 解析されたASTcontinue文ノード
 * 
 * 構文: continue;
 * ループの次の反復へスキップ
 */
ASTNode* StatementParser::parseContinueStatement() {
    return parser_->parseContinueStatement();
}

// ========================================
// 出力・デバッグ
// ========================================

/**
 * @brief assert文を解析
 * @return 解析されたASTassert文ノード
 * 
 * 構文: assert(condition);
 * 条件が偽の場合、プログラムを停止
 */
ASTNode* StatementParser::parseAssertStatement() {
    return parser_->parseAssertStatement();
}

/**
 * @brief println文を解析
 * @return 解析されたAST出力文ノード
 * 
 * 構文: println(arg1, arg2, ...);
 * 可変長引数をサポート、自動的に改行を追加
 */
ASTNode* StatementParser::parsePrintlnStatement() {
    return parser_->parsePrintlnStatement();
}

/**
 * @brief print文を解析
 * @return 解析されたAST出力文ノード
 * 
 * 構文: print(format, arg1, arg2, ...);
 * printf風のフォーマット指定子をサポート
 * - %d: 整数
 * - %lld: 長整数
 * - %u: 符号なし整数
 * - %s: 文字列
 * - %c: 文字
 * - %%: % のエスケープ
 */
ASTNode* StatementParser::parsePrintStatement() {
    return parser_->parsePrintStatement();
}
