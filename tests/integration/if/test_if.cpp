#include "../run_and_capture_util.h"
#include "test_if_case.h"
#include <string>
#include <cassert>
#include <iostream>

void test_integration_if_basic() {
    std::string output = run_and_capture("./main tests/cases/if/basic.cb");
    // 出力は "ok\nok2\nok3\n" になるはず
    assert(output.find("ok") != std::string::npos);
    assert(output.find("ok2") != std::string::npos);
    assert(output.find("ok3") != std::string::npos);
    std::cout << "[integration] if tests passed" << std::endl;
}
