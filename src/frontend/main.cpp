#include "../backend/interpreter.h"
#include "../common/ast.h"
#include "../frontend/parser_utils.h"
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

// デバッグモードフラグ
bool debug_mode = false;

// debug_print関数
void debug_print(const char *fmt, ...) {
    if (!debug_mode)
        return;

    va_list args;
    va_start(args, fmt);
    printf("[DEBUG] ");
    vprintf(fmt, args);
    va_end(args);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "使用方法: %s <input.cb> [--debug]\n", argv[0]);
        return 1;
    }

    // コマンドライン引数の解析
    std::string filename;
    debug_mode = false;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--debug") {
            debug_mode = true;
        } else {
            filename = argv[i];
        }
    }

    if (filename.empty()) {
        std::fprintf(stderr, "エラー: 入力ファイルが指定されていません\n");
        return 1;
    }

    // ファイルを開く
    FILE *file = std::fopen(filename.c_str(), "r");
    if (!file) {
        std::fprintf(stderr, "エラー: ファイル '%s' を開けません\n",
                     filename.c_str());
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
            std::fprintf(stderr, "[DEBUG] 構文解析を開始します: %s\n",
                         filename.c_str());
        }

        int parse_result = yyparse();
        std::fclose(file);

        if (parse_result != 0) {
            std::fprintf(stderr, "エラー: 構文解析に失敗しました (行: %d)\n",
                         yylineno);
            return 1;
        }

        if (!root_node) {
            std::fprintf(stderr, "エラー: ASTが生成されませんでした\n");
            return 1;
        }

        if (debug_mode) {
            std::fprintf(stderr, "[DEBUG] ASTが正常に生成されました\n");
        }

        // インタープリター実行
        try {
            Interpreter interpreter(debug_mode);
            interpreter.process(root_node.get());

            if (debug_mode) {
                std::fprintf(stderr, "[DEBUG] 実行が正常に終了しました\n");
            }
        } catch (const std::exception &e) {
            std::fprintf(stderr, "%s\n", e.what());
            return 1;
        }

        return 0;

    } catch (const std::exception &e) {
        std::fprintf(stderr, "エラー: %s\n", e.what());
        std::fclose(file);
        return 1;
    } catch (...) {
        std::fprintf(stderr, "エラー: 予期しない例外が発生しました\n");
        std::fclose(file);
        return 1;
    }
}
