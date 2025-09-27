#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_basic() {
    std::cout << "[integration] Running basic tests..." << std::endl;
    
    // Basic main function test (with timing)
    double execution_time;
    run_cb_test_with_output_and_time("../cases/basic/simple_main.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "simple_main.cb should execute successfully");
        }, execution_time);
    
    integration_test_passed_with_time("basic main function", "simple_main.cb", execution_time);
    
    std::cout << "[integration] Basic tests completed" << std::endl;
}
