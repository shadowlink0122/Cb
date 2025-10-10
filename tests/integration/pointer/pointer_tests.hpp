#ifndef POINTER_TESTS_HPP
#define POINTER_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include "pointer_basic_tests.hpp"
#include "pointer_arrow_tests.hpp"
#include "pointer_struct_tests.hpp"
#include "pointer_comprehensive_tests.hpp"
#include "function_pointer_tests.hpp"
#include "pointer_array_tests.hpp"
#include "pointer_type_tests.hpp"
#include "pointer_advanced_tests.hpp"

namespace PointerTests {

// ============================================================================
// 全てのポインタテストを実行（各カテゴリに委譲）
// ============================================================================

inline void run_all_pointer_tests() {
    printf("\n=== Pointer Tests ===\n");
    
    // 各カテゴリのテストを実行
    PointerBasicTests::run_all_tests();
    PointerArrowTests::run_all_tests();
    PointerStructTests::run_all_tests();
    PointerComprehensiveTests::run_all_tests();
    FunctionPointerTests::run_all_tests();
    PointerArrayTests::run_all_tests();
    PointerTypeTests::run_all_tests();
    PointerAdvancedTests::run_all_tests();
    
    printf("=== All Pointer Tests Passed ===\n\n");
}

} // namespace PointerTests

#endif // POINTER_TESTS_HPP
