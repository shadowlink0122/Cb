#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <cassert>
#include "../run_and_capture_util.h"

static std::string run_cb(const std::string& cbfile) {
    return run_and_capture("./main " + cbfile + " 2>&1");
}

void test_string_literal() {
    std::string out = run_cb("./tests/cases/string/literal.cb");
    assert(out.find("Hello, World!") != std::string::npos);
    assert(out.find("abc") != std::string::npos);
    // string要素アクセス: "abc" の 0,1,2番目
    std::string idx_out = run_cb("./tests/cases/string/element.cb");
    assert(idx_out.find("a") != std::string::npos);
    assert(idx_out.find("b") != std::string::npos);
    assert(idx_out.find("c") != std::string::npos);
}

void test_string_func() {
    std::string out = run_cb("./tests/cases/string/func.cb");
    assert(out.find("Hello, World!") != std::string::npos);
    assert(out.find("こんにちは") != std::string::npos);
}

void test_string_assign() {
    std::string out = run_cb("./tests/cases/string/assign.cb");
    assert(out.find("test string") != std::string::npos);
    assert(out.find("second") != std::string::npos);
}

void test_string_empty() {
    std::string out = run_cb("./tests/cases/string/empty.cb");
    // ダブルクォート付き空文字列が2回出力されることを確認
    int count = 0;
    size_t pos = 0;
    while ((pos = out.find("", pos)) != std::string::npos) {
        ++count;
        pos += 2;
    }
    assert(count == 2);
}

void test_integration_string() {
    test_string_literal();
    test_string_func();
    test_string_assign();
    test_string_empty();
    std::cout << "[integration] string tests passed" << std::endl;
}
