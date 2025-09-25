#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_compound_assign() {
    std::cout << "[integration] Running compound assign tests..." << std::endl;
    
    // Basic compound assignment operators
    run_cb_test_with_output("../../tests/cases/compound_assign/basic_compound.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "basic_compound.cb should execute successfully");
            
            auto lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(7, lines.size(), "Should have 7 output lines");
            
            INTEGRATION_ASSERT_EQ("15", lines[0], "a += 5 should result in 15");
            INTEGRATION_ASSERT_EQ("12", lines[1], "b -= 8 should result in 12");
            INTEGRATION_ASSERT_EQ("24", lines[2], "c *= 4 should result in 24");
            INTEGRATION_ASSERT_EQ("20", lines[3], "a /= 5 should result in 20");
            INTEGRATION_ASSERT_EQ("2", lines[4], "b %= 5 should result in 2");
            INTEGRATION_ASSERT_EQ("11", lines[5], "x += y should result in x=11");
            INTEGRATION_ASSERT_EQ("6", lines[6], "y *= 2 should result in y=6");
        });
    integration_test_passed("compound assign basic test", "basic_compound.cb");
    
    // Bitwise compound assignment operators
    run_cb_test_with_output("../../tests/cases/compound_assign/bitwise_compound.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "bitwise_compound.cb should execute successfully");
            
            auto lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(7, lines.size(), "Should have 7 output lines");
            
            INTEGRATION_ASSERT_EQ("10", lines[0], "flags &= mask should result in 10");
            INTEGRATION_ASSERT_EQ("15", lines[1], "flags |= mask should result in 15");
            INTEGRATION_ASSERT_EQ("6", lines[2], "flags ^= mask should result in 6");
            INTEGRATION_ASSERT_EQ("20", lines[3], "value <<= 2 should result in 20");
            INTEGRATION_ASSERT_EQ("5", lines[4], "value >>= 2 should result in 5");
            INTEGRATION_ASSERT_EQ("5", lines[5], "status |= 4; status |= 1 should result in 5");
            INTEGRATION_ASSERT_EQ("1", lines[6], "status &= ~4 should result in 1");
        });
    integration_test_passed("compound assign bitwise test", "bitwise_compound.cb");
    
    // Array compound assignment
    run_cb_test_with_output("../../tests/cases/compound_assign/array_compound.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "array_compound.cb should execute successfully");
            
            auto lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(10, lines.size(), "Should have 10 output lines");
            
            INTEGRATION_ASSERT_EQ("15", lines[0], "arr[0] += 5 should result in 15");
            INTEGRATION_ASSERT_EQ("12", lines[1], "arr[1] -= 8 should result in 12");
            INTEGRATION_ASSERT_EQ("60", lines[2], "arr[2] *= 2 should result in 60");
            INTEGRATION_ASSERT_EQ("10", lines[3], "arr[3] /= 4 should result in 10");
            INTEGRATION_ASSERT_EQ("1", lines[4], "arr[4] %= 7 should result in 1");
            
            // After adding indices: 15+0, 12+1, 60+2, 10+3, 1+4
            INTEGRATION_ASSERT_EQ("15", lines[5], "arr[0] += 0 should result in 15");
            INTEGRATION_ASSERT_EQ("13", lines[6], "arr[1] += 1 should result in 13");
            INTEGRATION_ASSERT_EQ("62", lines[7], "arr[2] += 2 should result in 62");
            INTEGRATION_ASSERT_EQ("13", lines[8], "arr[3] += 3 should result in 13");
            INTEGRATION_ASSERT_EQ("5", lines[9], "arr[4] += 4 should result in 5");
        });
    integration_test_passed("compound assign array test", "array_compound.cb");
    
    // Complex compound assignment combinations
    run_cb_test_with_output("../../tests/cases/compound_assign/complex_compound.cb", 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_EQ(0, exit_code, "complex_compound.cb should execute successfully");
            
            auto lines = split_lines(output);
            INTEGRATION_ASSERT_EQ(7, lines.size(), "Should have 7 output lines");
            
            INTEGRATION_ASSERT_EQ("20", lines[0], "a += ternary should result in 20");
            INTEGRATION_ASSERT_EQ("9", lines[1], "result += ++c should result in 9");
            INTEGRATION_ASSERT_EQ("9", lines[2], "++c should result in c=9");
            INTEGRATION_ASSERT_EQ("8", lines[3], "result += c++ should result in 8");
            INTEGRATION_ASSERT_EQ("9", lines[4], "c++ should result in c=9");
            INTEGRATION_ASSERT_EQ("1", lines[5], "flags &= (mask | 8) should result in 1");
            INTEGRATION_ASSERT_EQ("50", lines[6], "a *= complex expression should result in 50");
        });
    integration_test_passed("compound assign complex test", "complex_compound.cb");
    
    std::cout << "[integration] Compound assign tests completed" << std::endl;
}
