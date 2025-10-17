#pragma once

#include "directive_parser.h"
#include "frontend/recursive_parser/recursive_lexer.h"
#include "macro_expander.h"
#include <string>
#include <vector>

namespace RecursiveParserNS {

/**
 * @brief トークンベースのプリプロセッサ
 *
 * レキサーから出力されたトークン列を受け取り、プリプロセッサディレクティブを処理して
 * マクロ展開を行います。文字列リテラルやコメント内のマクロは展開されません。
 */
class TokenPreprocessor {
  public:
    TokenPreprocessor();
    ~TokenPreprocessor();

    /**
     * @brief トークン列を処理してマクロ展開を行う
     *
     * @param tokens 入力トークン列
     * @return std::vector<Token> 処理済みトークン列
     */
    std::vector<Token> process(const std::vector<Token> &tokens);

    /**
     * @brief エラーが発生したかどうかを確認
     *
     * @return true エラーが発生している
     * @return false エラーは発生していない
     */
    bool hasError() const;

    /**
     * @brief エラーメッセージを取得
     *
     * @return std::string エラーメッセージ
     */
    std::string getError() const;

    /**
     * @brief プリプロセッサの状態をリセット
     */
    void reset();

  private:
    class Impl;
    Impl *pImpl_;
};

} // namespace RecursiveParserNS
