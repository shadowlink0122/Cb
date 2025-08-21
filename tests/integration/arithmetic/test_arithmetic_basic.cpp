#include <iostream>
#include <cassert>
#include <string>
#include "../run_and_capture_util.h"

void test_integration_arithmetic_basic() {
    int exit_code = 0;
    std::string cmd = "./main tests/cases/arithmetic/ok.cb 2>&1";
    std::string output = run_and_capture(cmd, &exit_code);
    
    // 正常実行を確認
    if (exit_code != 0) {
        std::cerr << "arithmetic ok.cb failed with exit code: " << exit_code << std::endl;
        std::cerr << "Output: " << output << std::endl;
        assert(false);
    }
    
    // 期待する値が含まれているか確認
    assert(output.find("15") != std::string::npos);  // 10 + 5
    assert(output.find("5") != std::string::npos);   // 10 - 5, 10 / 2
    assert(output.find("30000") != std::string::npos); // long l1
    
    std::cout << "[integration] arithmetic basic test passed" << std::endl;
}
