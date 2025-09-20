#ifndef HELP_MESSAGES_H
#define HELP_MESSAGES_H

// ヘルプメッセージのID列挙
enum class HelpMsgId {
    USAGE,
    OPTIONS_HEADER,
    HELP_OPTION,
    HELP_JA_OPTION,
    DEBUG_OPTION,
    DEBUG_JA_OPTION,
    TARGET_OPTION,
    PLATFORMS_HEADER,
    PLATFORM_NATIVE,
    PLATFORM_BAREMETAL,
    PLATFORM_WASM,
    EXAMPLES_HEADER,
    EXAMPLE_BASIC,
    EXAMPLE_DEBUG,
    EXAMPLE_TARGET,
    ERROR_INPUT_NOT_SPECIFIED,
    ERROR_UNKNOWN_OPTION,
    ERROR_INVALID_TARGET,
    ERROR_CANNOT_OPEN_FILE,
    ERROR_PARSING_FAILED,
    ERROR_AST_NOT_GENERATED,
    ERROR_UNEXPECTED_EXCEPTION,
    USE_HELP_INFO,
    VALID_TARGETS_INFO
};

// 言語列挙
enum class HelpLanguage { ENGLISH, JAPANESE };

// ヘルプメッセージテンプレート構造体
struct HelpMessageTemplate {
    const char *en;
    const char *ja;
};

// ヘルプメッセージテンプレート配列（外部宣言）
extern const HelpMessageTemplate help_messages[];

// ヘルプメッセージ配列のサイズ
extern const int help_messages_size;

// ヘルプメッセージ表示関数
void show_help(HelpLanguage lang, const char *program_name);
const char *get_help_message(HelpMsgId msg_id, HelpLanguage lang);

#endif // HELP_MESSAGES_H
