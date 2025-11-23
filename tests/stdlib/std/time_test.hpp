#pragma once
#include "../framework/stdlib_test_framework.hpp"
#include <iostream>
#include <string>
#include <regex>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <stdexcept>

namespace stdlib_tests {

// Cb プログラムを実行して出力を取得するヘルパー関数
inline std::string run_cb_program(const std::string& filepath) {
    // tests/stdlib ディレクトリから実行されることを想定
    std::string command = "cd ../.. && ./cb " + filepath + " 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("Failed to run command: " + command);
    }
    
    std::stringstream ss;
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        ss << buffer;
    }
    
    int status = pclose(pipe);
    if (status != 0) {
        throw std::runtime_error("Command failed with exit code: " + std::to_string(status));
    }
    
    return ss.str();
}

inline void register_time_tests(StdlibTestRunner& runner) {
    // Test: Time comprehensive test
    runner.add_test("Time comprehensive test", []() {
        std::string output = run_cb_program("tests/cases/stdlib/std/time_comprehensive_test.cb");
        
        // Check basic structure
        if (output.find("=== Async/Await/Sleep Comprehensive Test ===") == std::string::npos) {
            throw std::runtime_error("Missing test header");
        }
        if (output.find("=== All Tests Completed ===") == std::string::npos) {
            throw std::runtime_error("Missing completion message");
        }
        if (output.find("✅ now() function works correctly") == std::string::npos) {
            throw std::runtime_error("now() function test failed");
        }
        
        // Check all test sections exist
        std::vector<std::string> required_tests = {
            "[Test 1] Basic async function",
            "[Test 2] Await async function",
            "[Test 3] Sleep without await",
            "[Test 4] Await sleep",
            "[Test 5] Sequential execution with await",
            "[Test 6] Concurrent execution without await",
            "[Test 7] Sequential vs Concurrent comparison",
            "[Test 8] now() function test"
        };
        
        for (const auto& test : required_tests) {
            if (output.find(test) == std::string::npos) {
                throw std::runtime_error("Missing test section: " + test);
            }
        }
        
        // Check expected outputs
        std::vector<std::string> expected_outputs = {
            "Basic async function executed",
            "Test sleep start",
            "Concurrent1 start",
            "Concurrent2 start",
            "Delayed task completed"
        };
        
        for (const auto& expected : expected_outputs) {
            if (output.find(expected) == std::string::npos) {
                throw std::runtime_error("Missing expected output: " + expected);
            }
        }
    });
}

} // namespace stdlib_tests
