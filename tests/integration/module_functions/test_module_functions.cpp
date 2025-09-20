#include "test_module_functions.hpp"

void test_module_function_calls() {
    // 簡単なバージョンでテスト（モジュールパス問題を回避）
    const std::string test_path = "../../tests/integration/module_functions/test_simple_module_functions.cb";
    
    run_cb_test_with_output(test_path, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "=== Module Function Call Test ===", "Expected module function call test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Module function call test completed!", "Expected test completion");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for module function call test");
        });
    integration_test_passed("module function calls test", test_path);
}
