#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_floating_point() {
    const std::string base_path = "../../tests/cases/floating_point/";

    // arithmetic_and_bitwise.cb
    {
        const std::string test_file = base_path + "arithmetic_and_bitwise.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for floating-point arithmetic and bitwise test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== arithmetic & bitwise coverage ===",
                    "Missing header for floating-point arithmetic coverage");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float_after_ops: 1",
                    "Unexpected float accumulation result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "double_after_ops: 6.5",
                    "Unexpected double accumulation result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mixed_times_three: 1",
                    "Unexpected mixed precision multiply result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mask_or: 175",
                    "Unexpected OR mask result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mask_and: 160",
                    "Unexpected AND mask result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mask_xor: 108",
                    "Unexpected XOR mask result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mask_shl: 432",
                    "Unexpected left shift mask result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mask_shr: 54",
                    "Unexpected right shift mask result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "unsigned_after_compound: 32",
                    "Unexpected unsigned compound result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== end ===",
                    "Missing footer for floating-point arithmetic coverage");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point arithmetic_and_bitwise", test_file, execution_time);
    }

    // functions_and_arrays.cb
    {
        const std::string test_file = base_path + "functions_and_arrays.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for floating-point functions test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== function and array coverage ===",
                    "Missing header for floating-point function coverage");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "average_pair: 2",
                    "Incorrect average calculation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "accumulate_double: 10.125",
                    "Incorrect double accumulation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "clamp_add: 0",
                    "Incorrect unsigned clamp result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float_grid_total: 23.25",
                    "Incorrect float grid total");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "generated grid values:",
                    "Missing generated grid header");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[0][0]: 0.5",
                    "Incorrect generated grid value g[0][0]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[0][1]: 0.75",
                    "Incorrect generated grid value g[0][1]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[0][2]: 1.0",
                    "Incorrect generated grid value g[0][2]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[1][0]: 1.5",
                    "Incorrect generated grid value g[1][0]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[1][1]: 2.0",
                    "Incorrect generated grid value g[1][1]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[1][2]: 2.5",
                    "Incorrect generated grid value g[1][2]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== end ===",
                    "Missing footer for floating-point functions coverage");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point functions_and_arrays", test_file, execution_time);
    }

    // print_and_literals.cb
    {
        const std::string test_file = base_path + "print_and_literals.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for floating-point literal print test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== print/println literal coverage ===",
                    "Missing header for literal coverage");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float_literal: 3.1415927",
                    "Unexpected float literal representation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "double_literal: 2.71828182845905",
                    "Unexpected double literal representation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "unsigned_literal: 4294967295",
                    "Unexpected unsigned literal representation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "--- implicit typing ---",
                    "Missing implicit typing section");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float_from_int: 5.0",
                    "Unexpected float_from_int representation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "double_from_float_literal: 6.5",
                    "Unexpected double_from_float_literal representation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "ulong_from_literal: 123456789",
                    "Unexpected ulong_from_literal representation");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "--- print chaining ---",
                    "Missing print chaining section");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "chain: 3.1415927",
                    "Unexpected chained float output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "| 2.71828182845905",
                    "Unexpected chained double output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "--- division precedence ---",
                    "Missing division precedence section");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "int_division: 0",
                    "Unexpected int division result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float_division: 0.333333333333333",
                    "Unexpected float division result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mixed_division: 0.3333333",
                    "Unexpected mixed division result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "mixed_add: 3.5",
                    "Unexpected mixed addition result");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== end ===",
                    "Missing footer for literal coverage");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point print_and_literals", test_file, execution_time);
    }

    // struct_and_union.cb - SKIPPED: Union type does not yet support float/double
    // TODO: Re-enable when union type supports float/double
    /*
    {
        const std::string test_file = base_path + "struct_and_union.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for floating-point struct/union test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== struct/union coverage ===",
                    "Missing header for struct/union coverage");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "initial.current: 0.5",
                    "Unexpected initial current value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "initial.measurement: 10.25",
                    "Unexpected initial measurement value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "initial.count: 7",
                    "Unexpected initial count value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "updated.current: 1.125",
                    "Unexpected updated current value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "updated.measurement: 10.125",
                    "Unexpected updated measurement value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "updated.count: 12",
                    "Unexpected updated count value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "history[0].current: 1.125",
                    "Unexpected history[0] current value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "history[1].measurement: 2.5",
                    "Unexpected history[1] measurement value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "history[1].count: 12",
                    "Unexpected history[1] count value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "union as float: 1.125",
                    "Unexpected union float value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "union as double: 10.125",
                    "Unexpected union double value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "union as unsigned: 12",
                    "Unexpected union unsigned value");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== end ===",
                    "Missing footer for struct/union coverage");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point struct_and_union", test_file, execution_time);
    }
    */

    // union_float_test.cb - Union型でのfloat/double - SKIPPED: Not yet supported
    // TODO: Re-enable when union type supports float/double
    /*
    {
        const std::string test_file = base_path + "union_float_test.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for union float/double test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== Union Float/Double Test ===",
                    "Missing header for union float/double test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "int value:",
                    "Missing int value output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float value:",
                    "Missing float value output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "double value:",
                    "Missing double value output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "3.14",
                    "Float value not found in output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "2.718281828459045",
                    "Double value not found in output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== End ===",
                    "Missing footer for union float/double test");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point union_float_test", test_file, execution_time);
    }
    */

    // precision_test.cb - Float/Doubleの精度の違い
    {
        const std::string test_file = base_path + "precision_test.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for precision test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== Float vs Double Precision Test ===",
                    "Missing header for precision test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float PI:",
                    "Missing float PI output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "double PI:",
                    "Missing double PI output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float 1/3:",
                    "Missing float division output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "double 1/3:",
                    "Missing double division output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "0.333",
                    "Expected 1/3 approximation not found");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== End ===",
                    "Missing footer for precision test");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point precision_test", test_file, execution_time);
    }

    // boundary_test.cb - 境界値テスト
    {
        const std::string test_file = base_path + "boundary_test.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for boundary test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== Float/Double Boundary Value Test ===",
                    "Missing header for boundary test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float zero:",
                    "Missing float zero output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "0.0",
                    "Zero value not displayed correctly");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float negative:",
                    "Missing negative value output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "-123.456",
                    "Negative value not found");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "grid[1][2]:",
                    "Missing multidimensional array access");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "-3.0",
                    "Multidim negative value not found");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== End ===",
                    "Missing footer for boundary test");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point boundary_test", test_file, execution_time);
    }

    // array_literal_test.cb - 配列リテラル総合テスト
    {
        const std::string test_file = base_path + "array_literal_test.cb";
        double execution_time;
        run_cb_test_with_output_and_time(
            test_file,
            [](const std::string &output, int exit_code) {
                INTEGRATION_ASSERT_EQ(0, exit_code,
                    "Expected successful exit for array literal test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== Array Literal Float/Double Test ===",
                    "Missing header for array literal test");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "float array:",
                    "Missing float array output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "3.14",
                    "Float array value not found");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "double 2D array:",
                    "Missing double 2D array output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "6.666",
                    "Double 2D array value not found");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "3D array:",
                    "Missing 3D array output");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "8.0",
                    "3D array value not found");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "=== End ===",
                    "Missing footer for array literal test");
            },
            execution_time);
        integration_test_passed_with_time(
            "floating_point array_literal_test", test_file, execution_time);
    }
}
