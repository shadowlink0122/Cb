#include "preprocessor.h"
#include "directive_parser.h"
#include "macro_expander.h"
#include <sstream>

namespace CbPreprocessor {

// Pimplイディオム実装クラス
class Preprocessor::Impl {
  public:
    MacroExpander expander;
    DirectiveParser parser;
    std::string last_error;
    std::string current_filename;
    int current_line;

    Impl() : current_line(0) {}

    std::string processLine(const std::string &line);
    bool isDirective(const std::string &line) const;
    std::string handleDirective(const std::string &line);
};

Preprocessor::Preprocessor() : impl_(std::make_unique<Impl>()) {}

Preprocessor::~Preprocessor() = default;

std::string Preprocessor::process(const std::string &source,
                                  const std::string &filename) {
    impl_->current_filename = filename;
    impl_->current_line = 0;
    impl_->last_error.clear();

    std::istringstream input(source);
    std::ostringstream output;
    std::string line;

    while (std::getline(input, line)) {
        impl_->current_line++;

        try {
            std::string processed = impl_->processLine(line);
            if (!processed.empty()) {
                output << processed << "\n";
            }
        } catch (const std::exception &e) {
            // エラーを記録
            std::ostringstream error_msg;
            error_msg << filename << ":" << impl_->current_line
                      << ": error: " << e.what();
            impl_->last_error = error_msg.str();
            return ""; // エラー時は空文字列を返す
        }
    }

    return output.str();
}

std::vector<std::string> Preprocessor::getDefinedMacros() const {
    return impl_->expander.getDefinedMacros();
}

std::string Preprocessor::getLastError() const { return impl_->last_error; }

bool Preprocessor::hasError() const { return !impl_->last_error.empty(); }

void Preprocessor::reset() {
    impl_->expander.clear();
    impl_->last_error.clear();
    impl_->current_line = 0;
}

// Impl のメソッド実装

std::string Preprocessor::Impl::processLine(const std::string &line) {
    // ディレクティブの処理
    if (isDirective(line)) {
        return handleDirective(line);
    }

    // マクロ展開
    return expander.expandAll(line);
}

bool Preprocessor::Impl::isDirective(const std::string &line) const {
    // 行の先頭（空白を除く）が '#' で始まるか確認
    size_t pos = 0;
    while (pos < line.length() && std::isspace(line[pos])) {
        pos++;
    }
    return pos < line.length() && line[pos] == '#';
}

std::string Preprocessor::Impl::handleDirective(const std::string &line) {
    // ディレクティブの種類を判定
    size_t pos = line.find('#');
    pos++;

    // 空白をスキップ
    while (pos < line.length() && std::isspace(line[pos])) {
        pos++;
    }

    // ディレクティブ名を抽出
    std::string directive_name;
    while (pos < line.length() && std::isalpha(line[pos])) {
        directive_name += line[pos];
        pos++;
    }

    if (directive_name == "define") {
        // #define を処理
        MacroDefinition macro = parser.parseDefine(line);
        macro.line = current_line;
        macro.filename = current_filename;
        expander.define(macro);
        return ""; // ディレクティブは出力しない
    } else if (directive_name == "undef") {
        // #undef を処理
        // マクロ名を抽出
        while (pos < line.length() && std::isspace(line[pos])) {
            pos++;
        }
        std::string macro_name;
        while (pos < line.length() &&
               (std::isalnum(line[pos]) || line[pos] == '_')) {
            macro_name += line[pos];
            pos++;
        }

        if (!macro_name.empty()) {
            expander.undefine(macro_name);
        }
        return ""; // ディレクティブは出力しない
    } else {
        // 未知のディレクティブ（将来実装: #if, #ifdef, #ifndef, #else, #endif）
        // 現在はそのまま通す
        return line;
    }
}

} // namespace CbPreprocessor
