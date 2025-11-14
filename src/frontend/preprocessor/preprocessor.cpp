#include "preprocessor.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>

namespace PreprocessorNS {

Preprocessor::Preprocessor() : current_line_(0) { initBuiltinMacros(); }

void Preprocessor::initBuiltinMacros() {
    // 日付と時刻
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);

    std::ostringstream date_stream, time_stream;
    date_stream << std::put_time(&tm, "%b %d %Y");
    time_stream << std::put_time(&tm, "%H:%M:%S");

    defines_["__DATE__"] =
        MacroDefinition("__DATE__", "\"" + date_stream.str() + "\"", 0);
    defines_["__TIME__"] =
        MacroDefinition("__TIME__", "\"" + time_stream.str() + "\"", 0);
    defines_["__VERSION__"] = MacroDefinition("__VERSION__", "\"0.13.0\"", 0);
}

std::string Preprocessor::process(const std::string &source_code,
                                  const std::string &filename) {
    current_file_ = filename.empty() ? "<input>" : filename;
    current_line_ = 0;
    errors_.clear();
    warnings_.clear();
    conditional_stack_.clear();

    std::istringstream input(source_code);
    std::ostringstream output;
    std::string line;

    while (std::getline(input, line)) {
        current_line_++;

        // __FILE__と__LINE__を動的に更新
        defines_["__FILE__"] = MacroDefinition(
            "__FILE__", "\"" + current_file_ + "\"", current_line_);
        defines_["__LINE__"] = MacroDefinition(
            "__LINE__", std::to_string(current_line_), current_line_);

        bool skip_output = false;

        // プリプロセッサディレクティブのチェック（#で始まる）
        std::string trimmed = trim(line);
        if (!trimmed.empty() && trimmed[0] == '#') {
            if (!processDirective(trimmed, line, skip_output)) {
                // エラーが発生した場合もコメントとして出力
                if (!shouldSkipOutput()) {
                    output << "// " << line << " [preprocessor error]\n";
                }
            }
            // ディレクティブはそのまま続ける（skip_outputがtrueなので出力されない）
            continue;
        }

        // 出力をスキップすべきか判定
        if (shouldSkipOutput() || skip_output) {
            continue;
        }

        // マクロ展開
        std::string expanded = expandMacros(line);
        output << expanded << "\n";
    }

    // 未閉じの条件分岐をチェック
    if (!conditional_stack_.empty()) {
        addError("Unclosed #ifdef/#ifndef (missing #endif)");
    }

    return output.str();
}

bool Preprocessor::processDirective(const std::string &line,
                                    std::string &output, bool &skip_output) {
    std::string trimmed = trim(line.substr(1)); // '#'を除去

    if (trimmed.empty()) {
        return true;
    }

    // ディレクティブ名を取得
    size_t space_pos = trimmed.find_first_of(" \t");
    std::string directive =
        space_pos == std::string::npos ? trimmed : trimmed.substr(0, space_pos);
    std::string content = space_pos == std::string::npos
                              ? ""
                              : trim(trimmed.substr(space_pos + 1));

    // 条件付きコンパイル中でも処理する必要があるディレクティブ
    if (directive == "ifdef") {
        skip_output = true;
        return handleIfdef(content);
    } else if (directive == "ifndef") {
        skip_output = true;
        return handleIfndef(content);
    } else if (directive == "elif" || directive == "elseif") {
        skip_output = true;
        return handleElif(content);
    } else if (directive == "else") {
        skip_output = true;
        return handleElse();
    } else if (directive == "endif") {
        skip_output = true;
        return handleEndif();
    }

    // スキップ中は以下のディレクティブを無視
    if (shouldSkipOutput()) {
        return true;
    }

    if (directive == "define") {
        skip_output = true;
        return handleDefine(content);
    } else if (directive == "undef") {
        skip_output = true;
        return handleUndef(content);
    } else if (directive == "error") {
        skip_output = true;
        return handleError(content);
    } else if (directive == "warning") {
        skip_output = true;
        return handleWarning(content);
    } else if (directive == "include") {
        skip_output = true;
        return handleInclude(content);
    } else {
        addError("Unknown preprocessor directive: #" + directive);
        return false;
    }
}

bool Preprocessor::handleDefine(const std::string &content) {
    if (content.empty()) {
        addError("#define requires a macro name");
        return false;
    }

    // マクロ名と本体を分離
    size_t space_pos = content.find_first_of(" \t(");
    if (space_pos == std::string::npos) {
        // 値なしの定義（フラグとして使用）
        defines_[content] = MacroDefinition(content, "1", current_line_);
        return true;
    }

    std::string name = content.substr(0, space_pos);

    // 関数マクロかどうかチェック
    if (content[space_pos] == '(') {
        // 関数マクロは後で実装
        addWarning("Function-like macros are not fully supported yet");
        size_t close_paren = content.find(')', space_pos);
        if (close_paren == std::string::npos) {
            addError("Unclosed parenthesis in macro definition");
            return false;
        }
        std::string body = close_paren + 1 < content.length()
                               ? trim(content.substr(close_paren + 1))
                               : "";
        defines_[name] = MacroDefinition(name, body, current_line_);
        defines_[name].is_function_like = true;
    } else {
        // オブジェクトマクロ
        std::string body = trim(content.substr(space_pos + 1));
        defines_[name] = MacroDefinition(name, body, current_line_);
    }

    return true;
}

bool Preprocessor::handleUndef(const std::string &content) {
    if (content.empty()) {
        addError("#undef requires a macro name");
        return false;
    }

    std::string name = trim(content);
    defines_.erase(name);
    return true;
}

bool Preprocessor::handleIfdef(const std::string &content) {
    if (content.empty()) {
        addError("#ifdef requires a macro name");
        return false;
    }

    std::string name = trim(content);
    bool is_defined = defines_.find(name) != defines_.end();

    ConditionalState state;
    state.condition_met = is_defined;
    state.else_seen = false;
    state.any_branch_taken = is_defined;
    state.line = current_line_;
    conditional_stack_.push_back(state);

    return true;
}

bool Preprocessor::handleIfndef(const std::string &content) {
    if (content.empty()) {
        addError("#ifndef requires a macro name");
        return false;
    }

    std::string name = trim(content);
    bool is_not_defined = defines_.find(name) == defines_.end();

    ConditionalState state;
    state.condition_met = is_not_defined;
    state.else_seen = false;
    state.any_branch_taken = is_not_defined;
    state.line = current_line_;
    conditional_stack_.push_back(state);

    return true;
}

bool Preprocessor::handleElif(const std::string &content) {
    if (conditional_stack_.empty()) {
        addError("#elif without #ifdef/#ifndef");
        return false;
    }

    ConditionalState &state = conditional_stack_.back();

    if (state.else_seen) {
        addError("#elif after #else");
        return false;
    }

    // 既にどれかのブランチが実行されていたらスキップ
    if (state.any_branch_taken) {
        state.condition_met = false;
        return true;
    }

    // 条件を評価（簡易版：defined()のみサポート）
    std::string name = trim(content);
    bool is_defined = defines_.find(name) != defines_.end();

    state.condition_met = is_defined;
    if (is_defined) {
        state.any_branch_taken = true;
    }

    return true;
}

bool Preprocessor::handleElse() {
    if (conditional_stack_.empty()) {
        addError("#else without #ifdef/#ifndef");
        return false;
    }

    ConditionalState &state = conditional_stack_.back();

    if (state.else_seen) {
        addError("Duplicate #else");
        return false;
    }

    state.else_seen = true;
    state.condition_met = !state.any_branch_taken;

    return true;
}

bool Preprocessor::handleEndif() {
    if (conditional_stack_.empty()) {
        addError("#endif without #ifdef/#ifndef");
        return false;
    }

    conditional_stack_.pop_back();
    return true;
}

bool Preprocessor::handleError(const std::string &content) {
    addError(content.empty() ? "#error" : content);
    return true;
}

bool Preprocessor::handleWarning(const std::string &content) {
    addWarning(content.empty() ? "#warning" : content);
    return true;
}

bool Preprocessor::handleInclude(const std::string &content) {
    // TODO: Cbの既存のimportシステムと統合
    addWarning("#include is not yet supported, use 'import' instead");
    return true;
}

std::string Preprocessor::expandMacros(const std::string &line) {
    std::string result = line;

    // 文字列リテラルの位置を記録
    std::vector<std::pair<size_t, size_t>> string_ranges;
    bool in_string = false;
    bool escaped = false;
    size_t string_start = 0;

    for (size_t i = 0; i < result.length(); i++) {
        if (escaped) {
            escaped = false;
            continue;
        }

        if (result[i] == '\\') {
            escaped = true;
            continue;
        }

        if (result[i] == '"') {
            if (!in_string) {
                in_string = true;
                string_start = i;
            } else {
                in_string = false;
                string_ranges.push_back({string_start, i});
            }
        }
    }

    // 文字列リテラル内かどうかをチェックする関数
    auto is_in_string = [&](size_t pos) {
        for (const auto &range : string_ranges) {
            if (pos > range.first && pos < range.second) {
                return true;
            }
        }
        return false;
    };

    // マクロを展開（複数回パス）
    bool changed = true;
    int max_iterations = 100; // 無限ループ防止
    int iterations = 0;

    while (changed && iterations < max_iterations) {
        changed = false;
        iterations++;

        for (const auto &[name, macro] : defines_) {
            if (macro.is_function_like) {
                continue; // 関数マクロは後で実装
            }

            // マクロ名を検索して置換
            size_t pos = 0;
            while ((pos = result.find(name, pos)) != std::string::npos) {
                // 文字列リテラル内はスキップ
                if (is_in_string(pos)) {
                    pos += name.length();
                    continue;
                }

                // 識別子の一部でないかチェック
                bool is_start_valid =
                    pos == 0 ||
                    !(std::isalnum(result[pos - 1]) || result[pos - 1] == '_');
                bool is_end_valid =
                    pos + name.length() >= result.length() ||
                    !(std::isalnum(result[pos + name.length()]) ||
                      result[pos + name.length()] == '_');

                if (is_start_valid && is_end_valid) {
                    result.replace(pos, name.length(), macro.body);
                    changed = true;
                    pos += macro.body.length();
                    // 文字列範囲を再計算
                    break;
                } else {
                    pos += name.length();
                }
            }
        }

        // 文字列範囲を再計算
        if (changed) {
            string_ranges.clear();
            in_string = false;
            escaped = false;
            string_start = 0;

            for (size_t i = 0; i < result.length(); i++) {
                if (escaped) {
                    escaped = false;
                    continue;
                }

                if (result[i] == '\\') {
                    escaped = true;
                    continue;
                }

                if (result[i] == '"') {
                    if (!in_string) {
                        in_string = true;
                        string_start = i;
                    } else {
                        in_string = false;
                        string_ranges.push_back({string_start, i});
                    }
                }
            }
        }
    }

    return result;
}

bool Preprocessor::shouldSkipOutput() const {
    for (const auto &state : conditional_stack_) {
        if (!state.condition_met) {
            return true;
        }
    }
    return false;
}

void Preprocessor::define(const std::string &name, const std::string &value) {
    defines_[name] = MacroDefinition(name, value, 0);
}

void Preprocessor::undefine(const std::string &name) { defines_.erase(name); }

bool Preprocessor::isDefined(const std::string &name) const {
    return defines_.find(name) != defines_.end();
}

std::string Preprocessor::trim(const std::string &str) const {
    size_t start = 0;
    size_t end = str.length();

    while (start < end && std::isspace(str[start])) {
        start++;
    }

    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }

    return str.substr(start, end - start);
}

void Preprocessor::addError(const std::string &message) {
    std::ostringstream oss;
    oss << current_file_ << ":" << current_line_ << ": error: " << message;
    errors_.push_back(oss.str());
}

void Preprocessor::addWarning(const std::string &message) {
    std::ostringstream oss;
    oss << current_file_ << ":" << current_line_ << ": warning: " << message;
    warnings_.push_back(oss.str());
}

} // namespace PreprocessorNS
