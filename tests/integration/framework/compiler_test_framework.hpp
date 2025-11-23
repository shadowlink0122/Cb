#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <limits>
#include <map>
#include <sys/stat.h>
#include <unistd.h>

// v0.14.0: Compiler mode support
namespace cb {
namespace test {

enum class ExecutionMode {
    Interpreter,  // ./main file.cb
    Compiler      // ./main -c file.cb && ./output
};

// グローバル実行モード設定
inline ExecutionMode g_execution_mode = ExecutionMode::Interpreter;
inline std::string g_compiler_output_dir = "/tmp/cb_test_compiler_output";

// 実行モードを設定
inline void set_execution_mode(ExecutionMode mode) {
    g_execution_mode = mode;
}

// 実行モードを取得
inline ExecutionMode get_execution_mode() {
    return g_execution_mode;
}

// コンパイラ出力ディレクトリを設定
inline void set_compiler_output_dir(const std::string& dir) {
    g_compiler_output_dir = dir;
    // ディレクトリを作成
    mkdir(dir.c_str(), 0755);
}

} // namespace test
} // namespace cb

// ユーティリティ関数: 文字列を行に分割
inline std::vector<std::string> split_lines(const std::string& str) {
    std::vector<std::string> lines;
    std::stringstream ss(str);
    std::string line;
    
    while (std::getline(ss, line)) {
        // 行末の改行文字を削除
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) { // 空行は除外
            lines.push_back(line);
        }
    }
    
    return lines;
}

// CBインタープリターのパスを補正する関数
inline std::string fix_cb_interpreter_path(const std::string& original_command) {
    // パス補正は不要 - 元のコマンドをそのまま使用
    return original_command;
}

// v0.14.0: コンパイラモードでCbファイルを実行
inline int run_cb_file_compiler_mode(const std::string& cb_file, std::string& output) {
    using namespace cb::test;
    
    // 一時的な実行ファイル名を生成
    std::string basename = cb_file;
    size_t last_slash = basename.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        basename = basename.substr(last_slash + 1);
    }
    size_t dot = basename.find_last_of('.');
    if (dot != std::string::npos) {
        basename = basename.substr(0, dot);
    }
    
    std::string output_binary = g_compiler_output_dir + "/" + basename + "_test";
    
    // ステップ1: コンパイル
    std::string compile_cmd = "./main -c " + cb_file + " -o " + output_binary + " 2>&1";
    std::string compile_output;
    
    FILE* compile_pipe = popen(compile_cmd.c_str(), "r");
    if (!compile_pipe) {
        output = "ERROR: Failed to run compiler\n";
        return -1;
    }
    
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), compile_pipe) != nullptr) {
        compile_output += buffer;
    }
    int compile_exit = pclose(compile_pipe);
    
    if (compile_exit != 0) {
        output = "COMPILE ERROR:\n" + compile_output;
        return compile_exit;
    }
    
    // ステップ2: 実行
    std::string run_cmd = output_binary + " 2>&1";
    FILE* run_pipe = popen(run_cmd.c_str(), "r");
    if (!run_pipe) {
        output = "ERROR: Failed to run compiled binary\n";
        return -1;
    }
    
    std::string result;
    while (fgets(buffer, sizeof(buffer), run_pipe) != nullptr) {
        result += buffer;
    }
    int run_exit = pclose(run_pipe);
    
    output = result;
    
    // 実行ファイルを削除
    unlink(output_binary.c_str());
    
    return run_exit;
}

// v0.14.0: インタプリタモードでCbファイルを実行
inline int run_cb_file_interpreter_mode(const std::string& cb_file, std::string& output) {
    std::string command = "./main " + cb_file + " 2>&1";
    
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        output = "ERROR: Failed to run interpreter\n";
        return -1;
    }
    
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    
    int exit_code = pclose(pipe);
    output = result;
    return exit_code;
}

// v0.14.0: 実行モードに応じてCbファイルを実行
inline int run_cb_file(const std::string& cb_file, std::string& output) {
    using namespace cb::test;
    
    if (g_execution_mode == ExecutionMode::Compiler) {
        return run_cb_file_compiler_mode(cb_file, output);
    } else {
        return run_cb_file_interpreter_mode(cb_file, output);
    }
}

inline int run_command_and_capture(const std::string& command, std::string& output) {
    // パスを自動補正
    std::string fixed_command = fix_cb_interpreter_path(command);
    
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(fixed_command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    
    int exit_code = pclose(pipe);
    output = result;
    return exit_code;
}

// 時間測定機能付きのコマンド実行関数
inline int run_command_and_capture_with_time(const std::string& command, std::string& output, double& execution_time_ms) {
    // パスを自動補正
    std::string fixed_command = fix_cb_interpreter_path(command);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(fixed_command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    try {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    
    int exit_code = pclose(pipe);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    execution_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    
    output = result;
    return exit_code;
}

// テンポラリファイルを使用したテスト実行
inline std::string run_temp_cb_code(const std::string& cb_code) {
    // 一時ファイルを作成
    std::string temp_file = "/tmp/cb_test_temp_" + std::to_string(rand()) + ".cb";
    std::ofstream out(temp_file);
    out << cb_code;
    out.close();
    
    // 実行
    std::string output;
    run_cb_file(temp_file, output);
    
    // 一時ファイルを削除
    std::remove(temp_file.c_str());
    
    return output;
}

// テストケース構造体
struct TestCase {
    std::string name;
    std::function<void()> test_func;
    bool should_run = true;
    std::string skip_reason;
};

// テストスイート
class TestSuite {
private:
    std::string suite_name;
    std::vector<TestCase> test_cases;
    int passed = 0;
    int failed = 0;
    int skipped = 0;
    
public:
    TestSuite(const std::string& name) : suite_name(name) {}
    
    void add_test(const std::string& name, std::function<void()> test_func) {
        TestCase tc;
        tc.name = name;
        tc.test_func = test_func;
        test_cases.push_back(tc);
    }
    
    void skip_test(const std::string& name, const std::string& reason) {
        for (auto& tc : test_cases) {
            if (tc.name == name) {
                tc.should_run = false;
                tc.skip_reason = reason;
                break;
            }
        }
    }
    
    void run() {
        using namespace cb::test;
        
        std::cout << "\n=== Running Test Suite: " << suite_name;
        if (g_execution_mode == ExecutionMode::Compiler) {
            std::cout << " (COMPILER MODE)";
        } else {
            std::cout << " (INTERPRETER MODE)";
        }
        std::cout << " ===" << std::endl;
        
        for (auto& tc : test_cases) {
            if (!tc.should_run) {
                std::cout << "  [SKIP] " << tc.name;
                if (!tc.skip_reason.empty()) {
                    std::cout << " (" << tc.skip_reason << ")";
                }
                std::cout << std::endl;
                skipped++;
                continue;
            }
            
            try {
                tc.test_func();
                std::cout << "  [PASS] " << tc.name << std::endl;
                passed++;
            } catch (const std::exception& e) {
                std::cout << "  [FAIL] " << tc.name << ": " << e.what() << std::endl;
                failed++;
            }
        }
        
        std::cout << "\nResults: " << passed << " passed, " << failed << " failed, " << skipped << " skipped" << std::endl;
    }
    
    int get_failed_count() const { return failed; }
    int get_passed_count() const { return passed; }
    int get_skipped_count() const { return skipped; }
};

// アサーション関数
inline void assert_equal(const std::string& actual, const std::string& expected, const std::string& message = "") {
    if (actual != expected) {
        std::stringstream ss;
        ss << "Assertion failed";
        if (!message.empty()) {
            ss << ": " << message;
        }
        ss << "\n  Expected: \"" << expected << "\"";
        ss << "\n  Actual:   \"" << actual << "\"";
        throw std::runtime_error(ss.str());
    }
}

inline void assert_contains(const std::string& haystack, const std::string& needle, const std::string& message = "") {
    if (haystack.find(needle) == std::string::npos) {
        std::stringstream ss;
        ss << "Assertion failed";
        if (!message.empty()) {
            ss << ": " << message;
        }
        ss << "\n  Expected to contain: \"" << needle << "\"";
        ss << "\n  Actual:              \"" << haystack << "\"";
        throw std::runtime_error(ss.str());
    }
}

inline void assert_true(bool condition, const std::string& message = "") {
    if (!condition) {
        std::stringstream ss;
        ss << "Assertion failed";
        if (!message.empty()) {
            ss << ": " << message;
        }
        throw std::runtime_error(ss.str());
    }
}
