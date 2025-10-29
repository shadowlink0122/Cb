#ifndef TEST_QUEUE_HPP
#define TEST_QUEUE_HPP

#include "../framework/stdlib_test_framework.hpp"
#include <iostream>
#include <string>

/**
 * @brief Queue<T> stdlib test
 * 
 * Tests the Queue<T> library from stdlib/collections/queue.cb
 * using the test file: tests/cases/stdlib/collections/test_queue_import.cb
 * 
 * This test verifies:
 * 1. Queue<int> import
 * 2. Basic operations (enqueue, dequeue, peek)
 * 3. Resize functionality
 * 4. Circular buffer behavior
 * 5. Destructor behavior
 */

inline void test_queue_import_comprehensive() {
    std::cout << "[Collections] Testing Queue<T> library import...\n";
    
    // Run the Cb test file
    auto [output, exit_code] = run_cb_test(
        "tests/cases/stdlib/collections/test_queue_import.cb");
    
    // Verify exit code (should be 0 for success)
    STDLIB_ASSERT_EQ(0, exit_code);
    
    // Verify output contains expected test results
    STDLIB_ASSERT_CONTAINS(output, "Queue<T> Library Import Test");
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Queue<int> Import ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> import and struct creation successful");
    
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Queue<int> Basic Operations ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> enqueue operations work");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> dequeue operations work");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> peek operations work");
    
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Queue<int> Resize ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> resize works");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> operations after resize work");
    
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Queue<int> Circular Buffer ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> circular buffer works");
    
    STDLIB_ASSERT_CONTAINS(output, "=== Test: Queue<int> Destructor ===");
    STDLIB_ASSERT_CONTAINS(output, "✅ Queue<int> destructor works");
    
    STDLIB_ASSERT_CONTAINS(output, "All Queue<int> library tests passed!");
}

inline void test_queue_simple_import() {
    std::cout << "[Collections] Testing Queue<int> simple import...\n";
    
    // Run the simple import test
    auto [output, exit_code] = run_cb_test(
        "tests/cases/stdlib/collections/test_simple_import.cb");
    
    STDLIB_ASSERT_EQ(0, exit_code);
    STDLIB_ASSERT_CONTAINS(output, "Queue<int> imported successfully!");
    STDLIB_ASSERT_CONTAINS(output, "Queue<int> variable declared!");
}

// Register all Queue tests
inline void register_queue_tests(StdlibTestRunner& runner) {
    runner.add_test("queue_import_comprehensive", test_queue_import_comprehensive);
    runner.add_test("queue_simple_import", test_queue_simple_import);
}

#endif // TEST_QUEUE_HPP
