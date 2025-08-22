#include "debug_messages.h"

// デバッグメッセージテンプレート配列
const DebugMessageTemplate debug_messages[] = {
    // ノード作成関連
    [static_cast<int>(
        DebugMsgId::NODE_CREATE_STMTLIST)] = {"Creating node: AST_STMT_LIST",
                                              "ノード作成: AST_STMT_LIST"},
    [static_cast<int>(DebugMsgId::NODE_CREATE_TYPESPEC)] =
        {"Creating node: AST_TYPE_SPEC (type=%d)",
         "ノード作成: AST_TYPE_SPEC (type=%d)"},
    [static_cast<int>(DebugMsgId::NODE_CREATE_VAR_DECL)] =
        {"Creating node: AST_VAR_DECL (name=%s)",
         "ノード作成: AST_VAR_DECL (name=%s)"},
    [static_cast<int>(DebugMsgId::NODE_CREATE_ASSIGN)] =
        {"Creating node: AST_ASSIGN (name=%s, with initialization)",
         "ノード作成: AST_ASSIGN (name=%s, 初期化あり)"},
    [static_cast<int>(DebugMsgId::NODE_CREATE_ARRAY_DECL)] =
        {"Creating node: AST_ARRAY_DECL (name=%s)",
         "ノード作成: AST_ARRAY_DECL (name=%s)"},
    [static_cast<int>(DebugMsgId::NODE_CREATE_FUNC_DECL)] =
        {"Creating node: AST_FUNC_DECL (name=%s)",
         "ノード作成: AST_FUNC_DECL (name=%s)"},

    // 関数定義関連
    [static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER)] =
        {"Registering function declaration: %s", "関数宣言を登録: %s"},
    [static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE)] =
        {"Function declaration registration complete: %s",
         "関数宣言登録完了: %s"},
    [static_cast<int>(
        DebugMsgId::PARAM_LIST_START)] = {"  Processing parameter list",
                                          "  パラメータリスト処理開始"},
    [static_cast<int>(
        DebugMsgId::PARAM_LIST_SIZE)] = {"  params->parameters.size() = %zu",
                                         "  params->parameters.size() = %zu"},
    [static_cast<int>(
        DebugMsgId::PARAM_LIST_COMPLETE)] = {"  Parameter move complete",
                                             "  パラメータ移動完了"},
    [static_cast<int>(
        DebugMsgId::PARAM_LIST_DELETE)] = {"  Params node deletion complete",
                                           "  paramsノード削除完了"},
    [static_cast<int>(DebugMsgId::PARAM_LIST_NONE)] = {"  No parameters",
                                                       "  パラメータなし"},
    [static_cast<int>(
        DebugMsgId::FUNC_BODY_START)] = {"  Processing function body",
                                         "  関数ボディ処理開始"},
    [static_cast<int>(DebugMsgId::FUNC_BODY_EXISTS)] = {"  Body exists",
                                                        "  ボディあり"},
    [static_cast<int>(
        DebugMsgId::FUNC_BODY_SET_COMPLETE)] = {"  Body setting complete",
                                                "  ボディ設定完了"},
    [static_cast<int>(DebugMsgId::FUNC_BODY_NONE)] = {"  No body",
                                                      "  ボディなし"},
    [static_cast<int>(DebugMsgId::FUNC_DEF_COMPLETE)] =
        {"  Function definition node creation complete",
         "  関数定義ノード作成完了"},

    // インタープリター関連
    [static_cast<int>(
        DebugMsgId::INTERPRETER_START)] = {"Interpreter::process() starting",
                                           "Interpreter::process() 開始"},
    [static_cast<int>(DebugMsgId::AST_IS_NULL)] = {"AST is null",
                                                   "ASTがnullです"},
    [static_cast<int>(DebugMsgId::GLOBAL_DECL_START)] =
        {"Starting global declaration registration",
         "グローバル宣言の登録を開始"},
    [static_cast<int>(DebugMsgId::GLOBAL_DECL_COMPLETE)] =
        {"Global declaration registration complete",
         "グローバル宣言の登録が完了"},
    [static_cast<int>(
        DebugMsgId::MAIN_FUNC_SEARCH)] = {"Searching for main function",
                                          "main関数を検索中"},
    [static_cast<int>(
        DebugMsgId::MAIN_FUNC_FOUND)] = {"Main function found",
                                         "main関数が見つかりました"},
    [static_cast<int>(
        DebugMsgId::MAIN_FUNC_EXIT)] = {"Main function exited with value %lld",
                                        "main関数が値 %lld で終了しました"},

    // 式評価関連
    [static_cast<int>(DebugMsgId::EXPR_EVAL_NUMBER)] =
        {"Expression evaluation: number literal = %lld",
         "式評価: 数値リテラル = %lld"},
    [static_cast<int>(DebugMsgId::EXPR_EVAL_VAR_REF)] =
        {"Expression evaluation: variable reference = %s",
         "式評価: 変数参照 = %s"},
    [static_cast<int>(DebugMsgId::VAR_VALUE)] = {"  Variable value = %lld",
                                                 "  変数値 = %lld"},
    [static_cast<int>(DebugMsgId::EXPR_EVAL_ARRAY_REF)] =
        {"Expression evaluation: array reference = %s[...]",
         "式評価: 配列参照 = %s[...]"},
    [static_cast<int>(DebugMsgId::ARRAY_INDEX)] = {"  Array index = %lld",
                                                   "  配列インデックス = %lld"},
    [static_cast<int>(DebugMsgId::STRING_ELEMENT_ACCESS)] =
        {"  String element access (UTF-8 compatible)",
         "  文字列要素アクセス (UTF-8対応)"},
    [static_cast<int>(DebugMsgId::STRING_LENGTH_UTF8)] =
        {"  String length (UTF-8 chars) = %zu",
         "  文字列長（UTF-8文字数）= %zu"},
    [static_cast<int>(DebugMsgId::STRING_ELEMENT_VALUE)] =
        {"  String element value = %lld (UTF-8 char: \"%s\")",
         "  文字列要素値 = %lld (UTF-8文字: \"%s\")"},
    [static_cast<int>(
        DebugMsgId::ARRAY_ELEMENT_ACCESS)] = {"  Array element access",
                                              "  配列要素アクセス"},
    [static_cast<int>(
        DebugMsgId::ARRAY_ELEMENT_VALUE)] = {"  Array element value = %lld",
                                             "  配列要素値 = %lld"},
    [static_cast<int>(DebugMsgId::EXPR_EVAL_BINARY_OP)] =
        {"Expression evaluation: binary operation = %s",
         "式評価: 二項演算 = %s"},
    [static_cast<int>(
        DebugMsgId::BINARY_OP_VALUES)] = {"  Left = %lld, Right = %lld",
                                          "  左辺 = %lld, 右辺 = %lld"},

    // メイン関数関連
    [static_cast<int>(
        DebugMsgId::PARSING_START)] = {"Starting syntax analysis: %s",
                                       "構文解析を開始します: %s"},
    [static_cast<int>(
        DebugMsgId::AST_GENERATED)] = {"AST generated successfully",
                                       "ASTが正常に生成されました"},
    [static_cast<int>(
        DebugMsgId::EXECUTION_COMPLETE)] = {"Execution completed successfully",
                                            "実行が正常に終了しました"},

    // 変数代入関連
    [static_cast<int>(DebugMsgId::VAR_ASSIGN)] =
        {"Variable assignment: %s = %lld (type=%d, const=%d)",
         "変数代入: %s = %lld (type=%d, const=%d)"},
    [static_cast<int>(DebugMsgId::VAR_CREATE_NEW)] = {"  Creating new variable",
                                                      "  新規変数を作成"},
    [static_cast<int>(DebugMsgId::VAR_ASSIGN_READABLE)] =
        {"Variable assignment: %s = %lld (type=%s, const=%s)",
         "変数代入: %s = %lld (型=%s, const=%s)"},
    [static_cast<int>(DebugMsgId::STRING_ASSIGN_READABLE)] =
        {"String assignment: %s = \"%s\" (const=%s)",
         "文字列代入: %s = \"%s\" (const=%s)"},
    [static_cast<int>(
        DebugMsgId::STRING_VAR_CREATE_NEW)] = {"  Creating new string variable",
                                               "  新規文字列変数を作成"},
};

// デバッグメッセージ配列のサイズ
const int debug_messages_size =
    static_cast<int>(sizeof(debug_messages) / sizeof(debug_messages[0]));
