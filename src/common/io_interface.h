#pragma once
#include <cstdint>
#include <string>

// 出力抽象化インターフェース
// ベアメタル、WebAssembly、ネイティブ環境での出力を統一するためのインターフェース
class IOInterface {
  public:
    virtual ~IOInterface() = default;

    // 基本出力機能
    virtual void write_char(char c) = 0;
    virtual void write_string(const char *str) = 0;

    // フォーマット出力（基本実装を提供）
    virtual void write_formatted(const char *format, ...);

    // 改行付き出力（便利メソッド）
    virtual void write_line(const char *str) {
        write_string(str);
        write_char('\n');
    }

    // 空行出力
    virtual void write_newline() { write_char('\n'); }

    // 数値出力（便利メソッド）
    virtual void write_number(int64_t value);
    virtual void write_float(double value);
};

// プラットフォーム固有の実装を取得するファクトリー
class IOFactory {
  public:
    static IOInterface *get_instance();
    static void set_instance(IOInterface *io);
    static void set_target_platform(const std::string &platform);

  private:
    static IOInterface *instance_;
    static std::string target_platform_;
};

// プラットフォーム固有の実装を作成するファクトリー関数
IOInterface *create_native_stdio_output();
IOInterface *create_baremetal_uart_output();
