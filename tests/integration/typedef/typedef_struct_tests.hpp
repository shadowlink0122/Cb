#ifndef TYPEDEF_STRUCT_TESTS_HPP
#define TYPEDEF_STRUCT_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <iostream>
#include <string>

namespace TypedefStructTests {

// 現実的なtypedef構造体テスト
inline void test_typedef_realistic() {
    std::cout << "[integration] Running test_typedef_realistic..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/typedef/test_typedef_realistic.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Typedef realistic test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "Person: id=1, name=Alice", "Basic typedef struct");
            INTEGRATION_ASSERT_CONTAINS(output, "Person2: id=100, name=Bob", "Individual assignment");
            INTEGRATION_ASSERT_CONTAINS(output, "people[0]: id=10, name=Charlie", "Typedef struct array");
            INTEGRATION_ASSERT_CONTAINS(output, "ClassRoom: Math101", "Primitive array member");
            INTEGRATION_ASSERT_CONTAINS(output, "Scores: [85, 90, 78]", "Array literal");
            INTEGRATION_ASSERT_CONTAINS(output, "All typedef struct tests passed!", "Test completion");
        }
    );
    
    integration_test_passed_with_time_auto("test_typedef_realistic", "../../tests/cases/typedef/test_typedef_realistic.cb");
}

// typedef構造体の関数テスト
inline void test_typedef_functions() {
    std::cout << "[integration] Running test_typedef_functions..." << std::endl;
    
    run_cb_test_with_output_and_time_auto("../../tests/cases/typedef/test_typedef_functions.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Typedef functions test should exit with code 0");
            INTEGRATION_ASSERT_CONTAINS(output, "Employee: id=1001", "Typedef struct return");
            INTEGRATION_ASSERT_CONTAINS(output, "Employee age: 30", "Typedef struct parameter");
            INTEGRATION_ASSERT_CONTAINS(output, "Box: 10x20x30, volume=6000", "Typedef with array return");
            INTEGRATION_ASSERT_CONTAINS(output, "Box dimensions: 10x20x30", "Typedef with array parameter");
            INTEGRATION_ASSERT_CONTAINS(output, "All typedef function tests passed!", "Test completion");
        }
    );
    
    integration_test_passed_with_time_auto("test_typedef_functions", "../../tests/cases/typedef/test_typedef_functions.cb");
}

// すべてのtypedef構造体テストを実行
inline void run_all_typedef_struct_tests() {
    std::cout << "\n=== Typedef Struct Tests ===" << std::endl;
    
    test_typedef_realistic();
    test_typedef_functions();
    
    std::cout << "✅ PASS: Typedef Struct Tests (2 tests)" << std::endl;
}

} // namespace TypedefStructTests

#endif // TYPEDEF_STRUCT_TESTS_HPP
