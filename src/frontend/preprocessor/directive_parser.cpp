#include "directive_parser.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace CbPreprocessor {

DirectiveParser::DirectiveParser() {}

DirectiveParser::~DirectiveParser() {}

MacroDefinition DirectiveParser::parseDefine(const std::string &line) {
    MacroDefinition macro;

    // "#define" の部分をスキップ
    size_t pos = line.find("#define");
    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid #define directive");
    }
    pos += 7; // "#define" の長さ

    // 空白をスキップ
    while (pos < line.length() && std::isspace(line[pos])) {
        pos++;
    }

    // マクロ名を抽出
    macro.name = extractMacroName(line, pos);
    if (macro.name.empty()) {
        throw std::runtime_error("Missing macro name");
    }

    // 空白をスキップ
    while (pos < line.length() && std::isspace(line[pos])) {
        pos++;
    }

    // 関数形式マクロか確認
    if (pos < line.length() && line[pos] == '(') {
        macro.type = MacroType::FUNCTION_LIKE;
        macro.parameters = extractParameters(line, pos);
    } else {
        macro.type = MacroType::OBJECT_LIKE;
    }

    // マクロ本体を抽出
    macro.body = extractBody(line, pos);

    return macro;
}

bool DirectiveParser::evaluateCondition(const std::string &condition) {
    // 将来実装: #if条件式の評価
    // 現在は常にtrueを返す
    return true;
}

bool DirectiveParser::parseMacroCall(const std::string &macro_call,
                                     std::string &name,
                                     std::vector<std::string> &args) {
    // マクロ名を抽出
    size_t paren_pos = macro_call.find('(');
    if (paren_pos == std::string::npos) {
        // オブジェクト形式マクロ
        name = trim(macro_call);
        args.clear();
        return true;
    }

    name = trim(macro_call.substr(0, paren_pos));

    // 引数を抽出
    size_t end_paren = macro_call.rfind(')');
    if (end_paren == std::string::npos || end_paren <= paren_pos) {
        return false; // 括弧が閉じていない
    }

    std::string args_str =
        macro_call.substr(paren_pos + 1, end_paren - paren_pos - 1);
    args.clear();

    // 引数を分割（カンマで区切る、ただしネストした括弧内は除く）
    int paren_depth = 0;
    std::string current_arg;

    for (char c : args_str) {
        if (c == ',' && paren_depth == 0) {
            args.push_back(trim(current_arg));
            current_arg.clear();
        } else {
            if (c == '(')
                paren_depth++;
            if (c == ')')
                paren_depth--;
            current_arg += c;
        }
    }

    if (!current_arg.empty() || args_str.empty()) {
        args.push_back(trim(current_arg));
    }

    return true;
}

// プライベートメソッド

std::vector<std::string> DirectiveParser::tokenize(const std::string &str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;

    while (iss >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

bool DirectiveParser::isBalanced(const std::string &str) {
    int balance = 0;
    for (char c : str) {
        if (c == '(')
            balance++;
        if (c == ')')
            balance--;
        if (balance < 0)
            return false;
    }
    return balance == 0;
}

std::string DirectiveParser::trim(const std::string &str) {
    size_t start = 0;
    while (start < str.length() && std::isspace(str[start])) {
        start++;
    }

    size_t end = str.length();
    while (end > start && std::isspace(str[end - 1])) {
        end--;
    }

    return str.substr(start, end - start);
}

std::string DirectiveParser::extractMacroName(const std::string &line,
                                              size_t &pos) {
    size_t start = pos;

    // 最初の文字は英字またはアンダースコア
    if (pos >= line.length() ||
        (!std::isalpha(static_cast<unsigned char>(line[pos])) &&
         line[pos] != '_')) {
        return "";
    }
    pos++;

    // 以降の文字は英数字またはアンダースコア
    while (pos < line.length() &&
           (std::isalnum(static_cast<unsigned char>(line[pos])) ||
            line[pos] == '_')) {
        pos++;
    }

    return line.substr(start, pos - start);
}

std::vector<std::string>
DirectiveParser::extractParameters(const std::string &line, size_t &pos) {
    std::vector<std::string> params;

    if (pos >= line.length() || line[pos] != '(') {
        return params;
    }
    pos++; // '(' をスキップ

    std::string current_param;
    int paren_depth = 0;

    while (pos < line.length()) {
        char c = line[pos];

        if (c == ')' && paren_depth == 0) {
            // パラメータリストの終わり
            if (!current_param.empty()) {
                params.push_back(trim(current_param));
            }
            pos++; // ')' をスキップ
            break;
        } else if (c == ',' && paren_depth == 0) {
            // パラメータの区切り
            params.push_back(trim(current_param));
            current_param.clear();
        } else {
            if (c == '(')
                paren_depth++;
            if (c == ')')
                paren_depth--;
            current_param += c;
        }

        pos++;
    }

    return params;
}

std::string DirectiveParser::extractBody(const std::string &line, size_t pos) {
    // 空白をスキップ
    while (pos < line.length() && std::isspace(line[pos])) {
        pos++;
    }

    if (pos >= line.length()) {
        return "";
    }

    // 行末までを本体として返す
    std::string body = line.substr(pos);

    // 末尾の空白を削除
    return trim(body);
}

} // namespace CbPreprocessor
