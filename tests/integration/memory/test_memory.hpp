#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_memory() {
    std::cout << "[integration-test] Running Memory Management tests..." << std::endl;
    
    // Test 0: 基本的なnew/delete機能（新規）
    double execution_time_newdelete_basic;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_new_delete_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_new_delete_basic.cb should execute successfully");
            
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Primitive types", "Should test primitive types");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Struct types", "Should test struct types");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Arrays", "Should test arrays");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: sizeof", "Should test sizeof");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: nullptr handling", "Should test nullptr");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: Zero initialization", "Should test zero-init");
            INTEGRATION_ASSERT_CONTAINS(output, "ALL 6 TESTS PASSED!", "All basic tests should pass");
        }, execution_time_newdelete_basic);
    integration_test_passed_with_time("new/delete basic operations", "test_new_delete_basic.cb", execution_time_newdelete_basic);
    
    // Test 1: 基本的なnew/delete/sizeof
    double execution_time_basic;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_new_delete_sizeof.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_new_delete_sizeof.cb should execute successfully");
            
            // sizeof テスト
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(int) = 4", "sizeof(int) should be 4 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(double) = 8", "sizeof(double) should be 8 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(char) = 1", "sizeof(char) should be 1 byte");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(bool) = 1", "sizeof(bool) should be 1 byte");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(long) = 8", "sizeof(long) should be 8 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(short) = 2", "sizeof(short) should be 2 bytes");
            
            // 構造体サイズ
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Point) = 8", "sizeof(Point) should be 8 bytes (int x + int y)");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Rectangle) = 24", "sizeof(Rectangle) should be 24 bytes");
            
            // new/delete テスト
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated int*", "Should allocate int pointer");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed ptr", "Should free pointer");
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated int[10]", "Should allocate int array");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed array", "Should free array");
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Point*", "Should allocate Point struct");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed Point", "Should free Point struct");
            
            // 16進数表示の確認
            INTEGRATION_ASSERT_CONTAINS(output, "0x", "Pointer should be displayed in hexadecimal");
            
            // 完了メッセージ
            INTEGRATION_ASSERT_CONTAINS(output, "All tests passed!", "All tests should pass");
        }, execution_time_basic);
    integration_test_passed_with_time("basic new/delete/sizeof", "test_new_delete_sizeof.cb", execution_time_basic);
    
    // Test 2: 高度なsizeof機能
    double execution_time_advanced;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_sizeof_advanced.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_sizeof_advanced.cb should execute successfully");
            
            // typedef テスト
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(int) = 4", "sizeof(int) should match");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Integer) = 4", "typedef Integer should match int");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Point) = 8", "Point struct size");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(PointAlias) = 8", "typedef PointAlias should match Point");
            INTEGRATION_ASSERT_CONTAINS(output, "typedef test passed", "typedef tests should pass");
            
            // ネスト構造体テスト
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Line) = 16", "Line should be 16 bytes (Point + Point)");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Shape) = 40", "Shape should be 40 bytes (Line + Line + int + padding)");
            INTEGRATION_ASSERT_CONTAINS(output, "nested struct test passed", "Nested struct tests should pass");
            
            // ジェネリクス構造体テスト
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Box<int>)", "Generic struct sizeof should work");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Box<long>)", "Generic struct with different types");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Box<Point>)", "Generic struct with struct type");
            INTEGRATION_ASSERT_CONTAINS(output, "generic struct test completed", "Generic tests should complete");
            
            // malloc/sizeof統合テスト
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Point[3]", "Should allocate Point array");
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Shape", "Should allocate Shape struct");
            INTEGRATION_ASSERT_CONTAINS(output, "size: 40 bytes", "Shape size should be 40 bytes");
            
            // 完了メッセージ
            INTEGRATION_ASSERT_CONTAINS(output, "All advanced tests passed!", "All advanced tests should pass");
        }, execution_time_advanced);
    integration_test_passed_with_time("advanced sizeof features", "test_sizeof_advanced.cb", execution_time_advanced);
    
    // Test 3: エッジケース
    double execution_time_edge;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_memory_edge_cases.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_memory_edge_cases.cb should execute successfully");
            
            // ポインタサイズテスト
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(int*) = 8", "int pointer should be 8 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(double*) = 8", "double pointer should be 8 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Node*) = 8", "struct pointer should be 8 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "All pointer sizes correct", "All pointers should be 8 bytes");
            
            // 自己参照構造体
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Node)", "Self-referencing struct size");
            INTEGRATION_ASSERT_CONTAINS(output, "Self-referencing struct size correct", "Node size calculation");
            
            // 大きな配列
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated int[1000]", "Large array allocation");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed large array", "Large array deallocation");
            
            // 構造体配列
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Node[5]", "Struct array allocation");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed struct array", "Struct array deallocation");
            
            // ジェネリクス割り当て (not yet fully supported)
            INTEGRATION_ASSERT_CONTAINS(output, "generic allocation test", "Generic test section exists");
            
            // 式のsizeof
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(int)", "sizeof on type");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(n)", "sizeof on variable");
            
            // 複数割り当て
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated 3 pointers", "Multiple allocations");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed all 3 pointers", "Multiple deallocations");
            
            // ネストされた構造体配列
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Node[3]", "Nested struct array");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed nested struct array", "Nested struct deallocation");
            
            // 完了メッセージ
            INTEGRATION_ASSERT_CONTAINS(output, "All edge case tests passed!", "All edge cases should pass");
        }, execution_time_edge);
    integration_test_passed_with_time("memory edge cases", "test_memory_edge_cases.cb", execution_time_edge);
    
    // Test 4: memcpy基本機能
    double execution_time_memcpy_verify;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_memcpy_verify.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_memcpy_verify.cb should execute successfully");
            
            INTEGRATION_ASSERT_CONTAINS(output, "Before: src=(100, 200), dest=(0, 0)", 
                "Should show initial values");
            INTEGRATION_ASSERT_CONTAINS(output, "After: src=(100, 200), dest=(100, 200)", 
                "Should show copied values");
            INTEGRATION_ASSERT_CONTAINS(output, "✅ PASS: memcpy works correctly", 
                "Should confirm memcpy success");
        }, execution_time_memcpy_verify);
    integration_test_passed_with_time("memcpy verification", "test_memcpy_verify.cb", execution_time_memcpy_verify);
    
    // Test 5: memcpy基本テスト
    double execution_time_memcpy_basic;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_memcpy_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_memcpy_basic.cb should execute successfully");
            
            // Test 1: 基本的なコピー
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Basic struct copy - PASSED", 
                "Basic struct copy should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Source: x=100, y=200", 
                "Source values should be correct");
            INTEGRATION_ASSERT_CONTAINS(output, "Dest:   x=100, y=200", 
                "Dest values should be copied correctly");
            
            // Test 2: データ独立性
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Data independence - PASSED", 
                "Data independence test should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Source: x=999, y=888", 
                "Modified source values");
            INTEGRATION_ASSERT_CONTAINS(output, "Dest:   x=10, y=20", 
                "Dest should remain unchanged");
            
            // Test 3: 複数のコピー
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: Multiple struct copies - PASSED", 
                "Multiple copies should pass");
            INTEGRATION_ASSERT_CONTAINS(output, "Dest1: (1, 2)", "First copy should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Dest2: (3, 4)", "Second copy should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Dest3: (5, 6)", "Third copy should work");
            
            // Test 4: サイズ0
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: Zero-size copy - PASSED", 
                "Zero-size copy should pass");
            
            // 完了メッセージ
            INTEGRATION_ASSERT_CONTAINS(output, "All memcpy tests passed!", 
                "All tests should complete successfully");
        }, execution_time_memcpy_basic);
    integration_test_passed_with_time("memcpy basic operations", "test_memcpy_basic.cb", execution_time_memcpy_basic);
    
    // Test 6: 配列アクセス関数
    double execution_time_array_access;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_array_access.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_array_access.cb should execute successfully");
            
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: Basic array access - PASSED", 
                "Basic array access should work");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: Multiple elements - PASSED", 
                "Multiple element access should work");
            INTEGRATION_ASSERT_CONTAINS(output, "All array access tests passed!", 
                "All array access tests should complete");
        }, execution_time_array_access);
    integration_test_passed_with_time("array access functions", "test_array_access.cb", execution_time_array_access);
    
    // Test 7: memcpy包括的テスト
    double execution_time_memcpy_comprehensive;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_memcpy_comprehensive.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_memcpy_comprehensive.cb should execute successfully");
            
            // 10個のテストがすべて実行されたことを確認
            INTEGRATION_ASSERT_CONTAINS(output, "Test 1: memcpy Point Struct", 
                "Test 1 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 2: memcpy Point Struct", 
                "Test 2 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 3: memcpy Data Struct", 
                "Test 3 should be executed (double member test)");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 4: memcpy Multiple Structs", 
                "Test 4 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 5: Copy Independence", 
                "Test 5 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 6: Overwrite Existing Data", 
                "Test 6 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 7: Zero Values", 
                "Test 7 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 8: Negative Values", 
                "Test 8 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 9: Large Values", 
                "Test 9 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "Test 10: Multiple Different Types", 
                "Test 10 should be executed");
            
            // すべてのテストが成功したことを確認
            INTEGRATION_ASSERT_CONTAINS(output, "✅ ALL 10 TESTS COMPLETED!", 
                "All 10 tests should complete successfully");
            
            // 各種データ型のテストを確認
            INTEGRATION_ASSERT_CONTAINS(output, "Single primitives (int)", 
                "Should test primitive types");
            INTEGRATION_ASSERT_CONTAINS(output, "Simple structs (Point)", 
                "Should test simple structs");
            INTEGRATION_ASSERT_CONTAINS(output, "Complex structs (Data with mixed types)", 
                "Should test complex structs with double members");
            INTEGRATION_ASSERT_CONTAINS(output, "Multiple independent copies", 
                "Should test multiple copies");
            INTEGRATION_ASSERT_CONTAINS(output, "Copy independence", 
                "Should test copy independence");
        }, execution_time_memcpy_comprehensive);
    integration_test_passed_with_time("memcpy comprehensive (10 tests)", "test_memcpy_comprehensive.cb", execution_time_memcpy_comprehensive);
    
    // Test 8: ジェネリクス + メモリ統合テスト
    double execution_time_generic_memory;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_generic_memory_integration.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_generic_memory_integration.cb should execute successfully");
            
            // 10個のテストがすべて実行されたことを確認
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 1] Vector<int> basic operations", 
                "Test 1 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 2] Vector<long> with sizeof", 
                "Test 2 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 3] Queue<int> basic operations", 
                "Test 3 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 4] Vector<Point> with struct type", 
                "Test 4 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 5] Queue<long> with large numbers", 
                "Test 5 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 6] sizeof() with generic types", 
                "Test 6 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 7] Memory leak prevention", 
                "Test 7 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 8] new/delete with Point struct", 
                "Test 8 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 9] memcpy() with Point struct", 
                "Test 9 should be executed");
            INTEGRATION_ASSERT_CONTAINS(output, "[Test 10] malloc/free basic operations", 
                "Test 10 should be executed");
            
            // すべてのテストが成功したことを確認
            INTEGRATION_ASSERT_CONTAINS(output, "ALL TESTS PASSED!", 
                "All 10 tests should complete successfully");
            
            // 主要な機能を確認
            INTEGRATION_ASSERT_CONTAINS(output, "Vector<T> and Queue<T> basic operations", 
                "Should verify generic struct operations");
            INTEGRATION_ASSERT_CONTAINS(output, "Generic structs with primitive and struct types", 
                "Should verify type parameter variations");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof() consistency", 
                "Should verify sizeof() with generics");
            INTEGRATION_ASSERT_CONTAINS(output, "Constructor/destructor memory management", 
                "Should verify RAII memory management");
        }, execution_time_generic_memory);
    integration_test_passed_with_time("generic structs + memory (10 tests)", "test_generic_memory_integration.cb", execution_time_generic_memory);
    
    // Test 9: エラーケース（個別テスト）
    // Note: これらのテストは意図的にエラーを引き起こすため、
    // 通常のintegration testからは除外し、別途手動テストを行う
    std::cout << "[integration-test] Error case tests (manual validation):" << std::endl;
    std::cout << "[integration-test]   - errors/double_delete.cb (expected: error)" << std::endl;
    std::cout << "[integration-test]   - errors/use_after_delete.cb (expected: error)" << std::endl;
    std::cout << "[integration-test]   - errors/delete_uninitialized.cb (expected: error)" << std::endl;
    std::cout << "[integration-test]   - errors/memory_leak_detection.cb (expected: leak warning)" << std::endl;
    std::cout << "[integration-test]   - errors/dangling_pointer_return.cb (expected: error)" << std::endl;
    std::cout << "[integration-test]   - errors/invalid_pointer_arithmetic.cb (expected: error)" << std::endl;
    std::cout << "[integration-test] (Error tests are validated separately to avoid test suite crashes)" << std::endl;
    
    std::cout << "[integration-test] Memory Management tests completed" << std::endl;
}
