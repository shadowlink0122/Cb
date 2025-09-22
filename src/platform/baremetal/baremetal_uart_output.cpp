#include "baremetal_uart_output.h"

void BaremetalUartOutput::write_char(char c) {
    // ベアメタル環境でのUART出力
    // この実装はプラットフォーム固有であり、実際のハードウェアに依存
    // ここでは空の実装（将来の拡張用）
    (void)c; // 未使用パラメータ警告の抑制
}

void BaremetalUartOutput::write_string(const char *str) {
    // ベアメタル環境でのUART文字列出力
    // この実装はプラットフォーム固有であり、実際のハードウェアに依存
    // ここでは空の実装（将来の拡張用）
    (void)str; // 未使用パラメータ警告の抑制
}

IOInterface *create_baremetal_uart_output() {
    return new BaremetalUartOutput();
}
