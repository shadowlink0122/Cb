#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <sstream>

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

// テンポラリファイルを使用したテスト実行
inline void run_cb_test_with_output(const std::string& test_file, 
                                   const std::function<void(const std::string&, int)>& validator) {
    std::string command = "../../main " + test_file + " 2>&1";
    std::string output;
    int exit_code = run_command_and_capture(command, output);
    
    try {
        validator(output, exit_code);
    } catch (const std::exception& e) {
        std::cerr << "[integration] TEST FAILURE in file: " << test_file << std::endl;
        std::cerr << "[integration] Error: " << e.what() << std::endl;
        std::cerr << "[integration] Command: " << command << std::endl;
        std::cerr << "[integration] Exit code: " << exit_code << std::endl;
        std::cerr << "[integration] Output:" << std::endl;
        std::cerr << output << std::endl;
        std::cerr << "[integration] --- End of output ---" << std::endl;
        throw;
    }
}

// 基本的なアサーション（ファイル名と行数付き）
#define INTEGRATION_ASSERT(condition, message) \
    do { \
        IntegrationTestCounter::increment_total(); \
        if (!(condition)) { \
            std::cerr << "[integration] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration] " << message << std::endl; \
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
            std::cerr << "[integration] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration] Expected: " << (expected) << std::endl; \
            std::cerr << "[integration] Actual: " << (actual) << std::endl; \
            std::cerr << "[integration] " << message << std::endl; \
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
            std::cerr << "[integration] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration] Expected NOT: " << (not_expected) << std::endl; \
            std::cerr << "[integration] Actual: " << (actual) << std::endl; \
            std::cerr << "[integration] " << message << std::endl; \
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
            std::cerr << "[integration] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration] Expected to find: " << (needle) << std::endl; \
            std::cerr << "[integration] In output: " << (haystack) << std::endl; \
            std::cerr << "[integration] " << message << std::endl; \
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
            std::cerr << "[integration] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[integration] Did not expect to find: " << (needle) << std::endl; \
            std::cerr << "[integration] In output: " << (haystack) << std::endl; \
            std::cerr << "[integration] " << message << std::endl; \
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
