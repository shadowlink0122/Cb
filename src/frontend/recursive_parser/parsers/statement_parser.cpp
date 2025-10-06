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
    parser_->advance(); // consume '{'
    
    ASTNode* compound = new ASTNode(ASTNodeType::AST_COMPOUND_STMT);
    
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        ASTNode* stmt = parser_->parseStatement();
        if (stmt) {
            compound->statements.push_back(std::unique_ptr<ASTNode>(stmt));
        }
    }
    
    parser_->consume(TokenType::TOK_RBRACE, "Expected '}'");
    return compound;
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
    parser_->advance(); // consume 'if'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after if");
    
    ASTNode* if_node = new ASTNode(ASTNodeType::AST_IF_STMT);
    if_node->condition = std::unique_ptr<ASTNode>(parser_->parseExpression());
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after if condition");
    
    // if本体をパース（then節はleftに格納してinterpreterと統一）
    if_node->left = std::unique_ptr<ASTNode>(parser_->parseStatement());
    
    // else節があるかチェック
    if (parser_->match(TokenType::TOK_ELSE)) {
        if_node->right = std::unique_ptr<ASTNode>(parser_->parseStatement());
    }
    
    return if_node;
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
    parser_->advance(); // consume 'for'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after for");
    
    ASTNode* for_node = new ASTNode(ASTNodeType::AST_FOR_STMT);
    
    // 初期化部分 (int i = 0;) - 文として扱う
    for_node->init_expr = std::unique_ptr<ASTNode>(parser_->parseStatement());
    
    // 条件部分 (i < 5) - 式として扱う
    for_node->condition = std::unique_ptr<ASTNode>(parser_->parseExpression());
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after for condition");
    
    // 更新部分 - 一般的な式として処理（i++, i--, i=i+1など）
    for_node->update_expr = std::unique_ptr<ASTNode>(parser_->parseExpression());
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after for update");
    
    // for本体
    for_node->body = std::unique_ptr<ASTNode>(parser_->parseStatement());
    
    return for_node;
}

/**
 * @brief while文を解析
 * @return 解析されたASTwhile文ノード
 * 
 * 構文: while (condition) statement
 */
ASTNode* StatementParser::parseWhileStatement() {
    parser_->advance(); // consume 'while'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after while");
    
    ASTNode* while_node = new ASTNode(ASTNodeType::AST_WHILE_STMT);
    
    // 条件部分
    while_node->condition = std::unique_ptr<ASTNode>(parser_->parseExpression());
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after while condition");
    
    // while本体
    while_node->body = std::unique_ptr<ASTNode>(parser_->parseStatement());
    
    return while_node;
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
    parser_->advance(); // consume 'return'
    ASTNode* return_node = new ASTNode(ASTNodeType::AST_RETURN_STMT);
    
    // return値があるかチェック
    if (!parser_->check(TokenType::TOK_SEMICOLON)) {
        return_node->left = std::unique_ptr<ASTNode>(parser_->parseExpression());
    }
    
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after return statement");
    return return_node;
}

/**
 * @brief break文を解析
 * @return 解析されたASTbreak文ノード
 * 
 * 構文: break;
 * ループまたはswitchから脱出（現在はループのみサポート）
 */
ASTNode* StatementParser::parseBreakStatement() {
    parser_->advance(); // consume 'break'
    ASTNode* break_node = new ASTNode(ASTNodeType::AST_BREAK_STMT);
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after break statement");
    return break_node;
}

/**
 * @brief continue文を解析
 * @return 解析されたASTcontinue文ノード
 * 
 * 構文: continue;
 * ループの次の反復へスキップ
 */
ASTNode* StatementParser::parseContinueStatement() {
    parser_->advance(); // consume 'continue'
    ASTNode* continue_node = new ASTNode(ASTNodeType::AST_CONTINUE_STMT);
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after continue statement");
    return continue_node;
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
    Token assert_token = parser_->advance(); // consume 'assert'
    
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after assert");
    
    // 条件式をパース
    ASTNode* condition = parser_->parseExpression();
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after assert condition");
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after assert statement");
    
    ASTNode* assert_node = new ASTNode(ASTNodeType::AST_ASSERT_STMT);
    assert_node->left = std::unique_ptr<ASTNode>(condition);
    assert_node->location.line = assert_token.line;
    
    return assert_node;
}

/**
 * @brief println文を解析
 * @return 解析されたAST出力文ノード
 * 
 * 構文: println(arg1, arg2, ...);
 * 可変長引数をサポート、自動的に改行を追加
 */
ASTNode* StatementParser::parsePrintlnStatement() {
    parser_->advance(); // consume 'println'
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after println");
    
    ASTNode* print_node = new ASTNode(ASTNodeType::AST_PRINTLN_STMT);
    
    // 複数の引数をパース
    if (!parser_->check(TokenType::TOK_RPAREN)) {
        do {
            ASTNode* arg = parser_->parseExpression();
            print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
        } while (parser_->match(TokenType::TOK_COMMA));
    }
    
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after println arguments");
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after println statement");
    return print_node;
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
    parser_->advance(); // consume 'print'
    
    ASTNode* print_node = new ASTNode(ASTNodeType::AST_PRINT_STMT);
    
    // 引数をパース - 任意の式を受け入れる
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // print(expression[, expression, ...]); 形式
        parser_->advance(); // consume '('
        
        // 複数の引数をパース
        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode* arg = parser_->parseExpression();
                print_node->arguments.push_back(std::unique_ptr<ASTNode>(arg));
            } while (parser_->match(TokenType::TOK_COMMA));
        }
        
        parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after print arguments");
    } else if (!parser_->check(TokenType::TOK_SEMICOLON)) {
        // print expression; 形式（括弧なし）
        print_node->left = std::unique_ptr<ASTNode>(parser_->parseExpression());
    } else {
        parser_->error("Expected expression after print");
        return nullptr;
    }
    
    parser_->consume(TokenType::TOK_SEMICOLON, "Expected ';' after print statement");
    return print_node;
}
