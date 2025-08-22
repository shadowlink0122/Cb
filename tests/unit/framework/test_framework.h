#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <sstream>
#include "../../../src/frontend/debug.h"

// 簡単なテストフレームワーク
class TestRunner {
private:
    struct TestCase {
        std::string name;
        std::function<void()> test_func;
        std::string category;
        bool expect_failure = false;  // 予想された失敗かどうか
    };
    
    std::vector<TestCase> tests;
    int passed = 0;
    int failed = 0;
    int expected_failures = 0;
    bool original_debug_mode;
    
public:
    TestRunner() {
        // テスト実行開始時にデバッグモードを保存して無効化
        original_debug_mode = debug_mode;
        debug_mode = false;
    }
    
    ~TestRunner() {
        // テスト実行終了時にデバッグモードを復元
        debug_mode = original_debug_mode;
    }
    
    void add_test(const std::string& category, const std::string& name, std::function<void()> test_func, bool expect_failure = false) {
        tests.push_back({name, test_func, category, expect_failure});
    }
    
    void run_all() {
        std::cout << "Running " << tests.size() << " tests...\n" << std::endl;
        
        // テスト全体でデバッグモードを無効化
        bool saved_debug_mode = debug_mode;
        debug_mode = false;
        
        for (const auto& test : tests) {
            std::cout << "[unit] " << test.name << " ... ";
            std::cout.flush();
            
            try {
                test.test_func();
                if (test.expect_failure) {
                    std::cout << "failed (expected failure but test passed)" << std::endl;
                    failed++;
                } else {
                    std::cout << "passed" << std::endl;
                    passed++;
                }
            } catch (const std::exception& e) {
                if (test.expect_failure) {
                    std::cout << "failed (expected: " << e.what() << ")" << std::endl;
                    expected_failures++;
                } else {
                    std::cout << "failed (" << e.what() << ")" << std::endl;
                    failed++;
                }
            } catch (...) {
                if (test.expect_failure) {
                    std::cout << "failed (expected: unknown exception)" << std::endl;
                    expected_failures++;
                } else {
                    std::cout << "failed (unknown exception)" << std::endl;
                    failed++;
                }
            }
        }
        
        // デバッグモードを復元
        debug_mode = saved_debug_mode;
        
        std::cout << "[unit] Results: " << passed << " passed, " << failed << " failed";
        if (expected_failures > 0) {
            std::cout << ", " << expected_failures << " expected failures";
        }
        std::cout << std::endl;
        
        // 予想外の失敗のみでエラー終了
        if (failed > 0) {
            exit(1);
        }
    }
};

// アサーション関数
#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            std::ostringstream oss; \
            oss << "Expected " << (expected) << " but got " << (actual); \
            throw std::runtime_error(oss.str()); \
        } \
    } while(0)

#define ASSERT_STREQ(expected, actual) \
    do { \
        if (std::string(expected) != std::string(actual)) { \
            throw std::runtime_error("Expected \"" + std::string(expected) + \
                                   "\" but got \"" + std::string(actual) + "\""); \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Expected true but got false"); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("Expected false but got true"); \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            throw std::runtime_error("Expected non-null pointer"); \
        } \
    } while(0)

// テストランナーのグローバルインスタンス
extern TestRunner test_runner;
