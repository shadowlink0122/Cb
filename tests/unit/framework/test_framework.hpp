#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <cassert>
#include <sstream>

// 単体テスト用のアサーションマクロ
#define UNIT_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "[unit] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[unit] " << message << std::endl; \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } \
    } while(0)

#define UNIT_ASSERT_EQ(expected, actual, message) \
    do { \
        if (!((expected) == (actual))) { \
            std::cerr << "[unit] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[unit] Expected: " << (expected) << std::endl; \
            std::cerr << "[unit] Actual: " << (actual) << std::endl; \
            std::cerr << "[unit] " << message << std::endl; \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } \
    } while(0)

#define UNIT_ASSERT_NE(not_expected, actual, message) \
    do { \
        if ((not_expected) == (actual)) { \
            std::cerr << "[unit] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[unit] Not expected: " << (not_expected) << std::endl; \
            std::cerr << "[unit] Actual: " << (actual) << std::endl; \
            std::cerr << "[unit] " << message << std::endl; \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } \
    } while(0)

#define UNIT_ASSERT_NULL(ptr, message) \
    do { \
        if ((ptr) != nullptr) { \
            std::cerr << "[unit] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[unit] Expected null pointer, got: " << (ptr) << std::endl; \
            std::cerr << "[unit] " << message << std::endl; \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } \
    } while(0)

#define UNIT_ASSERT_NOT_NULL(ptr, message) \
    do { \
        if ((ptr) == nullptr) { \
            std::cerr << "[unit] ASSERTION FAILED at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << "[unit] Expected non-null pointer, got null" << std::endl; \
            std::cerr << "[unit] " << message << std::endl; \
            throw std::runtime_error(std::string(__FILE__) + ":" + std::to_string(__LINE__) + " - " + message); \
        } \
    } while(0)

// 後方互換性のためのマクロ
#define ASSERT_STREQ(expected, actual) \
    UNIT_ASSERT_EQ(std::string(expected), std::string(actual), "String comparison failed")

#define ASSERT_EQ(expected, actual) \
    UNIT_ASSERT_EQ(expected, actual, "Value comparison failed")

#define ASSERT_NE(not_expected, actual) \
    UNIT_ASSERT_NE(not_expected, actual, "Value should not be equal")

#define ASSERT_TRUE(condition) \
    UNIT_ASSERT(condition, "Expected true condition")

#define ASSERT_FALSE(condition) \
    UNIT_ASSERT(!(condition), "Expected false condition")

#define ASSERT_NULL(ptr) \
    UNIT_ASSERT_NULL(ptr, "Expected null pointer")

#define ASSERT_NOT_NULL(ptr) \
    UNIT_ASSERT_NOT_NULL(ptr, "Expected non-null pointer")

// テスト実行フレームワーク
class UnitTestFramework {
private:
    int passed_tests = 0;
    int failed_tests = 0;
    
public:
    void run_test(const std::string& test_name, std::function<void()> test_func) {
        try {
            test_func();
            std::cout << "[unit] " << test_name << " ... passed" << std::endl;
            passed_tests++;
        } catch (const std::exception& e) {
            std::cout << "[unit] " << test_name << " ... failed" << std::endl;
            std::cerr << "[unit] Error: " << e.what() << std::endl;
            failed_tests++;
        }
    }
    
    void print_results() {
        std::cout << "[unit] Results: " << passed_tests << " passed, " << failed_tests << " failed" << std::endl;
    }
    
    int get_failed_count() const {
        return failed_tests;
    }
};

// グローバルインスタンス
extern UnitTestFramework* g_test_framework;

// テスト登録マクロ
#define RUN_TEST(test_name, test_func) \
    g_test_framework->run_test(test_name, test_func)

// ユーティリティ関数
inline bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

// テスト用のヘルパー関数
inline void unit_test_passed(const std::string& test_name) {
    // プライベート関数として使用される
}

inline void unit_test_failed(const std::string& test_name, const std::string& error) {
    throw std::runtime_error("Test failed: " + test_name + " - " + error);
}
