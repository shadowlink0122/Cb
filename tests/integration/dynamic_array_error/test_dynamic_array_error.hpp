#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_dynamic_array_error() {
    const std::string test_simple = "../../tests/cases/dynamic_array_error/simple_dynamic.cb";
    const std::string test_multidim = "../../tests/cases/dynamic_array_error/multidim_dynamic.cb";
    const std::string test_mixed = "../../tests/cases/dynamic_array_error/mixed_dynamic.cb";
    
    // 単純な動的配列エラーテスト
    run_cb_test_with_output_and_time_auto(test_simple, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for simple dynamic array test");
            INTEGRATION_ASSERT_CONTAINS(output, "Dynamic arrays are not supported yet", "should contain dynamic array error message");
        });
    integration_test_passed_with_time_auto("dynamic_array_error simple test", test_simple);
    
    // 多次元動的配列エラーテスト
    run_cb_test_with_output_and_time_auto(test_multidim, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for multidim dynamic array test");
            INTEGRATION_ASSERT_CONTAINS(output, "Dynamic arrays are not supported yet", "should contain dynamic array error message");
        });
    integration_test_passed_with_time_auto("dynamic_array_error multidim test", test_multidim);
    
    // 混合動的配列エラーテスト
    run_cb_test_with_output_and_time_auto(test_mixed, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "Expected error exit code for mixed dynamic array test");
            INTEGRATION_ASSERT_CONTAINS(output, "Dynamic arrays are not supported yet", "should contain dynamic array error message");
        });
    integration_test_passed_with_time_auto("dynamic_array_error mixed test", test_mixed);
}
