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
            INTEGRATION_ASSERT_CONTAINS(output, "MaybeValue<int> x = Some(42); works", "Expected initialization success");
        }, execution_time);
    integration_test_passed_with_time("Generic Enum Initialization", "enum_init_test.cb", execution_time);
    
    // Test 17: Generic Enum - Comprehensive
    run_cb_test_with_output_and_time("../../tests/cases/generics/enum_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "enum_comprehensive.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Comprehensive Enum Test Suite ===", "Expected test suite header");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1 - Basic Creation (Some):", "Expected test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "Created MaybeValue<int> with Some(42)", "Expected Some creation");
            INTEGRATION_ASSERT_CONTAINS(output, "opt1.value = 42", "Expected opt1 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2 - Basic Creation (None):", "Expected test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Created MaybeValue<int> with None", "Expected None creation");
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
    
    // Test 21: Recursive Generic Struct
    run_cb_test_with_output_and_time("../../tests/cases/generics/recursive_struct.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "recursive_struct.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Recursive Generic Struct Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Node 3 value: 30", "Expected node 3 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Node 2 value: 20", "Expected node 2 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Node 1 value: 10", "Expected node 1 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Long node 2 value: 200", "Expected long node 2 value");
            INTEGRATION_ASSERT_CONTAINS(output, "Long node 1 value: 100", "Expected long node 1 value");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Recursive Generic Struct Test Passed ===", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Recursive Generic Struct", "recursive_struct.cb", execution_time);
    
    // Test 22: Generic Struct as Function Parameter
    run_cb_test_with_output_and_time("../../tests/cases/generics/struct_as_function_param.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "struct_as_function_param.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Generic Struct as Function Parameter Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Extracted from Box<int>: 42", "Expected extract<int> result");
            INTEGRATION_ASSERT_CONTAINS(output, "Wrapped Box<long> value: 999", "Expected wrap_twice<long> result");
            INTEGRATION_ASSERT_CONTAINS(output, "get_first<int, long>: 10", "Expected get_first result");
            INTEGRATION_ASSERT_CONTAINS(output, "get_second<int, long>: 20", "Expected get_second result");
            INTEGRATION_ASSERT_CONTAINS(output, "Swapped pair: ( 20 , 10 )", "Expected swap_pair result");
            INTEGRATION_ASSERT_CONTAINS(output, "Box contains: 42", "Expected print_box result");
            INTEGRATION_ASSERT_CONTAINS(output, "Pair: ( 10 , 20 )", "Expected print_pair result");
            INTEGRATION_ASSERT_CONTAINS(output, "Nested call result: 123", "Expected nested call result");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Passed ===", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Generic Struct as Function Parameter", "struct_as_function_param.cb", execution_time);
    
    // Test 23: Advanced Nested Generic Struct
    run_cb_test_with_output_and_time("../../tests/cases/generics/advanced_nested_struct.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "advanced_nested_struct.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Advanced Nested Generic Struct Test ===", "Expected test header");
            INTEGRATION_ASSERT_CONTAINS(output, "Outer<int>.inner.data: 42", "Expected outer.inner.data");
            INTEGRATION_ASSERT_CONTAINS(output, "Outer<int>.direct: 43", "Expected outer.direct");
            INTEGRATION_ASSERT_CONTAINS(output, "get_inner_data<int>: 42", "Expected get_inner_data result");
            INTEGRATION_ASSERT_CONTAINS(output, "make_outer<long>.inner.data: 999", "Expected make_outer inner.data");
            INTEGRATION_ASSERT_CONTAINS(output, "make_outer<long>.direct: 999", "Expected make_outer direct");
            INTEGRATION_ASSERT_CONTAINS(output, "Container chain:  20  -> 10", "Expected container chain");
            INTEGRATION_ASSERT_CONTAINS(output, "Triple middle value: 2", "Expected triple middle value");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex nested value: 777", "Expected complex nested value");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Advanced Tests Passed ===", "Expected success message");
        }, execution_time);
    integration_test_passed_with_time("Advanced Nested Generic Struct", "advanced_nested_struct.cb", execution_time);
    
    // Test 24: Nested generics with Option and Result
    run_cb_test_with_output_and_time("../cases/generics/test_nested_generics_simple.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_nested_generics_simple.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 1: Option<Result<int, string> > ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Case 1: Some(Ok(42)) - Success", "Should handle Some(Ok)");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Case 2: Some(Err) - Success", "Should handle Some(Err)");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Case 3: None - Success", "Should handle None");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All nested generic tests passed ===", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("Nested generics with Option/Result", "test_nested_generics_simple.cb", execution_time);
    
    // Test 25: Generic Struct Array Comprehensive
    run_cb_test_with_output_and_time("../../tests/cases/generics/test_generic_struct_array_comprehensive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_generic_struct_array_comprehensive.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Comprehensive Generic Struct Array Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 1a: Container<int> Basic Operations ---", "Should have Test 1a");
            INTEGRATION_ASSERT_CONTAINS(output, "int_container.items[0] = 10", "Should set items[0]");
            INTEGRATION_ASSERT_CONTAINS(output, "int_container.items[4] = 50", "Should set items[4]");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 1a passed!", "Test 1a should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 1b: Container<long> Operations ---", "Should have Test 1b");
            INTEGRATION_ASSERT_CONTAINS(output, "long_container.items[0] = 100", "Should set long items");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ Test 1b passed!", "Test 1b should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 1c: Container<string> Operations ---", "Should have Test 1c");
            INTEGRATION_ASSERT_CONTAINS(output, "str_container.items[0] = Hello", "Should handle string arrays");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 2: Matrix<int> Operations ---", "Should have Test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "Matrix row1: [1, 2, 3]", "Should display matrix");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 3: Wrapper<int> with Container ---", "Should have Test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "Wrapper<int> initialized successfully", "Should initialize wrapper");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 4: Pair<int, long> with Arrays ---", "Should have Test 4");
            INTEGRATION_ASSERT_CONTAINS(output, "first_pair: [10, 20]", "Should handle multiple type params");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 5: Loop Operations on Container<int> ---", "Should have Test 5");
            INTEGRATION_ASSERT_CONTAINS(output, "Sum of all items: 100", "Should calculate sum");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 6: Array Element Copy Operations ---", "Should have Test 6");
            INTEGRATION_ASSERT_CONTAINS(output, "dst_container.items[0] = 777", "Should copy elements");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 7: Multiple Instances ---", "Should have Test 7");
            INTEGRATION_ASSERT_CONTAINS(output, "c1.items[0] = 11", "Should handle multiple instances");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 8: Zero Initialization ---", "Should have Test 8");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 9: Boundary Access ---", "Should have Test 9");
            INTEGRATION_ASSERT_CONTAINS(output, "First element: 1", "Should access boundaries");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 10: Update Operations ---", "Should have Test 10");
            INTEGRATION_ASSERT_CONTAINS(output, "After increment: 250", "Should update values");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Comprehensive Generic Struct Array Tests Passed! ===", "Should pass all tests");
            INTEGRATION_ASSERT_CONTAINS(output, "Total: 10 test sections", "Should complete 10 sections");
        }, execution_time);
    integration_test_passed_with_time("Generic Struct Array Comprehensive", "test_generic_struct_array_comprehensive.cb", execution_time);
    
    // Test 26: Array of Generic Structs
    run_cb_test_with_output_and_time("../../tests/cases/generics/test_array_of_generic_structs.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_array_of_generic_structs.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Array of Generic Structs Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 1: Array of Box<int> ---", "Should have Test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "int_boxes[0].value = 10", "Should handle Box<int> array");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 2: Array of Box<long> ---", "Should have Test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "long_boxes[0].value = 100", "Should handle Box<long> array");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 3: Array of Box<string> ---", "Should have Test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "str_boxes[0].value = First", "Should handle Box<string> array");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 4: Array of Pair<int, long> ---", "Should have Test 4");
            INTEGRATION_ASSERT_CONTAINS(output, "pairs[0]: (10, 100)", "Should handle Pair array");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 5: Loop Operations ---", "Should have Test 5");
            INTEGRATION_ASSERT_CONTAINS(output, "loop_boxes[0].value = 0", "Should loop over arrays");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 6: Array of Item<int> ---", "Should have Test 6");
            INTEGRATION_ASSERT_CONTAINS(output, "items[0]: data=42, id=1, name=Item1", "Should handle complex items");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 7: Copy Operations ---", "Should have Test 7");
            INTEGRATION_ASSERT_CONTAINS(output, "dst_boxes[0].value = 111", "Should copy array elements");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 8: Update Operations ---", "Should have Test 8");
            INTEGRATION_ASSERT_CONTAINS(output, "After: [100, 200]", "Should update elements");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 9: Swap Elements ---", "Should have Test 9");
            INTEGRATION_ASSERT_CONTAINS(output, "After swap: [999, 111]", "Should swap elements");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 10: Find Max Value ---", "Should have Test 10");
            INTEGRATION_ASSERT_CONTAINS(output, "Max value: 50", "Should find max");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Array of Generic Structs Tests Passed! ===", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("Array of Generic Structs", "test_array_of_generic_structs.cb", execution_time);
    
    // Test 27: Generic Functions with Arrays
    run_cb_test_with_output_and_time("../../tests/cases/generics/test_generic_functions_with_arrays.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_generic_functions_with_arrays.cb should execute successfully");
            INTEGRATION_ASSERT_CONTAINS(output, "=== Generic Functions with Arrays Test ===", "Should contain test header");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 1: Identity Function ---", "Should have Test 1");
            INTEGRATION_ASSERT_CONTAINS(output, "identity<int>(42) = 42", "Should use identity function");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 2: Container<int> Operations ---", "Should have Test 2");
            INTEGRATION_ASSERT_CONTAINS(output, "items[0] = 111", "Should access container items");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 3: Direct Array Access ---", "Should have Test 3");
            INTEGRATION_ASSERT_CONTAINS(output, "First element: 777", "Should access arrays directly");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 4: Long Container ---", "Should have Test 4");
            INTEGRATION_ASSERT_CONTAINS(output, "First long element: 999", "Should handle long containers");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 5: Make Box from Array Element ---", "Should have Test 5");
            INTEGRATION_ASSERT_CONTAINS(output, "box1.value = 42", "Should make boxes from array elements");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 6: Max with Container Elements ---", "Should have Test 6");
            INTEGRATION_ASSERT_CONTAINS(output, "Max value: 50", "Should find max value");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 7: Multiple Container Types ---", "Should have Test 7");
            INTEGRATION_ASSERT_CONTAINS(output, "c1 first: 100", "Should handle multiple types");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 8: Chained Operations ---", "Should have Test 8");
            INTEGRATION_ASSERT_CONTAINS(output, "In box: 1", "Should chain operations");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 9: Array Loop with Generic Functions ---", "Should have Test 9");
            INTEGRATION_ASSERT_CONTAINS(output, "Sum of all items: 100", "Should loop with generics");
            INTEGRATION_ASSERT_CONTAINS(output, "--- Test 10: Complex Scenario ---", "Should have Test 10");
            INTEGRATION_ASSERT_CONTAINS(output, "Result in box: 40", "Should handle complex scenarios");
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Generic Functions with Arrays Tests Passed! ===", "Should pass all tests");
        }, execution_time);
    integration_test_passed_with_time("Generic Functions with Arrays", "test_generic_functions_with_arrays.cb", execution_time);
    
    std::cout << "[integration-test] Generics tests completed (27 tests)" << std::endl;
}

} // namespace GenericsTests
