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
                    "  g[0][2]: 1",
                    "Incorrect generated grid value g[0][2]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[1][0]: 1.5",
                    "Incorrect generated grid value g[1][0]");
                INTEGRATION_ASSERT_CONTAINS(output,
                    "  g[1][1]: 2",
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
                    "float_from_int: 5",
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

    // struct_and_union.cb
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
}
