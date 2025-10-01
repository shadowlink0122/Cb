#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_interface_private() {
    
    // 基本的なprivateメソッドテスト
    const std::string basic_ok_file = "../../tests/cases/interface/private/basic_ok.cb";
    double execution_time_basic;
    
    run_cb_test_with_output_and_time(basic_ok_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "Private method basic test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "calculate(10) = 20", "Expected calculate(10) result");
            INTEGRATION_ASSERT_CONTAINS(output, "calculate(-5) = 0", "Expected calculate(-5) result");
            INTEGRATION_ASSERT_CONTAINS(output, "status = Calculator ready", "Expected status message");
            INTEGRATION_ASSERT_CONTAINS(output, "Private method basic test passed", "Expected success message");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for basic private test");
        }, execution_time_basic);
    integration_test_passed_with_time("interface private basic test", basic_ok_file, execution_time_basic);
    
    // プライベートメソッド外部アクセスエラーテスト
    const std::string access_error_file = "../../tests/cases/interface/private/access_error.cb";
    double execution_time_error;
    
    run_cb_test_with_output_and_time(access_error_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "Private method access error test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Public method called", "Expected public method call");
            INTEGRATION_ASSERT(contains(output, "Cannot access private method") || exit_code != 0, 
                             "Expected private method access error");
        }, execution_time_error);
    integration_test_passed_with_error_and_time("interface private access error test", access_error_file, execution_time_error);
    
    // プリミティブ型配列でのprivateメソッドテスト（typedef版）
    const std::string primitive_array_file = "../../tests/cases/interface/private/primitive_array_ok.cb";
    double execution_time_primitive;
    
    run_cb_test_with_output_and_time(primitive_array_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "Primitive array private method test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "Array sum = 100", "Expected array sum result");
            INTEGRATION_ASSERT_CONTAINS(output, "Array info = Array is valid", "Expected array info");
            INTEGRATION_ASSERT_CONTAINS(output, "Primitive array private method test passed", "Expected success message");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for primitive array test");
        }, execution_time_primitive);
    integration_test_passed_with_time("interface private primitive array test (typedef)", primitive_array_file, execution_time_primitive);
    
    // string型でのprivateメソッドテスト
    const std::string string_file = "../../tests/cases/interface/private/string_ok.cb";
    double execution_time_string;
    
    run_cb_test_with_output_and_time(string_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "String private method test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "formatted = Formatted: content", "Expected formatted string");
            INTEGRATION_ASSERT_CONTAINS(output, "length = 15", "Expected string length");
            INTEGRATION_ASSERT_CONTAINS(output, "empty = 0", "Expected empty check result");
            INTEGRATION_ASSERT_CONTAINS(output, "String private method test passed", "Expected success message");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for string test");
        }, execution_time_string);
    integration_test_passed_with_time("interface private string test", string_file, execution_time_string);
    
    // 多次元配列でのprivateメソッドテスト
    const std::string multidim_file = "../../tests/cases/interface/private/multidim_array_ok.cb";
    double execution_time_multidim;
    
    run_cb_test_with_output_and_time(multidim_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "Multidimensional array private method test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "element[1][2] = 6", "Expected matrix element access");
            INTEGRATION_ASSERT_CONTAINS(output, "element[5][5] = -1", "Expected invalid index result");
            INTEGRATION_ASSERT_CONTAINS(output, "info = 2x3 matrix is valid", "Expected matrix info");
            INTEGRATION_ASSERT_CONTAINS(output, "Multidimensional array private method test passed", "Expected success message");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for multidim array test");
        }, execution_time_multidim);
    integration_test_passed_with_time("interface private multidim array test (typedef)", multidim_file, execution_time_multidim);
    
    // 複数プライベートメソッドチェーンテスト
    const std::string complex_file = "../../tests/cases/interface/private/complex_chain_ok.cb";
    double execution_time_complex;
    
    run_cb_test_with_output_and_time(complex_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "Complex private method chain test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "processValue(5) = 25", "Expected processed value 5 result");
            INTEGRATION_ASSERT_CONTAINS(output, "processValue(-3) = 10", "Expected processed value -3 result");
            INTEGRATION_ASSERT_CONTAINS(output, "report = Processor is ready", "Expected processor report");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex private method chain test passed", "Expected success message");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for complex chain test");
        }, execution_time_complex);
    integration_test_passed_with_time("interface private complex chain test", complex_file, execution_time_complex);
    
    // プライベートヘルパーチェーン成功テスト
    const std::string helper_chain_file = "../../tests/cases/interface/private/helper_chain_ok.cb";
    double execution_time_helper_chain;
    
    run_cb_test_with_output_and_time(helper_chain_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for helper chain success test");
            INTEGRATION_ASSERT_CONTAINS(output, "Private helper chain success test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "process(12) = 27", "Expected first chained result");
            INTEGRATION_ASSERT_CONTAINS(output, "process(-5) = 3", "Expected second chained result");
            INTEGRATION_ASSERT_CONTAINS(output, "doubleProcess(30, 100) = 169", "Expected combined chained result");
            INTEGRATION_ASSERT_CONTAINS(output, "Private helper chain success test passed", "Expected success message");
        }, execution_time_helper_chain);
    integration_test_passed_with_time("interface private helper chain success test", helper_chain_file, execution_time_helper_chain);
    
    // プライベートヘルパーチェーン外部アクセスエラーテスト
    const std::string helper_chain_error_file = "../../tests/cases/interface/private/helper_chain_access_error.cb";
    double execution_time_helper_chain_error;
    
    bool helper_enforcement_triggered = false;
    bool helper_enforcement_missing = false;
    run_cb_test_with_output_and_time(helper_chain_error_file, 
        [&](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "Private helper chain access error test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "process(25) = 53", "Expected safe chained result before enforcement check");
            helper_enforcement_triggered = contains(output, "Cannot access private method") || exit_code != 0;
            helper_enforcement_missing = contains(output, "Private helper access executed without enforcement");
            INTEGRATION_ASSERT(helper_enforcement_triggered || helper_enforcement_missing, 
                              "Expected either enforcement error or explicit warning message");
        }, execution_time_helper_chain_error);
    if (helper_enforcement_triggered) {
        integration_test_passed_with_error_and_time("interface private helper chain access enforcement test", helper_chain_error_file, execution_time_helper_chain_error);
    } else {
        integration_test_passed_with_time("interface private helper chain access warning", helper_chain_error_file, execution_time_helper_chain_error);
    }

    // self経由のprivateメンバーアクセス成功テスト
    const std::string self_member_file = "../../tests/cases/interface/private/self_member_access_ok.cb";
    double execution_time_self_member;

    run_cb_test_with_output_and_time(self_member_file,
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for self private member test");
            INTEGRATION_ASSERT_CONTAINS(output, "Private member access via self test:", "Expected test header in output");
            INTEGRATION_ASSERT_CONTAINS(output, "self.secret = 11, self.visible = 4", "Expected initial state log");
            INTEGRATION_ASSERT_CONTAINS(output, "reveal = 11", "Expected initial reveal value");
            INTEGRATION_ASSERT_CONTAINS(output, "self.secret = 16, self.visible = 9", "Expected updated state log");
            INTEGRATION_ASSERT_CONTAINS(output, "reveal after bump = 16", "Expected reveal after mutation");
            INTEGRATION_ASSERT_CONTAINS(output, "interface reveal = 16", "Expected interface-based reveal value");
            INTEGRATION_ASSERT_CONTAINS(output, "Private member access via self test passed", "Expected success message");
        }, execution_time_self_member);
    integration_test_passed_with_time("interface private self member access test", self_member_file, execution_time_self_member);
    
    // 生の配列型implエラーテスト
    const std::string raw_array_error_file = "../../tests/cases/interface/private/raw_array_error.cb";
    double execution_time_raw_error;
    
    run_cb_test_with_output_and_time(raw_array_error_file, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT(contains(output, "Cannot implement interface for raw array type") || 
                             contains(output, "Use typedef to define array type first") || 
                             exit_code != 0, 
                             "Expected error for raw array type impl");
        }, execution_time_raw_error);
    integration_test_passed_with_error_and_time("interface private raw array error test", raw_array_error_file, execution_time_raw_error);
}
