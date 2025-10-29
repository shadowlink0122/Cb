#ifndef TEST_VECTOR_HPP
#define TEST_VECTOR_HPP

#include "../framework/stdlib_test_framework.hpp"
#include <iostream>
#include <string>

// Vector library import tests
// NOTE: Generic type import is not yet supported in the parser.
// These tests are disabled until parser-time import preprocessing is implemented.
// See: docs/features/import_system_limitations.md

inline void test_vector_library_import() {
    // DISABLED: Generic type import requires parser-time preprocessing
    std::cout << "[1/5] Testing Vector library import... SKIPPED (generic type import not supported)\n";
}

inline void test_vector_library_operations() {
    // DISABLED: Generic type import requires parser-time preprocessing
    std::cout << "[2/5] Testing Vector library operations... SKIPPED (generic type import not supported)\n";
}

inline void test_vector_library_resize() {
    // DISABLED: Generic type import requires parser-time preprocessing
    std::cout << "[3/5] Testing Vector library resize... SKIPPED (generic type import not supported)\n";
}

inline void test_vector_library_destructor() {
    // DISABLED: Generic type import requires parser-time preprocessing
    std::cout << "[4/5] Testing Vector library destructor... SKIPPED (generic type import not supported)\n";
}

inline void test_vector_demo_execution() {
    // Test that sample/vector_demo.cb also works
    std::cout << "[5/5] Testing Vector demo...\n";
    
    std::string cmd = "cd ../.. && ./main sample/vector_demo.cb > /dev/null 2>&1";
    int status = system(cmd.c_str());
    int exit_code = WEXITSTATUS(status);
    
    STDLIB_ASSERT_EQ(exit_code, 0);
}

// Register all Vector tests
inline void register_vector_tests(StdlibTestRunner& runner) {
    // Import tests disabled until generic type import support is implemented
    // runner.add_test("vector_library_import", test_vector_library_import);
    // runner.add_test("vector_library_operations", test_vector_library_operations);
    // runner.add_test("vector_library_resize", test_vector_library_resize);
    // runner.add_test("vector_library_destructor", test_vector_library_destructor);
    
    // Demo execution test still valid
    runner.add_test("vector_demo_execution", test_vector_demo_execution);
}

#endif // TEST_VECTOR_HPP
