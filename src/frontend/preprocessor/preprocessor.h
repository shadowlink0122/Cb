#pragma once
#include <memory>
#include <string>
#include <vector>

namespace CbPreprocessor {

/**
 * Cb言語のプリプロセッサ
 *
 * C/C++の伝統的なプリプロセッサと同様に、ソースコードを
 * パース前に処理します。
 *
 * 処理フロー:
 *   ソースコード → プリプロセッサ → Lexer → Parser → Interpreter
 *
 * 使用例:
 * ```cpp
 * CbPreprocessor::Preprocessor preprocessor;
 * std::string result = preprocessor.process(source_code, "myfile.cb");
 *
 * if (preprocessor.hasError()) {
 *     std::cerr << preprocessor.getLastError() << std::endl;
 * }
 * ```
 */
class Preprocessor {
  public:
    Preprocessor();
    ~Preprocessor();

    /**
     * ソースコードをプリプロセスする
     *
     * 処理内容:
     * 1. #defineディレクティブを検出して登録
     * 2. マクロ呼び出しを展開
     * 3. #undefディレクティブを処理
     * 4. #if/#else/#endif（将来実装）
     *
     * @param source 元のソースコード
     * @param filename ファイル名（エラーメッセージ用）
     * @return プリプロセス済みのソースコード
     */
    std::string process(const std::string &source,
                        const std::string &filename = "<stdin>");

    /**
     * デバッグ用: マクロ定義の一覧を取得
     * @return マクロ名のリスト
     */
    std::vector<std::string> getDefinedMacros() const;

    /**
     * エラーメッセージを取得
     * @return エラーメッセージ（エラーがない場合は空文字列）
     */
    std::string getLastError() const;

    /**
     * エラーが発生したかチェック
     * @return エラーがあればtrue
     */
    bool hasError() const;

    /**
     * プリプロセッサの状態をリセット
     * すべてのマクロ定義をクリアし、エラー状態を初期化
     */
    void reset();

  private:
    // Pimplイディオムによる実装の隠蔽
    class Impl;
    std::unique_ptr<Impl> impl_;

    // コピー禁止
    Preprocessor(const Preprocessor &) = delete;
    Preprocessor &operator=(const Preprocessor &) = delete;
};

} // namespace CbPreprocessor
