#include "macro_expander.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace CbPreprocessor {

// ヘルパー関数の前方宣言
static std::string trim(const std::string &str);

MacroExpander::MacroExpander() {}

MacroExpander::~MacroExpander() {}

void MacroExpander::define(const MacroDefinition &macro) {
    macros_[macro.name] = macro;
}

bool MacroExpander::isDefined(const std::string &name) const {
    return macros_.find(name) != macros_.end();
}

void MacroExpander::undefine(const std::string &name) { macros_.erase(name); }

const MacroDefinition *MacroExpander::get(const std::string &name) const {
    auto it = macros_.find(name);
    if (it != macros_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::string MacroExpander::expand(const std::string &name,
                                  const std::vector<std::string> &args) {
    const MacroDefinition *macro = get(name);
    if (!macro) {
        return name; // マクロが見つからない場合はそのまま返す
    }

    // オブジェクト形式マクロの場合
    if (macro->isObjectLike()) {
        return macro->body;
    }

    // 関数形式マクロの場合
    if (args.size() != macro->parameters.size()) {
        // 引数の数が合わない（エラー処理は後で追加）
        return name;
    }

    std::string result = macro->body;

    // パラメータを引数で置換
    for (size_t i = 0; i < macro->parameters.size(); i++) {
        const std::string &param = macro->parameters[i];
        const std::string &arg = args[i];

        // パラメータを引数で置換（単純な文字列置換）
        size_t pos = 0;
        while ((pos = result.find(param, pos)) != std::string::npos) {
            // 識別子の一部でないことを確認
            bool before_ok = (pos == 0) || !isIdentifier(result[pos - 1]);
            bool after_ok = (pos + param.length() >= result.length()) ||
                            !isIdentifier(result[pos + param.length()]);

            if (before_ok && after_ok) {
                result.replace(pos, param.length(), arg);
                pos += arg.length();
            } else {
                pos += param.length();
            }
        }
    }

    return result;
}

std::string MacroExpander::expandAll(const std::string &source) {
    return expandRecursive(source, 0);
}

std::vector<std::string> MacroExpander::getDefinedMacros() const {
    std::vector<std::string> names;
    names.reserve(macros_.size());

    for (const auto &pair : macros_) {
        names.push_back(pair.first);
    }

    std::sort(names.begin(), names.end());
    return names;
}

void MacroExpander::clear() { macros_.clear(); }

// プライベートメソッド

std::string MacroExpander::stringifyArgument(const std::string &arg) {
    // #演算子: 引数を文字列リテラルに変換
    return "\"" + arg + "\"";
}

std::string MacroExpander::concatenateTokens(const std::string &left,
                                             const std::string &right) {
    // ##演算子: トークンを結合
    return left + right;
}

std::string MacroExpander::expandRecursive(const std::string &text, int depth) {
    // 無限再帰防止（深さ制限）
    const int MAX_DEPTH = 100;
    if (depth > MAX_DEPTH) {
        return text;
    }

    std::string result;
    size_t pos = 0;

    while (pos < text.length()) {
        // 識別子を探す
        if (isIdentifier(text[pos])) {
            std::string token = extractToken(text, pos);

            // マクロか確認
            if (isDefined(token)) {
                const MacroDefinition *macro = get(token);

                // オブジェクト形式マクロの場合
                if (macro->isObjectLike()) {
                    std::string expanded = expand(token);
                    // 再帰的に展開
                    expanded = expandRecursive(expanded, depth + 1);
                    result += expanded;
                } else {
                    // 関数形式マクロの場合
                    // 空白をスキップして '(' を探す
                    skipWhitespace(text, pos);

                    if (pos < text.length() && text[pos] == '(') {
                        // 引数を抽出
                        std::vector<std::string> args;
                        if (extractMacroArguments(text, pos, args)) {
                            // 各引数を展開（ネストしたマクロに対応）
                            for (auto &arg : args) {
                                arg = expandRecursive(arg, depth + 1);
                            }

                            // マクロを展開
                            std::string expanded = expand(token, args);
                            // 展開結果も再帰的に展開
                            expanded = expandRecursive(expanded, depth + 1);
                            result += expanded;
                        } else {
                            // 引数抽出失敗 - そのまま追加
                            result += token + "(";
                        }
                    } else {
                        // '(' が見つからない - 関数形式マクロだが引数がない
                        // そのまま追加（マクロとして展開しない）
                        result += token;
                    }
                }
            } else {
                result += token;
            }
        } else {
            result += text[pos];
            pos++;
        }
    }

    return result;
}

bool MacroExpander::isIdentifier(char c) const {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

std::string MacroExpander::extractToken(const std::string &text, size_t &pos) {
    size_t start = pos;
    while (pos < text.length() && isIdentifier(text[pos])) {
        pos++;
    }
    return text.substr(start, pos - start);
}

void MacroExpander::skipWhitespace(const std::string &text, size_t &pos) {
    while (pos < text.length() &&
           std::isspace(static_cast<unsigned char>(text[pos]))) {
        pos++;
    }
}

bool MacroExpander::extractMacroArguments(const std::string &text, size_t &pos,
                                          std::vector<std::string> &args) {
    args.clear();

    // '(' をスキップ
    if (pos >= text.length() || text[pos] != '(') {
        return false;
    }
    pos++;

    int paren_depth = 1; // 括弧のネストレベル
    std::string current_arg;

    while (pos < text.length() && paren_depth > 0) {
        char c = text[pos];

        if (c == '(') {
            paren_depth++;
            current_arg += c;
        } else if (c == ')') {
            paren_depth--;
            if (paren_depth == 0) {
                // 引数リストの終わり
                // 空白を削除して追加
                current_arg = trim(current_arg);
                if (!current_arg.empty() || args.size() > 0) {
                    args.push_back(current_arg);
                }
                pos++; // ')' をスキップ
                break;
            } else {
                current_arg += c;
            }
        } else if (c == ',' && paren_depth == 1) {
            // 引数の区切り（最上位レベルのカンマのみ）
            args.push_back(trim(current_arg));
            current_arg.clear();
        } else {
            current_arg += c;
        }

        pos++;
    }

    // 括弧が閉じていない場合
    if (paren_depth != 0) {
        return false;
    }

    return true;
}

// trimヘルパー関数（既にdirective_parserにあるが、ここでも定義）
std::string trim(const std::string &str) {
    size_t start = 0;
    while (start < str.length() &&
           std::isspace(static_cast<unsigned char>(str[start]))) {
        start++;
    }

    size_t end = str.length();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        end--;
    }

    return str.substr(start, end - start);
}

} // namespace CbPreprocessor
