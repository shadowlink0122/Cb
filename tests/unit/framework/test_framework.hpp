#ifndef TEST_FRAMEWORK_HPP
#define TEST_FRAMEWORK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>

// テスト結果の統計
struct TestStats {
    int passed = 0;
    int failed = 0;
    int total() const { return passed + failed; }
};

// テストケースの定義
struct TestCase {
    std::string name;
    std::function<void()> test_func;
};

// テストランナークラス
class TestRunner {
private:
    std::vector<TestCase> test_cases;
    TestStats stats;
    
public:
    void add_test(const std::string& name, std::function<void()> test_func) {
        test_cases.push_back({name, test_func});
    }
    
    void run_all() {
        std::cout << "Starting unit tests..." << std::endl;
        std::cout << "======================" << std::endl;
        
        for (const auto& test_case : test_cases) {
            std::cout << "Running: " << test_case.name << " ... ";
            try {
                test_case.test_func();
                std::cout << "PASSED" << std::endl;
                stats.passed++;
            } catch (const std::exception& e) {
                std::cout << "FAILED" << std::endl;
                std::cout << "  Error: " << e.what() << std::endl;
                stats.failed++;
            }
        }
        
        std::cout << "======================" << std::endl;
        std::cout << "Test Results:" << std::endl;
        std::cout << "  Total:  " << stats.total() << std::endl;
        std::cout << "  Passed: " << stats.passed << std::endl;
        std::cout << "  Failed: " << stats.failed << std::endl;
        
        if (stats.failed > 0) {
            std::cout << "\nSome tests failed!" << std::endl;
        } else {
            std::cout << "\nAll tests passed!" << std::endl;
        }
    }
    
    bool all_passed() const { return stats.failed == 0; }
};

// グローバルテストランナーインスタンス
extern TestRunner test_runner;

// テストマクロの定義
#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            throw std::runtime_error("Assertion failed: expected " + std::to_string(expected) + " but got " + std::to_string(actual)); \
        } \
    } while(0)

#define ASSERT_STREQ(expected, actual) \
    do { \
        std::string exp_str = (expected); \
        std::string act_str = (actual); \
        if (exp_str != act_str) { \
            throw std::runtime_error("Assertion failed: expected \"" + exp_str + "\" but got \"" + act_str + "\""); \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            throw std::runtime_error("Assertion failed: expected non-null pointer but got nullptr"); \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            throw std::runtime_error("Assertion failed: expected true but got false"); \
        } \
    } while(0)

#define ASSERT_FALSE(condition) \
    do { \
        if (condition) { \
            throw std::runtime_error("Assertion failed: expected false but got true"); \
        } \
    } while(0)

#define RUN_TEST(name, test_func) \
    test_runner.add_test(name, test_func)

#endif // TEST_FRAMEWORK_HPP
