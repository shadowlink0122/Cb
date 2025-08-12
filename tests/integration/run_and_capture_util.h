#pragma once

#include <string>
#include <cstdio>
#include <sys/wait.h>

static std::string run_and_capture(const std::string &cmd, int *exit_code = nullptr) {
    std::string result;
    std::string sh_cmd = "sh -c '" + cmd + "'"; // cmdは既に 2>&1 を含む（呼び出し側で付与）
    FILE *pipe = popen(sh_cmd.c_str(), "r");
    if (!pipe) {
        if (exit_code) *exit_code = 255;
        return "";
    }
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    int status = pclose(pipe); // ここで終了ステータス取得
    if (exit_code) {
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
