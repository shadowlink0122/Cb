#pragma once
#include "../../common/io_interface.h"

// ネイティブ環境（Linux/macOS/Windows）向けの標準入出力実装
class NativeStdioOutput : public IOInterface {
public:
    void write_char(char c) override;
    void write_string(const char* str) override;
    void write_formatted(const char* format, ...) override;
};

// ファクトリー関数
IOInterface* create_native_stdio_output();
