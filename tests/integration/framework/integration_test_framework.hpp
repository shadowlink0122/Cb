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
    validator(output, exit_code);
}

// 基本的なアサーション
#define INTEGRATION_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "[integration] ASSERTION FAILED: " << message << std::endl; \
            throw std::runtime_error(message); \
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

inline void integration_test_passed_with_error(const std::string& test_name) {
    std::cout << "[integration] " << test_name << " ... passed (error detected)" << std::endl;
}

inline void integration_test_passed_with_overflow(const std::string& test_name) {
    std::cout << "[integration] " << test_name << " ... passed (overflow to negative)" << std::endl;
}
