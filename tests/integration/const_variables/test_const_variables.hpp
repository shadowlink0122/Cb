#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_const_variables() {
    const std::string test_file_string_reassign = "../../tests/cases/const_variables/const_string_reassign_error.cb";
    const std::string test_file_string_element = "../../tests/cases/const_variables/const_string_element_error.cb";
    const std::string test_file_numeric_reassign = "../../tests/cases/const_variables/const_numeric_reassign_error.cb";
    const std::string test_file_read_ok = "../../tests/cases/const_variables/const_variables_read_ok.cb";
    
    // const string変数再代入エラーテスト
    run_cb_test_with_output_and_time_auto(test_file_string_reassign, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const string reassignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reassign const variable", "should contain const reassignment error message");
        });
    integration_test_passed_with_time_auto("const string reassign error test", test_file_string_reassign);

    // const string要素代入エラーテスト
    run_cb_test_with_output_and_time_auto(test_file_string_element, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const string element assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Const string element assignment error", "should contain const string element error message");
        });
    integration_test_passed_with_time_auto("const string element error test", test_file_string_element);

    // const数値変数再代入エラーテスト
    run_cb_test_with_output_and_time_auto(test_file_numeric_reassign, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const numeric reassignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Const reassignment error: num", "should contain const reassignment error message");
        });
    integration_test_passed_with_time_auto("const numeric reassign error test", test_file_numeric_reassign);

    // const変数読み取り正常テスト
    run_cb_test_with_output_and_time_auto(test_file_read_ok, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for const variables read test");
            INTEGRATION_ASSERT_CONTAINS(output, "42", "should contain int value");
            INTEGRATION_ASSERT_CONTAINS(output, "5", "should contain tiny value");
            INTEGRATION_ASSERT_CONTAINS(output, "1000", "should contain short value");
            INTEGRATION_ASSERT_CONTAINS(output, "1000000", "should contain long value");
            INTEGRATION_ASSERT_CONTAINS(output, "47", "should contain sum value");
            INTEGRATION_ASSERT_CONTAINS(output, "completed successfully", "should contain success message");
        });
    integration_test_passed_with_time_auto("const variables read ok test", test_file_read_ok);
}
