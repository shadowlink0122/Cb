#pragma once
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

static std::string run_and_capture_func(const std::string &cmd, int *exit_code = nullptr) {
    std::string result;
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    int status = pclose(pipe);
    if (exit_code) *exit_code = WEXITSTATUS(status);
    return result;
}

inline void test_integration_func() {
    int exit_code = 0;
    std::string output = run_and_capture_func("./main ./tests/cases/func/integration_func.cb", &exit_code);
    assert(exit_code == 0);
    assert(output.find("1\n") != std::string::npos);
    assert(output.find("20\n") != std::string::npos);
    assert(output.find("30\n") != std::string::npos);
    std::cout << "[integration] func test passed\n";
}
