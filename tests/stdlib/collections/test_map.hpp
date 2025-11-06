#ifndef TEST_MAP_HPP
#define TEST_MAP_HPP

#include "../framework/stdlib_test_framework.hpp"
#include <iostream>
#include <string>

/**
 * @brief Map<K, V> stdlib test
 * 
 * Tests the Map<K, V> library from stdlib/collections/map.cb
 * using the test file: tests/cases/stdlib/collections/test_map.cb
 * 
 * This test verifies:
 * 1. Basic operations (insert, get, contains, size)
 * 2. Update operations (overwrite existing keys)
 * 3. Delete operations (remove, try_remove)
 * 4. Large dataset (100 elements with AVL balancing)
 * 5. Clear operations
 * 6. String key support (Map<string, V>)
 * 7. Error handling with get() method and default values
 * 8. Multiple type combinations (Map<int,int>, Map<string,int>, etc.)
 */

inline void test_map_comprehensive() {
    std::cout << "[Collections] Testing Map<K, V> comprehensive operations...\n";
    
    // Run the Cb test file
    auto [output, exit_code] = run_cb_test(
        "tests/cases/stdlib/collections/map/test_basic.cb");
    
    // Verify exit code (should be 0 for success)
    STDLIB_ASSERT_EQ(0, exit_code);
    
    // Verify output contains expected test results
    STDLIB_ASSERT_CONTAINS(output, "Map<K, V> Comprehensive Test Suite");
    STDLIB_ASSERT_CONTAINS(output, "New API with Error Handling");
    
    // Test 1: Basic Operations
    STDLIB_ASSERT_CONTAINS(output, "=== Test 1: Basic Operations ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ Key 1 exists");
    STDLIB_ASSERT_CONTAINS(output, "✓ Value for key 2 is correct");
    STDLIB_ASSERT_CONTAINS(output, "✓ Key 99 does not exist");
    STDLIB_ASSERT_CONTAINS(output, "✓ Size is correct: 3");
    
    // Test 2: Update Existing Key
    STDLIB_ASSERT_CONTAINS(output, "=== Test 2: Update Existing Key ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ Updated value correctly");
    STDLIB_ASSERT_CONTAINS(output, "✓ Size remains 1 after update");
    
    // Test 3: Delete Keys
    STDLIB_ASSERT_CONTAINS(output, "=== Test 3: Delete Keys ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ Key 20 no longer exists");
    STDLIB_ASSERT_CONTAINS(output, "✓ Other keys still exist");
    STDLIB_ASSERT_CONTAINS(output, "✓ Size is now 2");
    
    // Test 4: Many Elements (AVL Balance)
    STDLIB_ASSERT_CONTAINS(output, "=== Test 4: Many Elements (AVL Balance) ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ Inserted 100 elements");
    STDLIB_ASSERT_CONTAINS(output, "✓ All 100 keys exist");
    STDLIB_ASSERT_CONTAINS(output, "✓ Value for key 50 is correct");
    
    // Test 5: Clear
    STDLIB_ASSERT_CONTAINS(output, "=== Test 5: Clear ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ Map is empty after clear");
    STDLIB_ASSERT_CONTAINS(output, "✓ Size is 0");
    
    // Test 6: String Values
    STDLIB_ASSERT_CONTAINS(output, "=== Test 6: String Values ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ Value for key 2: two");
    STDLIB_ASSERT_CONTAINS(output, "✓ Size is correct: 3");
    
    // Test 7: Error Handling with get()
    STDLIB_ASSERT_CONTAINS(output, "=== Test 7: Error Handling with get() ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ get('Alice', -1) returns 100");
    STDLIB_ASSERT_CONTAINS(output, "✓ get('Bob', -1) returns 0");
    STDLIB_ASSERT_CONTAINS(output, "✓ get('Charlie', -1) returns -1 (not found)");
    STDLIB_ASSERT_CONTAINS(output, "✓ contains() + get() pattern works");
    
    // Test 8: try_remove() Method
    STDLIB_ASSERT_CONTAINS(output, "=== Test 8: try_remove() Method ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ try_remove() returns true for existing key");
    STDLIB_ASSERT_CONTAINS(output, "✓ try_remove() returns false for non-existing key");
    
    // Test 9: String Keys
    STDLIB_ASSERT_CONTAINS(output, "=== Test 9: String Keys ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ String key 'hello' works");
    STDLIB_ASSERT_CONTAINS(output, "✓ Empty string key works");
    STDLIB_ASSERT_CONTAINS(output, "✓ Non-existing string key returns default");
    STDLIB_ASSERT_CONTAINS(output, "✓ Size is correct: 3");
    
    // Test 10: Multiple Type Combinations
    STDLIB_ASSERT_CONTAINS(output, "=== Test 10: Multiple Type Combinations ===");
    STDLIB_ASSERT_CONTAINS(output, "✓ Map<int, int> works");
    STDLIB_ASSERT_CONTAINS(output, "✓ Map<string, int> works");
    STDLIB_ASSERT_CONTAINS(output, "✓ Map<int, string> works");
    STDLIB_ASSERT_CONTAINS(output, "✓ Map<string, string> works");
    
    // Final success message
    STDLIB_ASSERT_CONTAINS(output, "✓ All Map<K, V> tests passed!");
}

// Register all Map tests
inline void register_map_tests(StdlibTestRunner& runner) {
    runner.add_test("map_comprehensive", test_map_comprehensive);
}

#endif // TEST_MAP_HPP
