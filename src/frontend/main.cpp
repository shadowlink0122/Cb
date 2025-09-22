#include "../backend/interpreter.h"
#include "../common/ast.h"
#include "../frontend/parser_utils.h"
#include "debug.h"

// Recursive parser header first to avoid macro conflicts
#include "recursive_parser/recursive_parser.h"

// Then include Bison generated header (which defines conflicting macros)
#include "parser.h"

#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

using namespace RecursiveParserNS;

extern int yyparse();
extern FILE *yyin;
extern std::unique_ptr<ASTNode> root_node;
extern int yylineno;

// エラー表示用のグローバル変数
const char *current_filename = nullptr;
std::vector<std::string> file_lines;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(
            stderr, "使用方法: %s <input.cb> [--debug | --debug-ja | --yacc]\n",
            argv[0]);
        std::fprintf(stderr,
                     "  --debug    : デバッグモードを有効にする (英語)\n");
        std::fprintf(stderr,
                     "  --debug-ja : デバッグモードを有効にする (日本語)\n");
        std::fprintf(stderr,
                     "  --yacc     : レガシーYacc/Bisonパーサーを使用する\n");
        return 1;
    }

    // コマンドライン引数の解析
    std::string filename;
    debug_mode = false;
    debug_language = DebugLanguage::ENGLISH;
    bool use_yacc_parser = false; // デフォルトは再帰下降パーサー

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
            debug_language = DebugLanguage::ENGLISH;
        } else if (std::string(argv[i]) == "--debug-ja") {
            debug_mode = true;
            debug_language = DebugLanguage::JAPANESE;
        } else if (std::string(argv[i]) == "--yacc") {
            use_yacc_parser = true;
        } else {
            filename = argv[i];
        }
    }

    if (filename.empty()) {
        std::fprintf(stderr, "エラー: 入力ファイルが指定されていません\n");
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

    try {
        ASTNode *root = nullptr;

        if (!use_yacc_parser) {
            // デフォルト: 再帰下降パーサーを使用
            std::ifstream input(filename);
            if (!input) {
                std::fprintf(stderr, "エラー: ファイル '%s' を読み込めません\n",
                             filename.c_str());
                return 1;
            }

            std::string source((std::istreambuf_iterator<char>(input)),
                               std::istreambuf_iterator<char>());
            input.close();

            if (debug_mode) {
                std::cout << "再帰下降パーサーを使用してパースします...\n";
            }

            RecursiveParser parser(source);
            root = parser.parseProgram();
        } else {
            // レガシー: Bison パーサーを使用 (--yaccオプション指定時のみ)
            FILE *file = std::fopen(filename.c_str(), "r");
            if (!file) {
                std::fprintf(stderr, "エラー: ファイル '%s' を開けません\n",
                             filename.c_str());
                return 1;
            }

            // 字句解析器の入力を設定
            yyin = file;
            yylineno = 1;

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
                             "エラー: 構文解析に失敗しました (行: %d)\n",
                             yylineno);
                return 1;
            }

            if (!root_node) {
                std::fprintf(stderr, "エラー: ASTが生成されませんでした\n");
                return 1;
            }

            root = root_node.get();
        }

        if (!root) {
            std::fprintf(stderr, "エラー: ASTが生成されませんでした\n");
            return 1;
        }

        if (debug_mode) {
            debug_msg(DebugMsgId::AST_GENERATED);
        }

        // インタープリター実行
        try {
            Interpreter interpreter(debug_mode);
            interpreter.process(root);

            if (debug_mode) {
                debug_msg(DebugMsgId::EXECUTION_COMPLETE);
            }
        } catch (const std::exception &e) {
            std::fprintf(stderr, "%s\n", e.what());
            return 1;
        }

        return 0;

    } catch (const std::exception &e) {
        std::fprintf(stderr, "エラー: %s\n", e.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "エラー: 予期しない例外が発生しました\n");
        return 1;
    }
}
