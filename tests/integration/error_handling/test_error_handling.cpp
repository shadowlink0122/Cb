#include "test_error_handling.hpp"

void test_error_handling_basic() {
    const std::string test_path = "../../tests/integration/error_handling/test_error_handling.cb";
    
    run_cb_test_with_output(test_path, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "=== Error Handling Test ===", "Expected error handling test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Error handling test completed!", "Expected test completion message");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for error handling test");
        });
    integration_test_passed("error handling basic test", test_path);
}

void test_try_catch_syntax() {
    const std::string test_path = "../../tests/integration/error_handling/test_try_catch_syntax.cb";
    
    run_cb_test_with_output(test_path, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "=== Basic Try-Catch Syntax Test ===", "Expected try-catch test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Syntax test completed!", "Expected syntax test completion");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for try-catch syntax test");
        });
    integration_test_passed("try-catch syntax test", test_path);
}
