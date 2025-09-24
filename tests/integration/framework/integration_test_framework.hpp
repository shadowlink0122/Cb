#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cstdlib>
#include <fstream>
#include <cassert>

// テスト用のユーティリティ関数
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

// テスト結果の出力
inline void integration_test_passed(const std::string& test_name) {
    std::cout << "[integration] " << test_name << " ... passed" << std::endl;
}

inline void integration_test_passed(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration] " << test_name << " (" << test_file << ") ... passed" << std::endl;
}

inline void integration_test_passed_with_error(const std::string& test_name) {
    std::cout << "[integration] " << test_name << " ... passed (error detected)" << std::endl;
}

inline void integration_test_passed_with_error(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration] " << test_name << " (" << test_file << ") ... passed (error detected)" << std::endl;
}

inline void integration_test_passed_with_overflow(const std::string& test_name) {
    std::cout << "[integration] " << test_name << " ... passed (overflow to negative)" << std::endl;
}

inline void integration_test_passed_with_overflow(const std::string& test_name, const std::string& test_file) {
    std::cout << "[integration] " << test_name << " (" << test_file << ") ... passed (overflow to negative)" << std::endl;
}

inline void integration_test_failed(const std::string& test_name, const std::string& test_file, 
                                   const std::string& error_message) {
    std::cerr << "[integration] " << test_name << " (" << test_file << ") ... FAILED" << std::endl;
    std::cerr << "[integration] Error: " << error_message << std::endl;
}

// Test counter for integration tests
class IntegrationTestCounter {
private:
    static int total_tests;
    static int passed_tests;
    static int failed_tests;

public:
    static void increment_total() { total_tests++; }
    static void increment_passed() { passed_tests++; }
    static void increment_failed() { failed_tests++; }
    static void reset() { total_tests = 0; passed_tests = 0; failed_tests = 0; }
    
    static int get_total() { return total_tests; }
    static int get_passed() { return passed_tests; }
    static int get_failed() { return failed_tests; }
    
    static void print_summary() {
        std::cout << "=== Integration Test Summary ===" << std::endl;
        std::cout << "  Total:  " << total_tests << std::endl;
        std::cout << "  Passed: " << passed_tests << std::endl;
        std::cout << "  Failed: " << failed_tests << std::endl;
        std::cout << std::endl;
    }
};

// Initialize static members
int IntegrationTestCounter::total_tests = 0;
int IntegrationTestCounter::passed_tests = 0;
int IntegrationTestCounter::failed_tests = 0;
