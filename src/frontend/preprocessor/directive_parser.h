#pragma once
#include "macro_definition.h"
#include <string>
#include <vector>

namespace CbPreprocessor {

/**
 * プリプロセッサディレクティブを解析するクラス
 *
 * サポートされるディレクティブ:
 * - #define MACRO_NAME value
 * - #define MACRO_NAME(params) body
 * - #undef MACRO_NAME
 * - #if condition (将来実装)
 * - #ifdef MACRO_NAME (将来実装)
 * - #ifndef MACRO_NAME (将来実装)
 * - #else (将来実装)
 * - #endif (将来実装)
 */
class DirectiveParser {
  public:
    DirectiveParser();
    ~DirectiveParser();

    /**
     * #defineディレクティブを解析
     * @param line ディレクティブ行（例: "#define SQUARE(x) ((x) * (x))"）
     * @return マクロ定義
     * @throws std::runtime_error 構文エラーの場合
     */
    MacroDefinition parseDefine(const std::string &line);

    /**
     * #ifディレクティブの条件式を評価（将来実装）
     * @param condition 条件式
     * @return 評価結果
     */
    bool evaluateCondition(const std::string &condition);

    /**
     * 関数形式マクロの引数を解析
     * 例: "SQUARE(5)" → ["5"]
     * 例: "MAX(a+b, c*d)" → ["a+b", "c*d"]
     * 例: "FUNC(foo(1,2), bar(3))" → ["foo(1,2)", "bar(3)"]
     *
     * @param macro_call マクロ呼び出し文字列
     * @param name [out] マクロ名
     * @param args [out] 引数リスト
     * @return パースに成功したらtrue
     */
    bool parseMacroCall(const std::string &macro_call, std::string &name,
                        std::vector<std::string> &args);

  private:
    /**
     * 文字列をトークンに分割
     */
    std::vector<std::string> tokenize(const std::string &str);

    /**
     * 括弧のバランスをチェック
     */
    bool isBalanced(const std::string &str);

    /**
     * 先頭と末尾の空白を削除
     */
    std::string trim(const std::string &str);

    /**
     * マクロ名を抽出
     * 例: "#define SQUARE(x)" → "SQUARE"
     */
    std::string extractMacroName(const std::string &line, size_t &pos);

    /**
     * パラメータリストを抽出
     * 例: "SQUARE(x, y)" → ["x", "y"]
     */
    std::vector<std::string> extractParameters(const std::string &line,
                                               size_t &pos);

    /**
     * マクロ本体を抽出
     * 例: "#define SQUARE(x) ((x) * (x))" → "((x) * (x))"
     */
    std::string extractBody(const std::string &line, size_t pos);
};

} // namespace CbPreprocessor
