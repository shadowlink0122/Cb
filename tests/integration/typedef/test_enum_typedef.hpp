#ifndef TEST_ENUM_TYPEDEF_HPP
#define TEST_ENUM_TYPEDEF_HPP

#include "../framework/integration_test_framework.hpp"

void test_enum_typedef_basic() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/typedef/test_enum_typedef_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic enum typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Status: 0", "SUCCESS enum value should be 0");
            INTEGRATION_ASSERT_CONTAINS(output, "Error: 1", "ERROR enum value should be 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Negative: -1", "Negative enum values should work");
        });
    integration_test_passed_with_time_auto("test_enum_typedef_basic", "test_enum_typedef_basic.cb");
}

void test_enum_typedef_separated() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/typedef/test_enum_typedef_separated.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Separated enum typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Color1: 1", "RED should be 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Color2: 2", "GREEN should be 2"); 
            INTEGRATION_ASSERT_CONTAINS(output, "Color3: 3", "BLUE should be 3");
        });
    integration_test_passed_with_time_auto("test_enum_typedef_separated", "test_enum_typedef_separated.cb");
}

void test_enum_typedef_functions() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/typedef/test_enum_typedef_functions.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Enum typedef function test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Function result: 0", "Enum function should return SUCCESS");
            INTEGRATION_ASSERT_CONTAINS(output, "Status check: 1", "Error status should be ERROR");
        });
    integration_test_passed_with_time_auto("test_enum_typedef_functions", "test_enum_typedef_functions.cb");
}

void test_enum_typedef_comprehensive() {
    run_cb_test_with_output_and_time_auto("../../tests/cases/typedef/test_enum_typedef_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Comprehensive enum typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "All enum typedef tests passed", "Comprehensive test should pass");
        });
    integration_test_passed_with_time_auto("test_enum_typedef_comprehensive", "test_enum_typedef_comprehensive.cb");
}

// Main enum typedef test function
void test_integration_enum_typedef() {
    std::cout << "[integration-test] Running enum typedef tests..." << std::endl;
    test_enum_typedef_basic();
    test_enum_typedef_separated();  
    test_enum_typedef_functions();
    test_enum_typedef_comprehensive();
    std::cout << "[integration-test] Enum typedef tests completed" << std::endl;
}

#endif // TEST_ENUM_TYPEDEF_HPP
