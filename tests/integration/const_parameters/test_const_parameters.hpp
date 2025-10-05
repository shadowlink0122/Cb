#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_const_parameters() {
    const std::string test_file_read_ok = "../../tests/cases/const_parameters/const_param_read_ok.cb";
    const std::string test_file_all_types = "../../tests/cases/const_parameters/const_all_types_ok.cb";
    const std::string test_file_mixed = "../../tests/cases/const_parameters/const_mixed_params_ok.cb";
    const std::string test_file_reassign_error = "../../tests/cases/const_parameters/const_param_reassign_error.cb";
    const std::string test_file_compound_error = "../../tests/cases/const_parameters/const_param_compound_error.cb";
    const std::string test_file_array_error = "../../tests/cases/const_parameters/const_array_param_error.cb";
    
    // const引数読み取り正常テスト
    run_cb_test_with_output_and_time_auto(test_file_read_ok, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "square(5) =  25", "should contain square result");
            INTEGRATION_ASSERT_CONTAINS(output, "add(10, 20) =  30", "should contain add result");
            INTEGRATION_ASSERT_CONTAINS(output, "sum_array([1,2,3,4,5], 5) =  15", "should contain sum result");
            INTEGRATION_ASSERT_CONTAINS(output, "All const parameter read tests passed!", "should contain success message");
        });
    integration_test_passed_with_time_auto("const param read ok", test_file_read_ok);

    // const引数全型テスト
    run_cb_test_with_output_and_time_auto(test_file_all_types, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "test_tiny(10) =  20", "should contain tiny result");
            INTEGRATION_ASSERT_CONTAINS(output, "test_short(100) =  200", "should contain short result");
            INTEGRATION_ASSERT_CONTAINS(output, "test_int(1000) =  2000", "should contain int result");
            INTEGRATION_ASSERT_CONTAINS(output, "test_long(10000) =  20000", "should contain long result");
            INTEGRATION_ASSERT_CONTAINS(output, "All type const parameter tests passed!", "should contain success message");
        });
    integration_test_passed_with_time_auto("const all types ok", test_file_all_types);

    // const/non-const混在テスト
    run_cb_test_with_output_and_time_auto(test_file_mixed, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code");
            INTEGRATION_ASSERT_CONTAINS(output, "mixed_params(10, 20, 30) =  80", "should contain mixed result");
            INTEGRATION_ASSERT_CONTAINS(output, "modify_non_const(5, 10) =  115", "should contain modify result");
            INTEGRATION_ASSERT_CONTAINS(output, "All mixed parameter tests passed!", "should contain success message");
        });
    integration_test_passed_with_time_auto("const mixed params ok", test_file_mixed);

    // const引数への代入エラーテスト
    run_cb_test_with_output_and_time_auto(test_file_reassign_error, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const param reassignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reassign const variable: x", "should contain const reassignment error message");
        });
    integration_test_passed_with_time_auto("const param reassign error", test_file_reassign_error);

    // const引数への複合代入エラーテスト
    run_cb_test_with_output_and_time_auto(test_file_compound_error, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const param compound assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot reassign const variable: x", "should contain const reassignment error message");
        });
    integration_test_passed_with_time_auto("const param compound error", test_file_compound_error);

    // const配列引数要素への代入エラーテスト
    run_cb_test_with_output_and_time_auto(test_file_array_error, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for const array param element assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "Cannot assign to const variable: arr", "should contain const array error message");
        });
    integration_test_passed_with_time_auto("const array param error", test_file_array_error);
}
