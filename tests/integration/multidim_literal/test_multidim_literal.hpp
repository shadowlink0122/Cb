#ifndef TEST_MULTIDIM_LITERAL_HPP
#define TEST_MULTIDIM_LITERAL_HPP

#include "../framework/integration_test_framework.hpp"

void test_multidim_literal() {
    std::cout << "[integration] Running multidimensional literal tests..." << std::endl;
    
    // 実用的なN次元配列リテラルテスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_literal/practical_ndim.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for practical ndim literal test");
            INTEGRATION_ASSERT(contains(output, "1D array"), "Expected 1D array in output");
            INTEGRATION_ASSERT(contains(output, "2D array"), "Expected 2D array in output");
            INTEGRATION_ASSERT(contains(output, "10"), "Expected 10 in output");
            INTEGRATION_ASSERT(contains(output, "20"), "Expected 20 in output");
        });
    integration_test_passed_with_time_auto("test_practical_ndim_literal", "../../tests/cases/multidim_literal/practical_ndim.cb");
    
    // 文字列多次元配列リテラルテスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_literal/string_multidim.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for string multidim literal test");
            INTEGRATION_ASSERT(contains(output, "Hello"), "Expected Hello in output");
            INTEGRATION_ASSERT(contains(output, "World"), "Expected World in output");
            INTEGRATION_ASSERT(contains(output, "Good"), "Expected Good in output");
            INTEGRATION_ASSERT(contains(output, "Morning"), "Expected Morning in output");
        });
    integration_test_passed_with_time_auto("test_string_multidim_literal", "../../tests/cases/multidim_literal/string_multidim.cb");
    
    // 非対称多次元配列テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_literal/asymmetric.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for asymmetric multidim literal test");
            INTEGRATION_ASSERT(contains(output, "Asymmetric 2D array (3x2)"), "Expected Asymmetric 2D array (3x2) in output");
            INTEGRATION_ASSERT(contains(output, "Asymmetric 2D array (2x4)"), "Expected Asymmetric 2D array (2x4) in output");
            INTEGRATION_ASSERT(contains(output, "10"), "Expected 10 in output");
            INTEGRATION_ASSERT(contains(output, "80"), "Expected 80 in output");
        });
    integration_test_passed_with_time_auto("test_asymmetric_multidim_literal", "../../tests/cases/multidim_literal/asymmetric.cb");
    
    // 3次元配列総合テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_literal/ndim_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for 3D comprehensive test");
            INTEGRATION_ASSERT(contains(output, "3D array"), "Expected 3D array in output");
            INTEGRATION_ASSERT(contains(output, "1"), "Expected 1 in output");
            INTEGRATION_ASSERT(contains(output, "2"), "Expected 2 in output");
            INTEGRATION_ASSERT(contains(output, "3"), "Expected 3 in output");
            INTEGRATION_ASSERT(contains(output, "4"), "Expected 4 in output");
        });
    integration_test_passed_with_time_auto("test_ndim_comprehensive", "../../tests/cases/multidim_literal/ndim_comprehensive.cb");
    
    // 4次元・5次元配列テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_literal/4d_5d_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for 4D/5D test");
            INTEGRATION_ASSERT(contains(output, "4D array"), "Expected 4D array in output");
            INTEGRATION_ASSERT(contains(output, "5D array"), "Expected 5D array in output");
            INTEGRATION_ASSERT(contains(output, "1"), "Expected 1 in output");
            INTEGRATION_ASSERT(contains(output, "16"), "Expected 16 in output");
            INTEGRATION_ASSERT(contains(output, "8"), "Expected 8 in output");
        });
    integration_test_passed_with_time_auto("test_4d_5d_arrays", "../../tests/cases/multidim_literal/4d_5d_test.cb");
    
    // 高次元配列テスト（10次元・6次元）
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_literal/high_dim_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for high dimension test");
            INTEGRATION_ASSERT(contains(output, "10D array"), "Expected 10D array in output");
            INTEGRATION_ASSERT(contains(output, "6D array"), "Expected 6D array in output");
            INTEGRATION_ASSERT(contains(output, "1"), "Expected 1 in output");
            INTEGRATION_ASSERT(contains(output, "12"), "Expected 12 in output");
        });
    integration_test_passed_with_time_auto("test_high_dimension_arrays", "../../tests/cases/multidim_literal/high_dim_test.cb");
    
    std::cout << "[integration] Multidimensional literal tests completed" << std::endl;
}

#endif
