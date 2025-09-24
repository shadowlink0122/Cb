// dummy.cpp - Unit test用のダミー関数実装
#include "framework/test_framework.hpp"
#include <string>
#include <vector>

// グローバルテストランナーインスタンスの定義
TestRunner test_runner;

// リンクエラーを避けるためのダミー変数定義
extern "C" {
const char *current_filename = nullptr;
int yylineno = 1;
}
std::vector<std::string> file_lines;
