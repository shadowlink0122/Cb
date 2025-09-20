#include "help_messages.h"
#include <cstdio>

// ヘルプメッセージテンプレート配列
const HelpMessageTemplate help_messages[] = {
    [static_cast<int>(
        HelpMsgId::USAGE)] = {"Usage: %s <input.cb> [options]",
                              "使用方法: %s <input.cb> [オプション]"},
    [static_cast<int>(HelpMsgId::OPTIONS_HEADER)] = {"\nOptions:",
                                                     "\nオプション:"},
    [static_cast<int>(HelpMsgId::HELP_OPTION)] =
        {"  --help              Show this help message",
         "  --help              ヘルプメッセージを表示（英語）"},
    [static_cast<int>(HelpMsgId::HELP_JA_OPTION)] =
        {"  --help-ja           Show help message in Japanese",
         "  --help-ja           ヘルプメッセージを表示（日本語）"},
    [static_cast<int>(HelpMsgId::DEBUG_OPTION)] =
        {"  --debug             Enable debug mode (English)",
         "  --debug             デバッグモードを有効にする（英語）"},
    [static_cast<int>(HelpMsgId::DEBUG_JA_OPTION)] =
        {"  --debug-ja          Enable debug mode (Japanese)",
         "  --debug-ja          デバッグモードを有効にする（日本語）"},
    [static_cast<int>(HelpMsgId::TARGET_OPTION)] =
        {"  --target=PLATFORM   Set target platform (default: native)",
         "  --target=PLATFORM   ターゲットプラットフォームを設定（デフォルト: "
         "native）"},
    [static_cast<int>(HelpMsgId::PLATFORMS_HEADER)] =
        {"\nSupported platforms:", "\nサポートされているプラットフォーム:"},
    [static_cast<int>(HelpMsgId::PLATFORM_NATIVE)] =
        {"  native              Native environment (default)",
         "  native              ネイティブ環境（デフォルト）"},
    [static_cast<int>(HelpMsgId::PLATFORM_BAREMETAL)] =
        {"  baremetal           Bare-metal environment",
         "  baremetal           ベアメタル環境"},
    [static_cast<int>(HelpMsgId::PLATFORM_WASM)] =
        {"  wasm                WebAssembly environment",
         "  wasm                WebAssembly環境"},
    [static_cast<int>(HelpMsgId::EXAMPLES_HEADER)] = {"\nExamples:",
                                                      "\n使用例:"},
    [static_cast<int>(HelpMsgId::EXAMPLE_BASIC)] = {"  %s program.cb",
                                                    "  %s program.cb"},
    [static_cast<int>(HelpMsgId::EXAMPLE_DEBUG)] = {"  %s program.cb --debug",
                                                    "  %s program.cb --debug"},
    [static_cast<int>(
        HelpMsgId::EXAMPLE_TARGET)] = {"  %s program.cb --target=baremetal",
                                       "  %s program.cb --target=baremetal"},

    // エラーメッセージ
    [static_cast<int>(HelpMsgId::ERROR_INPUT_NOT_SPECIFIED)] =
        {"Error: Input file not specified",
         "エラー: 入力ファイルが指定されていません"},
    [static_cast<int>(
        HelpMsgId::ERROR_UNKNOWN_OPTION)] = {"Error: Unknown option '%s'",
                                             "エラー: 未知のオプション '%s'"},
    [static_cast<int>(
        HelpMsgId::ERROR_INVALID_TARGET)] = {"Error: Invalid target '%s'",
                                             "エラー: 無効なターゲット '%s'"},
    [static_cast<int>(HelpMsgId::ERROR_CANNOT_OPEN_FILE)] =
        {"Error: Cannot open file '%s'", "エラー: ファイル '%s' を開けません"},
    [static_cast<int>(HelpMsgId::ERROR_PARSING_FAILED)] =
        {"Error: Parsing failed (line: %d)",
         "エラー: 構文解析に失敗しました (行: %d)"},
    [static_cast<int>(HelpMsgId::ERROR_AST_NOT_GENERATED)] =
        {"Error: AST was not generated", "エラー: ASTが生成されませんでした"},
    [static_cast<int>(HelpMsgId::ERROR_UNEXPECTED_EXCEPTION)] =
        {"Error: Unexpected exception occurred",
         "エラー: 予期しない例外が発生しました"},
    [static_cast<int>(
        HelpMsgId::USE_HELP_INFO)] = {"Use --help for usage information",
                                      "--help で使用方法を確認してください"},
    [static_cast<int>(HelpMsgId::VALID_TARGETS_INFO)] = {
        "Valid targets: native, baremetal, wasm",
        "有効なターゲット: native, baremetal, wasm"}};

// ヘルプメッセージ配列のサイズ
const int help_messages_size =
    static_cast<int>(sizeof(help_messages) / sizeof(help_messages[0]));

// ヘルプメッセージを取得する関数
const char *get_help_message(HelpMsgId msg_id, HelpLanguage lang) {
    int index = static_cast<int>(msg_id);
    if (index < 0 || index >= help_messages_size) {
        return lang == HelpLanguage::ENGLISH ? "Unknown message"
                                             : "不明なメッセージ";
    }

    switch (lang) {
    case HelpLanguage::ENGLISH:
        return help_messages[index].en;
    case HelpLanguage::JAPANESE:
        return help_messages[index].ja;
    default:
        return help_messages[index].en;
    }
}

// ヘルプを表示する関数
void show_help(HelpLanguage lang, const char *program_name) {
    std::printf(get_help_message(HelpMsgId::USAGE, lang), program_name);
    std::printf("\n");

    std::printf("%s\n", get_help_message(HelpMsgId::OPTIONS_HEADER, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::HELP_OPTION, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::HELP_JA_OPTION, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::DEBUG_OPTION, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::DEBUG_JA_OPTION, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::TARGET_OPTION, lang));

    std::printf("%s\n", get_help_message(HelpMsgId::PLATFORMS_HEADER, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::PLATFORM_NATIVE, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::PLATFORM_BAREMETAL, lang));
    std::printf("%s\n", get_help_message(HelpMsgId::PLATFORM_WASM, lang));

    std::printf("%s\n", get_help_message(HelpMsgId::EXAMPLES_HEADER, lang));
    std::printf(get_help_message(HelpMsgId::EXAMPLE_BASIC, lang), program_name);
    std::printf("\n");
    std::printf(get_help_message(HelpMsgId::EXAMPLE_DEBUG, lang), program_name);
    std::printf("\n");
    std::printf(get_help_message(HelpMsgId::EXAMPLE_TARGET, lang),
                program_name);
    std::printf("\n");
}
