#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

int run_and_capture(const std::string &input, std::string &output) {
    std::ofstream ofs("test_input.cb");
    ofs << input;
    ofs.close();
    int ret = std::system("./main test_input.cb > test_output.txt");
    std::ifstream ifs("test_output.txt");
    std::stringstream ss;
    ss << ifs.rdbuf();
    output = ss.str();
    std::remove("test_input.cb");
    std::remove("test_output.txt");
    return ret;
}

int main() {
    std::string input = "a = 10; b = a + 5 * 2; print a; print b;";
    std::string output;
    int ret = run_and_capture(input, output);
    assert(ret == 0);
    assert(output.find("10") != std::string::npos);
    assert(output.find("20") != std::string::npos);

    input = "x = -3 + 7; y = x / 2; x = 42; print x; print y;";
    ret = run_and_capture(input, output);
    assert(ret == 0);
    assert(output.find("42") != std::string::npos);
    assert(output.find("2") != std::string::npos);

    input = "print 123;";
    ret = run_and_capture(input, output);
    assert(ret == 0);
    assert(output.find("123") != std::string::npos);

    input = "print z;";
    ret = run_and_capture(input, output);
    assert(ret == 0);
    assert(output.find("0") != std::string::npos);

    printf("All integration tests passed!\n");
    return 0;
}
