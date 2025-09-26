#ifndef TEST_STRUCT_TYPEDEF_HPP
#define TEST_STRUCT_TYPEDEF_HPP

#include "../framework/integration_test_framework.hpp"

void test_struct_typedef_basic() {
    run_cb_test_with_output("../../tests/cases/typedef/test_struct_typedef_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic struct typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Point: 10 20", "Basic struct typedef declaration should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Basic struct typedef test passed", "Test should complete successfully");
        });
    integration_test_passed("test_struct_typedef_basic", "test_struct_typedef_basic.cb");
}

void test_struct_typedef_separated() {
    run_cb_test_with_output("../../tests/cases/typedef/test_struct_typedef_separated.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Separated struct typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Separated struct typedef test passed", "Separated struct typedef should work");
        });
    integration_test_passed("test_struct_typedef_separated", "test_struct_typedef_separated.cb");
}

void test_struct_typedef_literals() {
    run_cb_test_with_output("../../tests/cases/typedef/test_struct_typedef_literals.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct typedef literal test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Struct literal typedef test passed", "Struct literal initialization should work");
        });
    integration_test_passed("test_struct_typedef_literals", "test_struct_typedef_literals.cb");
}

void test_struct_typedef_functions() {
    run_cb_test_with_output("../../tests/cases/typedef/test_struct_typedef_functions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct typedef function test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Struct typedef function test passed", "Struct function usage should work");
        });
    integration_test_passed("test_struct_typedef_functions", "test_struct_typedef_functions.cb");
}

void test_struct_typedef_comprehensive() {
    run_cb_test_with_output("../../tests/cases/typedef/test_struct_typedef_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Comprehensive struct typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "All struct typedef tests passed", "Comprehensive test should pass");
        });
    integration_test_passed("test_struct_typedef_comprehensive", "test_struct_typedef_comprehensive.cb");
}

// Main struct typedef test function
void test_integration_struct_typedef() {
    std::cout << "[integration] Running struct typedef tests..." << std::endl;
    test_struct_typedef_basic();
    test_struct_typedef_separated();
    test_struct_typedef_literals();
    test_struct_typedef_functions();
    test_struct_typedef_comprehensive();
    std::cout << "[integration] Struct typedef tests completed" << std::endl;
}

#endif // TEST_STRUCT_TYPEDEF_HPP
