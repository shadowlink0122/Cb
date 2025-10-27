#pragma once
#include "../framework/integration_test_framework.hpp"

namespace GenericsTests {

void run_all_generics_tests() {
    std::cout << "[integration-test] Running generics tests..." << std::endl;
    
    double execution_time;
    
    // Test 1: Basic Generic Struct
    run_cb_test_with_output_and_time("../../tests/cases/generics/basic_struct.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic_struct.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Generic struct parsed successfully", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Basic Struct", "basic_struct.cb", execution_time);

    // Test 2: Multiple Type Parameters
    run_cb_test_with_output_and_time("../../tests/cases/generics/multiple_params.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_params.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple type parameters parsed successfully", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Multiple Type Parameters", "multiple_params.cb", execution_time);

    // Test 3: Forward Declaration
    run_cb_test_with_output_and_time("../../tests/cases/generics/forward_decl.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "forward_decl.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Generic forward declarations parsed successfully", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Forward Declaration", "forward_decl.cb", execution_time);

    // Test 4: Execution - Basic
    run_cb_test_with_output_and_time("../../tests/cases/generics/execution_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "execution_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "42", "Expected int_box.value = 42");
            INTEGRATION_ASSERT_CONTAINS(output, "Hello, Generics!", "Expected string value");
            INTEGRATION_ASSERT_CONTAINS(output, "Generic struct execution successful!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Execution Basic", "execution_basic.cb", execution_time);

    // Test 5: Execution - Result Type
    run_cb_test_with_output_and_time("../../tests/cases/generics/result_type.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "result_type.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "Expected ok_value = 100");
            INTEGRATION_ASSERT_CONTAINS(output, "404", "Expected err_value = 404");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple type parameters test passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Result<T,E> Type", "result_type.cb", execution_time);

    // Test 6: Execution - Multiple Instantiations
    run_cb_test_with_output_and_time("../../tests/cases/generics/multiple_instantiations.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multiple_instantiations.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "10", "Expected int_pair.first = 10");
            INTEGRATION_ASSERT_CONTAINS(output, "20", "Expected int_pair.second = 20");
            INTEGRATION_ASSERT_CONTAINS(output, "Answer", "Expected str_int_pair.first");
            INTEGRATION_ASSERT_CONTAINS(output, "42", "Expected str_int_pair.second");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple instantiations test passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Multiple Instantiations", "multiple_instantiations.cb", execution_time);

    // Test 7: Main Test Suite
    run_cb_test_with_output_and_time("../../tests/cases/generics/main.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "main.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Basic Generic Struct", "Expected Test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Multiple Type Parameters", "Expected Test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Multiple Instantiations", "Expected Test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "All Generic Tests Passed!", "Expected overall success message");
        }, execution_time);
    integration_test_passed_with_time("Generics Comprehensive Test Suite", "main.cb", execution_time);
    
    // Test 8: Nested Generics (no space)
    run_cb_test_with_output_and_time("../../tests/cases/generics/nested_no_space.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "nested_no_space.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "999", "Expected container value");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested generics without space: SUCCESS!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Nested Generics (>> auto-split)", "nested_no_space.cb", execution_time);
    
    // Test 9: Deep Nested Generics
    run_cb_test_with_output_and_time("../../tests/cases/generics/deep_nested.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "deep_nested.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "42", "Expected first nested value");
            INTEGRATION_ASSERT_CONTAINS(output, "nested", "Expected string value");
            INTEGRATION_ASSERT_CONTAINS(output, "777", "Expected triple nested value");
            INTEGRATION_ASSERT_CONTAINS(output, "Deep nested generics: SUCCESS!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Deep Nested Generics", "deep_nested.cb", execution_time);
    
    // Test 10: Generic Function - Basic
    run_cb_test_with_output_and_time("../../tests/cases/generics/function_basic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "function_basic.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "identity<int>(42) = 42", "Expected identity<int> result");
            INTEGRATION_ASSERT_CONTAINS(output, "max<int>(10, 20) = 20", "Expected max<int> result");
            INTEGRATION_ASSERT_CONTAINS(output, "identity<long>(100) = 100", "Expected identity<long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "max<long>(50, 75) = 75", "Expected max<long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "Value: 999", "Expected print_value output");
            INTEGRATION_ASSERT_CONTAINS(output, "All generic function tests passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Function Basic", "function_basic.cb", execution_time);
    
    // Test 11: Generic Function - Multiple Parameters
    run_cb_test_with_output_and_time("../../tests/cases/generics/function_multiple_params.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "function_multiple_params.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "first<int, long>(42, 100) = 42", "Expected first<int,long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "second<int, long>(42, 100) = 100", "Expected second<int,long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "Pair: (10, 20)", "Expected Pair<int,int> output");
            INTEGRATION_ASSERT_CONTAINS(output, "Pair: (30, 40)", "Expected Pair<long,long> output");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple type parameter tests passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Function Multiple Parameters", "function_multiple_params.cb", execution_time);
    
    // Test 12: Generic Function - Swap (Pointer Operations)
    run_cb_test_with_output_and_time("../../tests/cases/generics/function_swap.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "function_swap.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Before swap: x= 10", "Expected before swap x value");
            INTEGRATION_ASSERT_CONTAINS(output, "y= 20", "Expected before swap y value");
            INTEGRATION_ASSERT_CONTAINS(output, "After swap: x= 20", "Expected after swap x value");
            INTEGRATION_ASSERT_CONTAINS(output, "y= 10", "Expected after swap y value");
            INTEGRATION_ASSERT_CONTAINS(output, "Before swap: a= 100", "Expected before swap a value");
            INTEGRATION_ASSERT_CONTAINS(output, "b= 200", "Expected before swap b value");
            INTEGRATION_ASSERT_CONTAINS(output, "After swap: a= 200", "Expected after swap a value");
            INTEGRATION_ASSERT_CONTAINS(output, "b= 100", "Expected after swap b value");
            INTEGRATION_ASSERT_CONTAINS(output, "Swap tests passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Function Swap", "function_swap.cb", execution_time);
    
    // Test 13: Generic Function - With Struct
    run_cb_test_with_output_and_time("../../tests/cases/generics/function_with_struct.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "function_with_struct.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "int_box.value = 42", "Expected Box<int> value");
            INTEGRATION_ASSERT_CONTAINS(output, "Extracted value = 42", "Expected unbox<int> result");
            INTEGRATION_ASSERT_CONTAINS(output, "Box contains: 42", "Expected print_box<int> output");
            INTEGRATION_ASSERT_CONTAINS(output, "long_box.value = 999", "Expected Box<long> value");
            INTEGRATION_ASSERT_CONTAINS(output, "Generic function with struct tests passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Function with Struct", "function_with_struct.cb", execution_time);
    
    // Test 14: Generic Function - Comprehensive
    run_cb_test_with_output_and_time("../../tests/cases/generics/function_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "function_comprehensive.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Basic Generic Function Tests ===", "Expected basic tests section");
            INTEGRATION_ASSERT_CONTAINS(output, "identity<int>(42) = 42", "Expected identity<int> result");
            INTEGRATION_ASSERT_CONTAINS(output, "identity<long>(100) = 100", "Expected identity<long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Multiple Type Parameters ===", "Expected multiple params section");
            INTEGRATION_ASSERT_CONTAINS(output, "first<int, long>(10, 20) = 10", "Expected first result");
            INTEGRATION_ASSERT_CONTAINS(output, "second<int, long>(10, 20) = 20", "Expected second result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Generic Struct Return ===", "Expected struct return section");
            INTEGRATION_ASSERT_CONTAINS(output, "Box<int> b1.value = 999", "Expected Box<int> value");
            INTEGRATION_ASSERT_CONTAINS(output, "Box<long> b2.value = 777", "Expected Box<long> value");
            INTEGRATION_ASSERT_CONTAINS(output, "unboxed int = 999", "Expected unboxed int");
            INTEGRATION_ASSERT_CONTAINS(output, "unboxed long = 777", "Expected unboxed long");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Multiple Type Param Struct ===", "Expected multiple type param section");
            INTEGRATION_ASSERT_CONTAINS(output, "Pair<int, long>: ( 5 , 10 )", "Expected Pair<int,long>");
            INTEGRATION_ASSERT_CONTAINS(output, "Pair<long, int>: ( 20 , 30 )", "Expected Pair<long,int>");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Pointer Operations ===", "Expected pointer ops section");
            INTEGRATION_ASSERT_CONTAINS(output, "After swap: x= 2 , y= 1", "Expected swap result");
            INTEGRATION_ASSERT_CONTAINS(output, "After swap: a= 200 , b= 100", "Expected long swap result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Void Return ===", "Expected void return section");
            INTEGRATION_ASSERT_CONTAINS(output, "Value: 42", "Expected print_value<int>");
            INTEGRATION_ASSERT_CONTAINS(output, "Value: 999", "Expected print_value<long>");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Comparison Functions ===", "Expected comparison section");
            INTEGRATION_ASSERT_CONTAINS(output, "max<int>(10, 20) = 20", "Expected max<int> result");
            INTEGRATION_ASSERT_CONTAINS(output, "min<int>(10, 20) = 10", "Expected min<int> result");
            INTEGRATION_ASSERT_CONTAINS(output, "max<long>(50, 75) = 75", "Expected max<long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "min<long>(50, 75) = 50", "Expected min<long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed! ===", "Expected final success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Function Comprehensive", "function_comprehensive.cb", execution_time);
    
    // Test 15: Generic Enum - Simple Access
    run_cb_test_with_output_and_time("../../tests/cases/generics/enum_simple_access.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "enum_simple_access.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Created enum variable", "Expected enum creation message");
            INTEGRATION_ASSERT_CONTAINS(output, "x.value = 42", "Expected enum value access");
        }, execution_time);
    integration_test_passed_with_time("Generic Enum Simple Access", "enum_simple_access.cb", execution_time);
    
    // Test 16: Generic Enum - Initialization Test
    run_cb_test_with_output_and_time("../../tests/cases/generics/enum_init_test.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "enum_init_test.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Option<int> x = Some(42); works", "Expected initialization success");
        }, execution_time);
    integration_test_passed_with_time("Generic Enum Initialization", "enum_init_test.cb", execution_time);
    
    // Test 17: Generic Enum - Comprehensive
    run_cb_test_with_output_and_time("../../tests/cases/generics/enum_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "enum_comprehensive.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Comprehensive Enum Test Suite ===", "Expected test suite header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - Basic Creation (Some):", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Created Option<int> with Some(42)", "Expected Some creation");
            INTEGRATION_ASSERT_CONTAINS(output, "opt1.value = 42", "Expected opt1 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - Basic Creation (None):", "Expected test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Created Option<int> with None", "Expected None creation");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3 - Multiple Instances:", "Expected test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "a.value = 10", "Expected instance a");
            INTEGRATION_ASSERT_CONTAINS(output, "b.value = 20", "Expected instance b");
            INTEGRATION_ASSERT_CONTAINS(output, "c.value = 30", "Expected instance c");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4 - Various Values:", "Expected test 4");
            INTEGRATION_ASSERT_CONTAINS(output, "large.value = 999999", "Expected large value");
            INTEGRATION_ASSERT_CONTAINS(output, "zero.value = 0", "Expected zero value");
            INTEGRATION_ASSERT_CONTAINS(output, "negative.value = -42", "Expected negative value");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5 - Reassignment:", "Expected test 5");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6 - Mixed Some/None:", "Expected test 6");
            INTEGRATION_ASSERT_CONTAINS(output, "mix1.value = 111", "Expected mix1 value");
            INTEGRATION_ASSERT_CONTAINS(output, "mix3.value = 222", "Expected mix3 value");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Completed Successfully ===", "Expected completion message");
        }, execution_time);
    integration_test_passed_with_time("Generic Enum Comprehensive", "enum_comprehensive.cb", execution_time);
    
    // Test 18: Pointer Type
    run_cb_test_with_output_and_time("../../tests/cases/generics/pointer_type.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "pointer_type.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Pointer value:", "Expected pointer value header");
            INTEGRATION_ASSERT_CONTAINS(output, "42", "Expected pointer value");
            INTEGRATION_ASSERT_CONTAINS(output, "Pointer generic test passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Pointer Type", "pointer_type.cb", execution_time);
    
    // Test 19: Complex Pointer Type
    run_cb_test_with_output_and_time("../../tests/cases/generics/complex_pointer_type.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "complex_pointer_type.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Double pointer value:", "Expected double pointer header");
            INTEGRATION_ASSERT_CONTAINS(output, "100", "Expected double pointer value");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex pointer generic test passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Complex Pointer Type", "complex_pointer_type.cb", execution_time);
    
    // Test 20: Complex Types
    run_cb_test_with_output_and_time("../../tests/cases/generics/complex_types.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "complex_types.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "Simple container:", "Expected simple container header");
            INTEGRATION_ASSERT_CONTAINS(output, "count =", "Expected count field");
            INTEGRATION_ASSERT_CONTAINS(output, "data =", "Expected data field");
            INTEGRATION_ASSERT_CONTAINS(output, "999", "Expected data value");
            INTEGRATION_ASSERT_CONTAINS(output, "Basic type test passed!", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Complex Types", "complex_types.cb", execution_time);
    
    std::cout << "[integration-test] Generics tests completed (20 tests)" << std::endl;
}

} // namespace GenericsTests
