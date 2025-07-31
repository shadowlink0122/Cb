#pragma once

#include <string>
#include <cstdio>
#include <sys/wait.h>

static std::string run_and_capture(const std::string &cmd, int *exit_code = nullptr) {
    std::string result;
    std::string sh_cmd = "sh -c '" + cmd + "'";
    // 1. 標準出力・標準エラー出力を取得
    FILE *pipe = popen(sh_cmd.c_str(), "r");
    if (!pipe)
        return "";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    pclose(pipe);
    // 2. exit codeだけsystem()で取得
    if (exit_code) {
        int status = system(sh_cmd.c_str());
        if (WIFEXITED(status)) {
            *exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            *exit_code = 128 + WTERMSIG(status);
        } else {
            *exit_code = 255;
        }
    }
    return result;
}
