#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_type() {
    std::cout << "[integration-test] Running type tests..." << std::endl;
    
    // Test tiny type range violation with assignment
    run_cb_test_with_output_and_time_auto("../cases/type/tiny/ng_assign.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "ng_assign.cb should fail due to range violation");
        });
    
    integration_test_passed_with_error_and_time_auto("tiny type range violation (assignment)", "ng_assign.cb");
    
    // Test tiny type range violation with literal
    run_cb_test_with_output_and_time_auto("../cases/type/tiny/ng_literal.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_NE(0, exit_code, "ng_literal.cb should fail due to range violation");
        });
    
    integration_test_passed_with_error_and_time_auto("tiny type range violation (literal)", "ng_literal.cb");
    
    std::cout << "[integration-test] Type tests completed" << std::endl;
}
