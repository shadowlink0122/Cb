#include "baremetal_uart_output.h"

void BareMetalUARTOutput::write_char(char c) {
    uart_write_byte(static_cast<uint8_t>(c));
}

void BareMetalUARTOutput::write_string(const char* str) {
    while (*str) {
        write_char(*str);
        ++str;
    }
}

void BareMetalUARTOutput::uart_init() {
    // TODO: 実際のハードウェア初期化
    // プラットフォーム固有のUART初期化コードをここに実装
}

void BareMetalUARTOutput::uart_write_byte(uint8_t data) {
    // TODO: 実際のUART書き込み実装
    // 現在はプレースホルダー
    
    // 例: ARM Cortex-M向けの実装例（コメントアウト）
    // volatile uint32_t* uart_data = reinterpret_cast<volatile uint32_t*>(UART_BASE_ADDR);
    // while (!uart_can_transmit()) {
    //     // 送信可能まで待機
    // }
    // *uart_data = data;
}

bool BareMetalUARTOutput::uart_can_transmit() {
    // TODO: 送信準備完了チェック実装
    // volatile uint32_t* uart_status = reinterpret_cast<volatile uint32_t*>(UART_BASE_ADDR + 0x04);
    // return (*uart_status & 0x01) != 0;  // Transmit Ready bit
    return true; // 仮実装
}

// ファクトリー関数の実装
IOInterface* create_baremetal_uart_output() {
    static BareMetalUARTOutput instance;
    return &instance;
}

// ベアメタルターゲット時のデフォルト実装
#ifdef CB_TARGET_BAREMETAL
IOInterface* create_default_io() {
    return create_baremetal_uart_output();
}
#endif
