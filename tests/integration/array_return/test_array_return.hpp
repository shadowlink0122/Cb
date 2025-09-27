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
    
    std::cout << "[integration] Array return tests completed" << std::endl;
}

#endif
