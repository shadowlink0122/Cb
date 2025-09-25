#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_const_array() {
    const std::string test_file_assign_error = "../../tests/cases/const_array/const_array_assign_error.cb";
    const std::string test_file_string_assign_error = "../../tests/cases/const_array/const_string_array_assign_error.cb";
    const std::string test_file_read_ok = "../../tests/cases/const_array/const_array_read_ok.cb";
    const std::string test_file_multidim_error = "../../tests/cases/const_array/const_multidim_array_error.cb";
    
    // const配列要素への代入エラーテスト
    run_cb_test_with_output(test_file_assign_error, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const array assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Assignment to const array", "should contain const array error message");
        });
    integration_test_passed("const array assign error test", test_file_assign_error);
    
    // const文字列配列要素への代入エラーテスト
    run_cb_test_with_output(test_file_string_assign_error, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const string array assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Assignment to const array", "should contain const array error message");
        });
    integration_test_passed("const string array assign error test", test_file_string_assign_error);
    
    // const配列読み取り正常テスト
    run_cb_test_with_output(test_file_read_ok, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for const array read test");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "should contain first element value");
            INTEGRATION_ASSERT_CONTAINS(output, "200", "should contain second element value");
            INTEGRATION_ASSERT_CONTAINS(output, "300", "should contain third element value");
            INTEGRATION_ASSERT_CONTAINS(output, "400", "should contain fourth element value");
            INTEGRATION_ASSERT_CONTAINS(output, "completed successfully", "should contain success message");
        });
    integration_test_passed("const array read ok test", test_file_read_ok);
    
    // const多次元配列要素への代入エラーテスト
    run_cb_test_with_output(test_file_multidim_error, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const multidimensional array assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot assign to const multidimensional array", "should contain const multidimensional array error message");
        });
    integration_test_passed("const multidim array error test", test_file_multidim_error);
}
