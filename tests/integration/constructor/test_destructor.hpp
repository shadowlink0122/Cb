#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_destructor() {
    std::cout << "[integration-test] Running destructor integration tests..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/constructor/destructor_integration_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "destructor_integration_test.cb should execute successfully");
            
            // === Test 1: Single Destructor with Private Methods ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 1: Single Destructor with Private Methods ===",
                "Test 1 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] TestObject created with id: 1",
                "Constructor should be called with id 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[DISPLAY] Object id: 1",
                "Display method should show id 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Starting cleanup for id: 1",
                "Destructor should start with correct id");
            INTEGRATION_ASSERT_CONTAINS(output, "[CLEANUP] Releasing resource: 100",
                "Destructor should access member variables correctly");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Cleanup completed for id: 1",
                "Destructor should complete successfully");
            
            // === Test 2: Multiple Destructors LIFO Order ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 2: Multiple Destructors LIFO Order ===",
                "Test 2 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] TestObject created with id: 10",
                "First object should be constructed");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] TestObject created with id: 20",
                "Second object should be constructed");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] TestObject created with id: 30",
                "Third object should be constructed");
            INTEGRATION_ASSERT_CONTAINS(output, "[MAIN] All objects created",
                "All objects should be created before destruction");
            
            // LIFO順の検証: obj3(30) -> obj2(20) -> obj1(10)
            size_t pos_create_30 = output.find("[CONSTRUCTOR] TestObject created with id: 30");
            size_t pos_destroy_30 = output.find("[DESTRUCTOR] Starting cleanup for id: 30");
            size_t pos_destroy_20 = output.find("[DESTRUCTOR] Starting cleanup for id: 20");
            size_t pos_destroy_10 = output.find("[DESTRUCTOR] Starting cleanup for id: 10");
            
            INTEGRATION_ASSERT(pos_create_30 != std::string::npos && pos_destroy_30 != std::string::npos,
                "Object 30 should be constructed and destructed");
            INTEGRATION_ASSERT(pos_destroy_30 < pos_destroy_20,
                "Object 30 should be destructed before object 20 (LIFO order)");
            INTEGRATION_ASSERT(pos_destroy_20 < pos_destroy_10,
                "Object 20 should be destructed before object 10 (LIFO order)");
            
            // === Test 3: Complex Destructor Logic ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 3: Complex Destructor Logic ===",
                "Test 3 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] ComplexObject 100 created",
                "ComplexObject should be constructed");
            INTEGRATION_ASSERT_CONTAINS(output, "[USE] Object 100 used, counter: 1",
                "First use should increment counter to 1");
            INTEGRATION_ASSERT_CONTAINS(output, "[USE] Object 100 used, counter: 2",
                "Second use should increment counter to 2");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] ComplexObject 100 cleaning up",
                "Destructor should start cleanup");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Counter incremented to: 3",
                "Destructor should increment counter (first time)");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Counter incremented to: 4",
                "Destructor should increment counter (second time)");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Final counter value: 4",
                "Destructor should calculate correct final count");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] ComplexObject 100 destroyed",
                "Destructor should complete");
            
            // === Test 4: Resource Manager Cleanup ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 4: Resource Manager Cleanup ===",
                "Test 4 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] ResourceManager 1 allocated 1024 bytes",
                "First ResourceManager should allocate 1024 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] ResourceManager 2 allocated 2048 bytes",
                "Second ResourceManager should allocate 2048 bytes");
            INTEGRATION_ASSERT_CONTAINS(output, "[MAIN] Resources allocated",
                "Resources should be allocated before cleanup");
            
            // LIFO順の検証: rm2 -> rm1
            size_t pos_alloc_rm2 = output.find("[CONSTRUCTOR] ResourceManager 2 allocated 2048 bytes");
            size_t pos_destroy_rm2 = output.find("[DESTRUCTOR] ResourceManager 2 starting destruction");
            size_t pos_destroy_rm1 = output.find("[DESTRUCTOR] ResourceManager 1 starting destruction");
            
            INTEGRATION_ASSERT(pos_alloc_rm2 != std::string::npos && pos_destroy_rm2 != std::string::npos,
                "ResourceManager 2 should be allocated and destroyed");
            INTEGRATION_ASSERT(pos_destroy_rm2 < pos_destroy_rm1,
                "ResourceManager 2 should be destroyed before ResourceManager 1 (LIFO order)");
            
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Logging destruction of manager 2",
                "Destructor should log destruction for rm2");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Freeing 2048 bytes of memory",
                "Destructor should free correct amount for rm2");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] ResourceManager 2 fully destroyed",
                "rm2 should be fully destroyed");
            
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Logging destruction of manager 1",
                "Destructor should log destruction for rm1");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Freeing 1024 bytes of memory",
                "Destructor should free correct amount for rm1");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] ResourceManager 1 fully destroyed",
                "rm1 should be fully destroyed");
            
            // === Test 5: Destructor on Function End ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 5: Destructor on Function End ===",
                "Test 5 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[CONSTRUCTOR] TestObject created with id: 999",
                "Object should be constructed");
            INTEGRATION_ASSERT_CONTAINS(output, "[MAIN] Function will end, destructor will be called",
                "Function end message should appear");
            INTEGRATION_ASSERT_CONTAINS(output, "[DESTRUCTOR] Starting cleanup for id: 999",
                "Destructor should be called at function end");
            INTEGRATION_ASSERT_CONTAINS(output, "[CLEANUP] Releasing resource: 99900",
                "Destructor should release resources at function end");
            
            // === Overall Test Suite ===
            INTEGRATION_ASSERT_CONTAINS(output, "===== Destructor Integration Test Suite =====",
                "Test suite header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "===== All Integration Tests Completed =====",
                "Test suite completion message should be present");
        }, execution_time);
    
    integration_test_passed_with_time("destructor integration", "destructor_integration_test.cb", execution_time);
    
    std::cout << "[integration-test] Destructor integration tests completed" << std::endl;
}

void test_integration_destructor_simple() {
    std::cout << "[integration-test] Running simple destructor tests..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/constructor/destructor_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "destructor_test.cb should execute successfully");
            
            // Test 1: Basic Destructor
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 1: Basic Destructor ===",
                "Test 1 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "Counter constructor: value= 10",
                "Constructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "Inside function: c.value= 10",
                "Value should be accessible");
            INTEGRATION_ASSERT_CONTAINS(output, "Counter destructor: value= 10",
                "Destructor should be called at function end");
            
            // Test 2: Multiple Destructors (LIFO)
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 2: Multiple Destructors (LIFO) ===",
                "Test 2 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "All counters created",
                "All counters should be created");
            
            // LIFO順の検証: Test 2のセクション内で検証
            size_t test2_start = output.find("=== Test 2: Multiple Destructors (LIFO) ===");
            size_t test3_start = output.find("=== Test 3: Early Return ===");
            
            INTEGRATION_ASSERT(test2_start != std::string::npos && test3_start != std::string::npos,
                "Test 2 and Test 3 sections should exist");
            
            // Test 2のセクション内でのみ検索
            std::string test2_section = output.substr(test2_start, test3_start - test2_start);
            size_t pos_destroy_3 = test2_section.find("Counter destructor: value= 3");
            size_t pos_destroy_2 = test2_section.find("Counter destructor: value= 2");
            size_t pos_destroy_1 = test2_section.find("Counter destructor: value= 1");
            
            INTEGRATION_ASSERT(pos_destroy_3 != std::string::npos && 
                              pos_destroy_2 != std::string::npos && 
                              pos_destroy_1 != std::string::npos,
                "All three destructors should be called in Test 2");
            INTEGRATION_ASSERT(pos_destroy_3 < pos_destroy_2,
                "Counter 3 should be destructed before counter 2");
            INTEGRATION_ASSERT(pos_destroy_2 < pos_destroy_1,
                "Counter 2 should be destructed before counter 1");
            
            // Test 3: Early Return
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 3: Early Return ===",
                "Test 3 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "Early return",
                "Early return message should appear");
            INTEGRATION_ASSERT_CONTAINS(output, "Counter destructor: value= 100",
                "Destructor should be called before return");
            
            // Test 5: Resource Management
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 5: Resource Management ===",
                "Test 5 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "Resource  1  acquired",
                "Resource 1 should be acquired");
            INTEGRATION_ASSERT_CONTAINS(output, "Resource  2  acquired",
                "Resource 2 should be acquired");
            
            // LIFO順の検証
            size_t pos_release_2 = output.find("Resource  2  released");
            size_t pos_release_1 = output.find("Resource  1  released");
            
            INTEGRATION_ASSERT(pos_release_2 < pos_release_1,
                "Resource 2 should be released before resource 1 (LIFO)");
            
            // Test suite completion
            INTEGRATION_ASSERT_CONTAINS(output, "===== All Destructor Tests Completed =====",
                "Test suite should complete");
        }, execution_time);
    
    integration_test_passed_with_time("destructor simple", "destructor_test.cb", execution_time);
    
    std::cout << "[integration-test] Simple destructor tests completed" << std::endl;
}

void test_integration_destructor_nested_value_members() {
    std::cout << "[integration-test] Running nested value member destructor tests..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../cases/constructor/nested_value_destructor_test.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "nested_value_destructor_test.cb should execute successfully");
            
            // === Test 1: Nested Struct Value Member ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 1: Nested Struct Value Member ===",
                "Test 1 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[Outer] Constructor: id= 1",
                "Outer constructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[Outer] Destructor: id= 1",
                "Outer destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[Inner] Destructor: value= 100",
                "Inner member destructor should be called with correct value");
            
            // 破壊順序の検証: Inner member → Outer
            size_t pos_outer_destroy = output.find("[Outer] Destructor: id= 1");
            size_t pos_inner_destroy = output.find("[Inner] Destructor: value= 100");
            INTEGRATION_ASSERT(pos_outer_destroy < pos_inner_destroy,
                "Outer should be destructed before its inner member (parent first, then members)");
            
            // === Test 2: Multiple Value Members ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 2: Multiple Value Members ===",
                "Test 2 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[MultiMember] Constructor: id= 2",
                "MultiMember constructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[MultiMember] Destructor: id= 2",
                "MultiMember destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[Inner] Destructor: value= 200",
                "First inner member destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[Inner] Destructor: value= 201",
                "Second inner member destructor should be called");
            
            // 破壊順序の検証: MultiMember → second(201) → first(200) (LIFO)
            size_t pos_multi_destroy = output.find("[MultiMember] Destructor: id= 2");
            size_t pos_second_destroy = output.find("[Inner] Destructor: value= 201");
            size_t pos_first_destroy = output.find("[Inner] Destructor: value= 200");
            
            INTEGRATION_ASSERT(pos_multi_destroy < pos_second_destroy,
                "MultiMember should be destructed before its members");
            INTEGRATION_ASSERT(pos_second_destroy < pos_first_destroy,
                "Second member should be destructed before first member (LIFO order)");
            
            // === Test 3: Deep Nested Members ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 3: Deep Nested Members ===",
                "Test 3 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[DeepNested] Constructor: depth= 3",
                "DeepNested constructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[DeepNested] Destructor: depth= 3",
                "DeepNested destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[Outer] Destructor: id= 300",
                "Outer member destructor should be called");
            
            // 破壊順序の検証: DeepNested → Outer → Inner (deep nesting)
            size_t pos_deep_destroy = output.find("[DeepNested] Destructor: depth= 3");
            size_t pos_outer_member_destroy = output.find("[Outer] Destructor: id= 300");
            
            INTEGRATION_ASSERT(pos_deep_destroy < pos_outer_member_destroy,
                "DeepNested should be destructed before its Outer member");
            
            // === Test 4: Mixed Value and Pointer Members ===
            INTEGRATION_ASSERT_CONTAINS(output, "=== Test 4: Mixed Value and Pointer Members ===",
                "Test 4 header should be present");
            INTEGRATION_ASSERT_CONTAINS(output, "[Inner] Constructor: value= 400",
                "Standalone Inner object should be constructed");
            INTEGRATION_ASSERT_CONTAINS(output, "[MixedMembers] Constructor: id= 4",
                "MixedMembers constructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[MixedMembers] Destructor: id= 4",
                "MixedMembers destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[Inner] Destructor: value= 401",
                "Value member destructor should be called");
            INTEGRATION_ASSERT_CONTAINS(output, "[Inner] Destructor: value= 400",
                "Standalone Inner destructor should be called");
            
            // 破壊順序の検証: MixedMembers → value_member(401) → standalone(400)
            size_t pos_mixed_destroy = output.find("[MixedMembers] Destructor: id= 4");
            size_t pos_value_member_destroy = output.find("[Inner] Destructor: value= 401");
            size_t pos_standalone_destroy_mixed = output.rfind("[Inner] Destructor: value= 400");
            
            INTEGRATION_ASSERT(pos_mixed_destroy < pos_value_member_destroy,
                "MixedMembers should be destructed before its value member");
            INTEGRATION_ASSERT(pos_value_member_destroy < pos_standalone_destroy_mixed,
                "Value member should be destructed before standalone object");
            
            // Test suite completion
            INTEGRATION_ASSERT_CONTAINS(output, "=== All Tests Completed ===",
                "Test suite should complete");
        }, execution_time);
    
    integration_test_passed_with_time("nested value member destructors", "nested_value_destructor_test.cb", execution_time);
    
    std::cout << "[integration-test] Nested value member destructor tests completed" << std::endl;
}

// すべてのデストラクタテストを実行
void run_all_destructor_tests() {
    test_integration_destructor();
    test_integration_destructor_simple();
    test_integration_destructor_nested_value_members();
}
