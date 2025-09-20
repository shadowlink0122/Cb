#pragma once

// 単体テスト用のダミー関数群
// これらは実際のプログラムで使用される外部関数のダミー実装です

#include <iostream>
#include <vector>
#include <string>

// file_lines のダミー実装
std::vector<std::string> file_lines;

extern "C" {
    // ダミーのyylex関数（パーサーテスト用）
    int yylex() {
        return 0; // EOFを返す
    }
    
    // パーサー関連のグローバル変数
    char* current_filename = nullptr;
    int yylineno = 1;
}

// ダミーのyyerror関数（オーバーロード版、C++）
// parser_utils.oが提供するyyerror関数を使用するので、オーバーロード版のみ定義
inline void yyerror(const char* s, const char* error) {
    // 単体テストでは何もしない
}

// ダミーのdebug_printf関数
inline void debug_printf(const char* fmt, ...) {
    // 単体テストでは何もしない
}
