#include "../backend/codegen/hir_to_cpp.h" // v0.14.0: C++コード生成
#include "../backend/interpreter/core/error_handler.h"
#include "../backend/interpreter/core/interpreter.h"
#include "../backend/ir/hir/hir_generator.h" // v0.14.0: HIR生成
#include "../common/ast.h"
#include "../common/debug.h"

// Recursive parser only
#include "recursive_parser/recursive_parser.h"

// Preprocessor (v0.13.0)
#include "preprocessor/preprocessor.h"

// Help messages
#include "help_messages.h"

#include <cstdarg>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <unistd.h> // for getpid
#include <vector>

using namespace RecursiveParserNS;
using namespace HelpMessages;

// エラー表示用のグローバル変数
const char *current_filename = nullptr;
std::vector<std::string> file_lines;

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse command
    std::string command = argv[1];
    bool compile_only = false;
    int arg_start = 2;

    // Handle version and help
    if (command == "--version" || command == "-v") {
        print_version();
        return 0;
    } else if (command == "--help" || command == "-h") {
        print_usage(argv[0]);
        return 0;
    }

    // Parse command with short options
    if (command == "run" || command == "-r") {
        compile_only = false;
    } else if (command == "compile" || command == "-c") {
        compile_only = true;
    } else {
        // Backward compatibility: assume 'run' if no command
        compile_only = false;
        arg_start = 1;
    }

    // Check for command-specific help
    if (argc > 2 &&
        (std::string(argv[2]) == "--help" || std::string(argv[2]) == "-h")) {
        if (compile_only) {
            print_compile_help(argv[0]);
        } else {
            print_run_help(argv[0]);
        }
        return 0;
    }

    // Parse arguments
    std::string filename;
    std::string output_file;
    std::string cpp_output_dir;
    debug_mode = false;
    debug_language = DebugLanguage::ENGLISH;
    bool enable_preprocessor = true;
    PreprocessorNS::Preprocessor preprocessor;

    for (int i = arg_start; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--debug" || arg == "-d") {
            debug_mode = true;
            debug_language = DebugLanguage::ENGLISH;
        } else if (arg == "--debug-ja") {
            debug_mode = true;
            debug_language = DebugLanguage::JAPANESE;
        } else if (arg == "--no-preprocess") {
            enable_preprocessor = false;
        } else if (arg == "-o") {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "Error: -o requires an output filename\n";
                return 1;
            }
        } else if (arg == "-cpp") {
            if (i + 1 < argc) {
                cpp_output_dir = argv[++i];
            } else {
                std::cerr << "Error: -cpp requires a directory path\n";
                return 1;
            }
        } else if (arg.substr(0, 2) == "-D") {
            std::string define_str = arg.substr(2);
            size_t eq_pos = define_str.find('=');
            if (eq_pos != std::string::npos) {
                std::string name = define_str.substr(0, eq_pos);
                std::string value = define_str.substr(eq_pos + 1);
                preprocessor.define(name, value);
            } else {
                preprocessor.define(define_str, "1");
            }
        } else if (arg[0] != '-') {
            filename = arg;
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: No input file specified\n";
        print_usage(argv[0]);
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

        // プリプロセッサ処理 (v0.13.0)
        if (enable_preprocessor) {
            source = preprocessor.process(source, filename);

            // プリプロセッサエラー/警告の表示
            for (const auto &warning : preprocessor.getWarnings()) {
                std::cerr << warning << std::endl;
            }
            for (const auto &error : preprocessor.getErrors()) {
                std::cerr << error << std::endl;
            }
            if (!preprocessor.getErrors().empty()) {
                return 1;
            }
        }

        debug_msg(DebugMsgId::PARSE_USING_RECURSIVE_PARSER);

        RecursiveParser parser(source, filename);
        parser.setDebugMode(debug_mode);
        root = parser.parseProgram();

        if (!root) {
            std::fprintf(stderr, "Error: AST generation failed\n");
            return 1;
        }

        // v0.14.0: コンパイルのみモード（HIR → C++ → Binary）
        if (compile_only) {
            std::cout << "Compile mode: Generating HIR from AST..."
                      << std::endl;

            // HIRGeneratorを使ってASTからHIRを生成
            // パーサーの定義情報（インポートされたモジュールを含む）も渡す
            cb::ir::HIRGenerator hir_gen;
            auto hir_program = hir_gen.generate_with_parser_definitions(
                root->statements, parser.get_struct_definitions(),
                parser.get_interface_definitions(),
                parser.get_impl_definitions());

            if (!hir_program) {
                std::cerr << "Error: HIR generation failed" << std::endl;
                return 1;
            }

            // v0.14.0: C++コード生成
            cb::codegen::HIRToCpp transpiler;
            std::string cpp_code = transpiler.generate(*hir_program);

            // C++出力ディレクトリの決定
            std::string cpp_dir;
            std::string cpp_filename;

            if (!cpp_output_dir.empty()) {
                // -cpp オプションで指定された場合
                cpp_dir = cpp_output_dir;
            } else {
                // デフォルト: ./tmp に元のファイルと同じ階層を作成
                cpp_dir = "./tmp";

                // 入力ファイルのディレクトリ構造を取得
                std::string input_dir = filename;
                size_t last_slash = input_dir.find_last_of("/\\");
                if (last_slash != std::string::npos) {
                    input_dir = input_dir.substr(0, last_slash);
                    cpp_dir = "./tmp/" + input_dir;
                }
            }

            // ディレクトリを作成
            std::string mkdir_cmd = "mkdir -p " + cpp_dir;
            system(mkdir_cmd.c_str());

            // ファイル名を決定（ベース名 + .cpp）
            std::string base_name = filename;
            size_t last_slash_base = base_name.find_last_of("/\\");
            if (last_slash_base != std::string::npos) {
                base_name = base_name.substr(last_slash_base + 1);
            }
            size_t dot_pos = base_name.find_last_of('.');
            if (dot_pos != std::string::npos) {
                base_name = base_name.substr(0, dot_pos);
            }

            cpp_filename = cpp_dir + "/" + base_name + ".cpp";

            // C++コードをファイルに保存
            std::ofstream cpp_out(cpp_filename);
            cpp_out << cpp_code;
            cpp_out.close();

            std::cout << "C++ code saved to: " << cpp_filename << std::endl;

            // 一時C++ファイル（コンパイル用）
            std::string temp_cpp =
                "./tmp/cb_compiled_" + std::to_string(getpid()) + ".cpp";
            std::ofstream temp_out(temp_cpp);
            temp_out << cpp_code;
            temp_out.close();

            // 出力ファイル名を決定
            std::string output_binary;
            if (!output_file.empty()) {
                // -o オプションで指定された場合はそのまま使用
                output_binary = output_file;
            } else {
                // デフォルト: 入力ファイルと同じディレクトリに .o 拡張子で出力
                output_binary = filename;
                size_t dot_pos = output_binary.find_last_of('.');
                if (dot_pos != std::string::npos) {
                    output_binary = output_binary.substr(0, dot_pos);
                }
                output_binary += ".o";
            }

            // C++コンパイラでコンパイル
            std::string compile_cmd = "g++ -std=c++17 " + temp_cpp + " -o " +
                                      output_binary + " -lm 2>&1";
            std::cout << "Compiling C++ code..." << std::endl;

            int compile_result = system(compile_cmd.c_str());

            // 一時ファイルを削除（デバッグモード以外）
            if (!debug_mode) {
                std::remove(temp_cpp.c_str());
            }

            if (compile_result != 0) {
                std::cerr << "Error: C++ compilation failed" << std::endl;
                return 1;
            }

            std::cout << "Compilation completed successfully!" << std::endl;
            std::cout << "Output binary: " << output_binary << std::endl;
            return 0;
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

        // v0.11.0: Parserからinterface/impl定義を同期
        interpreter.sync_interface_definitions_from_parser(&parser);
        interpreter.sync_impl_definitions_from_parser(&parser);

        // v0.11.1: パース時のimportは型情報のみを取り込む
        // 関数定義はインタプリタ側で登録する必要があるため、
        // loaded_modulesへの追加はhandle_import_statementに任せる
        // （重複パースは許容するが、loaded_modulesチェックで重複登録は防ぐ）
        /*
        if (root && !root->statements.empty()) {
            for (const auto &stmt : root->statements) {
                if (stmt && stmt->node_type == ASTNodeType::AST_IMPORT_STMT &&
                    !stmt->import_path.empty()) {
                    interpreter.mark_module_loaded(stmt->import_path);
                }
            }
        }
        */

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
