#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

namespace PreprocessorNS {

struct MacroDefinition {
    std::string name;
    std::vector<std::string> params;
    std::string body;
    bool is_function_like;
    int line;

    MacroDefinition() : is_function_like(false), line(0) {}
    MacroDefinition(const std::string &n, const std::string &b, int l)
        : name(n), body(b), is_function_like(false), line(l) {}
};

class Preprocessor {
  public:
    Preprocessor();

    // メイン処理
    std::string process(const std::string &source_code,
                        const std::string &filename = "");

    // コマンドラインから定義を追加
    void define(const std::string &name, const std::string &value = "1");
    void undefine(const std::string &name);
    bool isDefined(const std::string &name) const;

    // エラー取得
    std::vector<std::string> getErrors() const { return errors_; }
    std::vector<std::string> getWarnings() const { return warnings_; }

  private:
    std::map<std::string, MacroDefinition> defines_;
    std::set<std::string> included_files_;
    std::vector<std::string> errors_;
    std::vector<std::string> warnings_;
    std::string current_file_;
    int current_line_;

    // ディレクティブ処理
    bool processDirective(const std::string &line, std::string &output,
                          bool &skip_output);
    bool handleDefine(const std::string &content);
    bool handleUndef(const std::string &content);
    bool handleIfdef(const std::string &content);
    bool handleIfndef(const std::string &content);
    bool handleElif(const std::string &content);
    bool handleElse();
    bool handleEndif();
    bool handleError(const std::string &content);
    bool handleWarning(const std::string &content);
    bool handleInclude(const std::string &content);

    // マクロ展開
    std::string expandMacros(const std::string &line);
    std::string expandMacro(const std::string &name,
                            const std::vector<std::string> &args);

    // 組み込みマクロ
    void initBuiltinMacros();
    std::string getBuiltinMacro(const std::string &name) const;

    // 条件付きコンパイルのスタック管理
    struct ConditionalState {
        bool condition_met;    // 条件が満たされたか
        bool else_seen;        // #elseを見たか
        bool any_branch_taken; // どれかのブランチが実行されたか
        int line;
    };
    std::vector<ConditionalState> conditional_stack_;

    bool shouldSkipOutput() const;

    // ユーティリティ
    std::string trim(const std::string &str) const;
    std::vector<std::string> tokenize(const std::string &str) const;
    void addError(const std::string &message);
    void addWarning(const std::string &message);
};

} // namespace PreprocessorNS
