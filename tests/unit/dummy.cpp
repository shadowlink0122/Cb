// test専用のダミー変数・関数定義
#include <string>
#include <vector>

extern "C" {
char *current_filename = nullptr;
int yylineno = 1;
}

// file_linesは実際のコードと同じ型で定義
std::vector<std::string> file_lines;
