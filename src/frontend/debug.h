#ifndef DEBUG_H
#define DEBUG_H

// デバッグ言語設定
enum class DebugLanguage { ENGLISH, JAPANESE };

// デバッグメッセージID
enum class DebugMsgId {
    // ノード作成関連
    NODE_CREATE_STMTLIST,
    NODE_CREATE_TYPESPEC,
    NODE_CREATE_VAR_DECL,
    NODE_CREATE_ASSIGN,
    NODE_CREATE_ARRAY_DECL,
    NODE_CREATE_FUNC_DECL,

    // 関数定義関連
    FUNC_DECL_REGISTER,
    FUNC_DECL_REGISTER_COMPLETE,
    PARAM_LIST_START,
    PARAM_LIST_SIZE,
    PARAM_LIST_COMPLETE,
    PARAM_LIST_DELETE,
    PARAM_LIST_NONE,
    FUNC_BODY_START,
    FUNC_BODY_EXISTS,
    FUNC_BODY_SET_COMPLETE,
    FUNC_BODY_NONE,
    FUNC_DEF_COMPLETE,

    // インタープリター関連
    INTERPRETER_START,
    AST_IS_NULL,
    GLOBAL_DECL_START,
    GLOBAL_DECL_COMPLETE,
    MAIN_FUNC_SEARCH,
    MAIN_FUNC_FOUND,
    MAIN_FUNC_EXIT,

    // 式評価関連
    EXPR_EVAL_NUMBER,
    EXPR_EVAL_VAR_REF,
    VAR_VALUE,
    EXPR_EVAL_ARRAY_REF,
    ARRAY_INDEX,
    STRING_ELEMENT_ACCESS,
    STRING_LENGTH_UTF8,
    STRING_ELEMENT_VALUE,
    ARRAY_ELEMENT_ACCESS,
    ARRAY_ELEMENT_VALUE,
    EXPR_EVAL_BINARY_OP,
    BINARY_OP_VALUES,

    // メイン関数関連
    PARSING_START,
    AST_GENERATED,
    EXECUTION_COMPLETE,

    // 変数代入関連
    VAR_ASSIGN,
    VAR_CREATE_NEW,
};

// デバッグモードフラグ（外部宣言）
extern bool debug_mode;
extern DebugLanguage debug_language;

// デバッグ出力関数（既存）
void debug_print(const char *fmt, ...);

// 多言語対応デバッグ出力関数（新規）
void debug_msg(DebugMsgId msg_id, ...);

#endif // DEBUG_H
