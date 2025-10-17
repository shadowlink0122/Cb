#include "../backend/interpreter/core/error_handler.h"
#include "../backend/interpreter/core/interpreter.h"
#include "../common/ast.h"
#include "../common/debug.h"

// Recursive parser only
#include "preprocessor/token_preprocessor.h"
#include "recursive_parser/recursive_lexer.h"
#include "recursive_parser/recursive_parser.h"

#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

using namespace RecursiveParserNS;

// トークン列を文字列に変換（スペースを適切に挿入）
std::string tokensToString(const std::vector<Token> &tokens) {
    std::string result;
    TokenType lastType = TokenType::TOK_EOF;

    for (const auto &token : tokens) {
        // 前のトークンとの間にスペースが必要か判断
        bool needsSpace = false;
        if (!result.empty()) {
            // 識別子、数値、キーワードの後には通常スペースが必要
            if (lastType == TokenType::TOK_IDENTIFIER ||
                lastType == TokenType::TOK_NUMBER ||
                (lastType >= TokenType::TOK_MAIN &&
                 lastType <= TokenType::TOK_SCOPE_RESOLUTION)) {
                // 次のトークンが記号でない場合はスペースを追加
                if (token.type == TokenType::TOK_IDENTIFIER ||
                    token.type == TokenType::TOK_NUMBER ||
                    (token.type >= TokenType::TOK_MAIN &&
                     token.type <= TokenType::TOK_SCOPE_RESOLUTION)) {
                    needsSpace = true;
                }
            }
        }

        if (needsSpace) {
            result += " ";
        }

        // 文字列リテラルは引用符で囲む
        if (token.type == TokenType::TOK_STRING) {
            result += "\"" + token.value + "\"";
        } else {
            result += token.value;
        }
        lastType = token.type;
    }

    return result;
}

// エラー表示用のグローバル変数
const char *current_filename = nullptr;
std::vector<std::string> file_lines;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "使用法: " << argv[0]
                  << " <ファイル名> [--debug] [--debug-ja]" << std::endl;
        return 1;
    }

    // コマンドライン引数の解析
    std::string filename;
    debug_mode = false;
    debug_language = DebugLanguage::ENGLISH;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
            debug_language = DebugLanguage::ENGLISH;
        } else if (std::string(argv[i]) == "--debug-ja") {
            debug_mode = true;
            debug_language = DebugLanguage::JAPANESE;
        } else {
            filename = argv[i];
        }
    }

    if (filename.empty()) {
        std::fprintf(stderr, "Error: No input file specified\n");
        return 1;
    }

    // エラー表示のためにファイル名を設定し、行データを読み込み
    current_filename = filename.c_str();
    std::ifstream input_file(filename);
    std::string line;
    while (std::getline(input_file, line)) {
        file_lines.push_back(line);
    }
    input_file.close();

    try {
        ASTNode *root = nullptr;

        // RecursiveParserを使用
        std::ifstream input(filename);
        if (!input) {
            std::fprintf(stderr, "Error: Cannot read file '%s'\n",
                         filename.c_str());
            return 1;
        }

        std::string source((std::istreambuf_iterator<char>(input)),
                           std::istreambuf_iterator<char>());
        input.close();

        // TokenPreprocessor でマクロを展開（文字列リテラル保護）
        RecursiveLexer lexer(source);
        std::vector<Token> tokens;
        bool hasPreprocessorDirectives = false;

        while (!lexer.isAtEnd()) {
            Token token = lexer.nextToken();
            if (token.type == TokenType::TOK_EOF) {
                break;
            }
            // プリプロセッサディレクティブがあるか確認
            if (token.type == TokenType::TOK_PREPROCESSOR_DEFINE ||
                token.type == TokenType::TOK_PREPROCESSOR_UNDEF) {
                hasPreprocessorDirectives = true;
            }
            tokens.push_back(token);
        }

        std::string processedSource = source; // デフォルトは元のソース

        // プリプロセッサディレクティブがある場合のみ処理
        if (hasPreprocessorDirectives) {
            TokenPreprocessor preprocessor;
            std::vector<Token> processedTokens = preprocessor.process(tokens);

            if (preprocessor.hasError()) {
                std::fprintf(stderr, "Preprocessor Error: %s\n",
                             preprocessor.getError().c_str());
                return 1;
            }

            // トークン列を文字列に戻す
            processedSource = tokensToString(processedTokens);

            // Debug: output processed source
            if (debug_mode) {
                std::fprintf(stderr, "Processed source:\n%s\n",
                             processedSource.c_str());
            }
        }

        debug_msg(DebugMsgId::PARSE_USING_RECURSIVE_PARSER);

        RecursiveParser parser(processedSource, filename);
        parser.setDebugMode(debug_mode);
        root = parser.parseProgram();

        if (!root) {
            std::fprintf(stderr, "Error: AST generation failed\n");
            return 1;
        }

        // インタープリターでASTを実行
        if (debug_mode) {
            std::fprintf(stderr, "Debug mode is enabled\n");
        }
        debug_msg(DebugMsgId::INTERPRETER_START);

        Interpreter interpreter(debug_mode);

        // Parserからenum定義を同期
        interpreter.sync_enum_definitions_from_parser(&parser);

        // Parserからstruct定義を同期
        interpreter.sync_struct_definitions_from_parser(&parser);

        interpreter.process(root);

        // 正常終了：デストラクタをスキップして即座に終了
        // （メモリはOSが自動的に回収し、tagged pointer値の誤解放を回避）
        // 注：std::_Exit()はストリームのフラッシュをスキップするため、明示的にフラッシュ
        std::fflush(stdout);
        std::fflush(stderr);
        std::_Exit(0);

    } catch (const DetailedErrorException &e) {
        // 詳細なエラー表示は既に完了しているので何もしない
        std::fflush(stdout);
        std::fflush(stderr);
        std::_Exit(1);
    } catch (const std::exception &e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        std::fflush(stdout);
        std::fflush(stderr);
        std::_Exit(1);
    } catch (...) {
        std::fprintf(stderr, "Error: Unknown error occurred\n");
        std::fflush(stdout);
        std::fflush(stderr);
        std::_Exit(1);
    }

    // ここには到達しない
    return 0;
}
