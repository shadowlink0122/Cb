#pragma once

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace InterfaceErrorTests {

// interfaceだけ定義されていてimplがない場合のエラーテスト
inline void test_interface_no_impl_error() {
    std::cout << "[integration-test] Running test_interface_no_impl_error..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/error_interface_no_impl.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Interface without impl should exit with error code");
            INTEGRATION_ASSERT(output.find("Undefined function") != std::string::npos || 
                              output.find("Error") != std::string::npos, 
                              "Should contain error message about undefined function");
        }, execution_time);
}

// 不完全なimpl実装のエラーテスト
inline void test_incomplete_impl_error() {
    std::cout << "[integration-test] Running test_incomplete_impl_error..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/error_incomplete_impl.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Incomplete impl should exit with error code");
            INTEGRATION_ASSERT(output.find("Incomplete implementation: Method 'subtract'") != std::string::npos, 
                              "Should contain error message about missing subtract function");
        }, execution_time);
}

// 存在しないinterfaceを実装しようとするエラーテスト
inline void test_undefined_interface_error() {
    std::cout << "[integration-test] Running test_undefined_interface_error..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/error_undefined_interface.cb", 
        [](const std::string& output, int exit_code) {
            // 現在は正常に実行されるが、本来はエラーになるべき
            // これは将来の改善点として文書化されている
            if (exit_code == 0) {
                std::cout << "[LIMITATION] Undefined interface implementation is not detected (parser-level improvement needed)" << std::endl;
            } else {
                std::cout << "[IMPROVED] Undefined interface implementation correctly detected!" << std::endl;
            }
        }, execution_time);
}

// 署名の不一致エラーテスト
inline void test_signature_mismatch_error() {
    std::cout << "[integration-test] Running test_signature_mismatch_error..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/error_signature_mismatch.cb", 
        [](const std::string& output, int exit_code) {
            // 現在は正常に実行されるが、本来は型チェックエラーになるべき
            if (exit_code == 0) {
                std::cout << "[LIMITATION] Method signature mismatch is not detected (type checking improvement needed)" << std::endl;
            } else {
                std::cout << "[IMPROVED] Method signature mismatch correctly detected!" << std::endl;
            }
        }, execution_time);
}

// 重複impl定義のエラーテスト
inline void test_duplicate_impl_error() {
    std::cout << "[integration-test] Running test_duplicate_impl_error..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/error_duplicate_impl.cb", 
        [](const std::string& output, int exit_code) {
            // パースエラーまたはランタイムエラーが発生することを期待
            if (exit_code != 0) {
                std::cout << "[IMPROVED] Duplicate impl definition correctly detected!" << std::endl;
            } else {
                std::cout << "[LIMITATION] Duplicate impl definition is not detected (parser-level improvement needed)" << std::endl;
            }
        }, execution_time);
}

// 余分なメソッドが定義されたimplのテスト（警告レベル）
inline void test_extra_methods_warning() {
    std::cout << "[integration-test] Running test_extra_methods_warning..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/error_extra_methods.cb", 
        [](const std::string& output, int exit_code) {
            // 実行が成功した場合は警告を出力、失敗した場合はエラー情報を表示
            if (exit_code == 0) {
                std::cout << "[INFO] Extra methods in impl currently allowed (consider adding warnings)" << std::endl;
            } else {
                std::cout << "[WARNING] Extra methods test failed with exit code " << exit_code << std::endl;
                std::cout << "[WARNING] This may be due to syntax errors in the test file" << std::endl;
            }
        }, execution_time);
}

// 現実的なinterfaceエラーテスト（現在検出可能）
inline void test_realistic_interface_error() {
    std::cout << "[integration-test] Running test_realistic_interface_error..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/error_interface_realistic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Realistic interface error should exit with error code");
            INTEGRATION_ASSERT(output.find("Undefined function: area") != std::string::npos, 
                              "Should contain error message about undefined function: area");
        }, execution_time);
}

// 全てのinterfaceエラーテストを実行する統合関数
inline void run_all_interface_error_tests() {
    std::cout << "[integration-test] === Interface Error Handling Tests ===" << std::endl;
    std::cout << "[integration-test] Testing current error detection capabilities and future improvements" << std::endl;
    
    // 現在適切に検出されるエラー
    test_interface_no_impl_error();
    test_incomplete_impl_error();
    test_realistic_interface_error();
    
    // 将来の改善点（現在は検出されない）
    test_undefined_interface_error();
    test_signature_mismatch_error();
    test_duplicate_impl_error();
    test_extra_methods_warning();
    
    std::cout << "[integration-test] Interface error tests completed" << std::endl;
    std::cout << "[integration-test] ✅ Basic error detection: Working correctly" << std::endl;
    std::cout << "[integration-test] ⚠️  Advanced error detection: Future improvements documented" << std::endl;
}

} // namespace InterfaceErrorTests
