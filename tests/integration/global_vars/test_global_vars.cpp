#include <cassert>
#include <string>
#include <iostream>
#include "../run_and_capture_util.h"

static std::string run_cb(const std::string &cbfile, int *exit_code = nullptr) {
    return run_and_capture(std::string("./main ") + cbfile + " 2>&1", exit_code);
}

static void test_global_basic() {
    std::string out = run_cb("./tests/cases/global_vars/basic.cb");
    assert(out.find("10") != std::string::npos);
    assert(out.find("11") != std::string::npos);
    assert(out.find("1014") != std::string::npos);
    assert(out.find("1041") != std::string::npos);
    assert(out.find("ok") != std::string::npos);
}

static void test_global_array_share() {
    std::string out = run_cb("./tests/cases/global_vars/array_share.cb");
    assert(out.find("2") != std::string::npos);
    size_t pos = out.find("12");
    assert(pos != std::string::npos);
    pos = out.find("12", pos + 1);
    assert(pos != std::string::npos);
}

static void test_global_redeclare() {
    int exit_code = 0;
    std::string out = run_cb("./tests/cases/global_vars/redeclare.cb", &exit_code);
    assert(exit_code != 0);
    assert(out.find("再宣言") != std::string::npos || out.find("再宣言はできません") != std::string::npos);
}

void test_integration_global_vars() {
    test_global_basic();
    test_global_array_share();
    test_global_redeclare();
    std::cout << "[integration] global vars tests passed" << std::endl;
}
