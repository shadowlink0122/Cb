#pragma once
#include "../framework/integration_test_framework.hpp"

namespace StringArrayTests {

void test_string_array_basic() {
    std::cout << "[integration-test] String Array: Basic operations..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/string_array/test_string_array_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_string_array_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Array Basic Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - Declaration and assignment:", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - Literal initialization:", "Expected test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3 - After modification:", "Expected test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4 - Empty strings:", "Expected test 4");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("String Array Basic", "test_string_array_basic.cb", execution_time);
}

void test_string_array_const() {
    std::cout << "[integration-test] String Array: Const arrays..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/string_array/test_string_array_const.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_string_array_const.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Const String Array Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - Const array:", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - Const colors:", "Expected test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("String Array Const", "test_string_array_const.cb", execution_time);
}

void test_string_array_function() {
    std::cout << "[integration-test] String Array: Function parameters..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/string_array/test_string_array_function.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_string_array_function.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Array Function Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - First element:", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - Print array:", "Expected test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3 - After modification:", "Expected test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("String Array Function", "test_string_array_function.cb", execution_time);
}

void test_string_array_struct() {
    std::cout << "[integration-test] String Array: With structs..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/string_array/test_string_array_struct.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_string_array_struct.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== String Array with Struct Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - Parallel arrays:", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected completion");
        }, execution_time);
    
    integration_test_passed_with_time("String Array Struct", "test_string_array_struct.cb", execution_time);
}

void run_all_string_array_tests() {
    std::cout << "\n[integration-test] === String Array Tests ===" << std::endl;
    
    test_string_array_basic();
    test_string_array_const();
    test_string_array_function();
    test_string_array_struct();
}

} // namespace StringArrayTests
