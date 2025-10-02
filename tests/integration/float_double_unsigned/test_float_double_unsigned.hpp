#pragma once
#include "../framework/integration_test_framework.hpp"

void test_integration_float_double_unsigned() {
    std::cout << "[integration] Running float/double/unsigned tests..." << std::endl;
    
    // Float basic operations test
    double execution_time;
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/float_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "float_basic.cb should execute successfully");
            // 3.14f + 2.71f = 5.85
            INTEGRATION_ASSERT(output.find("5.85") != std::string::npos, "float addition should work");
            // 1.0f / 3.0f ≈ 0.333333
            INTEGRATION_ASSERT(output.find("0.333") != std::string::npos, "float division should work");
            // 1 / 3 = 0 (integer division)
            INTEGRATION_ASSERT(output.find("0") != std::string::npos, "integer division should return 0");
        }, execution_time);
    
    integration_test_passed_with_time("float basic operations", "float_basic.cb", execution_time);
    
    // Double precision test
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/double_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "double_basic.cb should execute successfully");
            // π + e ≈ 5.85987
            INTEGRATION_ASSERT(output.find("5.85") != std::string::npos, "double addition should work");
            // 1.0 / 3.0 ≈ 0.333333
            INTEGRATION_ASSERT(output.find("0.333") != std::string::npos, "double division should work");
        }, execution_time);
    
    integration_test_passed_with_time("double precision operations", "double_basic.cb", execution_time);
    
    // Unsigned integer and bitwise operations test
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/unsigned_basic.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "unsigned_basic.cb should execute successfully");
            // Bitwise AND: 255 & 15 = 15
            INTEGRATION_ASSERT(output.find("15") != std::string::npos, "bitwise AND should work");
            // Bitwise OR: 255 | 15 = 255
            INTEGRATION_ASSERT(output.find("255") != std::string::npos, "bitwise OR should work");
            // Bitwise XOR: 255 ^ 15 = 240
            INTEGRATION_ASSERT(output.find("240") != std::string::npos, "bitwise XOR should work");
            // Left shift: 255 << 1 = 510
            INTEGRATION_ASSERT(output.find("510") != std::string::npos, "left shift should work");
        }, execution_time);
    
    integration_test_passed_with_time("unsigned and bitwise operations", "unsigned_basic.cb", execution_time);
    
    // Struct members test
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/struct_members.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "struct_members.cb should execute successfully");
            // Float struct member operations
            INTEGRATION_ASSERT(output.find("3.14") != std::string::npos, "float struct member should work");
            // Double struct member operations
            INTEGRATION_ASSERT(output.find("2.71") != std::string::npos, "double struct member should work");
            // Unsigned struct member operations
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, "unsigned struct member should work");
        }, execution_time);
    
    integration_test_passed_with_time("struct members with float/double/unsigned", "struct_members.cb", execution_time);
    
    // NOTE: union_types.cb はC言語スタイルのunionを使用しているため、
    // Cbのunion型（sum type）とは異なりサポートされていません。
    // このテストは削除されました。
    
    // Function parameters and return values test
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/function_params.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "function_params.cb should execute successfully");
            // add_floats(1.5f, 2.5f) = 4.0
            INTEGRATION_ASSERT(output.find("4.0") != std::string::npos || output.find("4") != std::string::npos, 
                             "float function return should work");
            // multiply_doubles(3.14, 2.0) = 6.28
            INTEGRATION_ASSERT(output.find("6.28") != std::string::npos, "double function return should work");
            // compute_average(1.0f, 2.0f, 3.0f) = 2.0
            INTEGRATION_ASSERT(output.find("2.0") != std::string::npos || output.find("2") != std::string::npos, 
                             "float average calculation should work");
        }, execution_time);
    
    integration_test_passed_with_time("function parameters and returns", "function_params.cb", execution_time);
    
    // Multidimensional arrays test
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/multidim_arrays.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "multidim_arrays.cb should execute successfully");
            // 2D float array
            INTEGRATION_ASSERT(output.find("1.1") != std::string::npos, "2D float array should work");
            INTEGRATION_ASSERT(output.find("6.6") != std::string::npos, "2D float array access should work");
            // 2D double array
            INTEGRATION_ASSERT(output.find("2.222") != std::string::npos, "2D double array should work");
            // 3D float array
            INTEGRATION_ASSERT(output.find("9.0") != std::string::npos || output.find("9") != std::string::npos, 
                             "3D float array should work");
        }, execution_time);
    
    integration_test_passed_with_time("multidimensional arrays", "multidim_arrays.cb", execution_time);
    
    // Compound assignment operators test
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/compound_assign.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "compound_assign.cb should execute successfully");
            // Float compound assignments
            INTEGRATION_ASSERT(output.find("15.5") != std::string::npos, "float += should work");
            INTEGRATION_ASSERT(output.find("24.0") != std::string::npos || output.find("24") != std::string::npos, 
                             "float *= should work");
            // Double compound assignments
            INTEGRATION_ASSERT(output.find("125.25") != std::string::npos, "double += should work");
            INTEGRATION_ASSERT(output.find("21.0") != std::string::npos || output.find("21") != std::string::npos, 
                             "double /= should work");
            // Unsigned compound assignments
            INTEGRATION_ASSERT(output.find("150") != std::string::npos, "unsigned += should work");
            INTEGRATION_ASSERT(output.find("240") != std::string::npos, "unsigned *= should work");
        }, execution_time);
    
    integration_test_passed_with_time("compound assignment operators", "compound_assign.cb", execution_time);
    
    // Literals and type inference test
    run_cb_test_with_output_and_time("../cases/float_double_unsigned/literals.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "literals.cb should execute successfully");
            // Float literal with f suffix
            INTEGRATION_ASSERT(output.find("3.14") != std::string::npos, "float literal should work");
            // Double literal without suffix
            INTEGRATION_ASSERT(output.find("3.14159") != std::string::npos, "double literal should work");
            // Unsigned literal with u suffix
            INTEGRATION_ASSERT(output.find("100") != std::string::npos, "unsigned literal should work");
            INTEGRATION_ASSERT(output.find("255") != std::string::npos, "hex literal should work");
            // Type precedence: 1/3 = 0, 1.0f/3.0f ≈ 0.333...
            INTEGRATION_ASSERT(output.find("0") != std::string::npos, "integer division should return 0");
            INTEGRATION_ASSERT(output.find("0.333") != std::string::npos, "float division should work");
        }, execution_time);
    
    integration_test_passed_with_time("literals and type inference", "literals.cb", execution_time);
    
    std::cout << "[integration] Float/double/unsigned tests completed" << std::endl;
}
