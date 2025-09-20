#pragma once
#include "../../common/io_interface.h"

// ベアメタル環境向けのUART出力実装
// 実際のハードウェア依存部分は将来実装
class BareMetalUARTOutput : public IOInterface {
private:
    static constexpr uint32_t UART_BASE_ADDR = 0x10000000;  // 仮想アドレス
    
public:
    void write_char(char c) override;
    void write_string(const char* str) override;
    
private:
    void uart_init();
    void uart_write_byte(uint8_t data);
    bool uart_can_transmit();
};

// ファクトリー関数
IOInterface* create_baremetal_uart_output();
