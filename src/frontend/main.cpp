#include "../backend/interpreter.h"
#include "../common/ast.h"
#include "../common/debug.h"
#include "../common/io_interface.h"
#include "../frontend/parser_utils.h"
#include "help_messages.h"
#include "parser.h"
#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

extern int yyparse();
extern FILE *yyin;
extern std::unique_ptr<ASTNode> root_node;
extern int yylineno;

// エラー表示用のグローバル変数
const char *current_filename = nullptr;
std::vector<std::string> file_lines;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "%s\n",
                     get_help_message(HelpMsgId::ERROR_INPUT_NOT_SPECIFIED,
                                      HelpLanguage::ENGLISH));
        std::fprintf(
            stderr, "%s\n",
            get_help_message(HelpMsgId::USE_HELP_INFO, HelpLanguage::ENGLISH));
        return 1;
    }

    // コマンドライン引数の解析
    std::string filename;
    std::string target_platform = "native"; // デフォルトはネイティブ
    debug_mode = false;
    debug_language = DebugLanguage::ENGLISH;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            show_help(HelpLanguage::ENGLISH, argv[0]);
            return 0;
        } else if (arg == "--help-ja") {
            show_help(HelpLanguage::JAPANESE, argv[0]);
            return 0;
        } else if (arg == "--debug") {
            debug_mode = true;
            debug_language = DebugLanguage::ENGLISH;
        } else if (arg == "--debug-ja") {
            debug_mode = true;
            debug_language = DebugLanguage::JAPANESE;
        } else if (arg.find("--target=") == 0) {
            target_platform = arg.substr(9); // "--target="の後の部分を取得
            if (target_platform != "native" && target_platform != "baremetal" &&
                target_platform != "wasm") {
                std::fprintf(stderr,
                             get_help_message(HelpMsgId::ERROR_INVALID_TARGET,
                                              HelpLanguage::ENGLISH),
                             target_platform.c_str());
                std::fprintf(stderr, "\n");
                std::fprintf(stderr, "%s\n",
                             get_help_message(HelpMsgId::VALID_TARGETS_INFO,
                                              HelpLanguage::ENGLISH));
                std::fprintf(stderr, "%s\n",
                             get_help_message(HelpMsgId::USE_HELP_INFO,
                                              HelpLanguage::ENGLISH));
                return 1;
            }
        } else if (arg[0] == '-') {
            std::fprintf(stderr,
                         get_help_message(HelpMsgId::ERROR_UNKNOWN_OPTION,
                                          HelpLanguage::ENGLISH),
                         arg.c_str());
            std::fprintf(stderr, "\n");
            std::fprintf(stderr, "%s\n",
                         get_help_message(HelpMsgId::USE_HELP_INFO,
                                          HelpLanguage::ENGLISH));
            return 1;
        } else {
            filename = arg;
        }
    }

    if (filename.empty()) {
        std::fprintf(stderr, "%s\n",
                     get_help_message(HelpMsgId::ERROR_INPUT_NOT_SPECIFIED,
                                      HelpLanguage::ENGLISH));
        std::fprintf(
            stderr, "%s\n",
            get_help_message(HelpMsgId::USE_HELP_INFO, HelpLanguage::ENGLISH));
        return 1;
    }

    // ファイルを開く
    FILE *file = std::fopen(filename.c_str(), "r");
    if (!file) {
        std::fprintf(stderr,
                     get_help_message(HelpMsgId::ERROR_CANNOT_OPEN_FILE,
                                      HelpLanguage::ENGLISH),
                     filename.c_str());
        std::fprintf(stderr, "\n");
        return 1;
    }

    // エラー表示のためにファイル名を設定し、行データを読み込み
    current_filename = filename.c_str();
    file_lines.clear();

    // ファイルの全行を読み込んで保存（エラー表示用）
    std::ifstream input_file(filename);
    std::string line;
    while (std::getline(input_file, line)) {
        file_lines.push_back(line);
    }
    input_file.close();

    // 字句解析器の入力を設定
    yyin = file;
    yylineno = 1;

    try {
// デバッグモードの設定
#if YYDEBUG
        extern int yydebug;
        if (debug_mode) {
            yydebug = 1;
        }
#endif

        // 構文解析
        if (debug_mode) {
            debug_msg(DebugMsgId::PARSING_START, filename.c_str());
        }

        int parse_result = yyparse();
        std::fclose(file);

        if (parse_result != 0) {
            std::fprintf(stderr,
                         get_help_message(HelpMsgId::ERROR_PARSING_FAILED,
                                          HelpLanguage::ENGLISH),
                         yylineno);
            std::fprintf(stderr, "\n");
            return 1;
        }

        if (!root_node) {
            std::fprintf(stderr, "%s\n",
                         get_help_message(HelpMsgId::ERROR_AST_NOT_GENERATED,
                                          HelpLanguage::ENGLISH));
            return 1;
        }

        if (debug_mode) {
            debug_msg(DebugMsgId::AST_GENERATED);
        }

        // ターゲットプラットフォームを設定
        IOFactory::set_target_platform(target_platform);

        // インタープリター実行
        try {
            Interpreter interpreter(debug_mode);
            interpreter.process(root_node.get());

            if (debug_mode) {
                debug_msg(DebugMsgId::EXECUTION_COMPLETE);
            }
        } catch (const std::exception &e) {
            std::fprintf(stderr, "%s\n", e.what());
            return 1;
        }

        return 0;

    } catch (const std::exception &e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        std::fclose(file);
        return 1;
    } catch (...) {
        std::fprintf(stderr, "%s\n",
                     get_help_message(HelpMsgId::ERROR_UNEXPECTED_EXCEPTION,
                                      HelpLanguage::ENGLISH));
        std::fclose(file);
        return 1;
    }
}
