#ifndef TEST_MULTIDIM_ARRAY_HPP
#define TEST_MULTIDIM_ARRAY_HPP

#include "../framework/integration_test_framework.hpp"

void test_multidim_array_literal_2d() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/literal_2d_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "2D array literal test should execute successfully");
        });
    integration_test_passed_with_time_auto("test_multidim_array_literal_2d", "literal_2d_test.cb");
}

void test_multidim_array_literal_3d() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/literal_3d_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "3D array literal test should execute successfully");
        });
    integration_test_passed_with_time_auto("test_multidim_array_literal_3d", "literal_3d_test.cb");
}

void test_multidim_array_assignment() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/assignment_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multidim array assignment test should execute successfully");
        });
    integration_test_passed_with_time_auto("test_multidim_array_assignment", "assignment_test.cb");
}

// Main multidim array test function
void test_integration_multidim_array() {
    std::cout << "[integration] Running multidimensional array tests..." << std::endl;
    test_multidim_array_literal_2d();
    test_multidim_array_literal_3d();
    test_multidim_array_assignment();
    std::cout << "[integration] Multidimensional array tests completed" << std::endl;
}

#endif // TEST_MULTIDIM_ARRAY_HPP
