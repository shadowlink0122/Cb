#pragma once

#include "integration_test_framework.hpp"
#include <cstdlib>
#include <unistd.h>  // for getpid

// テスト実行モード
enum class TestMode {
    INTERPRETER,  // インタプリタモード (./cb run)
    COMPILER      // コンパイラモード (./cb compile)
};

// グローバル設定
namespace DualModeTest {
    static TestMode current_mode = TestMode::INTERPRETER;
    static std::string cb_binary = "../../cb";
    
    inline void set_test_mode(TestMode mode) {
        current_mode = mode;
    }
    
    inline TestMode get_test_mode() {
        return current_mode;
    }
    
    inline std::string get_mode_name() {
        return current_mode == TestMode::INTERPRETER ? "Interpreter" : "Compiler";
    }
}

// デュアルモード対応のテスト実行関数
inline int run_cb_test_dual_mode(const std::string& cb_file, std::string& output) {
    using namespace DualModeTest;
    
    if (current_mode == TestMode::INTERPRETER) {
        // インタプリタモード: ./cb run <file>
        std::string command = cb_binary + " run " + cb_file + " 2>&1";
        return run_command_and_capture(command, output);
    } else {
        // コンパイラモード: ./cb compile <file> -o /tmp/test_binary && /tmp/test_binary
        std::string temp_binary = "/tmp/cb_test_" + std::to_string(getpid()) + "_" + 
                                  std::to_string(rand());
        std::string compile_cmd = cb_binary + " compile " + cb_file + " -o " + temp_binary + " 2>&1";
        
        // コンパイル実行
        std::string compile_output;
        int compile_result = run_command_and_capture(compile_cmd, compile_output);
        
        if (compile_result != 0) {
            output = "Compilation failed:\n" + compile_output;
            return compile_result;
        }
        
        // 実行
        std::string run_cmd = temp_binary + " 2>&1";
        int run_result = run_command_and_capture(run_cmd, output);
        
        // クリーンアップ
        std::remove(temp_binary.c_str());
        
        return run_result;
    }
}

// 時間測定付きデュアルモードテスト実行
inline int run_cb_test_dual_mode_with_time(const std::string& cb_file, 
                                            std::string& output, 
                                            double& execution_time_ms) {
    auto start_time = std::chrono::high_resolution_clock::now();
    int result = run_cb_test_dual_mode(cb_file, output);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double, std::milli> duration = end_time - start_time;
    execution_time_ms = duration.count();
    
    return result;
}

// デュアルモード対応のrun_cb_test_with_output (既存コードとの互換性)
inline void run_cb_test_with_output_dual_mode(
    const std::string& cb_file,
    std::function<void(const std::string&, int)> checker) {
    
    std::string output;
    int exit_code = run_cb_test_dual_mode(cb_file, output);
    
    try {
        checker(output, exit_code);
    } catch (const std::exception& e) {
        std::cerr << "\n[integration-test] ❌ TEST FAILURE in file: " << cb_file << std::endl;
        std::cerr << "[integration-test] Mode: " << DualModeTest::get_mode_name() << std::endl;
        std::cerr << "[integration-test] Error: " << e.what() << std::endl;
        std::cerr << "[integration-test] Command exit code: " << exit_code << std::endl;
        std::cerr << "[integration-test] Output:\n" << output << std::endl;
        std::cerr << "[integration-test] --- End of output ---" << std::endl;
        throw;
    }
}

// 時間測定付きデュアルモード対応版
inline void run_cb_test_with_output_and_time_dual_mode(
    const std::string& cb_file,
    std::function<void(const std::string&, int)> checker,
    double& execution_time_ms) {
    
    std::string output;
    int exit_code = run_cb_test_dual_mode_with_time(cb_file, output, execution_time_ms);
    
    try {
        checker(output, exit_code);
    } catch (const std::exception& e) {
        std::cerr << "\n[integration-test] ❌ TEST FAILURE in file: " << cb_file << std::endl;
        std::cerr << "[integration-test] Mode: " << DualModeTest::get_mode_name() << std::endl;
        std::cerr << "[integration-test] Error: " << e.what() << std::endl;
        std::cerr << "[integration-test] Command exit code: " << exit_code << std::endl;
        std::cerr << "[integration-test] Execution time: " << execution_time_ms << " ms" << std::endl;
        std::cerr << "[integration-test] Output:\n" << output << std::endl;
        std::cerr << "[integration-test] --- End of output ---" << std::endl;
        throw;
    }
}

// デュアルモードテストランナー: 同じテストを両モードで実行
inline void run_dual_mode_test(
    const std::string& test_name,
    const std::string& cb_file,
    std::function<void(const std::string&, int)> checker) {
    
    std::cout << "\n[integration-test] === Testing: " << test_name << " ===" << std::endl;
    
    // インタプリタモードでテスト
    std::cout << "[integration-test] Mode: Interpreter" << std::endl;
    DualModeTest::set_test_mode(TestMode::INTERPRETER);
    double interpreter_time = 0.0;
    run_cb_test_with_output_and_time_dual_mode(cb_file, checker, interpreter_time);
    std::cout << "[integration-test] ✅ Interpreter passed (" 
              << std::fixed << std::setprecision(2) << interpreter_time << " ms)" << std::endl;
    
    // コンパイラモードでテスト
    std::cout << "[integration-test] Mode: Compiler" << std::endl;
    DualModeTest::set_test_mode(TestMode::COMPILER);
    double compiler_time = 0.0;
    run_cb_test_with_output_and_time_dual_mode(cb_file, checker, compiler_time);
    std::cout << "[integration-test] ✅ Compiler passed (" 
              << std::fixed << std::setprecision(2) << compiler_time << " ms)" << std::endl;
    
    // サマリー
    std::cout << "[integration-test] Summary: " << test_name << std::endl;
    std::cout << "[integration-test]   Interpreter: " << interpreter_time << " ms" << std::endl;
    std::cout << "[integration-test]   Compiler: " << compiler_time << " ms" << std::endl;
    std::cout << "[integration-test]   Speedup: " 
              << std::fixed << std::setprecision(2) 
              << (interpreter_time / compiler_time) << "x" << std::endl;
}
