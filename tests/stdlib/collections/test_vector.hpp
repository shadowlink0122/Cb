#ifndef TEST_VECTOR_HPP
#define TEST_VECTOR_HPP

#include "../framework/stdlib_test_framework.hpp"
#include <iostream>
#include <string>

/**
 * @brief Vector stdlib test
 * 
 * Tests the Vector library from stdlib/collections/vector.cb
 * using the test file: tests/cases/stdlib/collections/test_vector_import.cb
 * 
 * This test verifies:
 * 1. Vector import with generic types
 * 2. Basic operations (push, pop)
 * 3. Resize functionality
 * 4. Destructor behavior
 */

inline void test_vector_import_comprehensive() {
    std::cout << "[Collections] Testing Vector library import...\n";
    
    // Run the Cb test file
    auto [output, exit_code] = run_cb_test(
        "tests/cases/stdlib/collections/test_vector_import.cb");
    
    // Verify exit code (should be 0 for success)
    STDLIB_ASSERT_EQ(0, exit_code);
    
    // Verify output contains expected test results
    STDLIB_ASSERT_CONTAINS(output, "Vector Library Import Test");
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Vector Import ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Vector import and struct creation successful");
    
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Vector Basic Operations ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Vector push operations work");
    STDLIB_ASSERT_CONTAINS(output, "✅ Vector pop operations work");
    
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Vector Resize ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Vector resize works");
    STDLIB_ASSERT_CONTAINS(output, "✅ Vector operations after resize work");
    
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Vector Destructor ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Vector destructor works");
    
    STDLIB_ASSERT_CONTAINS(output, "All Vector library tests passed!");
}

inline void test_vector_selective_import() {
    std::cout << "[Collections] Testing Vector selective import...\n";
    
    // Run the selective import test
    auto [output, exit_code] = run_cb_test(
        "tests/cases/stdlib/collections/test_selective_import.cb");
    
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "✅ Default import works");
    STDLIB_ASSERT_CONTAINS(output, "✅ Selective import works");
}

inline void test_vector_advanced_selective() {
    std::cout << "[Collections] Testing Vector advanced selective import...\n";
    
    // Run the advanced selective import test
    auto [output, exit_code] = run_cb_test(
        "tests/cases/stdlib/collections/test_advanced_selective.cb");
    
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "✅ Multiple selective import works");
    STDLIB_ASSERT_CONTAINS(output, "✅ Single selective import works");
}

// Register all Vector tests
inline void register_vector_tests(StdlibTestRunner& runner) {
    runner.add_test("vector_import_comprehensive", test_vector_import_comprehensive);
    runner.add_test("vector_selective_import", test_vector_selective_import);
    runner.add_test("vector_advanced_selective", test_vector_advanced_selective);
}

#endif // TEST_VECTOR_HPP
