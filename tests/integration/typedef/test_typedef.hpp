#ifndef TEST_TYPEDEF_HPP
#define TEST_TYPEDEF_HPP

#include "../framework/integration_test_framework.hpp"

void test_typedef_basic() {
    run_cb_test_with_output("../../tests/integration/typedef/test_basic_typedef.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Typedef integration test completed!", 
                                      "Test should complete successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "MyInt number = 42", 
                                      "Typedef variables should work");
        });
    integration_test_passed("test_typedef_basic", "test_basic_typedef.cb");
}

void test_typedef_advanced() {
    run_cb_test_with_output("../../tests/integration/typedef/test_advanced_typedef.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Advanced typedef test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Advanced typedef test completed!", 
                                      "Test should complete successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Processing: 0 with count: 10", 
                                      "Typedef function parameters should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Total after doubling: 20", 
                                      "Typedef arithmetic operations should work");
        });
    integration_test_passed("test_typedef_advanced", "test_advanced_typedef.cb");
}

#endif // TEST_TYPEDEF_HPP
