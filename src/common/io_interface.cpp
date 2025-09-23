#include "io_interface.h"
#include "../platform/baremetal/baremetal_uart_output.h"
#include "../platform/native/native_stdio_output.h"
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <string>

// IOInterface の基本実装
void IOInterface::write_formatted(const char *format, ...) {
    char buffer[4096]; // 固定サイズバッファ（ベアメタル対応）

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    write_string(buffer);
}

void IOInterface::write_number(int64_t value) {
    char buffer[32]; // int64_tの最大桁数は20桁程度
    snprintf(buffer, sizeof(buffer), "%" PRId64, value);
    write_string(buffer);
}

// IOFactory の実装
IOInterface *IOFactory::instance_ = nullptr;
std::string IOFactory::target_platform_ = "native";

IOInterface *IOFactory::get_instance() {
    if (!instance_) {
        // 実行時指定されたターゲットプラットフォームに応じて実装を選択
        if (target_platform_ == "baremetal") {
            instance_ = create_baremetal_uart_output();
        } else if (target_platform_ == "wasm") {
            // 将来のWebAssembly対応予定
            // 現在は未実装のため、ネイティブ実装を暫定使用
            instance_ =
                create_native_stdio_output(); // 暫定的にネイティブを使用
        } else {
            // "native" またはその他の場合はネイティブ実装
            instance_ = create_native_stdio_output();
        }
    }
    return instance_;
}

void IOFactory::set_instance(IOInterface *io) { instance_ = io; }

void IOFactory::set_target_platform(const std::string &platform) {
    target_platform_ = platform;
    // ターゲットが変更された場合、既存のインスタンスをリセット
    instance_ = nullptr;
}
