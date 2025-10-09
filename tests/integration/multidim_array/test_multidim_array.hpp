#ifndef TEST_MULTIDIM_ARRAY_HPP
#define TEST_MULTIDIM_ARRAY_HPP

#include "../framework/integration_test_framework.hpp"

void test_multidim_array_literal_2d() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/literal_2d_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "2D array literal test should execute successfully");
            INTEGRATION_ASSERT(contains(output, "matrix[0][0] = 1"), "Expected matrix[0][0] = 1 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[0][1] = 2"), "Expected matrix[0][1] = 2 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[0][2] = 3"), "Expected matrix[0][2] = 3 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][0] = 4"), "Expected matrix[1][0] = 4 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][1] = 5"), "Expected matrix[1][1] = 5 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][2] = 6"), "Expected matrix[1][2] = 6 in output");
            INTEGRATION_ASSERT(contains(output, "All 2D literal tests passed"), "Expected success message in output");
        });
    integration_test_passed_with_time_auto("test_multidim_array_literal_2d", "literal_2d_test.cb");
}

void test_multidim_array_literal_3d() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/literal_3d_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "3D array literal test should execute successfully");
            INTEGRATION_ASSERT(contains(output, "cube[0][0][0] = 100"), "Expected cube[0][0][0] = 100 in output");
            INTEGRATION_ASSERT(contains(output, "cube[0][0][1] = 101"), "Expected cube[0][0][1] = 101 in output");
            INTEGRATION_ASSERT(contains(output, "cube[0][1][0] = 102"), "Expected cube[0][1][0] = 102 in output");
            INTEGRATION_ASSERT(contains(output, "cube[0][1][1] = 103"), "Expected cube[0][1][1] = 103 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][0][0] = 104"), "Expected cube[1][0][0] = 104 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][0][1] = 105"), "Expected cube[1][0][1] = 105 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][1][0] = 106"), "Expected cube[1][1][0] = 106 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][1][1] = 107"), "Expected cube[1][1][1] = 107 in output");
            INTEGRATION_ASSERT(contains(output, "All 3D tests passed"), "Expected success message in output");
        });
    integration_test_passed_with_time_auto("test_multidim_array_literal_3d", "literal_3d_test.cb");
}

void test_multidim_array_assignment() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/assignment_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multidim array assignment test should execute successfully");
            INTEGRATION_ASSERT(contains(output, "data[0][0] = 10"), "Expected data[0][0] = 10 in output");
            INTEGRATION_ASSERT(contains(output, "data[0][1] = 20"), "Expected data[0][1] = 20 in output");
            INTEGRATION_ASSERT(contains(output, "data[1][0] = 30"), "Expected data[1][0] = 30 in output");
            INTEGRATION_ASSERT(contains(output, "data[1][1] = 40"), "Expected data[1][1] = 40 in output");
            INTEGRATION_ASSERT(contains(output, "data[2][0] = 50"), "Expected data[2][0] = 50 in output");
            INTEGRATION_ASSERT(contains(output, "data[2][1] = 60"), "Expected data[2][1] = 60 in output");
            INTEGRATION_ASSERT(contains(output, "Total sum = 210"), "Expected total sum = 210 in output");
        });
    integration_test_passed_with_time_auto("test_multidim_array_assignment", "assignment_test.cb");
}

void test_multidim_array_matrix_operations() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/matrix_operations.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Matrix operations test should execute successfully");
            // 行列データの確認
            INTEGRATION_ASSERT(contains(output, "data[0][0] = 10"), "Expected data[0][0] = 10 in output");
            INTEGRATION_ASSERT(contains(output, "data[0][1] = 20"), "Expected data[0][1] = 20 in output");
            INTEGRATION_ASSERT(contains(output, "data[0][2] = 30"), "Expected data[0][2] = 30 in output");
            INTEGRATION_ASSERT(contains(output, "data[1][0] = 40"), "Expected data[1][0] = 40 in output");
            INTEGRATION_ASSERT(contains(output, "data[3][2] = 120"), "Expected data[3][2] = 120 in output");
            // 行の合計の確認
            INTEGRATION_ASSERT(contains(output, "row_sums[0] = 60"), "Expected row_sums[0] = 60 in output");
            INTEGRATION_ASSERT(contains(output, "row_sums[1] = 150"), "Expected row_sums[1] = 150 in output");
            INTEGRATION_ASSERT(contains(output, "row_sums[2] = 240"), "Expected row_sums[2] = 240 in output");
            INTEGRATION_ASSERT(contains(output, "row_sums[3] = 330"), "Expected row_sums[3] = 330 in output");
            INTEGRATION_ASSERT(contains(output, "All tests passed"), "Expected success message in output");
        });
    integration_test_passed_with_time_auto("test_multidim_array_matrix_operations", "matrix_operations.cb");
}

void test_multidim_array_cube_test() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/cube_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "3D cube test should execute successfully");
            INTEGRATION_ASSERT(contains(output, "cube[0][0][0] = 100"), "Expected cube[0][0][0] = 100 in output");
            INTEGRATION_ASSERT(contains(output, "cube[0][0][1] = 101"), "Expected cube[0][0][1] = 101 in output");
            INTEGRATION_ASSERT(contains(output, "cube[0][1][0] = 102"), "Expected cube[0][1][0] = 102 in output");
            INTEGRATION_ASSERT(contains(output, "cube[0][1][1] = 103"), "Expected cube[0][1][1] = 103 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][0][0] = 104"), "Expected cube[1][0][0] = 104 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][0][1] = 105"), "Expected cube[1][0][1] = 105 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][1][0] = 106"), "Expected cube[1][1][0] = 106 in output");
            INTEGRATION_ASSERT(contains(output, "cube[1][1][1] = 107"), "Expected cube[1][1][1] = 107 in output");
            INTEGRATION_ASSERT(contains(output, "All cube tests passed"), "Expected success message in output");
        });
    integration_test_passed_with_time_auto("test_multidim_array_cube_test", "cube_test.cb");
}

void test_multidim_array_literal_init() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/multidim_array/literal_init.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Literal init test should execute successfully");
            INTEGRATION_ASSERT(contains(output, "matrix[0][0] = 1"), "Expected matrix[0][0] = 1 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[0][1] = 2"), "Expected matrix[0][1] = 2 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[0][2] = 3"), "Expected matrix[0][2] = 3 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][0] = 4"), "Expected matrix[1][0] = 4 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][1] = 5"), "Expected matrix[1][1] = 5 in output");
            INTEGRATION_ASSERT(contains(output, "matrix[1][2] = 6"), "Expected matrix[1][2] = 6 in output");
            INTEGRATION_ASSERT(contains(output, "All literal init tests passed"), "Expected success message in output");
        });
    integration_test_passed_with_time_auto("test_multidim_array_literal_init", "literal_init.cb");
}

// Main multidim array test function
void test_integration_multidim_array() {
    std::cout << "[integration-test] Running multidimensional array tests..." << std::endl;
    test_multidim_array_literal_2d();
    test_multidim_array_literal_3d();
    test_multidim_array_assignment();
    test_multidim_array_matrix_operations();
    test_multidim_array_cube_test();
    test_multidim_array_literal_init();
    std::cout << "[integration-test] Multidimensional array tests completed" << std::endl;
}

#endif // TEST_MULTIDIM_ARRAY_HPP
