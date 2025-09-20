#pragma once
#include "../framework/test_framework.hpp"
#include "../../../src/common/io_interface.h"
#include "../../../src/backend/output/output_manager.h"
#include <sstream>
#include <string>
#include <cstdarg>

// テスト用モックIOInterface
class MockIOInterface : public IOInterface {
public:
    std::string captured_output;
    
    void write_char(char c) override {
        captured_output += c;
    }
    
    void write_string(const char* str) override {
        captured_output += str;
    }
    
    void write_formatted(const char* format, ...) override {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        captured_output += buffer;
    }
    
    // テスト用ヘルパー
    void clear() {
        captured_output.clear();
    }
    
    std::string get_output() const {
        return captured_output;
    }
};

// IOInterface基本機能テスト
inline void test_io_interface_write_char() {
    MockIOInterface mock;
    mock.write_char('A');
    mock.write_char('B');
    ASSERT_STREQ("AB", mock.get_output().c_str());
}

inline void test_io_interface_write_string() {
    MockIOInterface mock;
    mock.write_string("Hello");
    mock.write_string(" World");
    ASSERT_STREQ("Hello World", mock.get_output().c_str());
}

inline void test_io_interface_write_number() {
    MockIOInterface mock;
    mock.write_number(42);
    ASSERT_STREQ("42", mock.get_output().c_str());
    
    mock.clear();
    mock.write_number(-123);
    ASSERT_STREQ("-123", mock.get_output().c_str());
}

inline void test_io_interface_write_newline() {
    MockIOInterface mock;
    mock.write_string("Line1");
    mock.write_newline();
    mock.write_string("Line2");
    ASSERT_STREQ("Line1\nLine2", mock.get_output().c_str());
}

inline void test_io_interface_write_line() {
    MockIOInterface mock;
    mock.write_line("Hello");
    mock.write_line("World");
    ASSERT_STREQ("Hello\nWorld\n", mock.get_output().c_str());
}

// IOFactory テスト
inline void test_io_factory_get_instance() {
    IOInterface* io = IOFactory::get_instance();
    ASSERT_NE(nullptr, io);
}

inline void test_io_factory_set_instance() {
    MockIOInterface mock;
    IOInterface* original = IOFactory::get_instance();
    
    IOFactory::set_instance(&mock);
    IOInterface* current = IOFactory::get_instance();
    ASSERT_EQ(&mock, current);
    
    // 元に戻す
    IOFactory::set_instance(original);
}

// 抽象化レイヤー統合テスト
inline void test_output_manager_with_mock_io() {
    MockIOInterface mock;
    IOInterface* original = IOFactory::get_instance();
    IOFactory::set_instance(&mock);
    
    // 仮のInterpreterを作成（実際のテストでは適切に初期化）
    Interpreter* dummy_interpreter = nullptr;
    OutputManager manager(dummy_interpreter);
    
    // テスト実行は後で実装（ASTNode作成が必要）
    // 現在はIOFactoryの動作確認のみ
    ASSERT_EQ(&mock, manager.get_io_interface());
    
    // 元に戻す
    IOFactory::set_instance(original);
}

// プラットフォーム実装テスト
inline void test_native_stdio_output_creation() {
    extern IOInterface* create_native_stdio_output();
    IOInterface* native_io = create_native_stdio_output();
    ASSERT_NE(nullptr, native_io);
}

inline void test_baremetal_uart_output_creation() {
    extern IOInterface* create_baremetal_uart_output();
    IOInterface* baremetal_io = create_baremetal_uart_output();
    ASSERT_NE(nullptr, baremetal_io);
}

// テスト登録
inline void register_io_abstraction_tests() {
    RUN_TEST("io_interface_write_char", test_io_interface_write_char);
    RUN_TEST("io_interface_write_string", test_io_interface_write_string);
    RUN_TEST("io_interface_write_number", test_io_interface_write_number);
    RUN_TEST("io_interface_write_newline", test_io_interface_write_newline);
    RUN_TEST("io_interface_write_line", test_io_interface_write_line);
    RUN_TEST("io_factory_get_instance", test_io_factory_get_instance);
    RUN_TEST("io_factory_set_instance", test_io_factory_set_instance);
    RUN_TEST("output_manager_with_mock_io", test_output_manager_with_mock_io);
    RUN_TEST("native_stdio_output_creation", test_native_stdio_output_creation);
    RUN_TEST("baremetal_uart_output_creation", test_baremetal_uart_output_creation);
}
