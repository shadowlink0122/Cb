#include "test_incdec_case.h"

#include "../run_and_capture_util.h"
#include <cassert>
#include <string>

inline void test_integration_incdec_pre_post() {
    int exit_code = 0;
    const char* tmpfile = "/tmp/incdec_pre_post_out.txt";
    std::string cmd = "./main tests/cases/incdec/pre_post.cb > " + std::string(tmpfile) + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(exit_code == 0);
    assert(output.find("2") != std::string::npos); // a, b, d, e
    assert(output.find("3") != std::string::npos); // a after a++
    assert(output.find("1") != std::string::npos); // a after a--
}

inline void test_integration_incdec_for_loop() {
    int exit_code = 0;
    const char* tmpfile = "/tmp/incdec_for_loop_out.txt";
    std::string cmd = "./main tests/cases/incdec/for_loop.cb > " + std::string(tmpfile) + " 2>&1";
    run_and_capture(cmd.c_str(), &exit_code);
    FILE* fp = fopen(tmpfile, "r");
    std::string output;
    if (fp) { char buf[4096]; while (fgets(buf, sizeof(buf), fp)) output += buf; fclose(fp); }
    remove(tmpfile);
    assert(exit_code == 0);
    assert(output.find("3") != std::string::npos);
    assert(output.find("9") != std::string::npos);
}

void test_integration_incdec() {
    test_integration_incdec_pre_post();
    test_integration_incdec_for_loop();
}
