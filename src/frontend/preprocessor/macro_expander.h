#pragma once
#include "macro_definition.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace CbPreprocessor {

/**
 * マクロ展開を行うクラス
 *
 * 機能:
 * - マクロの定義と管理
 * - マクロの展開（オブジェクト形式、関数形式）
 * - #演算子（文字列化）の処理
 * - ##演算子（トークン結合）の処理
 * - ネストしたマクロの展開
 */
class MacroExpander {
  public:
    MacroExpander();
    ~MacroExpander();

    /**
     * マクロを定義する
     * @param macro マクロ定義
     */
    void define(const MacroDefinition &macro);

    /**
     * マクロが定義されているか確認
     * @param name マクロ名
     * @return 定義されていればtrue
     */
    bool isDefined(const std::string &name) const;

    /**
     * マクロを未定義にする（#undef用）
     * @param name マクロ名
     */
    void undefine(const std::string &name);

    /**
     * マクロを取得
     * @param name マクロ名
     * @return マクロ定義（見つからない場合はnullptr）
     */
    const MacroDefinition *get(const std::string &name) const;

    /**
     * マクロを展開する
     * @param name マクロ名
     * @param args 引数リスト（関数形式の場合）
     * @return 展開後の文字列
     */
    std::string expand(const std::string &name,
                       const std::vector<std::string> &args = {});

    /**
     * ソースコード内のマクロをすべて展開
     * @param source ソースコード（1行）
     * @return 展開後のソースコード
     */
    std::string expandAll(const std::string &source);

    /**
     * 定義済みマクロの一覧を取得
     * @return マクロ名のリスト
     */
    std::vector<std::string> getDefinedMacros() const;

    /**
     * すべてのマクロをクリア
     */
    void clear();

  private:
    std::unordered_map<std::string, MacroDefinition> macros_;

    // #演算子（文字列化）を処理
    std::string stringifyArgument(const std::string &arg);

    // ##演算子（トークン結合）を処理
    std::string concatenateTokens(const std::string &left,
                                  const std::string &right);

    // マクロ展開を再帰的に実行（ネストしたマクロ対応）
    // 無限再帰を防ぐため深さ制限あり
    std::string expandRecursive(const std::string &text, int depth = 0);

    // 識別子がマクロかチェック
    bool isIdentifier(char c) const;

    // トークンを抽出
    std::string extractToken(const std::string &text, size_t &pos);

    // 空白をスキップ
    void skipWhitespace(const std::string &text, size_t &pos);

    // 関数形式マクロの引数を抽出
    // 例: "SQUARE(5)" の "(" の位置から → ["5"] を返す
    // 例: "MAX(a+b, c*d)" → ["a+b", "c*d"]
    bool extractMacroArguments(const std::string &text, size_t &pos,
                               std::vector<std::string> &args);
};

} // namespace CbPreprocessor
