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

// テスト用のユーティリティ関数

// CBインタープリターのパスを補正する関数
inline std::string fix_cb_interpreter_path(const std::string& original_command) {
    // パス補正は不要 - 元のコマンドをそのまま使用
    return original_command;
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
inline void run_cb_test_with_output(const std::string& test_file, 
                                   const std::function<void(const std::string&, int)>& validator) {
    std::string command = "../../cb run " + test_file + " 2>&1";
    std::string output;
    int exit_code = run_command_and_capture(command, output);
    
    try {
        validator(output, exit_code);
    } catch (const std::exception& e) {
        std::cerr << "[integration-test] TEST FAILURE in file: " << test_file << std::endl;
        std::cerr << "[integration-test] Error: " << e.what() << std::endl;
        std::cerr << "[integration-test] Command: " << command << std::endl;
        std::cerr << "[integration-test] Exit code: " << exit_code << std::endl;
        std::cerr << "[integration-test] Output:" << std::endl;
        std::cerr << output << std::endl;
        std::cerr << "[integration-test] --- End of output ---" << std::endl;
        throw;
    }
}

// 時間測定機能付きのテスト実行関数
inline void run_cb_test_with_output_and_time(const std::string& test_file, 
                                             const std::function<void(const std::string&, int)>& validator,
                                             double& execution_time_ms) {
    std::string command = "../../cb run " + test_file + " 2>&1";
    std::string output;
    int exit_code = run_command_and_capture_with_time(command, output, execution_time_ms);
    
    try {
        validator(output, exit_code);
    } catch (const std::exception& e) {
        std::cerr << "[integration-test] TEST FAILURE in file: " << test_file << std::endl;
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

// より詳細なアサーションマクロ
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

#define INTEGRATION_ASSERT_NE(not_expected, actual, message) \
    do { \
        IntegrationTestCounter::increment_total(); \
        if ((not_expected) == (actual)) { \
            std::cerr << "[integration-test] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration-test] Expected NOT: " << (not_expected) << std::endl; \
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

#define INTEGRATION_ASSERT_NOT_CONTAINS(haystack, needle, message) \
    do { \
        IntegrationTestCounter::increment_total(); \
        if (contains((haystack), (needle))) { \
            std::cerr << "[integration-test] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration-test] Did not expect to find: " << (needle) << std::endl; \
            std::cerr << "[integration-test] In output: " << (haystack) << std::endl; \
            std::cerr << "[integration-test] " << message << std::endl; \
            IntegrationTestCounter::increment_failed(); \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } else { \
            IntegrationTestCounter::increment_passed(); \
        } \
    } while(0)

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
        std::cout << "=== Test Summary ===" << std::endl;
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
            std::cout << "=== Timing Summary ===" << std::endl;
            std::cout << "No timing data available" << std::endl;
            return;
        }
        
        std::cout << "=== Timing Summary ===" << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Tests with timing: " << execution_times.size() << std::endl;
        std::cout << "Total time: " << total_time << " ms" << std::endl;
        std::cout << "Average time: " << get_average() << " ms" << std::endl;
        std::cout << "Min time: " << min_time << " ms" << std::endl;
        std::cout << "Max time: " << max_time << " ms" << std::endl;
        
        // Show slowest tests (top 5)
        if (execution_times.size() > 1) {
            std::cout << "Performance insights:" << std::endl;
            if (max_time > get_average() * 2) {
                std::cout << "- Some tests are significantly slower than average" << std::endl;
            }
            if (max_time > 100.0) {
                std::cout << "- Consider optimizing tests taking >100ms" << std::endl;
            }
        }
        std::cout << std::resetiosflags(std::ios::fixed);
    }
};

// Category-based timing statistics
class CategoryTimingStats {
private:
    static inline std::map<std::string, std::vector<double>> category_times;
    static inline std::string current_category;

public:
    static void set_current_category(const std::string& category) {
        current_category = category;
    }
    
    static void add_time(double time_ms) {
        if (!current_category.empty()) {
            category_times[current_category].push_back(time_ms);
        }
    }
    
    static void reset() {
        category_times.clear();
        current_category.clear();
    }
    
    static double get_category_average(const std::string& category) {
        auto it = category_times.find(category);
        if (it == category_times.end() || it->second.empty()) {
            return 0.0;
        }
        double total = 0.0;
        for (double time : it->second) {
            total += time;
        }
        return total / it->second.size();
    }
    
    static size_t get_category_count(const std::string& category) {
        auto it = category_times.find(category);
        return (it == category_times.end()) ? 0 : it->second.size();
    }
    
    static void print_category_summary(const std::string& category) {
        auto it = category_times.find(category);
        if (it != category_times.end() && !it->second.empty()) {
            std::cout << "[integration-test] Average: " 
                      << std::fixed << std::setprecision(2) 
                      << get_category_average(category) << " ms (" 
                      << it->second.size() << " measured tests)" 
                      << std::resetiosflags(std::ios::fixed) << std::endl;
        }
    }
};

// テスト結果の出力（クラス定義後）
inline void integration_test_passed(const std::string& test_name) {
    std::cout << "[integration-test] [PASS] " << test_name << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ")" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_error(const std::string& test_name) {
    std::cout << "[integration-test] [PASS] " << test_name << " (error expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_error(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ") (error expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_overflow(const std::string& test_name) {
    std::cout << "[integration-test] [PASS] " << test_name << " (overflow expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_overflow(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ") (overflow expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_failed(const std::string& test_name, const std::string& test_file, 
                                   const std::string& error_message) {
    std::cerr << "[integration-test] [FAIL] " << test_name << " (" << test_file << ")" << std::endl;
    std::cerr << "[integration-test] Error: " << error_message << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_failed();
}

// 時間測定機能付きのテスト結果出力関数（時間は表示しない）
inline void integration_test_passed_with_time(const std::string& test_name, const std::string& test_file, double execution_time_ms) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ")" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
    TimingStats::add_time(execution_time_ms);
    CategoryTimingStats::add_time(execution_time_ms);
}

inline void integration_test_passed_with_error_and_time(const std::string& test_name, const std::string& test_file, double execution_time_ms) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ") (error expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
    TimingStats::add_time(execution_time_ms);
    CategoryTimingStats::add_time(execution_time_ms);
}

inline void integration_test_passed_with_overflow_and_time(const std::string& test_name, const std::string& test_file, double execution_time_ms) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ") (overflow expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
    TimingStats::add_time(execution_time_ms);
    CategoryTimingStats::add_time(execution_time_ms);
}

// 簡略化されたヘルパー関数 - 時間測定を内部で処理
inline void run_cb_test_with_output_and_time_auto(const std::string& test_file, 
                                                   const std::function<void(const std::string&, int)>& validator) {
    double execution_time;
    run_cb_test_with_output_and_time(test_file, validator, execution_time);
    CategoryTimingStats::add_time(execution_time);
}

inline void integration_test_passed_with_time_auto(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ")" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_error_and_time_auto(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ") (expected error)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_overflow_and_time_auto(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration-test] [PASS] " << test_name << " (" << test_file << ") (overflow expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

// 1引数版のヘルパー関数（後方互換性）
inline void integration_test_passed_with_time_auto(const std::string& test_name) {
    std::cout << "[integration-test] [PASS] " << test_name << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_error_and_time_auto(const std::string& test_name) {
    std::cout << "[integration-test] [PASS] " << test_name << " (expected error)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}

inline void integration_test_passed_with_overflow_and_time_auto(const std::string& test_name) {
    std::cout << "[integration-test] [PASS] " << test_name << " (overflow expected)" << std::endl;
    IntegrationTestCounter::increment_total();
    IntegrationTestCounter::increment_passed();
}
