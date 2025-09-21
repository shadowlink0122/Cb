#ifndef TEST_IMPORT_EXPORT_HPP
#define TEST_IMPORT_EXPORT_HPP

#include "../framework/integration_test_framework.hpp"

void test_import_export_basic() {
    run_cb_test_with_output("../../tests/integration/import_export/test_basic_import_export.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic import/export test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Import/Export test completed!", 
                                      "Test should complete successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Math constant: 42", 
                                      "Imported constant should be accessible");
            INTEGRATION_ASSERT_CONTAINS(output, "6 * 7 = 42", 
                                      "Imported function should work");
        });
    integration_test_passed("test_import_export_basic", "test_basic_import_export.cb");
}

void test_typedef_import_combo() {
    run_cb_test_with_output("../../tests/integration/import_export/test_typedef_import_combo.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Combined typedef/import test should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Combined test completed!", 
                                      "Test should complete successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Item: Widget, Count: 210", 
                                      "Typedef with imported functions should work");
        });
    integration_test_passed("test_typedef_import_combo", "test_typedef_import_combo.cb");
}

#endif // TEST_IMPORT_EXPORT_HPP
