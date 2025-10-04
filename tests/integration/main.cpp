#include "framework/integration_test_framework.hpp"
#include <cstdlib> // std::exitのため
#include <iostream>
#include <string>
#include <vector>

// 各テストモジュールをインクルード
#include "arithmetic/test_arithmetic.hpp"
#include "array/test_array.hpp"
#include "array_copy/test_array_copy.hpp"
#include "array_literal/test_array_literal.hpp"
#include "array_return/test_array_return.hpp"
#include "assert/assert_tests.hpp"
#include "assign/test_assign.hpp"
#include "basic/test_basic.hpp"
#include "bitwise/test_bitwise.hpp"
#include "bool_expr/test_bool_expr.hpp"
#include "boundary/test_boundary.hpp"
#include "compound_assign/test_compound_assign.hpp"
#include "const_array/test_const_array.hpp"
#include "const_variables/test_const_variables.hpp"
#include "cross_type/test_cross_type.hpp"
#include "dynamic_array_error/test_dynamic_array_error.hpp"
#include "enum/test_enum.hpp"
#include "error_handling/test_error_handling.hpp"
#include "float_double_unsigned/test_float_double_unsigned.hpp"
#include "floating_point/test_floating_point.hpp"
#include "func/test_func.hpp"
#include "func_return_type_check/test_func_return_type_check.hpp"
#include "func_type_check/test_func_type_check.hpp"
#include "global_array/test_global_array.hpp"
#include "global_vars/test_global_vars.hpp"
#include "if/test_if.hpp"
#include "import_export/test_import_export.hpp"
#include "incdec/test_incdec.hpp"
#include "interface/interface_error_tests.hpp"
#include "interface/interface_tests.hpp"
#include "interface/test_interface_private.hpp"
#include "interface/test_type_inference_chain.hpp"
#include "interface/test_typedef_impl.hpp"
#include "loop/test_loop.hpp"
#include "module_functions/test_module_functions.hpp"
#include "multidim_array/test_multidim_array.hpp"
#include "multidim_literal/test_multidim_literal.hpp"
#include "multiple_var_decl/test_multiple_var_decl.hpp"
#include "performance/test_performance.hpp"
#include "pointer/pointer_tests.hpp"
#include "printf/test_printf.hpp"
#include "println/test_println.hpp"
#include "reference/reference_tests.hpp"
#include "sample_scenarios/test_sample_scenarios.hpp"
#include "samples/test_actual_samples.hpp"
#include "self_assign/test_self_assign.hpp"
#include "static_variables/test_static_variables.hpp"
#include "string/test_string.hpp"
#include "struct/basic_struct_tests.hpp"
#include "struct/struct_tests.hpp"
#include "ternary/test_ternary.hpp"
#include "type/test_type.hpp"
#include "typedef/test_enum_typedef.hpp"
#include "typedef/test_struct_typedef.hpp"
#include "typedef/test_typedef.hpp"
#include "union/test_union.hpp"
#include "unsigned/test_unsigned.hpp"

// 失敗継続対応のテスト実行関数（マクロをリファクタリング）
void run_test_with_continue(void (*test_function)(), const char *test_name,
                            std::vector<std::string> &failed_tests) {
    std::cout << "[integration-test] Running " << test_name << "..."
              << std::endl;

    int prev_total = IntegrationTestCounter::get_total();
    int prev_passed = IntegrationTestCounter::get_passed();
    int prev_failed = IntegrationTestCounter::get_failed();

    try {
        test_function();

        int tests_run = IntegrationTestCounter::get_total() - prev_total;
        int tests_passed = IntegrationTestCounter::get_passed() - prev_passed;
        int tests_failed = IntegrationTestCounter::get_failed() - prev_failed;

        if (tests_failed > 0) {
            std::cout << "[integration-test] ✅ COMPLETED: " << test_name
                      << std::endl;
            std::cout << "[integration-test]   Results: " << tests_run
                      << " tests (" << tests_passed << " passed, "
                      << tests_failed << " failed)" << std::endl;
        } else {
            std::cout << "[integration-test] ✅ PASS: " << test_name << " ("
                      << tests_run << " tests)" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cout << "[integration-test] ❌ EXCEPTION: " << test_name
                  << std::endl;
        std::cout << "[integration-test]   Error: " << e.what() << std::endl;
        IntegrationTestCounter::increment_total();
        IntegrationTestCounter::increment_failed();
        failed_tests.push_back(std::string(test_name) + ": " +
                               std::string(e.what()));
    } catch (...) {
        std::cout << "[integration-test] ❌ UNKNOWN_ERROR: " << test_name
                  << std::endl;
        IntegrationTestCounter::increment_total();
        IntegrationTestCounter::increment_failed();
        failed_tests.push_back(std::string(test_name) + ": unknown error");
    }
}

int main() {
    std::vector<std::string> failed_tests;

    // Reset test counters
    IntegrationTestCounter::reset();
    TimingStats::reset();
    CategoryTimingStats::reset();

    std::cout << "[integration-test] Starting HPP Test Suite with failure "
                 "continuation\n"
              << std::endl;

    // 基本テスト群
    std::cout << "[integration-test] === Core Language Tests ===" << std::endl;
    CategoryTimingStats::set_current_category("Core Language");
    run_test_with_continue(test_integration_basic, "Basic Tests", failed_tests);
    run_test_with_continue(test_integration_arithmetic, "Arithmetic Tests",
                           failed_tests);
    run_test_with_continue(test_integration_floating_point,
                           "Floating Point Tests", failed_tests);
    run_test_with_continue(test_integration_float_double_unsigned,
                           "Float/Double/Unsigned Comprehensive Tests",
                           failed_tests);
    run_test_with_continue(test_integration_assign, "Assignment Tests",
                           failed_tests);
    run_test_with_continue(test_integration_boundary, "Boundary Tests",
                           failed_tests);
    run_test_with_continue(test_integration_type, "Type Tests", failed_tests);
    CategoryTimingStats::print_category_summary("Core Language");

    // 配列テスト群
    std::cout << "\n[integration-test] === Array Tests ===" << std::endl;
    CategoryTimingStats::set_current_category("Array");
    run_test_with_continue(test_integration_array, "Array Tests", failed_tests);
    run_test_with_continue(test_integration_array_literal,
                           "Array Literal Tests", failed_tests);
    run_test_with_continue(test_array_copy, "Array Copy Tests", failed_tests);
    run_test_with_continue(test_array_return, "Array Return Tests",
                           failed_tests);
    run_test_with_continue(test_integration_multidim_array,
                           "Multidimensional Array Tests", failed_tests);
    run_test_with_continue(test_multidim_literal,
                           "Multidimensional Literal Tests", failed_tests);
    run_test_with_continue(test_integration_global_array, "Global Array Tests",
                           failed_tests);
    CategoryTimingStats::print_category_summary("Array");

    // 制御フロー・演算子テスト群
    std::cout << "\n[integration-test] === Control Flow & Operators ==="
              << std::endl;
    CategoryTimingStats::set_current_category("Control Flow");
    run_test_with_continue(test_integration_if, "If Statement Tests",
                           failed_tests);
    run_test_with_continue(test_integration_loop, "Loop Tests", failed_tests);
    run_test_with_continue(test_bool_expr_basic, "Boolean Expression Tests",
                           failed_tests);
    run_test_with_continue(test_integration_bitwise, "Bitwise Operator Tests",
                           failed_tests);
    run_test_with_continue(test_integration_ternary, "Ternary Operator Tests",
                           failed_tests);
    run_test_with_continue(test_integration_compound_assign,
                           "Compound Assignment Tests", failed_tests);
    run_test_with_continue(test_integration_incdec, "Increment/Decrement Tests",
                           failed_tests);
    CategoryTimingStats::print_category_summary("Control Flow");

    // 関数・モジュールテスト群
    std::cout << "\n[integration-test] === Function & Module Tests ==="
              << std::endl;
    CategoryTimingStats::set_current_category("Functions");
    run_test_with_continue(test_integration_func, "Function Tests",
                           failed_tests);
    run_test_with_continue(test_integration_func_type_check,
                           "Function Type Check Tests", failed_tests);
    run_test_with_continue(test_integration_func_return_type_check,
                           "Function Return Type Check Tests", failed_tests);
    run_test_with_continue(test_integration_import_export,
                           "Import/Export Tests", failed_tests);
    run_test_with_continue(test_integration_module_functions,
                           "Module Function Tests", failed_tests);
    CategoryTimingStats::print_category_summary("Functions");

    // 変数・定数テスト群
    std::cout << "\n[integration-test] === Variable & Constant Tests ==="
              << std::endl;
    CategoryTimingStats::set_current_category("Variables");
    run_test_with_continue(test_integration_const_variables,
                           "Const Variable Tests", failed_tests);
    run_test_with_continue(test_integration_const_array, "Const Array Tests",
                           failed_tests);
    run_test_with_continue(test_integration_global_vars,
                           "Global Variable Tests", failed_tests);
    run_test_with_continue(test_integration_static_variables,
                           "Static Variable Tests", failed_tests);
    run_test_with_continue(test_integration_multiple_var_decl,
                           "Multiple Variable Declaration Tests", failed_tests);
    run_test_with_continue(test_integration_self_assign,
                           "Self Assignment Tests", failed_tests);
    run_test_with_continue(test_integration_unsigned, "Unsigned Tests",
                           failed_tests);
    CategoryTimingStats::print_category_summary("Variables");

    // 文字列・I/Oテスト群
    std::cout << "\n[integration-test] === String & I/O Tests ===" << std::endl;
    CategoryTimingStats::set_current_category("String & I/O");
    run_test_with_continue(test_integration_string, "String Tests",
                           failed_tests);
    run_test_with_continue(test_printf_all, "Printf Tests", failed_tests);
    run_test_with_continue(test_integration_println, "Println Tests",
                           failed_tests);
    CategoryTimingStats::print_category_summary("String & I/O");

    // 型システムテスト群
    std::cout << "\n[integration-test] === Type System Tests ===" << std::endl;
    CategoryTimingStats::set_current_category("Type System");
    run_test_with_continue(test_integration_typedef, "Typedef Tests",
                           failed_tests);
    run_test_with_continue(test_integration_enum_typedef, "Enum Typedef Tests",
                           failed_tests);
    run_test_with_continue(test_integration_struct_typedef,
                           "Struct Typedef Tests", failed_tests);
    run_test_with_continue(test_integration_cross_type, "Cross Type Tests",
                           failed_tests);
    run_test_with_continue(test_integration_enum, "Enum Tests", failed_tests);
    run_test_with_continue(UnionTests::run_all_union_tests, "Union Type Tests",
                           failed_tests);
    CategoryTimingStats::print_category_summary("Type System");

    // 構造体・インターフェーステスト群
    std::cout << "\n[integration-test] === Advanced Features ===" << std::endl;
    CategoryTimingStats::set_current_category("Advanced Features");
    run_test_with_continue(BasicStructTests::run_all_basic_struct_tests,
                           "Basic Struct Tests", failed_tests);
    run_test_with_continue(StructTests::run_all_struct_tests, "Struct Tests",
                           failed_tests);
    run_test_with_continue(InterfaceTests::run_all_interface_tests,
                           "Interface Tests", failed_tests);
    run_test_with_continue(test_interface_type_inference_chain,
                           "Interface Type Inference Chain Tests",
                           failed_tests);
    run_test_with_continue(test_integration_interface_private,
                           "Interface Private Method Tests", failed_tests);
    run_test_with_continue(test_typedef_impl_basic,
                           "Interface Typedef Implementation Tests",
                           failed_tests);
    run_test_with_continue(test_recursive_typedef_independence,
                           "Recursive Typedef Independence Tests",
                           failed_tests);
    run_test_with_continue(PointerTests::run_all_pointer_tests, "Pointer Tests",
                           failed_tests);
    run_test_with_continue(ReferenceTests::run_all_reference_tests,
                           "Reference Tests", failed_tests);
    run_test_with_continue(AssertTests::run_all_assert_tests, "Assert Tests",
                           failed_tests);
    CategoryTimingStats::print_category_summary("Advanced Features");

    // エラーハンドリング・特殊ケーステスト群
    std::cout << "\n[integration-test] === Error Handling & Special Cases ==="
              << std::endl;
    CategoryTimingStats::set_current_category("Error Handling");
    run_test_with_continue(test_integration_error_handling,
                           "Error Handling Tests", failed_tests);
    run_test_with_continue(InterfaceErrorTests::run_all_interface_error_tests,
                           "Interface Error Tests", failed_tests);
    run_test_with_continue(test_integration_dynamic_array_error,
                           "Dynamic Array Error Tests", failed_tests);
    CategoryTimingStats::print_category_summary("Error Handling");

    // パフォーマンステスト群
    std::cout << "\n[integration-test] === Performance Tests ===" << std::endl;
    CategoryTimingStats::set_current_category("Performance Tests");
    run_test_with_continue(test_integration_performance, "Performance Tests",
                           failed_tests);
    CategoryTimingStats::print_category_summary("Performance Tests");

    // サンプルシナリオテスト群
    std::cout << "\n[integration-test] === Sample Scenarios ===" << std::endl;
    CategoryTimingStats::set_current_category("Sample Scenarios");
    run_test_with_continue(test_integration_sample_scenarios,
                           "Sample Scenario Tests", failed_tests);
    run_test_with_continue(test_integration_actual_samples,
                           "Actual Sample Tests", failed_tests);
    CategoryTimingStats::print_category_summary("Sample Scenarios");

    // 最終サマリー
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "[integration-test] === FINAL SUMMARY ===" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    int failed_tests_count = IntegrationTestCounter::get_failed();

    std::cout << "[integration-test] HPP Test Suite Completed" << std::endl;
    std::cout << "[integration-test]" << std::endl;

    IntegrationTestCounter::print_summary();

    // 結果に応じたメッセージ
    if (failed_tests_count == 0) {
        std::cout << std::endl;
        std::cout << "🎉 ALL TESTS PASSED! 🎉" << std::endl;
        std::cout << std::endl;
    } else {
        std::cout << std::endl;
        std::cout << "⚠️  " << failed_tests_count << " TESTS FAILED ⚠️"
                  << std::endl;
        std::cout << std::endl;
    }

    // 失敗箇所の詳細表示（短縮形式）
    if (!failed_tests.empty()) {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "FAILED TEST SUMMARY:" << std::endl;
        std::cout << std::string(60, '-') << std::endl;
        for (size_t i = 0; i < failed_tests.size() && i < 10; ++i) {
            std::string error_msg = failed_tests[i];
            // エラーメッセージを80文字で切り詰め
            if (error_msg.length() > 80) {
                error_msg = error_msg.substr(0, 77) + "...";
            }
            std::cout << (i + 1) << ". " << error_msg << std::endl;
        }
        if (failed_tests.size() > 10) {
            std::cout << "[integration-test] ... and "
                      << (failed_tests.size() - 10) << " more failures"
                      << std::endl;
        }
    }

    std::cout << std::string(60, '=') << std::endl;

    // Display timing statistics
    TimingStats::print_timing_summary();
    std::cout << std::string(60, '=') << std::endl;

    // テスト結果に応じて異常終了または正常終了
    if (failed_tests_count == 0) {
        std::cout << "Test suite completed successfully." << std::endl;
        std::exit(0); // 正常終了
    } else {
        std::cout << "Test suite failed with " << failed_tests_count
                  << " failures." << std::endl;
        std::exit(1); // 異常終了
    }
}
