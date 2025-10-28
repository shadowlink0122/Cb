#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_memory() {
    std::cout << "[integration-test] Running Memory Management tests..." << std::endl;
    
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
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Rectangle) = 20", "sizeof(Rectangle) should be 20 bytes");
            
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
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Shape) = 36", "Shape should be 36 bytes (Line + Line + int)");
            INTEGRATION_ASSERT_CONTAINS(output, "nested struct test passed", "Nested struct tests should pass");
            
            // ジェネリクス構造体テスト
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Box<int>)", "Generic struct sizeof should work");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Box<long>)", "Generic struct with different types");
            INTEGRATION_ASSERT_CONTAINS(output, "sizeof(Box<Point>)", "Generic struct with struct type");
            INTEGRATION_ASSERT_CONTAINS(output, "generic struct test completed", "Generic tests should complete");
            
            // malloc/sizeof統合テスト
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Point[3]", "Should allocate Point array");
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Shape", "Should allocate Shape struct");
            INTEGRATION_ASSERT_CONTAINS(output, "size: 36 bytes", "Shape size should be 36 bytes");
            
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
            INTEGRATION_ASSERT_CONTAINS(output, "Allocated Rectangle[3]", "Nested struct array");
            INTEGRATION_ASSERT_CONTAINS(output, "Freed nested struct array", "Nested struct deallocation");
            
            // 完了メッセージ
            INTEGRATION_ASSERT_CONTAINS(output, "All edge case tests passed!", "All edge cases should pass");
        }, execution_time_edge);
    integration_test_passed_with_time("memory edge cases", "test_memory_edge_cases.cb", execution_time_edge);
    
    // Test 4: エラーケースとベストプラクティス
    double execution_time_errors;
    run_cb_test_with_output_and_time("../../tests/cases/memory/test_memory_errors.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "test_memory_errors.cb should execute successfully");
            
            // nullポインタ削除
            INTEGRATION_ASSERT_CONTAINS(output, "null pointer delete (safe)", "Null delete test");
            INTEGRATION_ASSERT_CONTAINS(output, "Delete null pointer is safe", "Null delete is safe");
            
            // ゼロサイズ配列
            INTEGRATION_ASSERT_CONTAINS(output, "zero size array", "Zero size array test");
            INTEGRATION_ASSERT_CONTAINS(output, "Zero size array returns valid pointer", "Zero size allocation");
            
            // 適切なメモリ管理
            INTEGRATION_ASSERT_CONTAINS(output, "proper memory management", "Proper management pattern");
            INTEGRATION_ASSERT_CONTAINS(output, "All memory properly managed", "Memory managed correctly");
            
            // 大量割り当て
            INTEGRATION_ASSERT_CONTAINS(output, "mass allocation", "Mass allocation test");
            INTEGRATION_ASSERT_CONTAINS(output, "All 5 pointers freed successfully", "Multiple allocations");
            
            // 混合型割り当て
            INTEGRATION_ASSERT_CONTAINS(output, "mixed type allocation", "Mixed type test");
            INTEGRATION_ASSERT_CONTAINS(output, "All different types freed successfully", "Different types");
            
            // ドキュメント化されたエラーパターン
            INTEGRATION_ASSERT_CONTAINS(output, "double delete", "Double delete documented");
            INTEGRATION_ASSERT_CONTAINS(output, "Double delete would cause undefined behavior", "Double delete warning");
            INTEGRATION_ASSERT_CONTAINS(output, "dangling pointer", "Dangling pointer documented");
            INTEGRATION_ASSERT_CONTAINS(output, "Accessing dangling pointer would cause undefined behavior", "Dangling warning");
            INTEGRATION_ASSERT_CONTAINS(output, "memory leak", "Memory leak documented");
            
            // 完了メッセージ
            INTEGRATION_ASSERT_CONTAINS(output, "All safe tests passed!", "Safe tests passed");
            INTEGRATION_ASSERT_CONTAINS(output, "Unsafe patterns documented", "Unsafe patterns documented");
        }, execution_time_errors);
    integration_test_passed_with_time("memory error cases and best practices", "test_memory_errors.cb", execution_time_errors);
    
    std::cout << "[integration-test] Memory Management tests completed" << std::endl;
}
