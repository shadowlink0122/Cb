#include "test_arithmetic_case.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

static std::string run_and_capture(const std::string &cmd) {
    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

void test_integration_arithmetic() {
    std::string output = run_and_capture("./main ./tests/cases/arithmetic/ok.cb");
    assert(output.find("15") != std::string::npos); // tiny t1
    assert(output.find("5") != std::string::npos);  // tiny t2
    assert(output.find("30000") != std::string::npos); // long l1
    std::cout << "[integration] arithmetic test passed\n";
}
