#pragma once

#include "../framework/integration_test_framework.hpp"
#include <string>
#include <vector>

namespace InterfaceTests {

// 基本的なinterface/implテスト
inline void test_basic_interface() {
    std::cout << "[integration] Running test_basic_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/basic_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Basic interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(2, lines.size(), "Should have 2 output lines");
            INTEGRATION_ASSERT_EQ("50", lines[0], "First line should be 50");
            INTEGRATION_ASSERT_EQ("30", lines[1], "Second line should be 30");
        }, execution_time);
}

// 複雑な引数・戻り値を持つinterfaceテスト
inline void test_complex_args() {
    std::cout << "[integration] Running test_complex_args..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/complex_args.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Complex args test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(3, lines.size(), "Should have 3 output lines");
            INTEGRATION_ASSERT_EQ("15", lines[0], "First line should be 15");
            INTEGRATION_ASSERT_EQ("42", lines[1], "Second line should be 42");
            INTEGRATION_ASSERT_EQ("42", lines[2], "Third line should be 42");
        }, execution_time);
}

// 同じインターフェースを複数構造体で使用するテスト
inline void test_multiple_structs() {
    std::cout << "[integration] Running test_multiple_structs..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/multiple_structs.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multiple structs test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(6, lines.size(), "Should have 6 output lines");
            INTEGRATION_ASSERT_EQ("15", lines[0], "First line should be 15");
            INTEGRATION_ASSERT_EQ("95", lines[1], "Second line should be 95");
            INTEGRATION_ASSERT_EQ("5", lines[2], "Third line should be 5");
            INTEGRATION_ASSERT_EQ("80", lines[3], "Fourth line should be 80");
            INTEGRATION_ASSERT_EQ("6", lines[4], "Fifth line should be 6");
            INTEGRATION_ASSERT_EQ("91", lines[5], "Sixth line should be 91");
        }, execution_time);
}

// 同じ構造体に複数インターフェースを実装するテスト
inline void test_multiple_interfaces() {
    std::cout << "[integration] Running test_multiple_interfaces..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/multiple_interfaces.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multiple interfaces test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(7, lines.size(), "Should have 7 output lines");
            INTEGRATION_ASSERT_EQ("110", lines[0], "First line should be 110");
            INTEGRATION_ASSERT_EQ("450", lines[1], "Second line should be 450");
            INTEGRATION_ASSERT_EQ("20", lines[2], "Third line should be 20");
            INTEGRATION_ASSERT_EQ("1", lines[3], "Fourth line should be 1");
            INTEGRATION_ASSERT_EQ("82", lines[4], "Fifth line should be 82");
            INTEGRATION_ASSERT_EQ("10", lines[5], "Sixth line should be 10");
            INTEGRATION_ASSERT_EQ("0", lines[6], "Seventh line should be 0");
        }, execution_time);
}

// return selfの動作テスト
inline void test_return_self() {
    std::cout << "[integration] Running test_return_self..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/return_self.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Return self test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(6, lines.size(), "Should have 6 output lines");
            INTEGRATION_ASSERT_EQ("3", lines[0], "First line should be 3");
            INTEGRATION_ASSERT_EQ("3", lines[1], "Second line should be 3");
            INTEGRATION_ASSERT_EQ("13", lines[2], "Third line should be 13");
            INTEGRATION_ASSERT_EQ("3", lines[3], "Fourth line should be 3");
            INTEGRATION_ASSERT_EQ("0", lines[4], "Fifth line should be 0");
            INTEGRATION_ASSERT_EQ("3", lines[5], "Sixth line should be 3");
        }, execution_time);
}

// 配列を含む構造体でのinterfaceテスト
inline void test_array_struct() {
    std::cout << "[integration] Running test_array_struct..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/array_struct.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Array struct test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(8, lines.size(), "Should have 8 output lines");
            INTEGRATION_ASSERT_EQ("60", lines[0], "First line should be 60");
            INTEGRATION_ASSERT_EQ("3", lines[1], "Second line should be 3");
            INTEGRATION_ASSERT_EQ("10", lines[2], "Third line should be 10");
            INTEGRATION_ASSERT_EQ("3", lines[3], "Fourth line should be 3");
            INTEGRATION_ASSERT_EQ("30", lines[4], "Fifth line should be 30");
            INTEGRATION_ASSERT_EQ("60", lines[5], "Sixth line should be 60");
            INTEGRATION_ASSERT_EQ("3", lines[6], "Seventh line should be 3");
            INTEGRATION_ASSERT_EQ("10", lines[7], "Eighth line should be 10");
        }, execution_time);
}

// ユニオン型とenumを扱うinterfaceテスト
inline void test_union_enum_interface() {
    std::cout << "[integration] Running test_union_enum_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/union_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Union enum interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(6, lines.size(), "Should have 6 output lines");
            INTEGRATION_ASSERT_EQ("42", lines[0], "First line should be 42");
            INTEGRATION_ASSERT_EQ("0", lines[1], "Second line should be 0");
            INTEGRATION_ASSERT_EQ("100", lines[2], "Third line should be 100");
            INTEGRATION_ASSERT_EQ("0", lines[3], "Fourth line should be 0");
            INTEGRATION_ASSERT_EQ("TestString", lines[4], "Fifth line should be TestString");
            INTEGRATION_ASSERT_EQ("1", lines[5], "Sixth line should be 1");
        }, execution_time);
}

// enumを引数・戻り値にするinterfaceテスト
inline void test_enum_interface() {
    std::cout << "[integration] Running test_enum_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/enum_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Enum interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(8, lines.size(), "Should have 8 output lines");
            INTEGRATION_ASSERT_EQ("0", lines[0], "First line should be 0");
            INTEGRATION_ASSERT_EQ("1", lines[1], "Second line should be 1");
            INTEGRATION_ASSERT_EQ("1", lines[2], "Third line should be 1");
            INTEGRATION_ASSERT_EQ("3", lines[3], "Fourth line should be 3");
            INTEGRATION_ASSERT_EQ("3", lines[4], "Fifth line should be 3");
            INTEGRATION_ASSERT_EQ("4", lines[5], "Sixth line should be 4");
            INTEGRATION_ASSERT_EQ("2", lines[6], "Seventh line should be 2");
            INTEGRATION_ASSERT_EQ("2", lines[7], "Eighth line should be 2");
        }, execution_time);
}

// 基本的な配列interfaceテスト
inline void test_simple_array_interface() {
    std::cout << "[integration] Running test_simple_array_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/simple_array_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Simple array interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(5, lines.size(), "Should have 5 output lines");
            INTEGRATION_ASSERT_EQ("3", lines[0], "First line should be 3");
            INTEGRATION_ASSERT_EQ("10", lines[1], "Second line should be 10");
            INTEGRATION_ASSERT_EQ("60", lines[2], "Third line should be 60");
            INTEGRATION_ASSERT_EQ("100", lines[3], "Fourth line should be 100");
            INTEGRATION_ASSERT_EQ("150", lines[4], "Fifth line should be 150");
        }, execution_time);
}

// 多次元配列を引数として受け取るinterfaceテスト
inline void test_multidim_param_interface() {
    std::cout << "[integration] Running test_multidim_param_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/multidim_param_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multidimensional parameter interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(6, lines.size(), "Should have 6 output lines");
            INTEGRATION_ASSERT_EQ("10", lines[0], "First line should be 10");
            INTEGRATION_ASSERT_EQ("10", lines[1], "Second line should be 10");
            INTEGRATION_ASSERT_EQ("9", lines[2], "Third line should be 9");
            INTEGRATION_ASSERT_EQ("24", lines[3], "Fourth line should be 24");
            INTEGRATION_ASSERT_EQ("1", lines[4], "Fifth line should be 1");
            INTEGRATION_ASSERT_EQ("36", lines[5], "Sixth line should be 36");
        }, execution_time);
}

// N次元配列interfaceテスト（5次元まで）
inline void test_ndim_interface() {
    std::cout << "[integration] Running test_ndim_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/ndim_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "N-dimensional interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(5, lines.size(), "Should have 5 output lines");
            INTEGRATION_ASSERT_EQ("10", lines[0], "First line should be 10 (2D sum)");
            INTEGRATION_ASSERT_EQ("36", lines[1], "Second line should be 36 (3D sum)");
            INTEGRATION_ASSERT_EQ("17", lines[2], "Third line should be 17 (4D partial sum)");
            INTEGRATION_ASSERT_EQ("33", lines[3], "Fourth line should be 33 (5D partial sum)");
            INTEGRATION_ASSERT_EQ("45", lines[4], "Fifth line should be 45 (3x3 matrix sum)");
        }, execution_time);
}

// 3次元配列をinterfaceで処理するテスト
inline void test_cube_interface() {
    std::cout << "[integration] Running test_cube_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/cube_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Cube interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(8, lines.size(), "Should have 8 output lines");
            INTEGRATION_ASSERT_EQ("1", lines[0], "First line should be 1");
            INTEGRATION_ASSERT_EQ("8", lines[1], "Second line should be 8");
            INTEGRATION_ASSERT_EQ("36", lines[2], "Third line should be 36");
            INTEGRATION_ASSERT_EQ("8", lines[3], "Fourth line should be 8");
            INTEGRATION_ASSERT_EQ("1", lines[4], "Fifth line should be 1");
            INTEGRATION_ASSERT_EQ("36", lines[5], "Sixth line should be 36");
            INTEGRATION_ASSERT_EQ("1", lines[6], "Seventh line should be 1");
            INTEGRATION_ASSERT_EQ("360", lines[7], "Eighth line should be 360");
        }, execution_time);
}

// 実用的な多次元配列interfaceテスト（構造体メンバーアクセス対応）
inline void test_expanded_multidim_interface() {
    std::cout << "[integration] Running test_expanded_multidim_interface..." << std::endl;
    
    double execution_time;
    run_cb_test_with_output_and_time("../../tests/cases/interface/expanded_multidim_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expanded multidimensional interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(15, lines.size(), "Should have 15 output lines");
            INTEGRATION_ASSERT_EQ("2D Matrix Test:", lines[0], "First line should be header");
            INTEGRATION_ASSERT_EQ("1", lines[1], "Second line should be 1");
            INTEGRATION_ASSERT_EQ("2", lines[2], "Third line should be 2");
            INTEGRATION_ASSERT_EQ("3", lines[3], "Fourth line should be 3");
            INTEGRATION_ASSERT_EQ("4", lines[4], "Fifth line should be 4");
            INTEGRATION_ASSERT_EQ("10", lines[5], "Sixth line should be 10");
            INTEGRATION_ASSERT_EQ("-2", lines[6], "Seventh line should be -2");
            INTEGRATION_ASSERT_EQ("10", lines[7], "Eighth line should be 10");
            INTEGRATION_ASSERT_EQ("3D Cube Test:", lines[8], "Ninth line should be header");
            INTEGRATION_ASSERT_EQ("1", lines[9], "Tenth line should be 1");
            INTEGRATION_ASSERT_EQ("8", lines[10], "Eleventh line should be 8");
            INTEGRATION_ASSERT_EQ("36", lines[11], "Twelfth line should be 36");
            INTEGRATION_ASSERT_EQ("8", lines[12], "13th line should be 8");
            INTEGRATION_ASSERT_EQ("80", lines[13], "14th line should be 80");
        }, execution_time);
}

inline void test_multidim_array_interface() {
    double execution_time = 0.0;
    std::cout << "[integration] Running test_multidim_array_interface..." << std::endl;
    
    run_cb_test_with_output_and_time("../../tests/cases/interface/multidim_array_interface.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "Multidimensional array interface test should exit with code 0");
            std::vector<std::string> lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(9, lines.size(), "Should have 9 output lines");
            INTEGRATION_ASSERT_EQ("1", lines[0], "First line should be 1 (matrix[0][0])");
            INTEGRATION_ASSERT_EQ("2", lines[1], "Second line should be 2 (matrix[0][1])");
            INTEGRATION_ASSERT_EQ("3", lines[2], "Third line should be 3 (matrix[1][0])");
            INTEGRATION_ASSERT_EQ("4", lines[3], "Fourth line should be 4 (matrix[1][1])");
            INTEGRATION_ASSERT_EQ("10", lines[4], "Fifth line should be 10 (sum of all elements)");
            INTEGRATION_ASSERT_EQ("1", lines[5], "Sixth line should be 1 (modified element)");
            INTEGRATION_ASSERT_EQ("1", lines[6], "Seventh line should be 1 (row[0])");
            INTEGRATION_ASSERT_EQ("2", lines[7], "Eighth line should be 2 (row[1])");
            INTEGRATION_ASSERT_EQ("4", lines[8], "Ninth line should be 4 (final element after matrix addition)");
        }, execution_time);
}

// 全てのinterfaceテストを実行する統合関数
inline void run_all_interface_tests() {
    std::cout << "[integration] === Interface/Impl System Tests ===" << std::endl;
    
    test_basic_interface();
    test_complex_args();
    test_multiple_structs();
    test_multiple_interfaces();
    test_return_self();
    test_array_struct();
    test_union_enum_interface();
    test_enum_interface();
    test_simple_array_interface();
    test_multidim_param_interface();
    test_ndim_interface();
    test_cube_interface();
    test_multidim_array_interface();
    
    std::cout << "[integration] Interface tests completed" << std::endl;
}

} // namespace InterfaceTests
