#include "debug_interpreter_messages.h"

namespace DebugMessages {
namespace Interpreter {

void init_interpreter_messages(std::vector<DebugMessageTemplate> &messages) {
    // Expression evaluation
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_NUMBER)] = {
        "[INTERPRETER_EXPR] Expression eval: number %lld",
        "[INTERPRETER_EXPR] 式評価: 数値 %lld"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_BINARY_OP)] = {
        "[INTERPRETER_EXPR] Expression eval: binary op %s",
        "[INTERPRETER_EXPR] 式評価: 二項演算 %s"};
    messages[static_cast<int>(DebugMsgId::BINARY_OP_VALUES)] = {
        "[INTERPRETER_EXPR] Binary op values: left=%lld, right=%lld",
        "[INTERPRETER_EXPR] 二項演算値: 左=%lld, 右=%lld"};
    messages[static_cast<int>(DebugMsgId::BINARY_OP_RESULT_DEBUG)] = {
        "[INTERPRETER_EXPR] Binary op result: %lld",
        "[INTERPRETER_EXPR] 二項演算結果: %lld"};

    // Variable management
    messages[static_cast<int>(DebugMsgId::VAR_ASSIGN_READABLE)] = {
        "[INTERPRETER_VAR] Variable assign: %s = %lld",
        "[INTERPRETER_VAR] 変数代入: %s = %lld"};
    messages[static_cast<int>(DebugMsgId::VAR_CREATE_NEW)] = {
        "[INTERPRETER_VAR] Creating new variable",
        "[INTERPRETER_VAR] 新しい変数を作成中"};
    messages[static_cast<int>(DebugMsgId::EXISTING_VAR_ASSIGN_DEBUG)] = {
        "[INTERPRETER_VAR] Assigning to existing variable",
        "[INTERPRETER_VAR] 既存変数に代入中"};
    // Array management messages
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_START)] = {
        "[INTERPRETER_ARRAY] Array declaration start: %s",
        "[INTERPRETER_ARRAY] 配列宣言開始: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_SUCCESS)] = {
        "[INTERPRETER_ARRAY] Array declaration success: %s",
        "[INTERPRETER_ARRAY] 配列宣言成功: %s"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_DECL_SUCCESS)] = {
        "[INTERPRETER_ARRAY] Multidimensional array declaration success: %s",
        "[INTERPRETER_ARRAY] 多次元配列宣言成功: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_TOTAL_SIZE)] = {
        "[INTERPRETER_ARRAY] Array total size: %d",
        "[INTERPRETER_ARRAY] 配列総サイズ: %d"};
    messages[static_cast<int>(DebugMsgId::SINGLE_DIM_ARRAY_PROCESSING)] = {
        "[INTERPRETER_ARRAY] Processing as single dimension array",
        "[INTERPRETER_ARRAY] 単次元配列として処理中"};

    // Function and parsing messages
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_ASSIGN)] = {
        "[PARSE_NODE] Creating assignment node: %s",
        "[PARSE_NODE] 代入ノード作成: %s"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_VAR_DECL)] = {
        "[PARSE_NODE] Creating variable declaration node: %s",
        "[PARSE_NODE] 変数宣言ノード作成: %s"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_FUNC_DECL)] = {
        "[PARSE_NODE] Creating function declaration node: %s",
        "[PARSE_NODE] 関数宣言ノード作成: %s"};

    // エラーメッセージ
    messages[static_cast<int>(DebugMsgId::TYPE_MISMATCH_ERROR)] = {
        "[INTERPRETER_ERROR] Type mismatch error",
        "[INTERPRETER_ERROR] 型不一致エラー"};
    messages[static_cast<int>(DebugMsgId::VAR_REDECLARE_ERROR)] = {
        "[INTERPRETER_ERROR] Variable redeclaration error: %s",
        "[INTERPRETER_ERROR] 変数再宣言エラー: %s"};
    messages[static_cast<int>(DebugMsgId::CONST_REASSIGN_ERROR)] = {
        "[INTERPRETER_ERROR] Cannot reassign const variable: %s",
        "[INTERPRETER_ERROR] const変数への再代入はできません: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR)] = {
        "[INTERPRETER_ERROR] Array index out of bounds",
        "[INTERPRETER_ERROR] 配列インデックスが範囲外です"};
    messages[static_cast<int>(DebugMsgId::UNDEFINED_FUNC_ERROR)] = {
        "[INTERPRETER_ERROR] Undefined function: %s",
        "[INTERPRETER_ERROR] 未定義の関数: %s"};
    messages[static_cast<int>(DebugMsgId::ARG_COUNT_MISMATCH_ERROR)] = {
        "[INTERPRETER_ERROR] Argument count mismatch",
        "[INTERPRETER_ERROR] 引数の数が一致しません"};

    // 実行時デバッグメッセージ
    messages[static_cast<int>(DebugMsgId::STRING_LITERAL_DEBUG)] = {
        "[INTERPRETER_EXPR] String literal: %s",
        "[INTERPRETER_EXPR] 文字列リテラル: %s"};
    messages[static_cast<int>(DebugMsgId::UNARY_OP_DEBUG)] = {
        "[INTERPRETER_EXPR] Unary operation: %s",
        "[INTERPRETER_EXPR] 単項演算: %s"};
    messages[static_cast<int>(DebugMsgId::UNARY_OP_RESULT_DEBUG)] = {
        "[INTERPRETER_EXPR] Unary operation result: %lld",
        "[INTERPRETER_EXPR] 単項演算結果: %lld"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_DEBUG)] = {
        "[INTERPRETER_ARRAY] Array element assignment: %s[%lld] = %lld",
        "[INTERPRETER_ARRAY] 配列要素代入: %s[%lld] = %lld"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_START)] = {
        "[INTERPRETER_ARRAY] Starting array element assignment",
        "[INTERPRETER_ARRAY] 配列要素代入開始"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_SUCCESS)] = {
        "[INTERPRETER_ARRAY] Array element assignment successful",
        "[INTERPRETER_ARRAY] 配列要素代入成功"};

    // 関数呼び出し関連
    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER)] = {
        "[INTERPRETER_FUNC] Registering function declaration: %s",
        "[INTERPRETER_FUNC] 関数宣言登録: %s"};
    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE)] = {
        "[INTERPRETER_FUNC] Function declaration registration complete",
        "[INTERPRETER_FUNC] 関数宣言登録完了"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_START)] = {
        "[INTERPRETER_FUNC] Parameter list processing start",
        "[INTERPRETER_FUNC] パラメータリスト処理開始"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_SIZE)] = {
        "[INTERPRETER_FUNC] Parameter list size: %d",
        "[INTERPRETER_FUNC] パラメータリストサイズ: %d"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_COMPLETE)] = {
        "[INTERPRETER_FUNC] Parameter list processing complete",
        "[INTERPRETER_FUNC] パラメータリスト処理完了"};

    // より多くのメッセージを追加
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_COMPLETE_DEBUG)] = {
        "Array declaration complete", "配列宣言完了"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_DECL_COMPLETE_DEBUG)] =
        {"Multidimensional array declaration complete", "多次元配列宣言完了"};
    messages[static_cast<int>(DebugMsgId::STRING_ASSIGN_READABLE)] = {
        "String assign: %s = \"%s\"", "文字列代入: %s = \"%s\""};
    messages[static_cast<int>(DebugMsgId::STRING_VAR_CREATE_NEW)] = {
        "Creating new string variable", "新しい文字列変数を作成中"};

    // パーサー関連の詳細メッセージ
    messages[static_cast<int>(DebugMsgId::PARSING_START)] = {
        "[PARSE_INIT] Parsing start", "[PARSE_INIT] 解析開始"};
    messages[static_cast<int>(DebugMsgId::AST_GENERATED)] = {
        "[PARSE_COMPLETE] AST generated", "[PARSE_COMPLETE] AST生成完了"};
    messages[static_cast<int>(DebugMsgId::GLOBAL_DECL_START)] = {
        "[INTERPRETER_INIT] Global declaration start",
        "[INTERPRETER_INIT] グローバル宣言開始"};
    messages[static_cast<int>(DebugMsgId::GLOBAL_DECL_COMPLETE)] = {
        "[INTERPRETER_INIT] Global declaration complete",
        "[INTERPRETER_INIT] グローバル宣言完了"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_SEARCH)] = {
        "[INTERPRETER_INIT] Searching for main function",
        "[INTERPRETER_INIT] main関数を検索中"};

    // 実行関連のメッセージ
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_VAR_REF)] = {
        "[INTERPRETER_EXPR] Expression eval: variable reference %s",
        "[INTERPRETER_EXPR] 式評価: 変数参照 %s"};
    messages[static_cast<int>(DebugMsgId::VAR_VALUE)] = {
        "[INTERPRETER_VAR] Variable value: %s = %lld",
        "[INTERPRETER_VAR] 変数値: %s = %lld"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_ARRAY_REF)] = {
        "[INTERPRETER_EXPR] Expression eval: array reference",
        "[INTERPRETER_EXPR] 式評価: 配列参照"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INDEX)] = {
        "[INTERPRETER_ARRAY] Array index: %lld",
        "[INTERPRETER_ARRAY] 配列インデックス: %lld"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ACCESS)] = {
        "[INTERPRETER_ARRAY] Array element access: %s[%lld]",
        "[INTERPRETER_ARRAY] 配列要素アクセス: %s[%lld]"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_VALUE)] = {
        "[INTERPRETER_ARRAY] Array element value: %lld",
        "[INTERPRETER_ARRAY] 配列要素値: %lld"};

    // 配列初期化関連
    messages[static_cast<int>(DebugMsgId::ARRAY_INIT_CALLED)] = {
        "[INTERPRETER_ARRAY] Array initialization called",
        "[INTERPRETER_ARRAY] 配列初期化呼び出し"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INIT_COMPLETED)] = {
        "[INTERPRETER_ARRAY] Array initialization completed",
        "[INTERPRETER_ARRAY] 配列初期化完了"};
    messages[static_cast<int>(DebugMsgId::ARRAY_LITERAL_CALLED)] = {
        "[INTERPRETER_ARRAY] Array literal called",
        "[INTERPRETER_ARRAY] 配列リテラル呼び出し"};
    messages[static_cast<int>(DebugMsgId::ARRAY_LITERAL_COMPLETED)] = {
        "[INTERPRETER_ARRAY] Array literal completed",
        "[INTERPRETER_ARRAY] 配列リテラル完了"};

    // 文字列関連
    messages[static_cast<int>(DebugMsgId::STRING_ELEMENT_ACCESS)] = {
        "[INTERPRETER_STRING] String element access: index %lld",
        "[INTERPRETER_STRING] 文字列要素アクセス: インデックス %lld"};
    messages[static_cast<int>(DebugMsgId::STRING_LENGTH_UTF8)] = {
        "[INTERPRETER_STRING] String length (UTF-8): %lld",
        "[INTERPRETER_STRING] 文字列長 (UTF-8): %lld"};
    messages[static_cast<int>(DebugMsgId::STRING_ELEMENT_VALUE)] = {
        "[INTERPRETER_STRING] String element value: %lld",
        "[INTERPRETER_STRING] 文字列要素値: %lld"};
    messages[static_cast<int>(DebugMsgId::STRING_ASSIGN_READABLE)] = {
        "[INTERPRETER_VAR] String assign: %s = \"%s\"",
        "[INTERPRETER_VAR] 文字列代入: %s = \"%s\""};
    messages[static_cast<int>(DebugMsgId::STRING_VAR_CREATE_NEW)] = {
        "[INTERPRETER_VAR] Creating new string variable",
        "[INTERPRETER_VAR] 新しい文字列変数を作成"};

    // Error messages
    messages[static_cast<int>(DebugMsgId::UNKNOWN_BINARY_OP_ERROR)] = {
        "[INTERPRETER_ERROR] Unknown binary operator: %s",
        "[INTERPRETER_ERROR] 不明な二項演算子: %s"};
    messages[static_cast<int>(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR)] = {
        "[INTERPRETER_ERROR] Unsupported expression node type",
        "[INTERPRETER_ERROR] サポートされていない式ノード型"};

    // 不足している重要なメッセージを追加
    messages[static_cast<int>(DebugMsgId::VAR_DECLARATION_DEBUG)] = {
        "[INTERPRETER_VAR] Variable declaration: %s",
        "[INTERPRETER_VAR] 変数宣言: %s"};
    messages[static_cast<int>(DebugMsgId::UNARY_OP_DEBUG)] = {
        "[INTERPRETER_EXPR] Unary operation: %s",
        "[INTERPRETER_EXPR] 単項演算: %s"};
    messages[static_cast<int>(DebugMsgId::UNARY_OP_RESULT_DEBUG)] = {
        "[INTERPRETER_EXPR] Unary op result: %lld",
        "[INTERPRETER_EXPR] 単項演算結果: %lld"};
    messages[static_cast<int>(DebugMsgId::EXISTING_VAR_ASSIGN_DEBUG)] = {
        "[INTERPRETER_VAR] Assigning to existing variable: %s",
        "[INTERPRETER_VAR] 既存変数への代入: %s"};
    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER)] = {
        "[INTERPRETER_FUNC] Registering function: %s",
        "[INTERPRETER_FUNC] 関数登録: %s"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_FOUND)] = {
        "[INTERPRETER_EXEC] Main function found",
        "[INTERPRETER_EXEC] main関数発見"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_EXECUTE)] = {
        "[INTERPRETER_EXEC] Executing main function",
        "[INTERPRETER_EXEC] main関数実行"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_BODY_EXISTS)] = {
        "[INTERPRETER_EXEC] Main function body exists",
        "[INTERPRETER_EXEC] main関数本体存在"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_BODY_NULL)] = {
        "[INTERPRETER_EXEC] Main function body is null",
        "[INTERPRETER_EXEC] main関数本体がnull"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_EXIT)] = {
        "[INTERPRETER_EXEC] Main function exit",
        "[INTERPRETER_EXEC] main関数終了"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_START)] = {
        "[INTERPRETER_INIT] Interpreter start",
        "[INTERPRETER_INIT] インタープリター開始"};
    messages[static_cast<int>(DebugMsgId::EXECUTION_COMPLETE)] = {
        "[INTERPRETER_COMPLETE] Execution complete",
        "[INTERPRETER_COMPLETE] 実行完了"};
    messages[static_cast<int>(DebugMsgId::AST_IS_NULL)] = {
        "[INTERPRETER_ERROR] AST is null", "[INTERPRETER_ERROR] ASTがnull"};
    messages[static_cast<int>(DebugMsgId::STRING_LITERAL_DEBUG)] = {
        "[INTERPRETER_EXPR] String literal: %s",
        "[INTERPRETER_EXPR] 文字列リテラル: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_DEBUG)] = {
        "[INTERPRETER_ARRAY] Array element assign: %s[%lld] = %lld",
        "[INTERPRETER_ARRAY] 配列要素代入: %s[%lld] = %lld"};
    messages[static_cast<int>(DebugMsgId::VARIABLE_NOT_FOUND)] = {
        "[INTERPRETER_ERROR] Variable not found: %s",
        "[INTERPRETER_ERROR] 変数が見つかりません: %s"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_STMTLIST)] = {
        "[PARSE_NODE] Creating statement list node",
        "[PARSE_NODE] 文リストノード作成"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_TYPESPEC)] = {
        "[PARSE_NODE] Creating type spec node",
        "[PARSE_NODE] 型指定ノード作成"};

    // メンバーアクセス再帰処理関連
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_RECURSIVE_START)] = {
        "[MEMBER_ACCESS] Starting recursive access with %zu levels",
        "[MEMBER_ACCESS] %zu段階の再帰的アクセス開始"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_LEVEL)] = {
        "[MEMBER_ACCESS] Accessing member[%zu] = %s",
        "[MEMBER_ACCESS] メンバー[%zu]にアクセス = %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_SUCCESS)] = {
        "[MEMBER_ACCESS] Successfully accessed member, type = %d",
        "[MEMBER_ACCESS] メンバーアクセス成功, 型 = %d"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_FAILED)] = {
        "[MEMBER_ACCESS] Failed to access member: %s",
        "[MEMBER_ACCESS] メンバーアクセス失敗: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_FINAL_TYPE)] = {
        "[MEMBER_ACCESS] Final result type = %d",
        "[MEMBER_ACCESS] 最終結果の型 = %d"};

    // 変数宣言関連
    messages[static_cast<int>(DebugMsgId::VAR_DECL_INIT_TYPE)] = {
        "[VAR_DECL] Init node type: %d for variable %s",
        "[VAR_DECL] 初期化ノード型: %d, 変数 %s"};
    messages[static_cast<int>(DebugMsgId::VAR_DECL_TYPED_VALUE)] = {
        "[VAR_DECL] TypedValue evaluated for %s",
        "[VAR_DECL] TypedValue評価完了: %s"};
    messages[static_cast<int>(DebugMsgId::VAR_DECL_STRUCT_MEMBERS)] = {
        "[VAR_DECL] Creating member variables for %s (type: %s), "
        "members.size=%zu",
        "[VAR_DECL] メンバー変数作成: %s (型: %s), メンバー数=%zu"};
    messages[static_cast<int>(DebugMsgId::VAR_DECL_ASSIGN_STRING)] = {
        "[VAR_DECL] Calling assign_variable for string: %s",
        "[VAR_DECL] 文字列変数代入: %s"};
    messages[static_cast<int>(DebugMsgId::VAR_DECL_POINTER_INIT)] = {
        "[VAR_DECL] Pointer init: name=%s, has_init_expr=%d, has_right=%d",
        "[VAR_DECL] ポインタ初期化: 名前=%s, 初期化式=%d, right=%d"};
    messages[static_cast<int>(DebugMsgId::VAR_DECL_POINTER_VALUE)] = {
        "[VAR_DECL] Setting pointer value for %s (type=%d)",
        "[VAR_DECL] ポインタ値設定: %s (型=%d)"};
    messages[static_cast<int>(DebugMsgId::VAR_DECL_STRING_PTR_INIT)] = {
        "[VAR_DECL] String pointer initialized: value=%p",
        "[VAR_DECL] 文字列ポインタ初期化: 値=%p"};

    // メンバー代入関連
    messages[static_cast<int>(DebugMsgId::MEMBER_ASSIGN_STRUCT)] = {
        "[MEMBER_ASSIGN] Assigning struct to member: %s.%s (type: %s)",
        "[MEMBER_ASSIGN] 構造体メンバー代入: %s.%s (型: %s)"};

    // 低レベルデバッグメッセージ (GENERIC_DEBUG置き換え用)
    // Method call / Self関連
    messages[static_cast<int>(DebugMsgId::METHOD_SELF_SETUP_START)] = {
        "[METHOD] Self setup start: %s", "[METHOD] selfセットアップ開始: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_SELF_SETUP_COMPLETE)] = {
        "[METHOD] Self setup complete: %s",
        "[METHOD] selfセットアップ完了: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_SELF_WRITEBACK_START)] = {
        "[METHOD] Self writeback start: %s", "[METHOD] self書き戻し開始: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_SELF_WRITEBACK_COMPLETE)] = {
        "[METHOD] Self writeback complete: %s",
        "[METHOD] self書き戻し完了: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_SELF_MERGE)] = {
        "[METHOD] Self merge: %s", "[METHOD] selfマージ: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_POINTER_DEREF)] = {
        "[METHOD] Pointer dereference: %s",
        "[METHOD] ポインタデリファレンス: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CONSTRUCTOR_SELF)] = {
        "[METHOD] Constructor self created: %s",
        "[METHOD] コンストラクタself作成: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_DEBUG)] = {
        "[METHOD] Call debug: %s", "[METHOD] 呼び出しデバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_EXEC_DEBUG)] = {
        "[METHOD] Exec debug: %s", "[METHOD] 実行デバッグ: %s"};

    // Arrow operator関連
    messages[static_cast<int>(DebugMsgId::ARROW_OP_MEMBER_ACCESS)] = {
        "[ARROW_OP] Member access: %s", "[ARROW_OP] メンバーアクセス: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_OP_NULL_CHECK)] = {
        "[ARROW_OP] Null check: %s", "[ARROW_OP] NULLチェック: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_OP_MEMORY_READ)] = {
        "[ARROW_OP] Memory read: %s", "[ARROW_OP] メモリ読み込み: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_OP_TYPE_CAST)] = {
        "[ARROW_OP] Type cast: %s", "[ARROW_OP] 型キャスト: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_OP_GENERIC_RESOLVE)] = {
        "[ARROW_OP] Generic resolve: %s", "[ARROW_OP] ジェネリック解決: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_ASSIGN_START)] = {
        "[ARROW_ASSIGN] Start: %s", "[ARROW_ASSIGN] 開始: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_ASSIGN_COMPLETE)] = {
        "[ARROW_ASSIGN] Complete: %s", "[ARROW_ASSIGN] 完了: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_ASSIGN_MEMBER_UPDATE)] = {
        "[ARROW_ASSIGN] Member update: %s", "[ARROW_ASSIGN] メンバー更新: %s"};
    messages[static_cast<int>(DebugMsgId::ARROW_ASSIGN_METADATA)] = {
        "[ARROW_ASSIGN] Metadata: %s", "[ARROW_ASSIGN] メタデータ: %s"};

    // Member access関連
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_DEBUG)] = {
        "[MEMBER] Access debug: %s", "[MEMBER] アクセスデバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_REFERENCE)] = {
        "[MEMBER] Reference resolve: %s", "[MEMBER] 参照解決: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ACCESS_FOUND)] = {
        "[MEMBER] Member found: %s", "[MEMBER] メンバー発見: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ASSIGN_START)] = {
        "[MEMBER] Assignment start: %s", "[MEMBER] 代入開始: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ASSIGN_COMPLETE)] = {
        "[MEMBER] Assignment complete: %s", "[MEMBER] 代入完了: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ASSIGN_NESTED)] = {
        "[MEMBER] Nested assignment: %s", "[MEMBER] ネスト代入: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_ARRAY_ACCESS)] = {
        "[MEMBER] Array access: %s", "[MEMBER] 配列アクセス: %s"};
    messages[static_cast<int>(DebugMsgId::MEMBER_EVAL_RESULT)] = {
        "[MEMBER] Eval result: %s", "[MEMBER] 評価結果: %s"};

    // Impl/Interface関連（既存のIMPL_METHOD_REGISTERなどを活用）
    messages[static_cast<int>(DebugMsgId::IMPL_REGISTER_DEBUG)] = {
        "[IMPL] Register debug: %s", "[IMPL] 登録デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_FIND_EXACT)] = {
        "[IMPL] Find exact match: %s", "[IMPL] 完全一致検索: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_FIND_GENERIC)] = {
        "[IMPL] Find generic: %s", "[IMPL] ジェネリック検索: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_GENERIC_INSTANTIATE)] = {
        "[IMPL] Generic instantiate: %s",
        "[IMPL] ジェネリックインスタンス化: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_GENERIC_CACHE_HIT)] = {
        "[IMPL] Generic cache hit: %s",
        "[IMPL] ジェネリックキャッシュヒット: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_GENERIC_CACHE_MISS)] = {
        "[IMPL] Generic cache miss: %s",
        "[IMPL] ジェネリックキャッシュミス: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_GENERIC_TYPE_MAP)] = {
        "[IMPL] Generic type map: %s", "[IMPL] ジェネリック型マップ: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_HANDLE_DEBUG)] = {
        "[IMPL] Handle debug: %s", "[IMPL] 処理デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_CONSTRUCTOR_DEBUG)] = {
        "[IMPL] Constructor debug: %s", "[IMPL] コンストラクタデバッグ: %s"};

    // Statement executor関連
    messages[static_cast<int>(DebugMsgId::STMT_EXEC_DEBUG)] = {
        "[STMT] Exec debug: %s", "[STMT] 実行デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::STMT_MEMBER_ARRAY_ASSIGN)] = {
        "[STMT] Member array assign: %s", "[STMT] メンバー配列代入: %s"};
    messages[static_cast<int>(DebugMsgId::STMT_NESTED_STRUCT_ARRAY)] = {
        "[STMT] Nested struct array: %s", "[STMT] ネスト構造体配列: %s"};
    messages[static_cast<int>(DebugMsgId::STMT_SELF_ASSIGN)] = {
        "[STMT] Self assign: %s", "[STMT] self代入: %s"};

    // Struct operations関連
    messages[static_cast<int>(DebugMsgId::STRUCT_OP_GET_MEMBER)] = {
        "[STRUCT_OP] Get member: %s", "[STRUCT_OP] メンバー取得: %s"};
    messages[static_cast<int>(DebugMsgId::STRUCT_OP_SYNC_MEMBER)] = {
        "[STRUCT_OP] Sync member: %s", "[STRUCT_OP] メンバー同期: %s"};
    messages[static_cast<int>(DebugMsgId::STRUCT_OP_MULTIDIM_ACCESS)] = {
        "[STRUCT_OP] Multidim access: %s", "[STRUCT_OP] 多次元アクセス: %s"};
    messages[static_cast<int>(DebugMsgId::STRUCT_OP_FLAT_INDEX)] = {
        "[STRUCT_OP] Flat index: %s", "[STRUCT_OP] フラットインデックス: %s"};

    // Return handler関連
    messages[static_cast<int>(DebugMsgId::RETURN_EXPR_DEBUG)] = {
        "[RETURN] Expr debug: %s", "[RETURN] 式デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::RETURN_POINTER_DEBUG)] = {
        "[RETURN] Pointer debug: %s", "[RETURN] ポインタデバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::RETURN_TYPED_VALUE)] = {
        "[RETURN] Typed value: %s", "[RETURN] 型付き値: %s"};

    // Call implementation関連
    messages[static_cast<int>(DebugMsgId::CALL_IMPL_DEBUG)] = {
        "[CALL_IMPL] Debug: %s", "[CALL_IMPL] デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::CALL_IMPL_BUILTIN)] = {
        "[CALL_IMPL] Builtin: %s", "[CALL_IMPL] 組み込み: %s"};
    messages[static_cast<int>(DebugMsgId::CALL_IMPL_MALLOC)] = {
        "[CALL_IMPL] Malloc: %s", "[CALL_IMPL] Malloc: %s"};
    messages[static_cast<int>(DebugMsgId::CALL_IMPL_SLEEP)] = {
        "[CALL_IMPL] Sleep: %s", "[CALL_IMPL] Sleep: %s"};
    messages[static_cast<int>(DebugMsgId::CALL_IMPL_RECEIVER)] = {
        "[CALL_IMPL] Receiver: %s", "[CALL_IMPL] レシーバー: %s"};

    // Parser関連
    messages[static_cast<int>(DebugMsgId::PARSER_TOKEN_DEBUG)] = {
        "[PARSER] Token debug: %s", "[PARSER] トークンデバッグ: %s"};

    // Expression service関連
    messages[static_cast<int>(DebugMsgId::EXPR_SERVICE_ERROR)] = {
        "[EXPR_SERVICE] Error: %s", "[EXPR_SERVICE] エラー: %s"};

    // 詳細デバッグカテゴリ（頻出パターン用）
    messages[static_cast<int>(DebugMsgId::DEBUG_GENERIC)] = {"DEBUG: %s",
                                                             "DEBUG: %s"};
    messages[static_cast<int>(DebugMsgId::ENUM_VAR_DECL_DEBUG)] = {
        "[ENUM_VAR_DECL_MANAGER] %s", "[ENUM_VAR_DECL_MANAGER] %s"};
    messages[static_cast<int>(DebugMsgId::EVAL_RESOLVER_DEBUG)] = {
        "[EVAL_RESOLVER] %s", "[EVAL_RESOLVER] %s"};
    messages[static_cast<int>(DebugMsgId::STRUCT_LITERAL_DEBUG)] = {
        "STRUCT_LITERAL_DEBUG: %s", "STRUCT_LITERAL_DEBUG: %s"};
    messages[static_cast<int>(DebugMsgId::SYNC_STRUCT_DEBUG)] = {
        "SYNC_STRUCT: %s", "SYNC_STRUCT: %s"};
    messages[static_cast<int>(DebugMsgId::GENERIC_CTOR_DEBUG)] = {
        "[GENERIC_CTOR] %s", "[GENERIC_CTOR] %s"};
    messages[static_cast<int>(DebugMsgId::UNION_TYPE_DEBUG)] = {
        "UNION_*_DEBUG: %s", "UNION_*_DEBUG: %s"};
    messages[static_cast<int>(DebugMsgId::TYPEDEF_DEBUG)] = {
        "TYPEDEF_DEBUG: %s", "TYPEDEF_DEBUG: %s"};
    messages[static_cast<int>(DebugMsgId::BUILTIN_TYPES_DEBUG)] = {
        "[BUILTIN_TYPES] %s", "[BUILTIN_TYPES] %s"};
    messages[static_cast<int>(DebugMsgId::ASSIGN_IFACE_DEBUG)] = {
        "ASSIGN_IFACE: %s", "ASSIGN_IFACE: %s"};
    messages[static_cast<int>(DebugMsgId::REGISTER_UNION_DEBUG)] = {
        "REGISTER_UNION_DEBUG: %s", "REGISTER_UNION_DEBUG: %s"};
    messages[static_cast<int>(DebugMsgId::VAR_DEBUG)] = {"VAR_DEBUG: %s",
                                                         "VAR_DEBUG: %s"};
    messages[static_cast<int>(DebugMsgId::GET_TYPE_SIZE_DEBUG)] = {
        "[get_type_size] %s", "[get_type_size] %s"};

    // 汎用デバッグ（最後の手段として残す）
    messages[static_cast<int>(DebugMsgId::GENERIC_DEBUG)] = {"%s", "%s"};

    // 関数関連のメッセージ
    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE)] = {
        "[INTERPRETER_FUNC] Function registration complete: %s",
        "[INTERPRETER_FUNC] 関数登録完了: %s"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_START)] = {
        "[INTERPRETER_FUNC] Parameter list start",
        "[INTERPRETER_FUNC] パラメータリスト開始"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_SIZE)] = {
        "[INTERPRETER_FUNC] Parameter list size: %d",
        "[INTERPRETER_FUNC] パラメータリストサイズ: %d"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_COMPLETE)] = {
        "[INTERPRETER_FUNC] Parameter list complete",
        "[INTERPRETER_FUNC] パラメータリスト完了"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_DELETE)] = {
        "[INTERPRETER_FUNC] Deleting parameter list",
        "[INTERPRETER_FUNC] パラメータリスト削除"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_NONE)] = {
        "[INTERPRETER_FUNC] No parameter list",
        "[INTERPRETER_FUNC] パラメータリストなし"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_START)] = {
        "[INTERPRETER_FUNC] Function body start",
        "[INTERPRETER_FUNC] 関数本体開始"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_EXISTS)] = {
        "[INTERPRETER_FUNC] Function body exists",
        "[INTERPRETER_FUNC] 関数本体存在"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_SET_COMPLETE)] = {
        "[INTERPRETER_FUNC] Function body set complete",
        "[INTERPRETER_FUNC] 関数本体設定完了"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_NONE)] = {
        "[INTERPRETER_FUNC] No function body",
        "[INTERPRETER_FUNC] 関数本体なし"};
    messages[static_cast<int>(DebugMsgId::FUNC_DEF_COMPLETE)] = {
        "[INTERPRETER_FUNC] Function definition complete",
        "[INTERPRETER_FUNC] 関数定義完了"};

    // 配列関連の詳細メッセージ
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_DEBUG)] = {
        "[INTERPRETER_ARRAY] Array declaration debug: %s",
        "[INTERPRETER_ARRAY] 配列宣言デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DIMENSIONS_COUNT)] = {
        "[INTERPRETER_ARRAY] Array dimensions count: %d",
        "[INTERPRETER_ARRAY] 配列次元数: %d"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_PROCESSING)] = {
        "[INTERPRETER_ARRAY] Multidimensional array processing",
        "[INTERPRETER_ARRAY] 多次元配列処理"};
    messages[static_cast<int>(DebugMsgId::SINGLE_DIM_ARRAY_PROCESSING)] = {
        "[INTERPRETER_ARRAY] Single dimension array processing",
        "[INTERPRETER_ARRAY] 一次元配列処理"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_DETECTED)] =
        {"[INTERPRETER_ARRAY] Multidimensional array assignment detected",
         "[INTERPRETER_ARRAY] 多次元配列代入検出"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_ACCESS_INFO)] = {
        "[INTERPRETER_ARRAY] Multidimensional array access info",
        "[INTERPRETER_ARRAY] 多次元配列アクセス情報"};
    messages[static_cast<int>(DebugMsgId::FLAT_INDEX_CALCULATED)] = {
        "[INTERPRETER_ARRAY] Flat index calculated: %lld",
        "[INTERPRETER_ARRAY] フラットインデックス計算: %lld"};
    messages[static_cast<int>(
        DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_COMPLETED)] = {
        "[INTERPRETER_ARRAY] Multidimensional array assignment completed",
        "[INTERPRETER_ARRAY] 多次元配列代入完了"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INFO)] = {
        "[INTERPRETER_ARRAY] Array info: %s",
        "[INTERPRETER_ARRAY] 配列情報: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INDEX_OUT_OF_BOUNDS)] = {
        "[INTERPRETER_ERROR] Array index out of bounds",
        "[INTERPRETER_ERROR] 配列インデックス範囲外"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_START)] = {
        "[INTERPRETER_ARRAY] Array element assignment start",
        "[INTERPRETER_ARRAY] 配列要素代入開始"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_SUCCESS)] = {
        "[INTERPRETER_ARRAY] Array element assignment success",
        "[INTERPRETER_ARRAY] 配列要素代入成功"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_DECL_INFO)] = {
        "[INTERPRETER_ARRAY] Multidimensional array declaration info",
        "[INTERPRETER_ARRAY] 多次元配列宣言情報"};

    // エラーメッセージ
    messages[static_cast<int>(DebugMsgId::PARSER_ERROR)] = {
        "[PARSE_ERROR] Parser error: %s", "[PARSE_ERROR] パーサーエラー: %s"};
    messages[static_cast<int>(DebugMsgId::TYPE_MISMATCH_ERROR)] = {
        "[INTERPRETER_ERROR] Type mismatch error: %s",
        "[INTERPRETER_ERROR] 型不一致エラー: %s"};
    messages[static_cast<int>(DebugMsgId::VAR_REDECLARE_ERROR)] = {
        "[INTERPRETER_ERROR] Variable redeclaration error: %s",
        "[INTERPRETER_ERROR] 変数再宣言エラー: %s"};
    messages[static_cast<int>(DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR)] = {
        "[INTERPRETER_ERROR] Negative array size error",
        "[INTERPRETER_ERROR] 負の配列サイズエラー"};
    messages[static_cast<int>(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED)] = {
        "[INTERPRETER_ERROR] Dynamic array not supported",
        "[INTERPRETER_ERROR] 動的配列はサポートされていません"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_NOT_FOUND_ERROR)] = {
        "[INTERPRETER_ERROR] Main function not found error",
        "[INTERPRETER_ERROR] main関数が見つからないエラー"};
    messages[static_cast<int>(DebugMsgId::UNDEFINED_VAR_ERROR)] = {
        "[INTERPRETER_ERROR] Undefined variable error: %s",
        "[INTERPRETER_ERROR] 未定義変数エラー: %s"};
    messages[static_cast<int>(DebugMsgId::DIRECT_ARRAY_REF_ERROR)] = {
        "[INTERPRETER_ERROR] Direct array reference error",
        "[INTERPRETER_ERROR] 直接配列参照エラー"};
    messages[static_cast<int>(DebugMsgId::UNDEFINED_ARRAY_ERROR)] = {
        "[INTERPRETER_ERROR] Undefined array error: %s",
        "[INTERPRETER_ERROR] 未定義配列エラー: %s"};
    messages[static_cast<int>(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR)] = {
        "[INTERPRETER_ERROR] String index out of bounds error",
        "[INTERPRETER_ERROR] 文字列インデックス範囲外エラー"};
    messages[static_cast<int>(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR)] = {
        "[INTERPRETER_ERROR] Array index out of bounds error",
        "[INTERPRETER_ERROR] 配列インデックス範囲外エラー"};
    messages[static_cast<int>(DebugMsgId::NON_ARRAY_REF_ERROR)] = {
        "Non-array reference error", "非配列参照エラー"};
    messages[static_cast<int>(DebugMsgId::ZERO_DIVISION_ERROR)] = {
        "Zero division error", "ゼロ除算エラー"};
    messages[static_cast<int>(DebugMsgId::UNKNOWN_UNARY_OP_ERROR)] = {
        "Unknown unary operator error: %s", "不明な単項演算子エラー: %s"};
    messages[static_cast<int>(DebugMsgId::UNDEFINED_FUNC_ERROR)] = {
        "Undefined function error: %s", "未定義関数エラー: %s"};
    messages[static_cast<int>(DebugMsgId::ARG_COUNT_MISMATCH_ERROR)] = {
        "Argument count mismatch error", "引数数不一致エラー"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_AS_EXPR_ERROR)] = {
        "Array declaration as expression error", "式としての配列宣言エラー"};
    messages[static_cast<int>(DebugMsgId::CONST_REASSIGN_ERROR)] = {
        "Const reassignment error: %s", "定数再代入エラー: %s"};
    messages[static_cast<int>(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR)] = {
        "Direct array assignment error", "直接配列代入エラー"};
    messages[static_cast<int>(DebugMsgId::CONST_ARRAY_ASSIGN_ERROR)] = {
        "Const array assignment error", "定数配列代入エラー"};
    messages[static_cast<int>(DebugMsgId::CONST_STRING_ELEMENT_ASSIGN_ERROR)] =
        {"Const string element assignment error", "定数文字列要素代入エラー"};
    messages[static_cast<int>(DebugMsgId::TYPE_RANGE_ERROR)] = {
        "Type range error: %s", "型範囲エラー: %s"};
    messages[static_cast<int>(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR)] = {
        "Non-string character assignment error", "非文字列文字代入エラー"};

    // 追加のデバッグメッセージ
    messages[static_cast<int>(DebugMsgId::UNARY_OP_OPERAND_DEBUG)] = {
        "Unary op operand: %lld", "単項演算オペランド: %lld"};
    messages[static_cast<int>(DebugMsgId::EXISTING_STRING_VAR_ASSIGN_DEBUG)] = {
        "Existing string variable assignment debug",
        "既存文字列変数代入デバッグ"};
    messages[static_cast<int>(DebugMsgId::STRING_ELEMENT_ASSIGN_DEBUG)] = {
        "String element assignment debug", "文字列要素代入デバッグ"};
    messages[static_cast<int>(DebugMsgId::STRING_LENGTH_UTF8_DEBUG)] = {
        "String length UTF-8 debug: %lld", "文字列長UTF-8デバッグ: %lld"};
    messages[static_cast<int>(DebugMsgId::STRING_ELEMENT_REPLACE_DEBUG)] = {
        "String element replace debug", "文字列要素置換デバッグ"};
    messages[static_cast<int>(DebugMsgId::STRING_AFTER_REPLACE_DEBUG)] = {
        "String after replace debug: %s", "置換後文字列デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_EVAL_DEBUG)] = {
        "Array declaration evaluation debug", "配列宣言評価デバッグ"};

    // Typedef関連
    messages[static_cast<int>(DebugMsgId::TYPEDEF_REGISTER)] = {
        "Typedef register: %s", "型定義登録: %s"};
    messages[static_cast<int>(DebugMsgId::TYPEDEF_REGISTER_SUCCESS)] = {
        "Typedef register success: %s", "型定義登録成功: %s"};
    messages[static_cast<int>(DebugMsgId::TYPE_ALIAS_RESOLVE)] = {
        "Type alias resolve: %s", "型エイリアス解決: %s"};
    messages[static_cast<int>(DebugMsgId::TYPE_ALIAS_CREATE_NODE)] = {
        "Type alias create node", "型エイリアスノード作成"};
    messages[static_cast<int>(DebugMsgId::TYPE_ALIAS_RUNTIME_RESOLVE)] = {
        "Type alias runtime resolve", "型エイリアス実行時解決"};

    // 配列リテラル関連
    messages[static_cast<int>(DebugMsgId::ARRAY_LITERAL_ASSIGN_DEBUG)] = {
        "Array literal assignment debug", "配列リテラル代入デバッグ"};
    messages[static_cast<int>(DebugMsgId::ARRAY_LITERAL_ELEMENTS)] = {
        "Array literal elements: %d", "配列リテラル要素数: %d"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INIT_ELEMENTS)] = {
        "Array init elements: %d", "配列初期化要素数: %d"};
    messages[static_cast<int>(DebugMsgId::TYPE_MISMATCH_ARRAY_INIT)] = {
        "Type mismatch in array initialization", "配列初期化での型不一致"};
    messages[static_cast<int>(DebugMsgId::CURRENT_TYPE_SET)] = {
        "Current type set: %s", "現在の型設定: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INIT_WITH_TYPE_CALLED)] = {
        "Array initialization with type called", "型指定配列初期化呼び出し"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INIT_WITH_TYPE_COMPLETED)] = {
        "Array initialization with type completed", "型指定配列初期化完了"};

    // printf関連
    messages[static_cast<int>(DebugMsgId::PRINTF_OFFSET_CALLED)] = {
        "[INTERPRETER_OUTPUT] Printf offset called",
        "[INTERPRETER_OUTPUT] printfオフセット呼び出し"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARG_LIST_INFO)] = {
        "Printf arg list info: %d args", "printf引数リスト情報: %d個"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARG_PROCESSING)] = {
        "Printf arg processing", "printf引数処理"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARRAY_REF_DEBUG)] = {
        "Printf array reference debug", "printf配列参照デバッグ"};

    // 配列リテラル処理詳細メッセージ（新規追加）
    messages[static_cast<int>(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING)] = {
        "Processing array literal initialization", "配列リテラル初期化処理中"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_PROCESSING_DEBUG)] = {
        "Processing element %d, type: %d", "要素 %d 処理中, 型: %d"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_EVAL_START)] = {
        "About to evaluate expression for element %d", "要素 %d の式評価開始"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_EVAL_VALUE)] = {
        "Evaluated value: %lld", "評価値: %lld"};
    messages[static_cast<int>(DebugMsgId::PRINT_MULTIPLE_PROCESSING)] = {
        "[INTERPRETER_OUTPUT] Processing %s with %d arguments",
        "[INTERPRETER_OUTPUT] %s を %d 個の引数で処理"};
    messages[static_cast<int>(DebugMsgId::PRINT_SINGLE_ARG_DEBUG)] = {
        "[INTERPRETER_OUTPUT] Single argument in %s, type: %d",
        "[INTERPRETER_OUTPUT] %s の単一引数, 型: %d"};
    messages[static_cast<int>(DebugMsgId::PRINT_PRINTF_FORMAT_FOUND)] = {
        "[INTERPRETER_OUTPUT] Format specifiers found, processing as printf",
        "[INTERPRETER_OUTPUT] "
        "フォーマット指定子が見つかりました、printfとして処理"};
    messages[static_cast<int>(DebugMsgId::PRINT_NO_ARGUMENTS_DEBUG)] = {
        "[INTERPRETER_OUTPUT] No arguments in statement",
        "[INTERPRETER_OUTPUT] 文に引数がありません"};
    messages[static_cast<int>(DebugMsgId::PRINT_EXECUTING_STATEMENT)] = {
        "Executing print statement", "print文実行中"};
    messages[static_cast<int>(DebugMsgId::PRINT_STATEMENT_HAS_ARGS)] = {
        "Print statement has arguments", "print文に引数があります"};
    messages[static_cast<int>(DebugMsgId::PRINT_CHECKING_ARGUMENT)] = {
        "[INTERPRETER_OUTPUT] Checking argument %d, type: %d",
        "[INTERPRETER_OUTPUT] 引数 %d 確認中, 型: %d"};
    messages[static_cast<int>(DebugMsgId::PRINT_FOUND_STRING_LITERAL)] = {
        "[INTERPRETER_OUTPUT] Found string literal '%s'",
        "[INTERPRETER_OUTPUT] 文字列リテラル '%s' 発見"};
    messages[static_cast<int>(DebugMsgId::PRINT_FORMAT_SPEC_CHECKING)] = {
        "has_unescaped_format_specifiers: checking string '%s'",
        "has_unescaped_format_specifiers: 文字列 '%s' 確認中"};
    messages[static_cast<int>(DebugMsgId::PRINT_NO_FORMAT_SPECIFIERS)] = {
        "has_unescaped_format_specifiers: no format specifiers found",
        "has_unescaped_format_specifiers: フォーマット指定子なし"};

    // 追加のメッセージID（ユーザー要求分）
    messages[static_cast<int>(DebugMsgId::PARSE_USING_RECURSIVE_PARSER)] = {
        "[PARSE_INIT] Using recursive descent parser...",
        "[PARSE_INIT] 再帰下降パーサーを使用..."};
    messages[static_cast<int>(DebugMsgId::PARSE_TYPE_CHECK)] = {
        "[PARSE_TYPE] Checking type: %s, is_typedef: %s, is_struct_type: %s",
        "[PARSE_TYPE] 型チェック: %s, typedef: %s, struct型: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_REGISTER_GLOBAL_DECL)] = {
        "[PARSE_DECL] register_global_declarations processing: %s (name: %s)",
        "[PARSE_DECL] グローバル宣言処理: %s (名前: %s)"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_REGISTER)] = {
        "[PARSE_STRUCT] Registering struct definition: %s",
        "[PARSE_STRUCT] struct定義登録: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_GLOBAL_VAR_INIT)] = {
        "[INTERPRETER_INIT] Initializing global variables",
        "[INTERPRETER_INIT] グローバル変数初期化"};
    messages[static_cast<int>(DebugMsgId::EXPRESSION_EVAL_ERROR)] = {
        "[INTERPRETER_ERROR] Expression evaluation error: %s",
        "[INTERPRETER_ERROR] 式評価エラー: %s"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_ARRAY_REF_START)] = {
        "[INTERPRETER_EXPR] AST_ARRAY_REF evaluation started",
        "[INTERPRETER_EXPR] AST_ARRAY_REF評価開始"};
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_TYPE_RESOLVED)] = {
        "[INTERPRETER_VAR] Variable: %s, Type: %s, Resolved: %s",
        "[INTERPRETER_VAR] 変数: %s, 型: %s, 解決後: %s"};

    // 不足しているメッセージIDの追加
    messages[static_cast<int>(DebugMsgId::PARSE_CURRENT_TOKEN)] = {
        "[PARSE_TOKEN] Current token: %s (type: %s)",
        "[PARSE_TOKEN] 現在のトークン: %s (型: %s)"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_EXEC_STMT)] = {
        "[INTERPRETER_EXEC] Executing statement: type %d",
        "[INTERPRETER_EXEC] 文実行: 型 %d"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_VAR_DECL)] = {
        "[INTERPRETER_VAR] Variable declaration: %s",
        "[INTERPRETER_VAR] 変数宣言: %s"};
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_STRUCT_CREATE)] = {
        "[INTERPRETER_STRUCT] Struct member creation: %s.%s",
        "[INTERPRETER_STRUCT] 構造体メンバー作成: %s.%s"};
    messages[static_cast<int>(DebugMsgId::PARSE_VAR_DECL)] = {
        "[PARSE_VAR] Variable declaration: %s of type %s",
        "[PARSE_VAR] 変数宣言: %s 型 %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_EXPR_ARRAY_ACCESS)] = {
        "[PARSE_EXPR] Array access expression: %s",
        "[PARSE_EXPR] 配列アクセス式: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_FUNCTION_CREATED)] = {
        "[PARSE_FUNC] Function created: %s", "[PARSE_FUNC] 関数作成: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_DEF)] = {
        "[PARSE_STRUCT] Struct definition: %s",
        "[PARSE_STRUCT] 構造体定義: %s"};
    messages[static_cast<int>(DebugMsgId::OUTPUT_FORMAT_SPEC_FOUND)] = {
        "Format specifier found: %s", "フォーマット指定子発見: %s"};
    messages[static_cast<int>(DebugMsgId::OUTPUT_FORMAT_COUNT)] = {
        "Format count: %s", "フォーマット数: %s"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARG_LIST_INFO)] = {
        "[INTERPRETER_OUTPUT] Printf arg list: %d args from index %d",
        "[INTERPRETER_OUTPUT] Printf引数リスト: %d個 開始インデックス %d"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARG_PROCESSING)] = {
        "[INTERPRETER_OUTPUT] Processing printf arg %d (type: %d)",
        "[INTERPRETER_OUTPUT] Printf引数 %d 処理 (型: %d)"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARRAY_REF_DEBUG)] = {
        "[INTERPRETER_OUTPUT] Printf array reference debug: %s",
        "[INTERPRETER_OUTPUT] Printf配列参照デバッグ: %s"};

    // ノード作成関連
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_STMTLIST)] = {
        "Creating statement list node", "文リストノード作成"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_TYPESPEC)] = {
        "Creating type spec node", "型指定ノード作成"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_ARRAY_DECL)] = {
        "Creating array declaration node", "配列宣言ノード作成"};

    // パーサー関連の追加メッセージ
    messages[static_cast<int>(DebugMsgId::PARSE_ENUM_DEF)] = {
        "Enum definition: %s", "列挙型定義: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_MEMBER_ARRAY)] = {
        "Struct member array: %s", "構造体メンバー配列: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_MEMBER_REGULAR)] = {
        "Struct member regular: %s", "構造体メンバー通常: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_ENUM_REGISTER)] = {
        "Enum register: %s", "列挙型登録: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_DECL_START)] = {
        "[PARSE_STRUCT] Struct declaration start at line %d",
        "[PARSE_STRUCT] 構造体宣言開始 行: %d"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_ARRAY_DECL)] = {
        "[PARSE_STRUCT] Struct array declaration: %s",
        "[PARSE_STRUCT] 構造体配列宣言: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_ARRAY_VAR_NAME)] = {
        "[PARSE_STRUCT] Struct array variable name: %s",
        "[PARSE_STRUCT] 構造体配列変数名: %s"};

    // インタープリター関連の追加メッセージ
    messages[static_cast<int>(DebugMsgId::INTERPRETER_RETURN_STMT)] = {
        "Interpreter return statement", "インタープリターreturn文"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_RETURN_VAR)] = {
        "Interpreter return variable: %s", "インタープリター変数返却: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_RETURN_ARRAY)] = {
        "Interpreter return array with %zu elements",
        "インタープリター配列返却 要素数: %zu"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_RETURN_ARRAY_VAR)] = {
        "Interpreter return array variable: %s",
        "インタープリター配列変数返却: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_MULTIDIM_ARRAY_SIZE)] = {
        "Multidimensional array size: %zu", "多次元配列サイズ: %zu"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_REGULAR_ARRAY_SIZE)] = {
        "Regular array size: %zu", "通常配列サイズ: %zu"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_MULTIDIM_PROCESSING)] = {
        "Multidimensional processing", "多次元処理"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_MULTIDIM_ELEMENT)] = {
        "Multidimensional element[%d]: %lld", "多次元要素[%d]: %lld"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_ARRAY_ELEMENT)] = {
        "Array element[%d]: %lld", "配列要素[%d]: %lld"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_RETURN_EXCEPTION)] = {
        "Interpreter return exception: %s", "インタープリター例外返却: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_VAR_NOT_FOUND)] = {
        "Variable not found in interpreter: %s",
        "インタープリターで変数が見つかりません: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_EXCEPTION_IN_VAR_DECL)] =
        {"Exception in variable declaration: %s", "変数宣言での例外: %s"};

    // 変数管理関連
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_PROCESS)] = {
        "Variable manager process: type=%d, name=%s",
        "変数マネージャー処理: 型=%d, 名前=%s"};
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_MULTIDIM_FLAG)] = {
        "Variable manager multidimensional flag: %s (dimensions: %zu)",
        "変数マネージャー多次元フラグ: %s (次元数: %zu)"};
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_STRUCT_VAR_CREATE)] = {
        "Struct variable creation: %s", "構造体変数作成: %s"};
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_MULTIDIM_MEMBER_CREATE)] =
        {"Multidimensional member creation", "多次元メンバー作成"};
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_ARRAY_MEMBER_INIT)] = {
        "Array member initialization", "配列メンバー初期化"};
    messages[static_cast<int>(DebugMsgId::VAR_MANAGER_MEMBER_ADDED)] = {
        "Member added: %s", "メンバー追加: %s"};

    // 構造体関連
    // パーサー関連の新規メッセージテンプレート
    messages[static_cast<int>(DebugMsgId::PARSE_PROGRAM_START)] = {
        "[PARSE_PROGRAM] Starting to parse program in file: %s",
        "[PARSE_PROGRAM] ファイル %s のプログラム解析開始"};
    messages[static_cast<int>(DebugMsgId::PARSE_STATEMENT_START)] = {
        "[PARSE_STATEMENT] Starting statement parse at line %d, column %d",
        "[PARSE_STATEMENT] 行 %d 列 %d で文の解析開始"};
    messages[static_cast<int>(DebugMsgId::PARSE_STATEMENT_SUCCESS)] = {
        "[PARSE_STATEMENT] Successfully parsed statement type: %s, name: %s",
        "[PARSE_STATEMENT] 文解析成功 - 型: %s, 名前: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_PROGRAM_COMPLETE)] = {
        "[PARSE_PROGRAM] Program parsing complete with %zu statements",
        "[PARSE_PROGRAM] プログラム解析完了 - 文の数: %zu"};
    messages[static_cast<int>(DebugMsgId::PARSE_STATIC_MODIFIER)] = {
        "[PARSE_MODIFIER] Static modifier found at line %d, column %d",
        "[PARSE_MODIFIER] static修飾子発見 - 行 %d 列 %d"};
    messages[static_cast<int>(DebugMsgId::PARSE_CONST_MODIFIER)] = {
        "[PARSE_MODIFIER] Const modifier found at line %d, column %d",
        "[PARSE_MODIFIER] const修飾子発見 - 行 %d 列 %d"};
    messages[static_cast<int>(DebugMsgId::PARSE_TYPEDEF_START)] = {
        "[PARSE_TYPEDEF] Starting typedef declaration parse at line %d",
        "[PARSE_TYPEDEF] typedef宣言解析開始 - 行 %d"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_DECL_START)] = {
        "[PARSE_STRUCT] Starting struct declaration parse at line %d",
        "[PARSE_STRUCT] struct宣言解析開始 - 行 %d"};
    messages[static_cast<int>(DebugMsgId::PARSE_ENUM_DECL_START)] = {
        "[PARSE_ENUM] Starting enum declaration parse at line %d",
        "[PARSE_ENUM] enum宣言解析開始 - 行 %d"};
    messages[static_cast<int>(DebugMsgId::PARSE_TYPEDEF_OR_STRUCT_TYPE_FOUND)] =
        {"[PARSE_TYPE] Typedef or struct type found: %s",
         "[PARSE_TYPE] typedef型または構造体型発見: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_IDENTIFIER_AFTER_TYPE)] = {
        "[PARSE_IDENTIFIER] Identifier found after type: %s",
        "[PARSE_IDENTIFIER] 型の後に識別子発見: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_FUNCTION_DETECTED)] = {
        "[PARSE_FUNCTION] Function declaration detected",
        "[PARSE_FUNCTION] 関数宣言を検出"};
    messages[static_cast<int>(DebugMsgId::PARSE_ARRAY_DETECTED)] = {
        "[PARSE_ARRAY] Array declaration detected",
        "[PARSE_ARRAY] 配列宣言を検出"};
    messages[static_cast<int>(DebugMsgId::PARSE_FUNCTION_DECL_FOUND)] = {
        "[PARSE_FUNCTION] Function declaration found: %s returning %s",
        "[PARSE_FUNCTION] 関数宣言発見: %s 戻り値型 %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_VAR_DECL_FOUND)] = {
        "[PARSE_STRUCT_VAR] Struct variable declaration found for type: %s",
        "[PARSE_STRUCT_VAR] 構造体変数宣言発見 - 型: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_ARRAY_DECL)] = {
        "[PARSE_STRUCT_ARRAY] Struct array declaration for type: %s",
        "[PARSE_STRUCT_ARRAY] 構造体配列宣言 - 型: %s"};
    messages[static_cast<int>(DebugMsgId::PARSE_STRUCT_ARRAY_VAR_NAME)] = {
        "[PARSE_STRUCT_ARRAY] Struct array variable name: %s",
        "[PARSE_STRUCT_ARRAY] 構造体配列変数名: %s"};

    // インタープリター構造体関連のメッセージ
    messages[static_cast<int>(
        DebugMsgId::INTERPRETER_STRUCT_ARRAY_MEMBER_ADDED)] = {
        "[INTERPRETER_STRUCT] Array member added: %s (type: %d, size: %d)",
        "[INTERPRETER_STRUCT] 配列メンバー追加: %s (型: %d, サイズ: %d)"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_ARRAY_DIMENSION_INFO)] = {
        "[INTERPRETER_ARRAY] Dimension info: size=%d, is_dynamic=%d, expr='%s'",
        "[INTERPRETER_ARRAY] 次元情報: サイズ=%d, 動的=%d, 式='%s'"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_STRUCT_MEMBER_ADDED)] = {
        "[INTERPRETER_STRUCT] Member added: %s (type: %d)",
        "[INTERPRETER_STRUCT] メンバー追加: %s (型: %d)"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_STRUCT_REGISTERED)] = {
        "[INTERPRETER_STRUCT] Struct registered: %s with %zu members",
        "[INTERPRETER_STRUCT] 構造体登録: %s (メンバー数: %zu)"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_ENUM_REGISTERING)] = {
        "[INTERPRETER_ENUM] Registering enum: %s",
        "[INTERPRETER_ENUM] enum登録: %s"};
    messages[static_cast<int>(
        DebugMsgId::INTERPRETER_MULTIPLE_VAR_DECL_START)] = {
        "[INTERPRETER_VAR] Multiple variable declaration with %zu children",
        "[INTERPRETER_VAR] 複数変数宣言 (子要素数: %zu)"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_GLOBAL_VAR_INIT_START)] =
        {"[INTERPRETER_VAR] Global variable initialization: %s",
         "[INTERPRETER_VAR] グローバル変数初期化: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_ARRAY_LITERAL_INIT)] = {
        "[INTERPRETER_ARRAY] Array literal initialization: %s",
        "[INTERPRETER_ARRAY] 配列リテラル初期化: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_NORMAL_VAR_INIT)] = {
        "[INTERPRETER_VAR] Normal variable initialization: %s",
        "[INTERPRETER_VAR] 通常変数初期化: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_GET_STRUCT_MEMBER)] = {
        "[INTERPRETER_STRUCT] Getting struct member: %s.%s",
        "[INTERPRETER_STRUCT] 構造体メンバー取得: %s.%s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_VAR_NOT_STRUCT)] = {
        "[INTERPRETER_STRUCT] Variable is not a struct: %s",
        "[INTERPRETER_STRUCT] 変数は構造体ではありません: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_STRUCT_MEMBERS_FOUND)] = {
        "[INTERPRETER_STRUCT] Struct members found: %zu",
        "[INTERPRETER_STRUCT] 構造体メンバー発見: %zu個"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_STRUCT_MEMBER_FOUND)] = {
        "[INTERPRETER_STRUCT] Struct member found: %s, is_array=%d",
        "[INTERPRETER_STRUCT] 構造体メンバー発見: %s, 配列=%d"};
    messages[static_cast<int>(
        DebugMsgId::INTERPRETER_NAMED_STRUCT_LITERAL_INIT)] = {
        "[INTERPRETER_STRUCT] Named struct literal initialization: %s",
        "[INTERPRETER_STRUCT] 名前付き構造体リテラル初期化: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_MEMBER_INIT_PROCESSING)] =
        {"[INTERPRETER_STRUCT] Processing member initialization: %s",
         "[INTERPRETER_STRUCT] メンバー初期化処理: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_NESTED_STRUCT_LITERAL)] =
        {"[INTERPRETER_STRUCT] Nested struct literal assignment: %s",
         "[INTERPRETER_STRUCT] ネストした構造体リテラル代入: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_VAR_PROCESS_EXCEPTION)] =
        {"[INTERPRETER_ERROR] Variable processing exception: %s",
         "[INTERPRETER_ERROR] 変数処理例外: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_STRUCT_SYNCED)] = {
        "[INTERPRETER_STRUCT] Synced struct definition: %s with %zu members",
        "[INTERPRETER_STRUCT] 構造体定義同期: %s (メンバー数: %zu)"};
    messages[static_cast<int>(
        DebugMsgId::INTERPRETER_STRUCT_DEFINITION_STORED)] = {
        "[INTERPRETER_STRUCT] Storing struct definition: %s (constant "
        "resolution deferred)",
        "[INTERPRETER_STRUCT] 構造体定義格納: %s (定数解決延期)"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_PROCESSING_STMT_LIST)] = {
        "[INTERPRETER_INIT] Processing AST_STMT_LIST with %zu statements",
        "[INTERPRETER_INIT] AST_STMT_LIST処理中 (文の数: %zu)"};
    messages[static_cast<int>(
        DebugMsgId::INTERPRETER_CHECKING_STATEMENT_TYPE)] = {
        "[INTERPRETER_INIT] Checking statement type: %d (name: %s)",
        "[INTERPRETER_INIT] 文の型チェック: %d (名前: %s)"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOUND_VAR_DECL)] = {
        "[INTERPRETER_INIT] Found AST_VAR_DECL: %s, recursing",
        "[INTERPRETER_INIT] AST_VAR_DECL発見: %s, 再帰処理"};
    messages[static_cast<int>(
        DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START)] = {
        "[INTERPRETER_STRUCT] Starting sync of struct members for variable: %s",
        "[INTERPRETER_STRUCT] 構造体メンバー同期開始: %s"};
    messages[static_cast<int>(
        DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_END)] = {
        "[INTERPRETER_STRUCT] Completed sync of struct members for variable: "
        "%s",
        "[INTERPRETER_STRUCT] 構造体メンバー同期完了: %s"};

    // インタープリター実行関連のメッセージ
    messages[static_cast<int>(DebugMsgId::INTERPRETER_STMT_DETAILS)] = {
        "[INTERPRETER_EXEC] Executing statement type: %d, name: %s",
        "[INTERPRETER_EXEC] 文実行 - 型: %d, 名前: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_STMT_LIST_EXEC)] = {
        "[INTERPRETER_STMT_LIST] Executing statement list with %zu statements",
        "[INTERPRETER_STMT_LIST] 文リスト実行 - 文の数: %zu"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_COMPOUND_STMT_EXEC)] = {
        "[INTERPRETER_COMPOUND] Executing compound statement with %zu "
        "statements",
        "[INTERPRETER_COMPOUND] 複合文実行 - 文の数: %zu"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_VAR_DECL_TYPE)] = {
        "[INTERPRETER_VAR] Variable declaration type: %d",
        "[INTERPRETER_VAR] 変数宣言型: %d"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_VAR_DECL_SUCCESS)] = {
        "[INTERPRETER_VAR] Variable declaration success: %s",
        "[INTERPRETER_VAR] 変数宣言成功: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_ASSIGNMENT)] = {
        "[INTERPRETER_ASSIGN] Processing assignment to: %s",
        "[INTERPRETER_ASSIGN] 代入処理: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_ASSIGNMENT_SUCCESS)] = {
        "[INTERPRETER_ASSIGN] Assignment completed successfully: %s",
        "[INTERPRETER_ASSIGN] 代入完了: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_MULTIPLE_VAR_DECL_EXEC)] =
        {"[INTERPRETER_MULTIPLE_VAR] Executing multiple variable declaration",
         "[INTERPRETER_MULTIPLE_VAR] 複数変数宣言実行"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_ARRAY_DECL_EXEC)] = {
        "[INTERPRETER_ARRAY] Executing array declaration: %s",
        "[INTERPRETER_ARRAY] 配列宣言実行: %s"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_IF_STMT_START)] = {
        "[INTERPRETER_IF] Starting if statement execution",
        "[INTERPRETER_IF] if文実行開始"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_IF_CONDITION_RESULT)] = {
        "[INTERPRETER_IF] Condition result: %lld",
        "[INTERPRETER_IF] 条件結果: %lld"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_IF_THEN_EXEC)] = {
        "[INTERPRETER_IF] Executing then branch",
        "[INTERPRETER_IF] then分岐実行"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_IF_ELSE_EXEC)] = {
        "[INTERPRETER_IF] Executing else branch",
        "[INTERPRETER_IF] else分岐実行"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_IF_STMT_END)] = {
        "[INTERPRETER_IF] If statement execution complete",
        "[INTERPRETER_IF] if文実行完了"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_WHILE_STMT_START)] = {
        "[INTERPRETER_WHILE] While loop start",
        "[INTERPRETER_WHILE] whileループ開始"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_WHILE_CONDITION_CHECK)] =
        {"[INTERPRETER_WHILE] Condition check iteration: %d",
         "[INTERPRETER_WHILE] 条件チェック回数: %d"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_WHILE_CONDITION_RESULT)] =
        {"[INTERPRETER_WHILE] Condition result: %lld",
         "[INTERPRETER_WHILE] 条件結果: %lld"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_WHILE_BODY_EXEC)] = {
        "[INTERPRETER_WHILE] Executing body iteration: %d",
        "[INTERPRETER_WHILE] ボディ実行回数: %d"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_WHILE_BREAK)] = {
        "[INTERPRETER_WHILE] Break detected", "[INTERPRETER_WHILE] break検出"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_WHILE_STMT_END)] = {
        "[INTERPRETER_WHILE] While loop complete",
        "[INTERPRETER_WHILE] whileループ完了"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOR_STMT_START)] = {
        "[INTERPRETER_FOR] For loop start", "[INTERPRETER_FOR] forループ開始"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOR_INIT_EXEC)] = {
        "[INTERPRETER_FOR] Executing initialization",
        "[INTERPRETER_FOR] 初期化実行"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOR_CONDITION_CHECK)] = {
        "[INTERPRETER_FOR] Condition check iteration: %d",
        "[INTERPRETER_FOR] 条件チェック回数: %d"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOR_CONDITION_RESULT)] = {
        "[INTERPRETER_FOR] Condition result: %lld",
        "[INTERPRETER_FOR] 条件結果: %lld"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOR_BODY_EXEC)] = {
        "[INTERPRETER_FOR] Executing body iteration: %d",
        "[INTERPRETER_FOR] ボディ実行回数: %d"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOR_CONTINUE)] = {
        "[INTERPRETER_FOR] Continue detected at iteration: %d",
        "[INTERPRETER_FOR] continue検出 回数: %d"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_FOR_UPDATE_EXEC)] = {
        "[INTERPRETER_FOR] Executing update iteration: %d",
        "[INTERPRETER_FOR] 更新実行回数: %d"};

    // SWITCH文関連のメッセージ
    messages[static_cast<int>(DebugMsgId::INTERPRETER_SWITCH_STMT_START)] = {
        "[INTERPRETER_SWITCH] Switch statement start",
        "[INTERPRETER_SWITCH] switch文開始"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_SWITCH_VALUE)] = {
        "[INTERPRETER_SWITCH] Switch value: %lld",
        "[INTERPRETER_SWITCH] switch値: %lld"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_SWITCH_CASE_MATCHED)] = {
        "[INTERPRETER_SWITCH] Case matched", "[INTERPRETER_SWITCH] caseマッチ"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_SWITCH_ELSE_EXEC)] = {
        "[INTERPRETER_SWITCH] Executing else clause",
        "[INTERPRETER_SWITCH] else節実行"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_SWITCH_STMT_END)] = {
        "[INTERPRETER_SWITCH] Switch statement end",
        "[INTERPRETER_SWITCH] switch文終了"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_SWITCH_RANGE_CHECK)] = {
        "[INTERPRETER_SWITCH] Range check: %lld...%lld",
        "[INTERPRETER_SWITCH] 範囲チェック: %lld...%lld"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_SWITCH_VALUE_CHECK)] = {
        "[INTERPRETER_SWITCH] Value check: %lld == %lld",
        "[INTERPRETER_SWITCH] 値チェック: %lld == %lld"};

    // 式評価関連のメッセージ
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_START)] = {
        "[EXPR_EVAL] Starting expression evaluation: %s",
        "[EXPR_EVAL] 式評価開始: %s"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_STRING_LITERAL)] = {
        "[EXPR_EVAL] String literal: %s", "[EXPR_EVAL] 文字列リテラル: %s"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_VAR_NOT_FOUND)] = {
        "[EXPR_EVAL] Variable not found: %s",
        "[EXPR_EVAL] 変数が見つかりません: %s"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_VAR_VALUE)] = {
        "[EXPR_EVAL] Variable %s value: %lld",
        "[EXPR_EVAL] 変数 %s の値: %lld"};
    messages[static_cast<int>(
        DebugMsgId::EXPR_EVAL_MULTIDIM_MEMBER_ARRAY_ACCESS)] = {
        "[EXPR_EVAL] Multidimensional member array access",
        "[EXPR_EVAL] 多次元メンバー配列アクセス"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_MEMBER_ACCESS_DETAILS)] = {
        "[EXPR_EVAL] Member access: object=%s, member=%s",
        "[EXPR_EVAL] メンバーアクセス: オブジェクト=%s, メンバー=%s"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_ARRAY_INDEX)] = {
        "[EXPR_EVAL] Array index: %lld", "[EXPR_EVAL] 配列インデックス: %lld"};

    // 多次元文字列配列アクセス関連
    messages[static_cast<int>(DebugMsgId::MULTIDIM_STRING_ARRAY_ACCESS)] = {
        "[MULTIDIM_STRING] Accessing array '%s'",
        "[MULTIDIM_STRING] 配列 '%s' にアクセス"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_STRING_ARRAY_INDICES)] = {
        "[MULTIDIM_STRING] Indices: %s", "[MULTIDIM_STRING] インデックス: %s"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_STRING_ARRAY_VALUE)] = {
        "[MULTIDIM_STRING] Retrieved value: '%s'",
        "[MULTIDIM_STRING] 取得された値: '%s'"};

    // printf処理関連
    messages[static_cast<int>(DebugMsgId::PRINTF_PROCESSING_ARRAY_REF)] = {
        "[PRINTF] Processing ARRAY_REF for printf",
        "[PRINTF] printf用のARRAY_REF処理中"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARRAY_NAME_FOUND)] = {
        "[PRINTF] Array name: %s", "[PRINTF] 配列名: %s"};
    messages[static_cast<int>(DebugMsgId::PRINTF_VARIABLE_FOUND)] = {
        "[PRINTF] Variable found: %s", "[PRINTF] 変数発見: %s"};
    messages[static_cast<int>(DebugMsgId::PRINTF_STRING_MULTIDIM_PROCESSING)] =
        {"[PRINTF] Processing string multidimensional array",
         "[PRINTF] 文字列多次元配列処理中"};
    messages[static_cast<int>(DebugMsgId::PRINTF_STRING_VALUE_RETRIEVED)] = {
        "[PRINTF] Got string value: '%s'", "[PRINTF] 文字列値取得: '%s'"};

    // 既存のstructおよび式評価関連メッセージ
    messages[static_cast<int>(DebugMsgId::STRUCT_DEF_STORE)] = {
        "Struct definition stored: %s", "構造体定義保存: %s"};
    messages[static_cast<int>(DebugMsgId::STRUCT_VAR_CREATE)] = {
        "Struct variable created: %s", "構造体変数作成: %s"};
    messages[static_cast<int>(DebugMsgId::STRUCT_MULTIDIM_ARRAY_CREATE)] = {
        "Struct multidimensional array created", "構造体多次元配列作成"};
    messages[static_cast<int>(DebugMsgId::STRUCT_ARRAY_MEMBER_CREATE)] = {
        "Struct array member created: %s", "構造体配列メンバー作成: %s"};
    messages[static_cast<int>(DebugMsgId::STRUCT_REGULAR_MEMBER_CREATE)] = {
        "Struct regular member created: %s", "構造体通常メンバー作成: %s"};

    // 式評価の追加メッセージ
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_STRUCT_MEMBER)] = {
        "[INTERPRETER_STRUCT] Struct member evaluation: %s",
        "[INTERPRETER_STRUCT] 構造体メンバー評価: %s"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_MULTIDIM_ACCESS)] = {
        "[INTERPRETER_ARRAY] Multidimensional access evaluation",
        "[INTERPRETER_ARRAY] 多次元アクセス評価"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_CONDITION_FAILED)] = {
        "Expression evaluation condition failed", "式評価条件失敗"};
    messages[static_cast<int>(DebugMsgId::VARIABLE_ACCESS_ERROR)] = {
        "Variable access error: %s", "変数アクセスエラー: %s"};

    // print関連の追加メッセージ
    messages[static_cast<int>(DebugMsgId::PRINT_NO_ARGUMENTS)] = {
        "Print with no arguments", "引数なしのprint"};

    // interface/impl関連のメッセージ
    messages[static_cast<int>(DebugMsgId::INTERFACE_DECL_START)] = {
        "[INTERFACE] Starting interface declaration: %s",
        "[INTERFACE] インターフェース宣言開始: %s"};
    messages[static_cast<int>(DebugMsgId::INTERFACE_DECL_COMPLETE)] = {
        "[INTERFACE] Interface declaration complete: %s",
        "[INTERFACE] インターフェース宣言完了: %s"};
    messages[static_cast<int>(DebugMsgId::INTERFACE_METHOD_FOUND)] = {
        "[INTERFACE] Method found in interface: %s",
        "[INTERFACE] インターフェースメソッド発見: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_DECL_START)] = {
        "[IMPL] Starting impl declaration: %s", "[IMPL] impl宣言開始: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_DECL_COMPLETE)] = {
        "[IMPL] Impl declaration complete: %s", "[IMPL] impl宣言完了: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_METHOD_REGISTER)] = {
        "[IMPL] Registering method: %s", "[IMPL] メソッド登録: %s"};
    messages[static_cast<int>(DebugMsgId::IMPL_METHOD_REGISTER_COMPLETE)] = {
        "[IMPL] Method registration complete: %s",
        "[IMPL] メソッド登録完了: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_START)] = {
        "[METHOD] Method call started: %s",
        "[METHOD] メソッド呼び出し開始: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_RECEIVER_FOUND)] = {
        "[METHOD] Receiver found: %s", "[METHOD] レシーバー発見: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_INTERFACE)] = {
        "[METHOD] Interface method call: %s on type: %s",
        "[METHOD] interfaceメソッド呼び出し: %s 型: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_CHAIN)] = {
        "[METHOD] Processing method chain: %s",
        "[METHOD] メソッドチェーン処理: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_CHAIN_TEMP)] = {
        "[METHOD] Created temporary variable for chain: %s",
        "[METHOD] チェーン用一時変数作成: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_SELF_CONTEXT_SET)] = {
        "[METHOD] Self context set for: %s",
        "[METHOD] selfコンテキスト設定: %s"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_SELF_MEMBER_SETUP)] = {
        "[METHOD] Self member setup complete", "[METHOD] selfメンバー設定完了"};
    messages[static_cast<int>(DebugMsgId::METHOD_CALL_EXECUTE)] = {
        "[METHOD] Executing method: %s", "[METHOD] メソッド実行: %s"};
    messages[static_cast<int>(DebugMsgId::SELF_MEMBER_ACCESS_START)] = {
        "[SELF] Accessing self member: %s", "[SELF] selfメンバーアクセス: %s"};
    messages[static_cast<int>(DebugMsgId::SELF_MEMBER_ACCESS_FOUND)] = {
        "[SELF] Self member found: %s", "[SELF] selfメンバー発見: %s"};
    messages[static_cast<int>(DebugMsgId::SELF_MEMBER_ACCESS_VALUE)] = {
        "[SELF] Self member value: %d", "[SELF] selfメンバー値: %d"};

    // interface変数代入関連
    messages[static_cast<int>(DebugMsgId::INTERFACE_VARIABLE_ASSIGN)] = {
        "[INTERFACE] Assigning struct to interface variable: %s <- %s",
        "[INTERFACE] 構造体をinterface変数に代入: %s <- %s"};

    // 三項演算子型推論関連
    messages[static_cast<int>(DebugMsgId::TERNARY_EVAL_START)] = {
        "[TERNARY] Evaluating ternary expression with typed inference",
        "[TERNARY] 型推論付き三項演算子を評価"};
    messages[static_cast<int>(DebugMsgId::TERNARY_NODE_TYPE)] = {
        "[TERNARY] Selected node type: %d, inferred type: %d",
        "[TERNARY] 選択されたノード型: %d, 推論型: %d"};
    messages[static_cast<int>(DebugMsgId::TERNARY_TYPE_INFERENCE)] = {
        "[TERNARY] Type inference result - Type: %d, TypeName: %s",
        "[TERNARY] 型推論結果 - 型: %d, 型名: %s"};
    messages[static_cast<int>(DebugMsgId::TERNARY_STRING_MEMBER_ACCESS)] = {
        "[TERNARY] Processing string member access",
        "[TERNARY] 文字列メンバアクセス処理"};
    messages[static_cast<int>(DebugMsgId::TERNARY_NUMERIC_EVAL)] = {
        "[TERNARY] Numeric evaluation result: %lld",
        "[TERNARY] 数値評価結果: %lld"};
    messages[static_cast<int>(DebugMsgId::TERNARY_STRING_EVAL)] = {
        "[TERNARY] String evaluation result: %s",
        "[TERNARY] 文字列評価結果: %s"};

    // 三項演算子変数初期化関連
    messages[static_cast<int>(DebugMsgId::TERNARY_VAR_INIT_START)] = {
        "[TERNARY_VAR] Starting ternary variable initialization",
        "[TERNARY_VAR] 三項演算子変数初期化開始"};
    messages[static_cast<int>(DebugMsgId::TERNARY_VAR_CONDITION)] = {
        "[TERNARY_VAR] Condition evaluated: %lld",
        "[TERNARY_VAR] 条件評価結果: %lld"};
    messages[static_cast<int>(DebugMsgId::TERNARY_VAR_BRANCH_TYPE)] = {
        "[TERNARY_VAR] Selected branch node type: %d",
        "[TERNARY_VAR] 選択された分岐ノード型: %d"};
    messages[static_cast<int>(DebugMsgId::TERNARY_VAR_STRING_SET)] = {
        "[TERNARY_VAR] Setting string value: %s",
        "[TERNARY_VAR] 文字列値設定: %s"};
    messages[static_cast<int>(DebugMsgId::TERNARY_VAR_NUMERIC_SET)] = {
        "[TERNARY_VAR] Setting numeric value: %lld",
        "[TERNARY_VAR] 数値設定: %lld"};

    // インクリメント/デクリメント関連
    messages[static_cast<int>(DebugMsgId::INCDEC_ARRAY_ELEMENT_START)] = {
        "[INCDEC] Array element increment/decrement started",
        "[INCDEC] 配列要素インクリメント/デクリメント開始"};
    messages[static_cast<int>(DebugMsgId::INCDEC_ARRAY_NAME_FOUND)] = {
        "[INCDEC] Array name: %s", "[INCDEC] 配列名: %s"};
    messages[static_cast<int>(DebugMsgId::INCDEC_ARRAY_INDEX_EVAL)] = {
        "[INCDEC] Array index evaluated: %lld",
        "[INCDEC] 配列インデックス評価: %lld"};
    messages[static_cast<int>(DebugMsgId::INCDEC_ELEMENT_TYPE_CHECK)] = {
        "[INCDEC] Checking element type: is_multidim=%d, has_int=%d, "
        "has_float=%d, has_double=%d",
        "[INCDEC] 要素型チェック: 多次元=%d, int有=%d, float有=%d, "
        "double有=%d"};
    messages[static_cast<int>(DebugMsgId::INCDEC_INT_ARRAY_PROCESSING)] = {
        "[INCDEC] Processing integer array element",
        "[INCDEC] 整数配列要素処理"};
    messages[static_cast<int>(DebugMsgId::INCDEC_FLOAT_ARRAY_PROCESSING)] = {
        "[INCDEC] Processing float array element",
        "[INCDEC] float配列要素処理"};
    messages[static_cast<int>(DebugMsgId::INCDEC_DOUBLE_ARRAY_PROCESSING)] = {
        "[INCDEC] Processing double array element",
        "[INCDEC] double配列要素処理"};
    messages[static_cast<int>(DebugMsgId::INCDEC_OLD_VALUE)] = {
        "[INCDEC] Old value: %s", "[INCDEC] 旧値: %s"};
    messages[static_cast<int>(DebugMsgId::INCDEC_NEW_VALUE)] = {
        "[INCDEC] New value: %s", "[INCDEC] 新値: %s"};
    messages[static_cast<int>(DebugMsgId::INCDEC_OPERATION_COMPLETE)] = {
        "[INCDEC] Operation complete: op=%s, result=%lld",
        "[INCDEC] 操作完了: op=%s, 結果=%lld"};
    messages[static_cast<int>(DebugMsgId::INCDEC_UNSUPPORTED_TYPE_ERROR)] = {
        "[INCDEC_ERROR] Unsupported array type for increment/decrement",
        "[INCDEC_ERROR] インクリメント/デクリメント未対応の配列型"};

    // assert関連
    messages[static_cast<int>(DebugMsgId::ASSERT_CHECK_START)] = {
        "[ASSERT] Assertion check started",
        "[ASSERT] アサーションチェック開始"};
    messages[static_cast<int>(DebugMsgId::ASSERT_CONDITION_TRUE)] = {
        "[ASSERT] Condition is true, continuing execution",
        "[ASSERT] 条件が真、実行継続"};
    messages[static_cast<int>(DebugMsgId::ASSERT_CONDITION_FALSE)] = {
        "[ASSERT] Condition is false at line %d", "[ASSERT] 条件が偽: 行 %d"};
    messages[static_cast<int>(DebugMsgId::ASSERT_FAILURE)] = {
        "[ASSERT_ERROR] Assertion failed at line %d: %s",
        "[ASSERT_ERROR] アサーション失敗: 行 %d: %s"};

    // ネストした構造体メンバーアクセス関連
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_EVAL_START)] = {
        "[NESTED_MEMBER] Evaluating nested member access: %s",
        "[NESTED_MEMBER] ネストメンバーアクセス評価開始: %s"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_BASE_PATH)] = {
        "[NESTED_MEMBER] Base path='%s', member='%s'",
        "[NESTED_MEMBER] ベースパス='%s', メンバー='%s'"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_BASE_VAR_FOUND)] = {
        "[NESTED_MEMBER] Base variable found, type=%d",
        "[NESTED_MEMBER] ベース変数発見, 型=%d"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_BASE_VAR_NOT_FOUND)] = {
        "[NESTED_MEMBER] Base variable not found",
        "[NESTED_MEMBER] ベース変数未発見"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_RESOLVE_FROM_BASE)] = {
        "[NESTED_MEMBER] Resolving from base name",
        "[NESTED_MEMBER] ベース名から解決中"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_RESOLVE_SUCCESS)] = {
        "[NESTED_MEMBER] Resolution successful, value=%lld",
        "[NESTED_MEMBER] 解決成功, 値=%lld"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_RESOLVE_FAILED)] = {
        "[NESTED_MEMBER] Resolution failed", "[NESTED_MEMBER] 解決失敗"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_INDIVIDUAL_VAR_FOUND)] =
        {"[NESTED_MEMBER] Individual variable found: '%s' = %lld",
         "[NESTED_MEMBER] 個別変数発見: '%s' = %lld"};
    messages[static_cast<int>(DebugMsgId::NESTED_MEMBER_FULL_PATH)] = {
        "[NESTED_MEMBER] Full path: '%s'", "[NESTED_MEMBER] 完全パス: '%s'"};
    messages[static_cast<int>(DebugMsgId::TYPED_EVAL_ENTRY)] = {
        "[TYPED_EVAL] Entry: node_type=%d",
        "[TYPED_EVAL] エントリー: ノード型=%d"};
    messages[static_cast<int>(DebugMsgId::TYPED_EVAL_INTERNAL_ENTRY)] = {
        "[TYPED_EVAL_INTERNAL] Entry: node_type=%d",
        "[TYPED_EVAL_INTERNAL] エントリー: ノード型=%d"};
    messages[static_cast<int>(DebugMsgId::TYPED_MEMBER_ACCESS_CASE)] = {
        "[TYPED_MEMBER_ACCESS] Processing member='%s', chain_size=%zu",
        "[TYPED_MEMBER_ACCESS] メンバー処理='%s', チェーンサイズ=%zu"};

    // v0.12.0: async/await関連メッセージ (Phase 1)
    messages[static_cast<int>(DebugMsgId::ASYNC_FUNCTION_CALL)] = {
        "[ASYNC] Calling async function: %s", "[ASYNC] async関数呼び出し: %s"};
    messages[static_cast<int>(DebugMsgId::ASYNC_FUNCTION_RETURNED)] = {
        "[ASYNC] Function returned value: %lld (type=%d)",
        "[ASYNC] 関数が値を返却: %lld (型=%d)"};
    messages[static_cast<int>(DebugMsgId::ASYNC_WRAPPING_FUTURE)] = {
        "[ASYNC] Wrapping return value in Future (is_ready=true)",
        "[ASYNC] 戻り値をFutureでラップ (is_ready=true)"};
    messages[static_cast<int>(DebugMsgId::AWAIT_EXPRESSION_START)] = {
        "[AWAIT] Awaiting Future from variable: %s",
        "[AWAIT] 変数からFutureを待機: %s"};
    messages[static_cast<int>(DebugMsgId::AWAIT_FUTURE_READY_CHECK)] = {
        "[AWAIT] Future is_ready=%s", "[AWAIT] Future is_ready=%s"};
    messages[static_cast<int>(DebugMsgId::AWAIT_VALUE_EXTRACTED)] = {
        "[AWAIT] Extracted value: %lld (type=%d)",
        "[AWAIT] 抽出された値: %lld (型=%d)"};
    messages[static_cast<int>(DebugMsgId::AWAIT_FUTURE_RECEIVED)] = {
        "[AWAIT] Received Future: is_struct=%d, type_name=%s, task_id=%d",
        "[AWAIT] Future受信: is_struct=%d, 型名=%s, task_id=%d"};
    messages[static_cast<int>(DebugMsgId::AWAIT_RUN_UNTIL_COMPLETE)] = {
        "[AWAIT] Running until complete for task_id=%lld",
        "[AWAIT] task_id=%lldの完了まで実行"};

    // v0.13.0: async/await Phase 2 - Event Loop & yield
    messages[static_cast<int>(DebugMsgId::ASYNC_YIELD_CONTROL)] = {
        "[ASYNC] Task yielded control to event loop",
        "[ASYNC] タスクがイベントループに制御を渡しました"};

    // v0.13.0: 追加の async/await デバッグメッセージ
    messages[static_cast<int>(DebugMsgId::ASYNC_TASK_ID_SET)] = {
        "[ASYNC] Task registered with ID: %d, Future.task_id set to: %d",
        "[ASYNC] タスク登録 ID: %d, Future.task_id設定: %d"};
    messages[static_cast<int>(DebugMsgId::ASYNC_TASK_RETURN_FUTURE)] = {
        "[ASYNC] Returning Future: struct_type_name=%s, members=%d",
        "[ASYNC] Futureを返す: struct_type_name=%s, メンバー数=%d"};
    messages[static_cast<int>(DebugMsgId::ASYNC_INTERNAL_FUTURE_MEMBERS)] = {
        "[ASYNC] Before register_task, internal_future members: %d",
        "[ASYNC] register_task前、internal_futureメンバー数: %d"};
    messages[static_cast<int>(DebugMsgId::AWAIT_TASK_WAITING)] = {
        "[AWAIT] Task %d is now waiting for task %d",
        "[AWAIT] タスク %d がタスク %d を待機中"};
    messages[static_cast<int>(DebugMsgId::AWAIT_VALUE_EXTRACT)] = {
        "[AWAIT] Extracting value from Future: type=%d, value=%lld",
        "[AWAIT] Futureから値を抽出: 型=%d, 値=%lld"};
    messages[static_cast<int>(DebugMsgId::AWAIT_INTERNAL_FUTURE)] = {
        "[AWAIT] Value found in internal_future",
        "[AWAIT] internal_futureから値を取得"};
    messages[static_cast<int>(DebugMsgId::AWAIT_TASK_COMPLETED)] = {
        "[AWAIT] Task already ready, retrieving value from task %d",
        "[AWAIT] タスクは既に完了、タスク %d から値を取得"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_REGISTER_TASK)] = {
        "[SIMPLE_EVENT_LOOP] Registering task %d with %d members",
        "[SIMPLE_EVENT_LOOP] タスク %d を登録、メンバー数 %d"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_STORE_TASK)] = {
        "[SIMPLE_EVENT_LOOP] About to store task %d",
        "[SIMPLE_EVENT_LOOP] タスク %d を保存"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_RUN_ONE_CYCLE)] = {
        "[SIMPLE_EVENT_LOOP] run_one_cycle: processing %d task(s)",
        "[SIMPLE_EVENT_LOOP] run_one_cycle: %d タスク処理中"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_SKIP_EXECUTING)] = {
        "[SIMPLE_EVENT_LOOP] run_one_cycle: skipping task %d (currently "
        "executing)",
        "[SIMPLE_EVENT_LOOP] run_one_cycle: タスク %d をスキップ（実行中）"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_TASK_RESUME)] = {
        "[EVENT_LOOP] Task %d resumed (waited task completed)",
        "[EVENT_LOOP] タスク %d 再開（待機タスク完了）"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_TASK_SKIP)] = {
        "[EVENT_LOOP] Skipping task %d (currently executing)",
        "[EVENT_LOOP] タスク %d をスキップ（実行中）"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_TASK_COMPLETED)] = {
        "[SIMPLE_EVENT_LOOP] Task %d completed, set is_ready=true",
        "[SIMPLE_EVENT_LOOP] タスク %d 完了、is_ready=trueに設定"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_SET_VALUE)] = {
        "[SIMPLE_EVENT_LOOP] Setting return value to internal_future (type=%d)",
        "[SIMPLE_EVENT_LOOP] internal_futureに戻り値を設定（型=%d）"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_GET_TASK)] = {
        "[SIMPLE_EVENT_LOOP] get_task(%d) returned: %s",
        "[SIMPLE_EVENT_LOOP] get_task(%d) の結果: %s"};
    messages[static_cast<int>(DebugMsgId::EVENT_LOOP_RUN_UNTIL_COMPLETE)] = {
        "[SIMPLE_EVENT_LOOP] run_until_complete: task %d, status: %s",
        "[SIMPLE_EVENT_LOOP] run_until_complete: タスク %d、ステータス: %s"};
    messages[static_cast<int>(DebugMsgId::SLEEP_TASK_REGISTER)] = {
        "[SLEEP] Registered sleep task %d for %lldms (wake_up_time=%lld)",
        "[SLEEP] sleepタスク %d を登録、%lldミリ秒（wake_up_time=%lld）"};
    messages[static_cast<int>(DebugMsgId::SLEEP_RETURN_FUTURE)] = {
        "[SLEEP] Returning Future with task_id=%d",
        "[SLEEP] task_id=%d のFutureを返す"};
    messages[static_cast<int>(DebugMsgId::SLEEP_TASK_SLEEPING)] = {
        "[SIMPLE_EVENT_LOOP] Task %d still sleeping (remaining: %lldms)",
        "[SIMPLE_EVENT_LOOP] タスク %d はまだsleep中（残り: %lldミリ秒）"};
    messages[static_cast<int>(DebugMsgId::SLEEP_TASK_WOKE_UP)] = {
        "[SIMPLE_EVENT_LOOP] Task %d woke up",
        "[SIMPLE_EVENT_LOOP] タスク %d が起床"};

    // TypedValue pointer debug messages
    messages[static_cast<int>(DebugMsgId::TYPED_VALUE_POINTER_CONSTRUCT)] = {
        "[TypedValue] Pointer constructor: value=%lld (0x%llx)",
        "[TypedValue] ポインタコンストラクタ: value=%lld (0x%llx)"};
    messages[static_cast<int>(
        DebugMsgId::TYPED_VALUE_POINTER_CONSTRUCT_LD)] = {
        "[TypedValue] Pointer constructor (long double): val=%Lf, value=%lld "
        "(0x%llx)",
        "[TypedValue] ポインタコンストラクタ (long double): val=%Lf, "
        "value=%lld (0x%llx)"};
    messages[static_cast<int>(DebugMsgId::TYPED_VALUE_AS_NUMERIC_POINTER)] = {
        "[TypedValue] as_numeric pointer: value=%lld (0x%llx)",
        "[TypedValue] as_numeric ポインタ: value=%lld (0x%llx)"};

    // 他の未設定のメッセージにはデフォルト値を設定
    for (size_t i = 0; i < messages.size(); ++i) {
        if (messages[i].en == nullptr) {
            messages[i] = {"Debug message", "デバッグメッセージ"};
        }
    }

    messages[static_cast<int>(DebugMsgId::GENERIC_DEBUG)] = {"[DEBUG] %s",
                                                             "[DEBUG] %s"};
}

} // namespace Interpreter
} // namespace DebugMessages
