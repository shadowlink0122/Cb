// Expression Parser - 式解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、式（Expression）の解析を担当します。
// 演算子の優先順位に従って、トップダウン方式（再帰下降法）で解析します。
//
// 【演算子の優先順位（高い順）】:
//   1. Primary (リテラル、変数、括弧、配列リテラル、構造体リテラル)
//   2. Postfix (配列アクセス[], 関数呼び出し(), メンバーアクセス., アロー->, 後置++/--)
//   3. Unary (単項演算子: !, -, ~, &, *, 前置++/--)
//   4. Multiplicative (*, /, %)
//   5. Additive (+, -)
//   6. Shift (<<, >>)
//   7. Comparison (<, >, <=, >=, ==, !=)
//   8. Bitwise AND (&)
//   9. Bitwise XOR (^)
//   10. Bitwise OR (|)
//   11. Logical AND (&&)
//   12. Logical OR (||)
//   13. Ternary (?:)
//   14. Assignment (=, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=)
//
// 【設計パターン】:
// - 委譲パターン: 現時点ではRecursiveParserの実装を呼び出す
// - friend宣言: RecursiveParserの内部状態にアクセス可能
// - 将来の拡張: Phase 3以降で完全な実装に置き換え予定
//
#include "expression_parser.h"
#include "../recursive_parser.h"

ExpressionParser::ExpressionParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// ========================================
// 最上位の式解析
// ========================================

/**
 * @brief 式解析のエントリーポイント
 * @return 解析されたAST式ノード
 * 
 * 式の最上位から解析を開始します。
 * 実際には代入式（parseAssignment）を呼び出します。
 */
ASTNode* ExpressionParser::parseExpression() {
    return parser_->parseExpression();
}

// ========================================
// 代入式（優先順位: 最低）
// ========================================

/**
 * @brief 代入式を解析
 * @return 解析されたAST代入ノード
 * 
 * 代入演算子:
 * - 単純代入: =
 * - 複合代入: +=, -=, *=, /=, %=, &=, |=, ^=, <<=, >>=
 * 
 * 複合代入は、自動的に二項演算に展開されます。
 * 例: a += 5  →  a = a + 5
 */
ASTNode* ExpressionParser::parseAssignment() {
    return parser_->parseAssignment();
}

// ========================================
// 三項演算子（優先順位: 13）
// ========================================

/**
 * @brief 三項演算子 (条件演算子) を解析
 * @return 解析されたAST三項演算ノード
 * 
 * 構文: condition ? true_value : false_value
 */
ASTNode* ExpressionParser::parseTernary() {
    return parser_->parseTernary();
}

// ========================================
// 論理演算子（優先順位: 11-12）
// ========================================

/**
 * @brief 論理OR演算子 (||) を解析
 * @return 解析されたAST二項演算ノード
 * 
 * 短絡評価を行います（左辺がtrueなら右辺を評価しない）
 */
ASTNode* ExpressionParser::parseLogicalOr() {
    ASTNode* left = parseLogicalAnd();
    
    while (parser_->check(TokenType::TOK_OR)) {
        Token op = parser_->advance();
        ASTNode* right = parseLogicalAnd();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

/**
 * @brief 論理AND演算子 (&&) を解析
 * @return 解析されたAST二項演算ノード
 * 
 * 短絡評価を行います（左辺がfalseなら右辺を評価しない）
 */
ASTNode* ExpressionParser::parseLogicalAnd() {
    ASTNode* left = parseBitwiseOr();
    
    while (parser_->check(TokenType::TOK_AND)) {
        Token op = parser_->advance();
        ASTNode* right = parseBitwiseOr();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

// ========================================
// ビット演算子（優先順位: 8-10）
// ========================================

/**
 * @brief ビットOR演算子 (|) を解析
 * @return 解析されたAST二項演算ノード
 */
ASTNode* ExpressionParser::parseBitwiseOr() {
    ASTNode* left = parseBitwiseXor();
    
    while (parser_->check(TokenType::TOK_BIT_OR)) {
        Token op = parser_->advance();
        ASTNode* right = parseBitwiseXor();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

/**
 * @brief ビットXOR演算子 (^) を解析
 * @return 解析されたAST二項演算ノード
 */
ASTNode* ExpressionParser::parseBitwiseXor() {
    ASTNode* left = parseBitwiseAnd();
    
    while (parser_->check(TokenType::TOK_BIT_XOR)) {
        Token op = parser_->advance();
        ASTNode* right = parseBitwiseAnd();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

/**
 * @brief ビットAND演算子 (&) を解析
 * @return 解析されたAST二項演算ノード
 * 
 * 注意: アドレス演算子(&)とは異なります
 */
ASTNode* ExpressionParser::parseBitwiseAnd() {
    ASTNode* left = parseComparison();
    
    while (parser_->check(TokenType::TOK_BIT_AND)) {
        Token op = parser_->advance();
        ASTNode* right = parseComparison();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

// ========================================
// 比較演算子（優先順位: 7）
// ========================================

/**
 * @brief 比較演算子を解析
 * @return 解析されたAST二項演算ノード
 * 
 * サポートする演算子:
 * - ==, != (等価比較)
 * - <, >, <=, >= (大小比較)
 */
ASTNode* ExpressionParser::parseComparison() {
    ASTNode* left = parseShift();
    
    while (parser_->check(TokenType::TOK_EQ) || parser_->check(TokenType::TOK_NE) || 
           parser_->check(TokenType::TOK_LT) || parser_->check(TokenType::TOK_LE) ||
           parser_->check(TokenType::TOK_GT) || parser_->check(TokenType::TOK_GE)) {
        Token op = parser_->advance();
        ASTNode* right = parseShift();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

// ========================================
// シフト演算子（優先順位: 6）
// ========================================

/**
 * @brief ビットシフト演算子を解析
 * @return 解析されたAST二項演算ノード
 * 
 * サポートする演算子:
 * - << (左シフト)
 * - >> (右シフト)
 */
ASTNode* ExpressionParser::parseShift() {
    ASTNode* left = parseAdditive();
    
    while (parser_->check(TokenType::TOK_LEFT_SHIFT) || parser_->check(TokenType::TOK_RIGHT_SHIFT)) {
        Token op = parser_->advance();
        ASTNode* right = parseAdditive();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

// ========================================
// 加減算（優先順位: 5）
// ========================================

/**
 * @brief 加算・減算演算子を解析
 * @return 解析されたAST二項演算ノード
 * 
 * サポートする演算子: +, -
 * 
 * 左結合の二項演算子として処理します。
 * 例: a + b - c は (a + b) - c として解析されます。
 */
ASTNode* ExpressionParser::parseAdditive() {
    ASTNode* left = parseMultiplicative();
    
    while (parser_->check(TokenType::TOK_PLUS) || parser_->check(TokenType::TOK_MINUS)) {
        Token op = parser_->advance();
        ASTNode* right = parseMultiplicative();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

// ========================================
// 乗除算（優先順位: 4）
// ========================================

/**
 * @brief 乗算・除算・剰余演算子を解析
 * @return 解析されたAST二項演算ノード
 * 
 * サポートする演算子: *, /, %
 * 
 * 左結合の二項演算子として処理します。
 * 例: a * b / c は (a * b) / c として解析されます。
 */
ASTNode* ExpressionParser::parseMultiplicative() {
    ASTNode* left = parseUnary();
    
    while (parser_->check(TokenType::TOK_MUL) || 
           parser_->check(TokenType::TOK_DIV) || 
           parser_->check(TokenType::TOK_MOD)) {
        Token op = parser_->advance();
        ASTNode* right = parseUnary();
        
        ASTNode* binary = new ASTNode(ASTNodeType::AST_BINARY_OP);
        binary->op = op.value;
        binary->left = std::unique_ptr<ASTNode>(left);
        binary->right = std::unique_ptr<ASTNode>(right);
        
        left = binary;
    }
    
    return left;
}

// ========================================
// 単項演算子（優先順位: 3）
// ========================================

/**
 * @brief 単項演算子を解析
 * @return 解析されたAST単項演算ノード
 * 
 * サポートする演算子:
 * - ! (論理NOT)
 * - - (符号反転)
 * - ~ (ビットNOT)
 * - & (アドレス演算子)
 * - * (間接参照演算子)
 * - ++, -- (前置インクリメント・デクリメント)
 */
ASTNode* ExpressionParser::parseUnary() {
    return parser_->parseUnary();
}

// ========================================
// 後置演算子（優先順位: 2）
// ========================================

/**
 * @brief 後置演算子を解析
 * @return 解析されたASTノード
 * 
 * サポートする演算子:
 * - [] (配列アクセス)
 * - () (関数呼び出し)
 * - . (メンバーアクセス)
 * - -> (アロー演算子)
 * - ++, -- (後置インクリメント・デクリメント)
 */
ASTNode* ExpressionParser::parsePostfix() {
    return parser_->parsePostfix();
}

// ========================================
// 基本式（優先順位: 1、最高）
// ========================================

/**
 * @brief 基本式（プライマリ式）を解析
 * @return 解析されたASTノード
 * 
 * サポートする要素:
 * - 数値リテラル (整数、浮動小数点数)
 * - 文字列リテラル
 * - 文字リテラル
 * - 真偽値リテラル (true, false)
 * - nullptr
 * - 識別子（変数、関数）
 * - 括弧式 (expr)
 * - 配列リテラル [1, 2, 3]
 * - 構造体リテラル {member: value}
 * - enum値アクセス EnumName::member
 */
ASTNode* ExpressionParser::parsePrimary() {
    return parser_->parsePrimary();
}

// ========================================
// メンバーアクセス
// ========================================

/**
 * @brief メンバーアクセス演算子 (.) を解析
 * @param object アクセス対象のオブジェクト
 * @return 解析されたASTメンバーアクセスノード
 * 
 * 構文: object.member
 * ネストしたアクセス可能: obj.member.submember
 */
ASTNode* ExpressionParser::parseMemberAccess(ASTNode* object) {
    parser_->consume(TokenType::TOK_DOT, "Expected '.'");
    
    std::string member_name;
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_PRINT) || parser_->check(TokenType::TOK_PRINTLN) || parser_->check(TokenType::TOK_PRINTF)) {
        // 予約キーワードだが、メソッド名として許可
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else {
        parser_->error("Expected member name after '.'");
        return nullptr;
    }
    
    // メソッド呼び出しかチェック（obj.method()）
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // メソッド呼び出し
        ASTNode* method_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        method_call->name = member_name;
        method_call->left = std::unique_ptr<ASTNode>(object); // レシーバー
        parser_->setLocation(method_call, parser_->current_token_);
        
        // パラメータリストを解析
        parser_->advance(); // consume '('
        
        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode* arg = parser_->parseExpression();
                if (!arg) {
                    parser_->error("Expected argument expression");
                    return nullptr;
                }
                method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                
                if (!parser_->check(TokenType::TOK_COMMA)) {
                    break;
                }
                parser_->advance(); // consume ','
            } while (!parser_->check(TokenType::TOK_RPAREN) && !parser_->isAtEnd());
        }
        
        parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after method arguments");
        
        return method_call;
    }
    
    // 通常のメンバアクセス (連続ドット処理はparsePostfixループに任せる)
    ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
    member_access->left = std::unique_ptr<ASTNode>(object);
    member_access->name = member_name; // メンバ名を保存
    parser_->setLocation(member_access, parser_->current_token_);
    
    return member_access;
}

/**
 * @brief アロー演算子 (->) を解析
 * @param object アクセス対象のポインタ
 * @return 解析されたASTアロー演算子ノード
 * 
 * 構文: pointer->member
 * (*pointer).member の糖衣構文
 */
ASTNode* ExpressionParser::parseArrowAccess(ASTNode* object) {
    parser_->consume(TokenType::TOK_ARROW, "Expected '->'");
    
    std::string member_name;
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else if (parser_->check(TokenType::TOK_PRINT) || parser_->check(TokenType::TOK_PRINTLN) || parser_->check(TokenType::TOK_PRINTF)) {
        // 予約キーワードだが、メソッド名として許可
        member_name = parser_->current_token_.value;
        parser_->advance();
    } else {
        parser_->error("Expected member name after '->'");
        return nullptr;
    }
    
    // メソッド呼び出しかチェック（ptr->method()）
    if (parser_->check(TokenType::TOK_LPAREN)) {
        // メソッド呼び出し
        ASTNode* method_call = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        method_call->name = member_name;
        method_call->left = std::unique_ptr<ASTNode>(object); // レシーバー（ポインタ）
        method_call->is_arrow_call = true; // アロー演算子経由のフラグ
        parser_->setLocation(method_call, parser_->current_token_);
        
        // パラメータリストを解析
        parser_->advance(); // consume '('
        
        if (!parser_->check(TokenType::TOK_RPAREN)) {
            do {
                ASTNode* arg = parser_->parseExpression();
                if (!arg) {
                    parser_->error("Expected argument expression");
                    return nullptr;
                }
                method_call->arguments.push_back(std::unique_ptr<ASTNode>(arg));
                
                if (!parser_->check(TokenType::TOK_COMMA)) {
                    break;
                }
                parser_->advance(); // consume ','
            } while (!parser_->check(TokenType::TOK_RPAREN) && !parser_->isAtEnd());
        }
        
        parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after method arguments");
        
        return method_call;
    }
    
    // アロー演算子アクセス: ptr->member は (*ptr).member と等価
    ASTNode* arrow_access = new ASTNode(ASTNodeType::AST_ARROW_ACCESS);
    arrow_access->left = std::unique_ptr<ASTNode>(object);
    arrow_access->name = member_name; // メンバ名を保存
    parser_->setLocation(arrow_access, parser_->current_token_);
    
    return arrow_access;
}

// ========================================
// リテラル
// ========================================

/**
 * @brief 構造体リテラルを解析
 * @return 解析されたAST構造体リテラルノード
 * 
 * 構文: {member1: value1, member2: value2, ...}
 * 末尾カンマ対応
 */
ASTNode* ExpressionParser::parseStructLiteral() {
    parser_->consume(TokenType::TOK_LBRACE, "Expected '{'");
    
    ASTNode* struct_literal = new ASTNode(ASTNodeType::AST_STRUCT_LITERAL);
    
    // 空の構造体リテラル {}
    if (parser_->check(TokenType::TOK_RBRACE)) {
        parser_->advance(); // consume '}'
        return struct_literal;
    }
    
    // 最初の要素をチェックして、名前付きか位置ベースかを判断
    bool is_named_initialization = false;
    
    // 先読みして名前付き初期化かチェック
    if (parser_->check(TokenType::TOK_IDENTIFIER)) {
        RecursiveLexer temp_lexer = parser_->lexer_;
        Token temp_current = parser_->current_token_;
        parser_->advance();
        if (parser_->check(TokenType::TOK_COLON)) {
            is_named_initialization = true;
        }
        parser_->lexer_ = temp_lexer;
        parser_->current_token_ = temp_current;
    }
    
    if (is_named_initialization) {
        // 名前付き初期化: {name: "Bob", age: 25}
        while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
            // メンバ名の解析
            if (!parser_->check(TokenType::TOK_IDENTIFIER)) {
                parser_->error("Expected member name in struct literal");
                return nullptr;
            }
            
            std::string member_name = parser_->current_token_.value;
            parser_->advance();
            
            parser_->consume(TokenType::TOK_COLON, "Expected ':' after member name");
            
            // メンバ値の解析
            ASTNode* member_value = parser_->parseExpression();
            
            // メンバ代入ノードを作成（name: valueの形で保存）
            ASTNode* member_init = new ASTNode(ASTNodeType::AST_ASSIGN);
            member_init->name = member_name;
            member_init->right = std::unique_ptr<ASTNode>(member_value);
            
            struct_literal->arguments.push_back(std::unique_ptr<ASTNode>(member_init));
            
            if (parser_->check(TokenType::TOK_COMMA)) {
                parser_->advance(); // consume ','
            } else if (!parser_->check(TokenType::TOK_RBRACE)) {
                parser_->error("Expected ',' or '}' in struct literal");
                return nullptr;
            }
        }
    } else {
        // 位置ベース初期化: {25, "Bob"}
        while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
            ASTNode* value = parser_->parseExpression();
            struct_literal->arguments.push_back(std::unique_ptr<ASTNode>(value));
            
            if (parser_->check(TokenType::TOK_COMMA)) {
                parser_->advance(); // consume ','
            } else if (!parser_->check(TokenType::TOK_RBRACE)) {
                parser_->error("Expected ',' or '}' in struct literal");
                return nullptr;
            }
        }
    }
    
    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after struct literal");
    return struct_literal;
}

/**
 * @brief 配列リテラルを解析
 * @return 解析されたAST配列リテラルノード
 * 
 * 構文: [element1, element2, ...]
 * 空配列もサポート: []
 */
ASTNode* ExpressionParser::parseArrayLiteral() {
    parser_->consume(TokenType::TOK_LBRACKET, "Expected '[' at start of array literal");
    
    ASTNode* array_literal = new ASTNode(ASTNodeType::AST_ARRAY_LITERAL);
    
    // 空の配列リテラル []
    if (parser_->check(TokenType::TOK_RBRACKET)) {
        parser_->advance(); // consume ']'
        return array_literal;
    }
    
    // 配列要素を解析
    while (!parser_->check(TokenType::TOK_RBRACKET) && !parser_->isAtEnd()) {
        ASTNode* element;
        
        if (parser_->check(TokenType::TOK_LBRACE)) {
            // struct literal要素: {25, "Alice"}
            element = parseStructLiteral();
        } else {
            // 通常の式要素
            element = parser_->parseExpression();
        }
        
        array_literal->arguments.push_back(std::unique_ptr<ASTNode>(element));
        
        if (parser_->check(TokenType::TOK_COMMA)) {
            parser_->advance(); // consume ','
        } else if (!parser_->check(TokenType::TOK_RBRACKET)) {
            parser_->error("Expected ',' or ']' in array literal");
            return nullptr;
        }
    }
    
    parser_->consume(TokenType::TOK_RBRACKET, "Expected ']' after array literal");
    return array_literal;
}
