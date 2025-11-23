#pragma once

// ============================================================================
// Integration Test Framework v2
// ============================================================================
// 目的: Cb言語の機能をエンドツーエンドでテストする
// 
// 統合テストの責務:
//   - Cbプログラムの実行結果を検証する
//   - 言語機能（構文、セマンティクス）が正しく動作することを確認する
//   - インタプリタモードとコンパイラモードの両方で動作を保証する
//
// 統合テストで検証しないもの:
//   - HIR/MIR/LIRなどの中間表現の詳細
//     → これらはユニットテスト (tests/unit/) で検証する
//   - 内部実装の詳細やアーキテクチャ
//     → これらはユニットテスト (tests/unit/) で検証する
//
// 統合テストは「Cb言語のユーザー視点」でのテストです。
// ============================================================================

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

// v0.14.0: 実行モードの定義
enum class ExecutionMode {
    Interpreter,  // インタプリタモード（デフォルト）
    Compiler,     // コンパイラモード（-c オプション）
    Both          // 両方実行
};

// v0.14.0: テスト設定クラス
class IntegrationTestConfig {
private:
    static inline ExecutionMode current_mode = ExecutionMode::Interpreter;
    static inline std::string cb_executable_path = "../../cb";

public:
    static void set_execution_mode(ExecutionMode mode) {
        current_mode = mode;
    }

    static ExecutionMode get_execution_mode() {
        return current_mode;
    }

    static void set_cb_executable_path(const std::string& path) {
        cb_executable_path = path;
    }

    static std::string get_cb_executable_path() {
        return cb_executable_path;
    }

    // モードに応じたコマンドを生成
    static std::string build_command(const std::string& test_file, ExecutionMode mode) {
        std::string cmd = cb_executable_path;

        if (mode == ExecutionMode::Compiler) {
            cmd += " compile";
        } else {
            cmd += " run";
        }

        cmd += " " + test_file + " 2>&1";
        return cmd;
    }
};

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

inline int run_command_and_capture(const std::string& command, std::string& output) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
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
    auto start_time = std::chrono::high_resolution_clock::now();

    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(command.c_str(), "r");
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

// v0.14.0: モード指定可能なテスト実行
inline void run_cb_test_with_output(const std::string& test_file,
                                   const std::function<void(const std::string&, int)>& validator,
                                   ExecutionMode mode = ExecutionMode::Interpreter) {
    std::string command = IntegrationTestConfig::build_command(test_file, mode);
    std::string output;
    int exit_code = run_command_and_capture(command, output);

    try {
        validator(output, exit_code);
    } catch (const std::exception& e) {
        std::cerr << "[integration-test] TEST FAILURE in file: " << test_file << std::endl;
        std::cerr << "[integration-test] Mode: " << (mode == ExecutionMode::Compiler ? "Compiler" : "Interpreter") << std::endl;
        std::cerr << "[integration-test] Error: " << e.what() << std::endl;
        std::cerr << "[integration-test] Command: " << command << std::endl;
        std::cerr << "[integration-test] Exit code: " << exit_code << std::endl;
        std::cerr << "[integration-test] Output:" << std::endl;
        std::cerr << output << std::endl;
        std::cerr << "[integration-test] --- End of output ---" << std::endl;
        throw;
    }
}

// v0.14.0: 両モードでテストを実行
inline void run_cb_test_with_output_both_modes(const std::string& test_file,
                                               const std::function<void(const std::string&, int)>& validator) {
    // インタプリタモード
    std::cout << "[integration-test] Testing in INTERPRETER mode..." << std::endl;
    run_cb_test_with_output(test_file, validator, ExecutionMode::Interpreter);

    // コンパイラモード
    std::cout << "[integration-test] Testing in COMPILER mode..." << std::endl;
    run_cb_test_with_output(test_file, validator, ExecutionMode::Compiler);
}

// 時間測定機能付きのテスト実行関数
inline void run_cb_test_with_output_and_time(const std::string& test_file,
                                             const std::function<void(const std::string&, int)>& validator,
                                             double& execution_time_ms,
                                             ExecutionMode mode = ExecutionMode::Interpreter) {
    std::string command = IntegrationTestConfig::build_command(test_file, mode);
    std::string output;
    int exit_code = run_command_and_capture_with_time(command, output, execution_time_ms);

    try {
        validator(output, exit_code);
    } catch (const std::exception& e) {
        std::cerr << "[integration-test] TEST FAILURE in file: " << test_file << std::endl;
        std::cerr << "[integration-test] Mode: " << (mode == ExecutionMode::Compiler ? "Compiler" : "Interpreter") << std::endl;
        std::cerr << "[integration-test] Error: " << e.what() << std::endl;
        std::cerr << "[integration-test] Command: " << command << std::endl;
        std::cerr << "[integration-test] Exit code: " << exit_code << std::endl;
        std::cerr << "[integration-test] Execution time: " << execution_time_ms << " ms" << std::endl;
        std::cerr << "[integration-test] Output:" << std::endl;
        std::cerr << output << std::endl;
        std::cerr << "[integration-test] --- End of output ---" << std::endl;
        throw;
    }
}

// 出力チェック用のヘルパー
inline bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

// Test counter for integration tests
class IntegrationTestCounter {
private:
    static inline int total_tests = 0;
    static inline int passed_tests = 0;
    static inline int failed_tests = 0;

public:
    static void increment_total() { total_tests++; }
    static void increment_passed() { passed_tests++; }
    static void increment_failed() { failed_tests++; }
    static void reset() { total_tests = 0; passed_tests = 0; failed_tests = 0; }

    static int get_total() { return total_tests; }
    static int get_passed() { return passed_tests; }
    static int get_failed() { return failed_tests; }

    static void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total:  " << total_tests << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << failed_tests << std::endl;
    }
};

// Timing statistics tracker
class TimingStats {
private:
    static inline std::vector<double> execution_times;
    static inline double total_time = 0.0;
    static inline double min_time = std::numeric_limits<double>::max();
    static inline double max_time = 0.0;

public:
    static void add_time(double time_ms) {
        execution_times.push_back(time_ms);
        total_time += time_ms;
        if (time_ms < min_time) min_time = time_ms;
        if (time_ms > max_time) max_time = time_ms;
    }

    static void reset() {
        execution_times.clear();
        total_time = 0.0;
        min_time = std::numeric_limits<double>::max();
        max_time = 0.0;
    }

    static double get_average() {
        return execution_times.empty() ? 0.0 : total_time / execution_times.size();
    }

    static double get_total() { return total_time; }
    static double get_min() { return execution_times.empty() ? 0.0 : min_time; }
    static double get_max() { return execution_times.empty() ? 0.0 : max_time; }
    static size_t get_count() { return execution_times.size(); }

    static void print_timing_summary() {
        if (execution_times.empty()) {
            std::cout << "\n=== Timing Summary ===" << std::endl;
            std::cout << "No timing data available" << std::endl;
            return;
        }

        std::cout << "\n=== Timing Summary ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Tests with timing: " << execution_times.size() << std::endl;
        std::cout << "Total time: " << total_time << " ms" << std::endl;
        std::cout << "Average time: " << get_average() << " ms" << std::endl;
        std::cout << "Min time: " << min_time << " ms" << std::endl;
        std::cout << "Max time: " << max_time << " ms" << std::endl;
        std::cout << std::resetiosflags(std::ios::fixed);
    }
};

// 基本的なアサーション（ファイル名と行数付き）
#define INTEGRATION_ASSERT(condition, message) \
    do { \
        IntegrationTestCounter::increment_total(); \
        if (!(condition)) { \
            std::cerr << "[integration-test] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration-test] " << message << std::endl; \
            IntegrationTestCounter::increment_failed(); \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } else { \
            IntegrationTestCounter::increment_passed(); \
        } \
    } while(0)

#define INTEGRATION_ASSERT_EQ(expected, actual, message) \
    do { \
        IntegrationTestCounter::increment_total(); \
        if (!((expected) == (actual))) { \
            std::cerr << "[integration-test] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration-test] Expected: " << (expected) << std::endl; \
            std::cerr << "[integration-test] Actual: " << (actual) << std::endl; \
            std::cerr << "[integration-test] " << message << std::endl; \
            IntegrationTestCounter::increment_failed(); \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } else { \
            IntegrationTestCounter::increment_passed(); \
        } \
    } while(0)

#define INTEGRATION_ASSERT_CONTAINS(haystack, needle, message) \
    do { \
        IntegrationTestCounter::increment_total(); \
        if (!contains((haystack), (needle))) { \
            std::cerr << "[integration-test] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration-test] Expected to find: " << (needle) << std::endl; \
            std::cerr << "[integration-test] In output: " << (haystack) << std::endl; \
            std::cerr << "[integration-test] " << message << std::endl; \
            IntegrationTestCounter::increment_failed(); \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } else { \
            IntegrationTestCounter::increment_passed(); \
        } \
    } while(0)

// テスト結果の出力
inline void integration_test_passed(const std::string& test_name, ExecutionMode mode = ExecutionMode::Interpreter) {
    std::string mode_str = (mode == ExecutionMode::Compiler) ? "[COMPILER]" : "[INTERPRETER]";
    std::cout << "[integration-test] [PASS] " << mode_str << " " << test_name << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_time(const std::string& test_name, const std::string& test_file,
                                             double execution_time_ms, ExecutionMode mode = ExecutionMode::Interpreter) {
    std::string mode_str = (mode == ExecutionMode::Compiler) ? "[COMPILER]" : "[INTERPRETER]";
    std::cout << "[integration-test] [PASS] " << mode_str << " " << test_name << " (" << test_file << ")" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
    TimingStats::add_time(execution_time_ms);
}
