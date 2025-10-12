#ifndef RVALUE_REFERENCE_TESTS_HPP
#define RVALUE_REFERENCE_TESTS_HPP

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <chrono>

namespace RvalueReferenceTests {

// ============================================================================
// T&& (Rvalue Reference) Tests - v0.10.0 Partial Implementation
// ============================================================================

inline void test_syntax_parse() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/rvalue_reference/syntax_parse.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "T&& syntax parse test should succeed");
            
            // 期待される出力
            INTEGRATION_ASSERT_CONTAINS(output, "Syntax parse test: PASS", 
                "Should print success message");
        },
        execution_time
    );
    integration_test_passed_with_time("T&& syntax parse", "syntax_parse.cb", execution_time);
}

inline void test_type_restriction() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/rvalue_reference/type_restriction.cb",
        [](const std::string& output, int exit_code) {
            // プリミティブ型でのT&&使用はエラーになるべき
            INTEGRATION_ASSERT_NE(0, exit_code, "T&& for primitive types should fail");
            
            // エラーメッセージを確認（パーサーエラーまたはランタイムエラー）
            INTEGRATION_ASSERT(
                output.find("error:") != std::string::npos || 
                output.find("Error:") != std::string::npos,
                "Should show error message for T&& on primitives");
        },
        execution_time
    );
    integration_test_passed_with_time("T&& type restriction", "type_restriction.cb", execution_time);
}

inline void test_lvalue_ref_primitive() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/rvalue_reference/lvalue_ref_primitive.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "T& for primitive types should succeed");
            
            // 期待される出力
            INTEGRATION_ASSERT_CONTAINS(output, "T& syntax for primitive types: PASS", 
                "Should print success message");
        },
        execution_time
    );
    integration_test_passed_with_time("T& for primitives", "lvalue_ref_primitive.cb", execution_time);
}

// ============================================================================
// Known Issues Tests (v0.10.0 - These will FAIL until v0.10.1)
// ============================================================================

inline void test_member_access_known_issue() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/rvalue_reference/member_access.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Should execute without crash");
            
            // v0.10.0 fix applied: ref.x now correctly returns 10
            INTEGRATION_ASSERT_CONTAINS(output, "p1.x = 10", 
                "p1.x should be 10");
            INTEGRATION_ASSERT_CONTAINS(output, "ref.x = 10", 
                "ref.x should equal p1.x");
        },
        execution_time
    );
    integration_test_passed_with_time("T&& member access", "member_access.cb", execution_time);
}

inline void test_member_assignment_known_issue() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/rvalue_reference/member_assignment.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Should execute without crash");
            
            // v0.10.0 fix applied: p1.x and p1.y now correctly modified through ref
            INTEGRATION_ASSERT_CONTAINS(output, "Before: p1.x = 10", 
                "p1.x initial value should be 10");
            INTEGRATION_ASSERT_CONTAINS(output, "After: p1.x = 100", 
                "p1.x should be modified through ref");
            INTEGRATION_ASSERT_CONTAINS(output, "After: p1.y = 200", 
                "p1.y should be modified through ref");
        },
        execution_time
    );
    integration_test_passed_with_time("T&& member assignment", "member_assignment.cb", execution_time);
}

inline void test_aliasing_known_issue() {
    double execution_time = 0.0;
    run_cb_test_with_output_and_time(
        "../../tests/cases/rvalue_reference/aliasing.cb",
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Should execute without crash");
            
            // v0.10.0 fix applied: ref now correctly reflects changes to p1
            INTEGRATION_ASSERT_CONTAINS(output, "p1.x = 99", 
                "p1.x should be modified to 99");
            INTEGRATION_ASSERT_CONTAINS(output, "ref.x = 99", 
                "ref should reflect changes to p1");
            INTEGRATION_ASSERT_CONTAINS(output, "p1.y = 88", 
                "p1.y should be modified to 88");
            INTEGRATION_ASSERT_CONTAINS(output, "ref.y = 88", 
                "ref should reflect changes to p1");
        },
        execution_time
    );
    integration_test_passed_with_time("T&& aliasing", "aliasing.cb", execution_time);
}

// ============================================================================
// All Rvalue Reference Tests
// ============================================================================

inline void run_all_rvalue_reference_tests() {
    std::cout << "\n============================================================" << std::endl;
    std::cout << "Running Rvalue Reference (T&&) Tests - v0.10.0 Complete Implementation" << std::endl;
    std::cout << "============================================================" << std::endl;
    
    std::cout << "\n--- Syntax Tests ---" << std::endl;
    test_syntax_parse();
    test_type_restriction();
    test_lvalue_ref_primitive();
    
    std::cout << "\n--- Semantics Tests (Fixed in v0.10.0) ---" << std::endl;
    test_member_access_known_issue();
    test_member_assignment_known_issue();
    test_aliasing_known_issue();
    
    std::cout << "\n✅ PASS: Rvalue Reference Tests (6 tests)" << std::endl;
    std::cout << "   - All reference semantics now working correctly" << std::endl;
}

} // namespace RvalueReferenceTests

#endif // RVALUE_REFERENCE_TESTS_HPP
