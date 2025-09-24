#include "../backend/error_handler.h"
#include "../backend/interpreter.h"
#include "../common/ast.h"
#include "../common/debug.h"

// Recursive parser only
#include "recursive_parser/recursive_parser.h"

#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

using namespace RecursiveParserNS;

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
        std::fprintf(stderr, "エラー: 入力ファイルが指定されていません\n");
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

        RecursiveParser parser(source, filename);
        root = parser.parseProgram();

        if (!root) {
            std::fprintf(stderr, "エラー: ASTが生成されませんでした\n");
            return 1;
        }

        // インタープリターでASTを実行
        if (debug_mode) {
            std::fprintf(stderr, "Debug mode is enabled\n");
            debug_msg(DebugMsgId::INTERPRETER_START);
        }

        Interpreter interpreter(debug_mode);
        interpreter.process(root);

        delete root;

    } catch (const DetailedErrorException &e) {
        // 詳細なエラー表示は既に完了しているので何もしない
        return 1;
    } catch (const std::exception &e) {
        std::fprintf(stderr, "エラー: %s\n", e.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "エラー: 不明なエラーが発生しました\n");
        return 1;
    }

    return 0;
}
