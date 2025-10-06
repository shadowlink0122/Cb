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
    return parser_->parseLogicalOr();
}

/**
 * @brief 論理AND演算子 (&&) を解析
 * @return 解析されたAST二項演算ノード
 * 
 * 短絡評価を行います（左辺がfalseなら右辺を評価しない）
 */
ASTNode* ExpressionParser::parseLogicalAnd() {
    return parser_->parseLogicalAnd();
}

// ========================================
// ビット演算子（優先順位: 8-10）
// ========================================

/**
 * @brief ビットOR演算子 (|) を解析
 * @return 解析されたAST二項演算ノード
 */
ASTNode* ExpressionParser::parseBitwiseOr() {
    return parser_->parseBitwiseOr();
}

/**
 * @brief ビットXOR演算子 (^) を解析
 * @return 解析されたAST二項演算ノード
 */
ASTNode* ExpressionParser::parseBitwiseXor() {
    return parser_->parseBitwiseXor();
}

/**
 * @brief ビットAND演算子 (&) を解析
 * @return 解析されたAST二項演算ノード
 * 
 * 注意: アドレス演算子(&)とは異なります
 */
ASTNode* ExpressionParser::parseBitwiseAnd() {
    return parser_->parseBitwiseAnd();
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
    return parser_->parseComparison();
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
    return parser_->parseShift();
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
    return parser_->parseMemberAccess(object);
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
    return parser_->parseArrowAccess(object);
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
    return parser_->parseStructLiteral();
}

/**
 * @brief 配列リテラルを解析
 * @return 解析されたAST配列リテラルノード
 * 
 * 構文: [element1, element2, ...]
 * 空配列もサポート: []
 */
ASTNode* ExpressionParser::parseArrayLiteral() {
    return parser_->parseArrayLiteral();
}
