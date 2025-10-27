#ifndef PRIMARY_EXPRESSION_PARSER_H
#define PRIMARY_EXPRESSION_PARSER_H

#include "src/common/ast.h"

class RecursiveParser;

/**
 * @brief プライマリ式解析を担当
 *
 * 式解析の最も基本的な要素を処理します：
 * - リテラル（数値、文字列、文字、真偽値、nullptr）
 * - 識別子（変数、enum値アクセス）
 * - 関数呼び出し
 * - 括弧式
 * - 配列リテラル
 * - 構造体リテラル
 */
class PrimaryExpressionParser {
  public:
    explicit PrimaryExpressionParser(RecursiveParser *parser);

    /**
     * @brief プライマリ式を解析
     * @return 解析されたASTノード
     */
    ASTNode *parsePrimary();

    /**
     * @brief 構造体リテラルを解析
     * @return 解析されたAST構造体リテラルノード
     */
    ASTNode *parseStructLiteral();

    /**
     * @brief 配列リテラルを解析
     * @return 解析されたAST配列リテラルノード
     */
    ASTNode *parseArrayLiteral();

    /**
     * @brief 無名関数（ラムダ式）を解析
     * @return 解析されたASTラムダ式ノード
     *
     * 構文: 型 func(パラメータ) { 本体 }
     * 例: int func(int x) { return x * 2; }
     */
    ASTNode *parseLambda();

    /**
     * @brief 補間文字列を解析
     * @param str 補間文字列の内容
     * @return 解析されたAST補間文字列ノード
     *
     * v0.11.0 文字列補間
     * 構文: "text {expression:format} text"
     * 例: "Hello, {name}!" または "Pi: {pi:.2}"
     */
    ASTNode *parseInterpolatedString(const std::string &str);

  private:
    RecursiveParser *parser_;
};

#endif // PRIMARY_EXPRESSION_PARSER_H
