#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <array>
#include <memory>
#include <cassert>
#include "../run_and_capture_util.h"

// ユーティリティ: コマンド実行＆標準出力取得
std::string run_cb(const std::string& cbfile) {
    std::array<char, 256> buffer;
    std::string result;
    std::string cmd = "./main " + cbfile + " 2>&1";
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void test_string_literal() {
    std::string out = run_cb("./tests/cases/string/literal.cb");
    assert(out.find("Hello, World!") != std::string::npos);
    assert(out.find("abc") != std::string::npos);
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
