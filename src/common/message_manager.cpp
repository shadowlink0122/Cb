#include "message_manager.h"
#include "debug.h"
#include <iostream>

Language MessageManager::current_language_ = Language::JAPANESE;
std::map<MessageId, std::map<Language, std::string>> MessageManager::messages_;

void MessageManager::initialize_messages() {
    // Debug messages
    messages_[MessageId::DEBUG_MAIN_FUNCTION_EXECUTE][Language::JAPANESE] =
        "メイン関数を実行します";
    messages_[MessageId::DEBUG_MAIN_FUNCTION_EXECUTE][Language::ENGLISH] =
        "About to execute main function body";

    messages_[MessageId::DEBUG_MAIN_FUNCTION_BODY_EXISTS][Language::JAPANESE] =
        "メイン関数本体が存在します";
    messages_[MessageId::DEBUG_MAIN_FUNCTION_BODY_EXISTS][Language::ENGLISH] =
        "Main function body exists";

    messages_[MessageId::DEBUG_MAIN_FUNCTION_BODY_NULL][Language::JAPANESE] =
        "メイン関数本体がnullです";
    messages_[MessageId::DEBUG_MAIN_FUNCTION_BODY_NULL][Language::ENGLISH] =
        "Main function body is null";

    messages_[MessageId::DEBUG_EXECUTING_STATEMENT][Language::JAPANESE] =
        "文を実行中: %s";
    messages_[MessageId::DEBUG_EXECUTING_STATEMENT][Language::ENGLISH] =
        "Executing statement: %s";

    messages_[MessageId::DEBUG_VARIABLE_DECLARATION_INIT][Language::JAPANESE] =
        "変数宣言（初期化あり）: %s";
    messages_[MessageId::DEBUG_VARIABLE_DECLARATION_INIT][Language::ENGLISH] =
        "Variable declaration with initialization: %s";

    messages_[MessageId::DEBUG_PROCESSING_MULTIDIM_ARRAY][Language::JAPANESE] =
        "多次元配列を処理中: %s";
    messages_[MessageId::DEBUG_PROCESSING_MULTIDIM_ARRAY][Language::ENGLISH] =
        "Processing multidimensional array: %s";

    messages_[MessageId::DEBUG_EMPTY_ARRAY_LITERAL][Language::JAPANESE] =
        "空の配列リテラル";
    messages_[MessageId::DEBUG_EMPTY_ARRAY_LITERAL][Language::ENGLISH] =
        "Empty array literal";

    messages_[MessageId::DEBUG_PROCESSING_2D_ARRAY][Language::JAPANESE] =
        "2次元配列リテラルを処理中";
    messages_[MessageId::DEBUG_PROCESSING_2D_ARRAY][Language::ENGLISH] =
        "Processing 2D array literal";

    messages_[MessageId::DEBUG_SET_ELEMENT][Language::JAPANESE] =
        "要素[%d][%d] = %ld を設定";
    messages_[MessageId::DEBUG_SET_ELEMENT][Language::ENGLISH] =
        "Set element[%d][%d] = %ld";

    messages_[MessageId::DEBUG_PROCESSING_1D_ARRAY][Language::JAPANESE] =
        "1次元配列リテラルを処理中（多次元配列内）";
    messages_[MessageId::DEBUG_PROCESSING_1D_ARRAY][Language::ENGLISH] =
        "Processing 1D array literal in multidimensional array";

    messages_[MessageId::DEBUG_ARRAY_LITERAL_INIT][Language::JAPANESE] =
        "配列リテラル初期化を処理中";
    messages_[MessageId::DEBUG_ARRAY_LITERAL_INIT][Language::ENGLISH] =
        "Processing array literal initialization";

    messages_[MessageId::DEBUG_PROCESSING_ELEMENT][Language::JAPANESE] =
        "要素 %zu を処理中、型: %d";
    messages_[MessageId::DEBUG_PROCESSING_ELEMENT][Language::ENGLISH] =
        "Processing element %zu, type: %d";

    messages_[MessageId::DEBUG_TYPE_MISMATCH_STRING_EXPECTED]
             [Language::JAPANESE] =
                 "型不一致: 文字列配列には文字列リテラルが必要";
    messages_[MessageId::DEBUG_TYPE_MISMATCH_STRING_EXPECTED]
             [Language::ENGLISH] =
                 "Type mismatch: expected string literal in string array";

    messages_[MessageId::DEBUG_TYPE_MISMATCH_STRING_FOUND][Language::JAPANESE] =
        "型不一致: 整数配列に文字列リテラルが見つかりました";
    messages_[MessageId::DEBUG_TYPE_MISMATCH_STRING_FOUND][Language::ENGLISH] =
        "Type mismatch: found string literal in integer array";

    messages_[MessageId::DEBUG_EVALUATED_VALUE][Language::JAPANESE] =
        "評価値: %ld";
    messages_[MessageId::DEBUG_EVALUATED_VALUE][Language::ENGLISH] =
        "Evaluated value: %ld";

    messages_[MessageId::DEBUG_PRINT_STATEMENT_EXECUTE][Language::JAPANESE] =
        "print文を実行中";
    messages_[MessageId::DEBUG_PRINT_STATEMENT_EXECUTE][Language::ENGLISH] =
        "Executing print statement";

    messages_[MessageId::DEBUG_PRINT_STATEMENT_HAS_ARGS][Language::JAPANESE] =
        "print文に引数があります";
    messages_[MessageId::DEBUG_PRINT_STATEMENT_HAS_ARGS][Language::ENGLISH] =
        "Print statement has arguments";

    messages_[MessageId::DEBUG_PRINT_STATEMENT_HAS_LEFT][Language::JAPANESE] =
        "print文にleftノードがあります";
    messages_[MessageId::DEBUG_PRINT_STATEMENT_HAS_LEFT][Language::ENGLISH] =
        "Print statement has left node";

    messages_[MessageId::DEBUG_PRINT_STATEMENT_NO_ARGS][Language::JAPANESE] =
        "print文に引数がありません";
    messages_[MessageId::DEBUG_PRINT_STATEMENT_NO_ARGS][Language::ENGLISH] =
        "Print statement has no arguments";

    messages_[MessageId::DEBUG_ARRAY_REF_EVALUATION][Language::JAPANESE] =
        "配列参照の評価を開始";
    messages_[MessageId::DEBUG_ARRAY_REF_EVALUATION][Language::ENGLISH] =
        "AST_ARRAY_REF evaluation started";

    messages_[MessageId::DEBUG_NODE_POINTER][Language::JAPANESE] =
        "ノードポインタ: %p";
    messages_[MessageId::DEBUG_NODE_POINTER][Language::ENGLISH] =
        "node pointer: %p";

    // Error messages
    messages_[MessageId::ERROR_UNDEFINED_VARIABLE][Language::JAPANESE] =
        "未定義変数: %s";
    messages_[MessageId::ERROR_UNDEFINED_VARIABLE][Language::ENGLISH] =
        "Undefined variable: %s";

    messages_[MessageId::ERROR_TYPE_MISMATCH][Language::JAPANESE] = "型不一致";
    messages_[MessageId::ERROR_TYPE_MISMATCH][Language::ENGLISH] =
        "Type mismatch";

    messages_[MessageId::ERROR_ARRAY_INDEX_OUT_OF_BOUNDS][Language::JAPANESE] =
        "配列インデックスが範囲外";
    messages_[MessageId::ERROR_ARRAY_INDEX_OUT_OF_BOUNDS][Language::ENGLISH] =
        "Array index out of bounds";

    messages_[MessageId::ERROR_DIVISION_BY_ZERO][Language::JAPANESE] =
        "ゼロ除算エラー";
    messages_[MessageId::ERROR_DIVISION_BY_ZERO][Language::ENGLISH] =
        "Division by zero";

    // Info messages
    messages_[MessageId::INFO_PROGRAM_START][Language::JAPANESE] =
        "プログラム開始";
    messages_[MessageId::INFO_PROGRAM_START][Language::ENGLISH] =
        "Program start";

    messages_[MessageId::INFO_PROGRAM_END][Language::JAPANESE] =
        "プログラム終了";
    messages_[MessageId::INFO_PROGRAM_END][Language::ENGLISH] = "Program end";
}

void MessageManager::set_language(Language lang) {
    current_language_ = lang;
    if (messages_.empty()) {
        initialize_messages();
    }
}

std::string MessageManager::get_message(MessageId id) {
    if (messages_.empty()) {
        initialize_messages();
    }

    auto msg_it = messages_.find(id);
    if (msg_it == messages_.end()) {
        return "Unknown message";
    }

    auto lang_it = msg_it->second.find(current_language_);
    if (lang_it == msg_it->second.end()) {
        // フォールバックとして英語を使用
        lang_it = msg_it->second.find(Language::ENGLISH);
        if (lang_it == msg_it->second.end()) {
            return "Message not found";
        }
    }

    return lang_it->second;
}

std::string MessageManager::get_debug_message(MessageId id) {
    if (!debug_mode) {
        return "";
    }
    return get_message(id);
}
