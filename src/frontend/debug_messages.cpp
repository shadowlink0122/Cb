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

    // 配列初期化関連
    [static_cast<int>(DebugMsgId::ARRAY_INIT_CALLED)] =
        {"create_array_init called: name=%s",
         "create_array_init呼び出し: 名前=%s"},
    [static_cast<int>(DebugMsgId::ARRAY_INIT_WITH_TYPE_CALLED)] =
        {"create_array_init_with_type called: name=%s, expected_type=%d",
         "create_array_init_with_type呼び出し: 名前=%s, 期待型=%d"},
    [static_cast<int>(
        DebugMsgId::ARRAY_LITERAL_CALLED)] = {"create_array_literal called",
                                              "create_array_literal呼び出し"},
    [static_cast<int>(DebugMsgId::ARRAY_LITERAL_COMPLETED)] =
        {"create_array_literal completed", "create_array_literal完了"},
    [static_cast<int>(
        DebugMsgId::ARRAY_INIT_COMPLETED)] = {"create_array_init completed",
                                              "create_array_init完了"},
    [static_cast<int>(DebugMsgId::ARRAY_INIT_WITH_TYPE_COMPLETED)] =
        {"create_array_init_with_type completed",
         "create_array_init_with_type完了"},
    [static_cast<int>(DebugMsgId::ARRAY_LITERAL_ELEMENTS)] =
        {"array literal has %zu elements",
         "配列リテラルは%zu個の要素を持ちます"},
    [static_cast<int>(DebugMsgId::ARRAY_INIT_ELEMENTS)] =
        {"init_list has %zu elements", "初期化リストは%zu個の要素を持ちます"},
    [static_cast<int>(DebugMsgId::TYPE_MISMATCH_ARRAY_INIT)] =
        {"Type mismatch in array initialization: element %zu expected %s but "
         "got %s",
         "配列初期化で型不整合: 要素%zuは%s型が期待されましたが%s型でした"},
    [static_cast<int>(
        DebugMsgId::CURRENT_TYPE_SET)] = {"Current declared type set to: %d",
                                          "現在宣言されている型を設定: %d"},

    // エラーメッセージ
    [static_cast<int>(DebugMsgId::PARSER_ERROR)] = {"Parser error",
                                                    "パーサーエラー"},
    [static_cast<int>(DebugMsgId::TYPE_MISMATCH_ERROR)] =
        {"Error: Array '%s' element %zu: %s type expected but %s type provided",
         "エラー: 配列'%s'の要素%zu: %s型が期待されましたが%s型が渡されました"},

    // 型システム関連
    [static_cast<int>(
        DebugMsgId::TYPEDEF_REGISTER)] = {"Registering type alias: %s -> %s",
                                          "型エイリアスを登録: %s -> %s"},
    [static_cast<int>(DebugMsgId::TYPEDEF_REGISTER_SUCCESS)] =
        {"Type alias registered successfully: %s",
         "型エイリアスの登録が完了: %s"},
    [static_cast<int>(
        DebugMsgId::TYPE_ALIAS_RESOLVE)] = {"Resolving type alias: %s",
                                            "型エイリアスを解決: %s"},
    [static_cast<int>(
        DebugMsgId::TYPE_ALIAS_CREATE_NODE)] = {"Creating type alias node: %s",
                                                "型エイリアスノードを作成: %s"},
    [static_cast<int>(DebugMsgId::TYPE_ALIAS_RUNTIME_RESOLVE)] =
        {"Runtime type alias resolution: %s -> %s",
         "実行時型エイリアス解決: %s -> %s"},

    // インタープリターエラー関連
    [static_cast<int>(DebugMsgId::VAR_REDECLARE_ERROR)] =
        {"Variable '%s' redeclaration not allowed",
         "変数 '%s' の再宣言はできません"},
    [static_cast<int>(
        DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR)] = {"Array size is negative: %s",
                                                   "配列サイズが負です: %s"},
    [static_cast<int>(
        DebugMsgId::MAIN_FUNC_NOT_FOUND_ERROR)] = {"main function not found",
                                                   "main関数が見つかりません"},
    [static_cast<int>(
        DebugMsgId::UNDEFINED_VAR_ERROR)] = {"Undefined variable: %s",
                                             "未定義の変数です: %s"},
    [static_cast<int>(DebugMsgId::DIRECT_ARRAY_REF_ERROR)] =
        {"Direct reference to array variable not allowed: %s",
         "配列変数への直接参照はできません: %s"},
    [static_cast<int>(
        DebugMsgId::UNDEFINED_ARRAY_ERROR)] = {"Undefined array: %s",
                                               "未定義の配列です: %s"},
    [static_cast<int>(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR)] =
        {"String out of bounds access: %s (index=%lld, length=%zu)",
         "文字列の範囲外アクセス: %s (index=%lld, length=%zu)"},
    [static_cast<int>(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR)] =
        {"Array out of bounds access: %s", "配列の範囲外アクセス: %s"},
    [static_cast<int>(
        DebugMsgId::NON_ARRAY_REF_ERROR)] = {"Array reference to non-array: %s",
                                             "配列以外への配列参照です: %s"},
    [static_cast<int>(
        DebugMsgId::ZERO_DIVISION_ERROR)] = {"Division by zero error",
                                             "ゼロ除算エラー"},
    [static_cast<int>(
        DebugMsgId::UNKNOWN_BINARY_OP_ERROR)] = {"Unknown binary operator: %s",
                                                 "未知の二項演算子: %s"},
    [static_cast<int>(
        DebugMsgId::UNKNOWN_UNARY_OP_ERROR)] = {"Unknown unary operator: %s",
                                                "未知の単項演算子: %s"},
    [static_cast<int>(
        DebugMsgId::UNDEFINED_FUNC_ERROR)] = {"Undefined function: %s",
                                              "未定義の関数です: %s"},
    [static_cast<int>(
        DebugMsgId::ARG_COUNT_MISMATCH_ERROR)] = {"Argument count mismatch: %s",
                                                  "引数の数が一致しません: %s"},
    [static_cast<int>(DebugMsgId::ARRAY_DECL_AS_EXPR_ERROR)] =
        {"Array declaration cannot be evaluated as expression: %s",
         "配列宣言は式として評価できません: %s"},
    [static_cast<int>(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR)] =
        {"Unsupported expression node: %s", "未対応の式ノード: %s"},
    [static_cast<int>(DebugMsgId::CONST_REASSIGN_ERROR)] =
        {"Cannot reassign to const variable: %s", "再代入できません: %s"},
    [static_cast<int>(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR)] =
        {"Direct assignment to array variable: %s", "配列変数への直接代入: %s"},
    [static_cast<int>(DebugMsgId::CONST_ARRAY_ASSIGN_ERROR)] =
        {"Assignment to const array: %s", "const配列への代入: %s"},
    [static_cast<int>(DebugMsgId::CONST_STRING_ELEMENT_ASSIGN_ERROR)] =
        {"Cannot modify const string element: %s", "要素は変更できません: %s"},
    [static_cast<int>(DebugMsgId::TYPE_RANGE_ERROR)] =
        {"Value out of type range", "型の範囲外の値を代入しようとしました"},
    [static_cast<int>(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR)] =
        {"Only string literals can be assigned to string elements",
         "文字列要素には文字列リテラルのみ代入可能"},

    // デバッグ情報
    [static_cast<int>(DebugMsgId::STRING_LITERAL_DEBUG)] =
        {"Expression evaluation: String literal = \"%s\"",
         "式評価: 文字列リテラル = \"%s\""},
    [static_cast<int>(
        DebugMsgId::BINARY_OP_RESULT_DEBUG)] = {"  Operation result = %lld",
                                                "  演算結果 = %lld"},
    [static_cast<int>(DebugMsgId::UNARY_OP_DEBUG)] =
        {"Expression evaluation: Unary operation = %s",
         "式評価: 単項演算 = %s"},
    [static_cast<int>(
        DebugMsgId::UNARY_OP_OPERAND_DEBUG)] = {"  Operand = %lld",
                                                "  オペランド = %lld"},
    [static_cast<int>(DebugMsgId::UNARY_OP_RESULT_DEBUG)] =
        {"  Unary operation result = %lld", "  単項演算結果 = %lld"},
    [static_cast<int>(DebugMsgId::EXISTING_VAR_ASSIGN_DEBUG)] =
        {"  Assigning to existing variable", "  既存変数に代入"},
    [static_cast<int>(DebugMsgId::EXISTING_STRING_VAR_ASSIGN_DEBUG)] =
        {"  Assigning to existing string variable", "  既存文字列変数に代入"},
    [static_cast<int>(DebugMsgId::STRING_ELEMENT_ASSIGN_DEBUG)] =
        {"String element assignment: %s[%lld] = \"%s\" (UTF-8 support)",
         "文字列要素代入: %s[%lld] = \"%s\" (UTF-8対応)"},
    [static_cast<int>(DebugMsgId::STRING_LENGTH_UTF8_DEBUG)] =
        {"  Original string length (UTF-8 chars) = %zu",
         "  元の文字列長（UTF-8文字数）= %zu"},
    [static_cast<int>(DebugMsgId::STRING_ELEMENT_REPLACE_DEBUG)] =
        {"  Replacing character at position %lld with \"%s\"",
         "  位置 %lld の文字を \"%s\" に置換"},
    [static_cast<int>(DebugMsgId::STRING_AFTER_REPLACE_DEBUG)] =
        {"  String after replacement: \"%s\"", "  置換後の文字列: \"%s\""},
    [static_cast<int>(DebugMsgId::ARRAY_DECL_EVAL_DEBUG)] =
        {"AST_ARRAY_DECL called in evaluate_expression: %s",
         "AST_ARRAY_DECL が evaluate_expression で呼び出されました: %s"},
};

// デバッグメッセージ配列のサイズ
const int debug_messages_size =
    static_cast<int>(sizeof(debug_messages) / sizeof(debug_messages[0]));
