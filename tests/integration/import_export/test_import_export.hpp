#ifndef TEST_IMPORT_EXPORT_HPP
#define TEST_IMPORT_EXPORT_HPP

#include "../framework/integration_test_framework.hpp"

void test_import_export_basic() {
    std::cout << "[integration-test] Running basic import/export test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_basic_import_export.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic import/export should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Import/Export Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "6 * 7 = 42", "Should correctly multiply using imported function");
            INTEGRATION_ASSERT_CONTAINS(output, "20 / 4 = 5", "Should correctly divide using imported function");
            INTEGRATION_ASSERT_CONTAINS(output, "Error: Division by zero", "Should handle division by zero");
            INTEGRATION_ASSERT_CONTAINS(output, "10 / 0 = 0", "Should return 0 for division by zero");
            INTEGRATION_ASSERT_CONTAINS(output, "Length of 'Hello World' = 11", "Should correctly calculate string length");
            INTEGRATION_ASSERT_CONTAINS(output, "Import/Export test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("basic import/export", "test_basic_import_export.cb", execution_time);
}

void test_import_export_module_functions() {
    std::cout << "[integration-test] Running module function tests..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/math_module.cb", 
        [](const std::string& output, int exit_code) {
            // モジュールファイル単体での実行は失敗するべき（main関数がないため）
            INTEGRATION_ASSERT_NE(0, exit_code, "Module without main should fail");
            INTEGRATION_ASSERT_CONTAINS(output, "Main function not found", "Should report missing main function");
        }, execution_time);
    integration_test_passed_with_time("module file validation (no main)", "math_module.cb", execution_time);
}

void test_import_export_exported_only() {
    std::cout << "[integration-test] Running export visibility test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_module_helper.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Module with helpers should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Module Helper Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "public_add(5, 3) = 8", "Should call exported function");
            INTEGRATION_ASSERT_CONTAINS(output, "calculate(4, 3) = 19", "Should call exported function using private helper");
            INTEGRATION_ASSERT_CONTAINS(output, "Module helper test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("module with helper functions", "test_module_helper.cb", execution_time);
}

void test_import_export_multiple_modules() {
    std::cout << "[integration-test] Running multiple module import test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_multiple_modules.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multiple module import should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Multiple Module Import Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "add(10, 5) = 15", "Should use module_a function");
            INTEGRATION_ASSERT_CONTAINS(output, "subtract(10, 5) = 5", "Should use module_a function");
            INTEGRATION_ASSERT_CONTAINS(output, "multiply(10, 5) = 50", "Should use module_b function");
            INTEGRATION_ASSERT_CONTAINS(output, "divide(10, 5) = 2", "Should use module_b function");
            INTEGRATION_ASSERT_CONTAINS(output, "27", "Should correctly combine functions from both modules");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple module test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("multiple independent modules", "test_multiple_modules.cb", execution_time);
}

void test_import_export_duplicate_import() {
    std::cout << "[integration-test] Running duplicate import test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_duplicate_import.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Duplicate import should be handled gracefully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Duplicate Import Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "multiply(6, 7) = 42", "Should still work with duplicate import");
            INTEGRATION_ASSERT_CONTAINS(output, "Duplicate import test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("duplicate import handling", "test_duplicate_import.cb", execution_time);
}

void test_import_export_struct() {
    std::cout << "[integration-test] Running struct import test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_import_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Struct import should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Import Struct Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Point: ( 10 ,  20 )", "Should access imported struct members");
            INTEGRATION_ASSERT_CONTAINS(output, "Import struct test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("struct import", "test_import_struct.cb", execution_time);
}

void test_import_export_qualified_call() {
    std::cout << "[integration-test] Running qualified call test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_qualified_call.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Qualified call should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Qualified Call Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "math.add(3, 5) =  8", "Should call qualified function");
            INTEGRATION_ASSERT_CONTAINS(output, "math.multiply(4, 7) =  28", "Should call qualified function");
            INTEGRATION_ASSERT_CONTAINS(output, "math.subtract(10, 3) =  7", "Should call qualified default export");
            INTEGRATION_ASSERT_CONTAINS(output, "math.PI =  3", "Should access qualified constant");
            INTEGRATION_ASSERT_CONTAINS(output, "math.E =  2", "Should access qualified constant");
            INTEGRATION_ASSERT_CONTAINS(output, "Qualified call test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("qualified call (module.function())", "test_qualified_call.cb", execution_time);
}

void test_import_export_const() {
    std::cout << "[integration-test] Running const import test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_import_const.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Const import should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Import Const Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "PI =  3", "Should access imported const PI");
            INTEGRATION_ASSERT_CONTAINS(output, "E =  2", "Should access imported const E");
            INTEGRATION_ASSERT_CONTAINS(output, "Import const test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("const import with selective syntax", "test_import_const.cb", execution_time);
}

void test_import_export_integration() {
    std::cout << "[integration-test] Running import/export integration test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_integration.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Integration test should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Qualified Call & Import Integration Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "math.add(10, 20) =  30", "Should call qualified function");
            INTEGRATION_ASSERT_CONTAINS(output, "math.PI =  3", "Should access qualified constant");
            INTEGRATION_ASSERT_CONTAINS(output, "Point: ( 100 ,  200 )", "Should use imported struct");
            INTEGRATION_ASSERT_CONTAINS(output, "multiply(3, 7) =  21", "Should call unqualified function");
            INTEGRATION_ASSERT_CONTAINS(output, "Integration test completed!", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("qualified call & struct import integration", "test_integration.cb", execution_time);
}

void test_import_export_impl() {
    std::cout << "[integration-test] Running impl import test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_import_constructor.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Impl import should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Constructor/Destructor/Impl Import Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Rectangle constructor called:  10  x  20", "Should call constructor");
            INTEGRATION_ASSERT_CONTAINS(output, "Rectangle:  10  x  20  (Area:  200 )", "Should call impl method (display)");
            INTEGRATION_ASSERT_CONTAINS(output, "Calculated area:  200", "Should call impl method (getArea)");
            INTEGRATION_ASSERT_CONTAINS(output, "Width:  10", "Should access struct member");
            INTEGRATION_ASSERT_CONTAINS(output, "Height:  20", "Should access struct member");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple instances:", "Should support multiple instances");
            INTEGRATION_ASSERT_CONTAINS(output, "Instance independence:", "Should test instance independence");
            INTEGRATION_ASSERT_CONTAINS(output, "All tests passed!", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("impl import with constructor/interface methods", "test_import_constructor.cb", execution_time);
}

void test_import_export_impl_types() {
    std::cout << "[integration-test] Running impl types export test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_impl_types.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Impl types export should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Impl Types Export Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 1] Struct definition imported:", "Should import struct definition");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 2] Constructor (impl Rectangle) imported:", "Should import impl Rectangle");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 3] Interface methods (impl Shape for Rectangle) imported:", "Should import impl Shape for Rectangle");
            INTEGRATION_ASSERT_CONTAINS(output, "All impl types export tests passed!", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("impl types (constructor & interface) export", "test_impl_types.cb", execution_time);
}

void test_import_export_simple_constructor() {
    std::cout << "[integration-test] Running simple constructor import test..." << std::endl;
    
    double execution_time;
    
    run_cb_test_with_output_and_time("../../tests/cases/import_export/test_simple_constructor.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Simple constructor import should succeed");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Simple Constructor Import Test ===", "Should print test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Rectangle constructor called:  10  x  20", "Should call constructor");
            INTEGRATION_ASSERT_CONTAINS(output, "Width:  10", "Should access width member");
            INTEGRATION_ASSERT_CONTAINS(output, "Height:  20", "Should access height member");
            INTEGRATION_ASSERT_CONTAINS(output, "Test completed", "Should complete test");
        }, execution_time);
    integration_test_passed_with_time("simple constructor import", "test_simple_constructor.cb", execution_time);
}

// Main import_export test function
void test_integration_import_export() {
    std::cout << "\n[integration-test] === Import/Export Tests ===" << std::endl;
    
    test_import_export_basic();
    test_import_export_module_functions();
    test_import_export_multiple_modules();
    test_import_export_exported_only();
    test_import_export_duplicate_import();
    test_import_export_struct();
    test_import_export_qualified_call();
    test_import_export_const();
    test_import_export_integration();
    test_import_export_impl();
    test_import_export_impl_types();
    test_import_export_simple_constructor();
    
    std::cout << "[integration-test] Import/Export tests completed" << std::endl;
}

#endif // TEST_IMPORT_EXPORT_HPP
