#pragma once
#include <map>
#include <string>

enum class MessageId {
    // Debug messages
    DEBUG_MAIN_FUNCTION_EXECUTE,
    DEBUG_MAIN_FUNCTION_BODY_EXISTS,
    DEBUG_MAIN_FUNCTION_BODY_NULL,
    DEBUG_EXECUTING_STATEMENT,
    DEBUG_VARIABLE_DECLARATION_INIT,
    DEBUG_PROCESSING_MULTIDIM_ARRAY,
    DEBUG_EMPTY_ARRAY_LITERAL,
    DEBUG_PROCESSING_2D_ARRAY,
    DEBUG_SET_ELEMENT,
    DEBUG_PROCESSING_1D_ARRAY,
    DEBUG_ARRAY_LITERAL_INIT,
    DEBUG_PROCESSING_ELEMENT,
    DEBUG_TYPE_MISMATCH_STRING_EXPECTED,
    DEBUG_TYPE_MISMATCH_STRING_FOUND,
    DEBUG_EVALUATED_VALUE,
    DEBUG_PRINT_STATEMENT_EXECUTE,
    DEBUG_PRINT_STATEMENT_HAS_ARGS,
    DEBUG_PRINT_STATEMENT_HAS_LEFT,
    DEBUG_PRINT_STATEMENT_NO_ARGS,
    DEBUG_ARRAY_REF_EVALUATION,
    DEBUG_NODE_POINTER,

    // Error messages
    ERROR_UNDEFINED_VARIABLE,
    ERROR_TYPE_MISMATCH,
    ERROR_ARRAY_INDEX_OUT_OF_BOUNDS,
    ERROR_DIVISION_BY_ZERO,

    // Info messages
    INFO_PROGRAM_START,
    INFO_PROGRAM_END
};

enum class Language { JAPANESE, ENGLISH };

class MessageManager {
  private:
    static Language current_language_;
    static std::map<MessageId, std::map<Language, std::string>> messages_;
    static void initialize_messages();

  public:
    static void set_language(Language lang);
    static std::string get_message(MessageId id);
    static std::string get_debug_message(MessageId id);
};
