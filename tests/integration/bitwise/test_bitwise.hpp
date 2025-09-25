#pragma once

#include "../framework/integration_test_framework.hpp"

inline void test_integration_bitwise() {
    std::cout << "[integration] Running bitwise tests..." << std::endl;
    
    // ビット演算子の基本テスト
    const std::string test_file_basic = "../../tests/cases/bitwise/basic_bitwise.cb";
    run_cb_test_with_output(test_file_basic, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "8", "Expected '8' for bitwise AND: 12 & 10");
            INTEGRATION_ASSERT_CONTAINS(output, "14", "Expected '14' for bitwise OR: 12 | 10");
            INTEGRATION_ASSERT_CONTAINS(output, "6", "Expected '6' for bitwise XOR: 12 ^ 10");
            INTEGRATION_ASSERT_CONTAINS(output, "-6", "Expected '-6' for bitwise NOT: ~5");
            INTEGRATION_ASSERT_CONTAINS(output, "20", "Expected '20' for left shift: 5 << 2");
            INTEGRATION_ASSERT_CONTAINS(output, "5", "Expected '5' for right shift: 20 >> 2");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for bitwise basic test");
        });
    integration_test_passed("bitwise basic test", test_file_basic);
    
    // ビット演算子の複合テスト
    const std::string test_file_complex = "../../tests/cases/bitwise/complex_bitwise.cb";
    run_cb_test_with_output(test_file_complex, 
        [](const std::string& output, int exit_code) {
            INTEGRATION_ASSERT_CONTAINS(output, "5", "Expected '5' for flag setting: 4|1");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Expected '1' for flag check: (5&4)!=0");
            INTEGRATION_ASSERT_CONTAINS(output, "0", "Expected '0' for flag check: (5&2)!=0");
            INTEGRATION_ASSERT_CONTAINS(output, "1", "Expected '1' for flag clear: 5&~4");
            INTEGRATION_ASSERT_CONTAINS(output, "8", "Expected '8' for complex operation: (15^7)&(15|7)");
            INTEGRATION_ASSERT_EQ(0, exit_code, "Expected successful exit code for bitwise complex test");
        });
    integration_test_passed("bitwise complex test", test_file_complex);
    
    std::cout << "[integration] Bitwise tests completed" << std::endl;
}
