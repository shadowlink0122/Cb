#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <cassert>
#include "../run_and_capture_util.h"

static std::string run_cb(const std::string& cbfile) {
    return run_and_capture("./main " + cbfile + " 2>&1");
}

void test_array_basic() {
    std::string out = run_cb("./tests/cases/array/basic.cb");
    assert(out.find("10") != std::string::npos);
    assert(out.find("50") != std::string::npos);
    assert(out.find("99") != std::string::npos);
    assert(out.find("219") != std::string::npos);
}

void test_array_assign() {
    std::string out = run_cb("./tests/cases/array/assign.cb");
    assert(out.find("42") != std::string::npos);
    assert(out.find("43") != std::string::npos);
    assert(out.find("86") != std::string::npos);
    assert(out.find("76") != std::string::npos);
}

void test_array_literal() {
    std::string out = run_cb("./tests/cases/array/literal.cb");
    assert(out.find("5") != std::string::npos);
    assert(out.find("6") != std::string::npos);
    assert(out.find("7") != std::string::npos);
    assert(out.find("10") != std::string::npos);
    assert(out.find("20") != std::string::npos);
}

void test_array_boundary() {
    std::string out = run_cb("./tests/cases/array/boundary.cb");
    assert(out.find("1") != std::string::npos);
    assert(out.find("3") != std::string::npos);
    // 範囲外アクセスはコメントアウト中
}

void test_multidim_basic() {
    std::string out = run_cb("./tests/cases/array/multidim_basic.cb");
    assert(out.find("10") != std::string::npos);
    assert(out.find("20") != std::string::npos);
    assert(out.find("30") != std::string::npos);
    assert(out.find("40") != std::string::npos);
    assert(out.find("50") != std::string::npos);
    assert(out.find("60") != std::string::npos);
}

void test_nested_literal() {
    std::string out = run_cb("./tests/cases/array/nested_literal.cb");
    assert(out.find("1") != std::string::npos);
    assert(out.find("2") != std::string::npos);
    assert(out.find("3") != std::string::npos);
    assert(out.find("4") != std::string::npos);
    assert(out.find("5") != std::string::npos);
    assert(out.find("6") != std::string::npos);
    assert(out.find("99") != std::string::npos);
}

void test_multidim_3d() {
    std::string out = run_cb("./tests/cases/array/multidim_3d.cb");
    assert(out.find("1") != std::string::npos);
    assert(out.find("4") != std::string::npos);
    assert(out.find("5") != std::string::npos);
    assert(out.find("8") != std::string::npos);
}

void test_integration_array() {
    test_array_basic();
    test_array_assign();
    test_array_literal();
    test_array_boundary();
    test_multidim_basic();
    test_nested_literal();
    test_multidim_3d();
    std::cout << "[integration] array tests passed" << std::endl;
}
