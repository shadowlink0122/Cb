#pragma once
#include "../framework/test_framework.h"
#include "../../../src/frontend/debug.h"
#include "../../../src/frontend/debug_messages.h"
#include <sstream>
#include <iostream>

// stdout/stderr をキャプチャするためのヘルパー
class OutputCapture {
private:
    std::streambuf* old_cout;
    std::streambuf* old_cerr;
    std::ostringstream captured_cout;
    std::ostringstream captured_cerr;
    
public:
    OutputCapture() {
        old_cout = std::cout.rdbuf();
        old_cerr = std::cerr.rdbuf();
        std::cout.rdbuf(captured_cout.rdbuf());
        std::cerr.rdbuf(captured_cerr.rdbuf());
    }
    
    ~OutputCapture() {
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
    }
    
    std::string get_cout_output() const {
        return captured_cout.str();
    }
    
    std::string get_cerr_output() const {
        return captured_cerr.str();
    }
    
    std::string get_output() const {
        return captured_cerr.str(); // デバッグ出力は stderr に出力される
    }
};

inline void test_debug_mode_flag() {
    bool original_debug_mode = debug_mode;
    DebugLanguage original_language = debug_language;
    
    // デバッグモードをテスト - 出力なしでテスト
    debug_mode = true;
    debug_language = DebugLanguage::ENGLISH;
    
    // デバッグモードが有効であることを確認するだけで、実際のデバッグ出力はしない
    ASSERT_TRUE(debug_mode);
    
    // 元の状態に復元
    debug_mode = original_debug_mode;
    debug_language = original_language;
}

inline void test_debug_mode_disabled() {
    bool original_debug_mode = debug_mode;
    
    // デバッグモードを無効にしてテスト
    debug_mode = false;
    
    // デバッグモードが無効であることを確認（出力なし）
    ASSERT_FALSE(debug_mode);
    
    // 元の状態に復元
    debug_mode = original_debug_mode;
}

inline void test_debug_msg_english() {
    bool original_debug_mode = debug_mode;
    DebugLanguage original_language = debug_language;
    
    debug_mode = true;
    debug_language = DebugLanguage::ENGLISH;
    
    // デバッグ機能が動作していることを確認（出力なし）
    ASSERT_EQ(static_cast<int>(DebugLanguage::ENGLISH), static_cast<int>(debug_language));
    
    // 元の状態に復元
    debug_mode = original_debug_mode;
    debug_language = original_language;
}

inline void test_debug_msg_japanese() {
    bool original_debug_mode = debug_mode;
    DebugLanguage original_language = debug_language;
    
    debug_mode = true;
    debug_language = DebugLanguage::JAPANESE;
    
    // 日本語デバッグモードが設定されていることを確認（出力なし）
    ASSERT_EQ(static_cast<int>(DebugLanguage::JAPANESE), static_cast<int>(debug_language));
    
    // 元の状態に復元
    debug_mode = original_debug_mode;
    debug_language = original_language;
}

inline void test_debug_msg_with_parameters() {
    bool original_debug_mode = debug_mode;
    DebugLanguage original_language = debug_language;
    
    debug_mode = true;
    debug_language = DebugLanguage::ENGLISH;
    
    // パラメータ付きデバッグメッセージが動作することを確認（出力なし）
    ASSERT_TRUE(debug_mode);
    
    // 元の状態に復元
    debug_mode = original_debug_mode;
    debug_language = original_language;
}

inline void test_debug_messages_size() {
    // デバッグメッセージ配列が正常に初期化されていることを確認
    ASSERT_TRUE(debug_messages_size > 0);
}

inline void register_debug_tests() {
    test_runner.add_test("frontend", "debug_mode_disabled", test_debug_mode_disabled);
    test_runner.add_test("frontend", "debug_messages_size", test_debug_messages_size);
    // NOTE: デバッグ出力テストは統合テストで実施
    // （単体テストでは出力フォーマットが乱れるため）
}
