#ifndef TEST_ARRAY_RETURN_HPP
#define TEST_ARRAY_RETURN_HPP

#include "../framework/integration_test_framework.hpp"

void test_array_return() {
    std::cout << "[integration] Running array return tests..." << std::endl;
    
    // 基本的な配列戻り値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_return/basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array return basic test");
            INTEGRATION_ASSERT(contains(output, "100"), "Expected 100 in output");
            INTEGRATION_ASSERT(contains(output, "200"), "Expected 200 in output");
            INTEGRATION_ASSERT(contains(output, "300"), "Expected 300 in output");
        });
    integration_test_passed_with_time_auto("test_array_return_basic", "../../tests/cases/array_return/basic.cb");
    
    // 文字列配列戻り値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_return/string_array.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array return string test");
            INTEGRATION_ASSERT(contains(output, "Hello"), "Expected Hello in output");
            INTEGRATION_ASSERT(contains(output, "World"), "Expected World in output");
        });
    integration_test_passed_with_time_auto("test_array_return_string", "../../tests/cases/array_return/string_array.cb");
    
    // 複数型配列戻り値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_return/multiple_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for array return multiple types test");
            INTEGRATION_ASSERT(contains(output, "1000000"), "Expected 1000000 in output");
            INTEGRATION_ASSERT(contains(output, "2000000"), "Expected 2000000 in output");
            INTEGRATION_ASSERT(contains(output, "Long array"), "Expected Long array in output");
            INTEGRATION_ASSERT(contains(output, "Bool array"), "Expected Bool array in output");
            INTEGRATION_ASSERT(contains(output, "Tiny array"), "Expected Tiny array in output");
        });
    integration_test_passed_with_time_auto("test_array_return_multiple_types", "../../tests/cases/array_return/multiple_types.cb");
    
    // typedef配列戻り値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_return/test_typedef_array_return.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for typedef array return test");
            INTEGRATION_ASSERT(contains(output, "0 8"), "Expected first element 0 8 in output");
            INTEGRATION_ASSERT(contains(output, "5 8"), "Expected middle element 5 8 in output");
            INTEGRATION_ASSERT(contains(output, "9 8"), "Expected last element 9 8 in output");
        });
    integration_test_passed_with_time_auto("test_array_return_typedef", "../../tests/cases/array_return/test_typedef_array_return.cb");
    
    // 2次元配列戻り値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_return/multidim_2d.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for 2D array return test");
            INTEGRATION_ASSERT(contains(output, "arr[0][0] = 1"), "Expected arr[0][0] = 1 in output");
            INTEGRATION_ASSERT(contains(output, "arr[0][1] = 2"), "Expected arr[0][1] = 2 in output");
            INTEGRATION_ASSERT(contains(output, "arr[0][2] = 3"), "Expected arr[0][2] = 3 in output");
            INTEGRATION_ASSERT(contains(output, "arr[1][0] = 4"), "Expected arr[1][0] = 4 in output");
            INTEGRATION_ASSERT(contains(output, "arr[1][1] = 5"), "Expected arr[1][1] = 5 in output");
            INTEGRATION_ASSERT(contains(output, "arr[1][2] = 6"), "Expected arr[1][2] = 6 in output");
        });
    integration_test_passed_with_time_auto("test_array_return_multidim_2d", "../../tests/cases/array_return/multidim_2d.cb");
    
    // 複合多次元配列戻り値テスト
    run_cb_test_with_output_and_time_auto("../../tests/cases/array_return/multidim_return.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for multidim array return test");
            // 整数配列の期待値
            INTEGRATION_ASSERT(contains(output, "matrix[0][0] = 10"), "Expected matrix[0][0] = 10 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[0][1] = 20"), "Expected matrix[0][1] = 20 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[0][2] = 30"), "Expected matrix[0][2] = 30 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][0] = 40"), "Expected matrix[1][0] = 40 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][1] = 50"), "Expected matrix[1][1] = 50 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][2] = 60"), "Expected matrix[1][2] = 60 in output");
            // 文字列配列の期待値
            INTEGRATION_ASSERT(contains(output, "str_matrix[0][0] = Test"), "Expected str_matrix[0][0] = Test in output");
            INTEGRATION_ASSERT(contains(output, "str_matrix[0][1] = Case"), "Expected str_matrix[0][1] = Case in output");
            INTEGRATION_ASSERT(contains(output, "str_matrix[1][0] = Array"), "Expected str_matrix[1][0] = Array in output");
            INTEGRATION_ASSERT(contains(output, "str_matrix[1][1] = Return"), "Expected str_matrix[1][1] = Return in output");
        });
    integration_test_passed_with_time_auto("test_array_return_multidim_comprehensive", "../../tests/cases/array_return/multidim_return.cb");
    
    std::cout << "[integration] Array return tests completed" << std::endl;
}

#endif
