#include "token_preprocessor.h"
#include <iostream>
#include <sstream>

namespace RecursiveParserNS {

class TokenPreprocessor::Impl {
  public:
    CbPreprocessor::MacroExpander expander;
    CbPreprocessor::DirectiveParser directiveParser;
    std::string errorMessage;
    bool hasError;

    Impl() : hasError(false) {}

    std::vector<Token> process(const std::vector<Token> &tokens) {
        std::vector<Token> result;
        hasError = false;
        errorMessage.clear();

        for (size_t i = 0; i < tokens.size(); i++) {
            const Token &token = tokens[i];

            // プリプロセッサディレクティブの処理
            if (token.type == TokenType::TOK_PREPROCESSOR_DEFINE) {
                if (!processDefineDirective(token)) {
                    hasError = true;
                    return result;
                }
                // ディレクティブトークンは出力に含めない
                continue;
            }

            if (token.type == TokenType::TOK_PREPROCESSOR_UNDEF) {
                if (!processUndefDirective(token)) {
                    hasError = true;
                    return result;
                }
                // ディレクティブトークンは出力に含めない
                continue;
            }

            // 文字列リテラルはマクロ展開しない
            if (token.type == TokenType::TOK_STRING) {
                result.push_back(token);
                continue;
            }

            // 識別子のマクロ展開
            if (token.type == TokenType::TOK_IDENTIFIER) {
                auto expanded = expandMacroToken(token, tokens, i);
                result.insert(result.end(), expanded.begin(), expanded.end());
                continue;
            }

            // その他のトークンはそのまま出力
            result.push_back(token);
        }

        return result;
    }

    bool processDefineDirective(const Token &token) {
        // トークンの value から #define ディレクティブをパース
        std::string directive = token.value;

        // DirectiveParser は "#define..." 全体を期待しているので、そのまま渡す
        CbPreprocessor::MacroDefinition macroDef;
        try {
            macroDef = directiveParser.parseDefine(directive);
        } catch (const std::exception &e) {
            errorMessage = "Failed to parse #define directive at line " +
                           std::to_string(token.line) + ": " + e.what();
            return false;
        }

        if (macroDef.name.empty()) {
            errorMessage =
                "Empty macro name at line " + std::to_string(token.line);
            return false;
        }

        // MacroExpander に登録
        expander.define(macroDef);
        return true;
    }

    bool processUndefDirective(const Token &token) {
        // トークンの value から #undef ディレクティブをパース
        std::string directive = token.value;

        // "#undef " の後のマクロ名を取得
        size_t undefPos = directive.find("undef");
        if (undefPos == std::string::npos) {
            errorMessage = "Invalid #undef directive at line " +
                           std::to_string(token.line);
            return false;
        }

        size_t contentStart = undefPos + 5; // "undef" の長さ
        while (contentStart < directive.length() &&
               (directive[contentStart] == ' ' ||
                directive[contentStart] == '\t')) {
            contentStart++;
        }

        if (contentStart >= directive.length()) {
            errorMessage =
                "Empty #undef directive at line " + std::to_string(token.line);
            return false;
        }

        std::string macroName = directive.substr(contentStart);

        // 末尾の空白を削除
        while (!macroName.empty() &&
               (macroName.back() == ' ' || macroName.back() == '\t' ||
                macroName.back() == '\n' || macroName.back() == '\r')) {
            macroName.pop_back();
        }

        // MacroExpander から削除
        expander.undefine(macroName);
        return true;
    }

    std::vector<Token> expandMacroToken(const Token &token,
                                        const std::vector<Token> &allTokens,
                                        size_t &index) {
        std::vector<Token> result;
        std::string identifier = token.value;

        // マクロが定義されているか確認
        if (!expander.isDefined(identifier)) {
            result.push_back(token);
            return result;
        }

        // 関数マクロの場合、引数を処理
        std::string expanded;
        if (index + 1 < allTokens.size() &&
            allTokens[index + 1].type == TokenType::TOK_LPAREN) {

            // 引数リストを抽出
            std::string argsString = extractFunctionArguments(allTokens, index);

            if (!argsString.empty()) {
                // 引数文字列 "(a, b, c)" をパースして std::vector<std::string>
                // に変換
                auto args = parseArguments(argsString);
                expanded = expander.expand(identifier, args);
            } else {
                // 引数なし
                expanded = expander.expand(identifier, {});
            }
        } else {
            // オブジェクト形式マクロ
            expanded = expander.expand(identifier, {});
        }

        // 展開されたテキストをトークン化
        if (expanded != identifier) {
            auto expandedTokens =
                tokenizeExpansion(expanded, token.line, token.column);

            // 再帰的にマクロ展開（展開されたトークンに対しても処理を行う）
            auto recursivelyExpanded = process(expandedTokens);
            result.insert(result.end(), recursivelyExpanded.begin(),
                          recursivelyExpanded.end());
        } else {
            result.push_back(token);
        }

        return result;
    }

    std::vector<std::string> parseArguments(const std::string &argsString) {
        std::vector<std::string> args;

        // "(a, b, c)" から "a, b, c" を取り出す
        if (argsString.empty() || argsString.front() != '(' ||
            argsString.back() != ')') {
            return args;
        }

        std::string content = argsString.substr(1, argsString.length() - 2);

        // カンマで分割（ネストした括弧を考慮）
        std::string currentArg;
        int parenDepth = 0;

        for (char ch : content) {
            if (ch == '(' || ch == '[' || ch == '{') {
                parenDepth++;
                currentArg += ch;
            } else if (ch == ')' || ch == ']' || ch == '}') {
                parenDepth--;
                currentArg += ch;
            } else if (ch == ',' && parenDepth == 0) {
                // トリム
                size_t start = currentArg.find_first_not_of(" \t");
                size_t end = currentArg.find_last_not_of(" \t");
                if (start != std::string::npos) {
                    args.push_back(currentArg.substr(start, end - start + 1));
                }
                currentArg.clear();
            } else {
                currentArg += ch;
            }
        }

        // 最後の引数
        if (!currentArg.empty()) {
            size_t start = currentArg.find_first_not_of(" \t");
            size_t end = currentArg.find_last_not_of(" \t");
            if (start != std::string::npos) {
                args.push_back(currentArg.substr(start, end - start + 1));
            }
        }

        return args;
    }

    std::string extractFunctionArguments(const std::vector<Token> &tokens,
                                         size_t &index) {
        if (index + 1 >= tokens.size() ||
            tokens[index + 1].type != TokenType::TOK_LPAREN) {
            return "";
        }

        std::string args = "(";
        index++; // '(' をスキップ
        int parenDepth = 1;

        while (index + 1 < tokens.size() && parenDepth > 0) {
            index++;
            const Token &token = tokens[index];

            if (token.type == TokenType::TOK_LPAREN) {
                parenDepth++;
            } else if (token.type == TokenType::TOK_RPAREN) {
                parenDepth--;
            }

            args += token.value;

            if (parenDepth > 0 && token.type == TokenType::TOK_COMMA) {
                args += " "; // カンマの後にスペースを追加
            }
        }

        return args;
    }

    std::vector<Token> tokenizeExpansion(const std::string &text, int line,
                                         int column) {
        std::vector<Token> result;

        // 展開されたテキストを再度レキサーでトークン化
        RecursiveLexer lexer(text);
        while (!lexer.isAtEnd()) {
            Token token = lexer.nextToken();
            if (token.type == TokenType::TOK_EOF) {
                break;
            }
            // 元のトークンの位置情報を保持
            token.line = line;
            token.column = column;
            result.push_back(token);
        }

        return result;
    }

    void reset() {
        expander = CbPreprocessor::MacroExpander();
        errorMessage.clear();
        hasError = false;
    }
};

// TokenPreprocessor の実装

TokenPreprocessor::TokenPreprocessor() : pImpl_(new Impl()) {}

TokenPreprocessor::~TokenPreprocessor() { delete pImpl_; }

std::vector<Token>
TokenPreprocessor::process(const std::vector<Token> &tokens) {
    return pImpl_->process(tokens);
}

bool TokenPreprocessor::hasError() const { return pImpl_->hasError; }

std::string TokenPreprocessor::getError() const { return pImpl_->errorMessage; }

void TokenPreprocessor::reset() { pImpl_->reset(); }

} // namespace RecursiveParserNS
