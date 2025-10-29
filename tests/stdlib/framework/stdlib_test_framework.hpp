#ifndef STDLIB_TEST_FRAMEWORK_HPP
#define STDLIB_TEST_FRAMEWORK_HPP

#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstdlib>
#include <sstream>
#include <array>

// テスト結果の統計
struct StdlibTestStats {
    int passed = 0;
    int failed = 0;
    int total() const { return passed + failed; }
};

// テストケースの定義
struct StdlibTestCase {
    std::string name;
    std::function<void()> test_func;
};

// テストランナークラス
class StdlibTestRunner {
private:
    std::vector<StdlibTestCase> test_cases;
    StdlibTestStats stats;
    
public:
    void add_test(const std::string& name, std::function<void()> test_func) {
        test_cases.push_back({name, test_func});
    }
    
    void run_all() {
        std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
        std::cout << "║     Cb Standard Library Tests             ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════╝\n" << std::endl;
        
        for (const auto& test_case : test_cases) {
            std::cout << "Running: " << test_case.name << " ... ";
            try {
                test_case.test_func();
                std::cout << "✅ PASSED" << std::endl;
                stats.passed++;
            } catch (const std::exception& e) {
                std::cout << "❌ FAILED" << std::endl;
                std::cout << "  Error: " << e.what() << std::endl;
                stats.failed++;
            }
        }
        
        print_summary();
    }
    
    void print_summary() const {
        std::cout << "\n╔════════════════════════════════════════════╗" << std::endl;
        std::cout << "║           Test Summary                     ║" << std::endl;
        std::cout << "╠════════════════════════════════════════════╣" << std::endl;
        std::cout << "║  Total:  " << stats.total() << std::endl;
        std::cout << "║  Passed: " << stats.passed << std::endl;
        std::cout << "║  Failed: " << stats.failed << std::endl;
        std::cout << "╚════════════════════════════════════════════╝" << std::endl;
        
        if (stats.failed == 0) {
            std::cout << "\n✅ All stdlib tests passed!" << std::endl;
        } else {
            std::cout << "\n❌ Some stdlib tests failed!" << std::endl;
        }
    }
    
    bool all_passed() const {
        return stats.failed == 0;
    }
};

// アサーションマクロ
#define STDLIB_ASSERT_TRUE(expr) \
    if (!(expr)) { \
        throw std::runtime_error("Assertion failed: " #expr); \
    }

#define STDLIB_ASSERT_FALSE(expr) \
    if (expr) { \
        throw std::runtime_error("Assertion failed: !(" #expr ")"); \
    }

#define STDLIB_ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    }

#define STDLIB_ASSERT_NEQ(a, b) \
    if ((a) == (b)) { \
        throw std::runtime_error("Assertion failed: " #a " != " #b); \
    }

#define STDLIB_ASSERT_CONTAINS(output, substring) \
    if ((output).find(substring) == std::string::npos) { \
        throw std::runtime_error(std::string("Assertion failed: output does not contain \"") + substring + "\""); \
    }

// ヘルパー関数: Cbテストファイルを実行して出力とexit codeを取得
inline std::pair<std::string, int> run_cb_test(const std::string& test_file) {
    std::string command = "../../main " + test_file + " 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return {"", -1};
    }
    
    std::stringstream output;
    std::array<char, 128> buffer;
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output << buffer.data();
    }
    
    int exit_code = pclose(pipe);
    // WEXITSTATUS extracts the actual exit code from the status returned by pclose
    return {output.str(), WEXITSTATUS(exit_code)};
}

#endif // STDLIB_TEST_FRAMEWORK_HPP
