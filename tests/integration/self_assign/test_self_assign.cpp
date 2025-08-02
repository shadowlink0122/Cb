#include "../run_and_capture_util.h"
#include <cassert>
#include <string>
#include <cstdlib>
#include <cstdio>

inline void test_integration_self_assign_basic() {
    int exit_code = 0;
    const char* tmpfile = "/tmp/self_assign_basic_out.txt";
    std::string cmd = "./main tests/cases/self_assign/basic.cb > " + std::string(tmpfile) + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(exit_code == 0);
    assert(output.find("3") != std::string::npos);
    assert(output.find("2") != std::string::npos);
    assert(output.find("10") != std::string::npos);
    assert(output.find("5") != std::string::npos);
}

inline void test_integration_self_assign_for_update() {
    int exit_code = 0;
    const char* tmpfile = "/tmp/self_assign_for_update_out.txt";
    std::string cmd = "./main tests/cases/self_assign/for_update.cb > " + std::string(tmpfile) + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(exit_code == 0);
    assert(output.find("10") != std::string::npos);
    assert(output.find("40") != std::string::npos);
}

inline void test_integration_self_assign_for_init() {
    int exit_code = 0;
    const char* tmpfile = "/tmp/self_assign_for_init_out.txt";
    std::string cmd = "./main tests/cases/self_assign/for_init.cb > " + std::string(tmpfile) + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(exit_code == 0);
    assert(output.find("2") != std::string::npos);
    assert(output.find("5") != std::string::npos);
    assert(output.find("8") != std::string::npos);
}

inline void test_integration_self_assign_nested() {
    int exit_code = 0;
    const char* tmpfile = "/tmp/self_assign_nested_out.txt";
    std::string cmd = "./main tests/cases/self_assign/nested.cb > " + std::string(tmpfile) + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(exit_code == 0);
    assert(output.find("8") != std::string::npos);
}

void test_integration_self_assign() {
    test_integration_self_assign_basic();
    test_integration_self_assign_for_update();
    test_integration_self_assign_for_init();
    test_integration_self_assign_nested();
}

