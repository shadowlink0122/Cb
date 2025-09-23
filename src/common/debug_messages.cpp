#include "debug_messages.h"
#include <vector>

// デバッグメッセージテンプレート配列を動的に初期化する関数
static std::vector<DebugMessageTemplate> init_debug_messages() {
    // enumの最大値まで初期化
    std::vector<DebugMessageTemplate> messages(200);

    // 基本的なメッセージのみ設定（使用頻度の高いもの）
    messages[static_cast<int>(DebugMsgId::INTERPRETER_START)] = {
        "Interpreter::process() starting", "Interpreter::process() 開始"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_FOUND)] = {
        "Main function found", "main関数が見つかりました"};
    messages[static_cast<int>(DebugMsgId::EXECUTION_COMPLETE)] = {
        "Execution completed successfully", "実行が正常に終了しました"};
    messages[static_cast<int>(DebugMsgId::VAR_DECLARATION_DEBUG)] = {
        "Variable declaration: %s", "変数宣言: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_DEBUG)] = {
        "Processing array declaration: %s", "配列宣言処理: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DIMENSIONS_COUNT)] = {
        "Array dimensions: %d", "配列次元数: %d"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_PROCESSING)] = {
        "Processing as multidimensional array", "多次元配列として処理"};
    messages[static_cast<int>(DebugMsgId::PRINTF_OFFSET_CALLED)] = {
        "printf offset called", "printf offset 呼び出し"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_EVAL_DEBUG)] = {
        "Array declaration evaluation: %s", "配列宣言評価: %s"};
    messages[static_cast<int>(DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR)] = {
        "Array size is negative: %s", "配列サイズが負です: %s"};
    messages[static_cast<int>(DebugMsgId::UNDEFINED_VAR_ERROR)] = {
        "Undefined variable: %s", "未定義の変数です: %s"};
    messages[static_cast<int>(DebugMsgId::ZERO_DIVISION_ERROR)] = {
        "Division by zero error", "ゼロ除算エラー"};
    messages[static_cast<int>(DebugMsgId::PARSER_ERROR)] = {"Parser error",
                                                            "パーサーエラー"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_NOT_FOUND_ERROR)] = {
        "main function not found", "main関数が見つかりません"};

    // Expression evaluation messages
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_NUMBER)] = {
        "Expression eval: number %lld", "式評価: 数値 %lld"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_BINARY_OP)] = {
        "Expression eval: binary op %s", "式評価: 二項演算 %s"};
    messages[static_cast<int>(DebugMsgId::BINARY_OP_VALUES)] = {
        "Binary op values: left=%lld, right=%lld",
        "二項演算値: 左=%lld, 右=%lld"};
    messages[static_cast<int>(DebugMsgId::BINARY_OP_RESULT_DEBUG)] = {
        "Binary op result: %lld", "二項演算結果: %lld"};

    // Variable management messages
    messages[static_cast<int>(DebugMsgId::VAR_ASSIGN_READABLE)] = {
        "Variable assign: %s = %lld", "変数代入: %s = %lld"};
    messages[static_cast<int>(DebugMsgId::VAR_CREATE_NEW)] = {
        "Creating new variable", "新しい変数を作成"};
    messages[static_cast<int>(DebugMsgId::EXISTING_VAR_ASSIGN_DEBUG)] = {
        "Assigning to existing variable", "既存変数に代入"};

    // Array management messages
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_START)] = {
        "Array declaration start: %s", "配列宣言開始: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_SUCCESS)] = {
        "Array declaration success: %s", "配列宣言成功: %s"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_DECL_SUCCESS)] = {
        "Multidimensional array declaration success: %s",
        "多次元配列宣言成功: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_TOTAL_SIZE)] = {
        "Array total size: %d", "配列総サイズ: %d"};

    // Function and parsing messages
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_ASSIGN)] = {
        "Creating assignment node: %s", "代入ノード作成: %s"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_VAR_DECL)] = {
        "Creating variable declaration node: %s", "変数宣言ノード作成: %s"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_FUNC_DECL)] = {
        "Creating function declaration node: %s", "関数宣言ノード作成: %s"};

    // パーサー関連の詳細メッセージ
    messages[static_cast<int>(DebugMsgId::PARSING_START)] = {"Parsing start",
                                                             "解析開始"};
    messages[static_cast<int>(DebugMsgId::AST_GENERATED)] = {"AST generated",
                                                             "AST生成完了"};
    messages[static_cast<int>(DebugMsgId::GLOBAL_DECL_START)] = {
        "Global declaration start", "グローバル宣言開始"};
    messages[static_cast<int>(DebugMsgId::GLOBAL_DECL_COMPLETE)] = {
        "Global declaration complete", "グローバル宣言完了"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_SEARCH)] = {
        "Searching for main function", "main関数を検索中"};

    // 実行関連のメッセージ
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_VAR_REF)] = {
        "Expression eval: variable reference %s", "式評価: 変数参照 %s"};
    messages[static_cast<int>(DebugMsgId::VAR_VALUE)] = {
        "Variable value: %s = %lld", "変数値: %s = %lld"};
    messages[static_cast<int>(DebugMsgId::EXPR_EVAL_ARRAY_REF)] = {
        "Expression eval: array reference", "式評価: 配列参照"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INDEX)] = {
        "Array index: %lld", "配列インデックス: %lld"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ACCESS)] = {
        "Array element access: %s[%lld]", "配列要素アクセス: %s[%lld]"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_VALUE)] = {
        "Array element value: %lld", "配列要素値: %lld"};

    // 配列初期化関連
    messages[static_cast<int>(DebugMsgId::ARRAY_INIT_CALLED)] = {
        "Array initialization called", "配列初期化呼び出し"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INIT_COMPLETED)] = {
        "Array initialization completed", "配列初期化完了"};
    messages[static_cast<int>(DebugMsgId::ARRAY_LITERAL_CALLED)] = {
        "Array literal called", "配列リテラル呼び出し"};
    messages[static_cast<int>(DebugMsgId::ARRAY_LITERAL_COMPLETED)] = {
        "Array literal completed", "配列リテラル完了"};

    // 文字列関連
    messages[static_cast<int>(DebugMsgId::STRING_ELEMENT_ACCESS)] = {
        "String element access: index %lld",
        "文字列要素アクセス: インデックス %lld"};
    messages[static_cast<int>(DebugMsgId::STRING_LENGTH_UTF8)] = {
        "String length (UTF-8): %lld", "文字列長 (UTF-8): %lld"};
    messages[static_cast<int>(DebugMsgId::STRING_ELEMENT_VALUE)] = {
        "String element value: %lld", "文字列要素値: %lld"};
    messages[static_cast<int>(DebugMsgId::STRING_ASSIGN_READABLE)] = {
        "String assign: %s = \"%s\"", "文字列代入: %s = \"%s\""};
    messages[static_cast<int>(DebugMsgId::STRING_VAR_CREATE_NEW)] = {
        "Creating new string variable", "新しい文字列変数を作成"};

    // Error messages
    messages[static_cast<int>(DebugMsgId::UNKNOWN_BINARY_OP_ERROR)] = {
        "Unknown binary operator: %s", "不明な二項演算子: %s"};
    messages[static_cast<int>(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR)] = {
        "Unsupported expression node type", "サポートされていない式ノード型"};

    // 不足している重要なメッセージを追加
    messages[static_cast<int>(DebugMsgId::VAR_DECLARATION_DEBUG)] = {
        "Variable declaration: %s", "変数宣言: %s"};
    messages[static_cast<int>(DebugMsgId::UNARY_OP_DEBUG)] = {
        "Unary operation: %s", "単項演算: %s"};
    messages[static_cast<int>(DebugMsgId::UNARY_OP_RESULT_DEBUG)] = {
        "Unary op result: %lld", "単項演算結果: %lld"};
    messages[static_cast<int>(DebugMsgId::EXISTING_VAR_ASSIGN_DEBUG)] = {
        "Assigning to existing variable: %s", "既存変数への代入: %s"};
    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER)] = {
        "Registering function: %s", "関数登録: %s"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_FOUND)] = {
        "Main function found", "main関数発見"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_EXIT)] = {
        "Main function exit", "main関数終了"};
    messages[static_cast<int>(DebugMsgId::INTERPRETER_START)] = {
        "Interpreter start", "インタープリター開始"};
    messages[static_cast<int>(DebugMsgId::EXECUTION_COMPLETE)] = {
        "Execution complete", "実行完了"};
    messages[static_cast<int>(DebugMsgId::AST_IS_NULL)] = {"AST is null",
                                                           "ASTがnull"};
    messages[static_cast<int>(DebugMsgId::STRING_LITERAL_DEBUG)] = {
        "String literal: %s", "文字列リテラル: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_DEBUG)] = {
        "Array element assign: %s[%lld] = %lld",
        "配列要素代入: %s[%lld] = %lld"};
    messages[static_cast<int>(DebugMsgId::VARIABLE_NOT_FOUND)] = {
        "Variable not found: %s", "変数が見つかりません: %s"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_STMTLIST)] = {
        "Creating statement list node", "文リストノード作成"};
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_TYPESPEC)] = {
        "Creating type spec node", "型指定ノード作成"};

    // 関数関連のメッセージ
    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE)] = {
        "Function registration complete: %s", "関数登録完了: %s"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_START)] = {
        "Parameter list start", "パラメータリスト開始"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_SIZE)] = {
        "Parameter list size: %d", "パラメータリストサイズ: %d"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_COMPLETE)] = {
        "Parameter list complete", "パラメータリスト完了"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_DELETE)] = {
        "Deleting parameter list", "パラメータリスト削除"};
    messages[static_cast<int>(DebugMsgId::PARAM_LIST_NONE)] = {
        "No parameter list", "パラメータリストなし"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_START)] = {
        "Function body start", "関数本体開始"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_EXISTS)] = {
        "Function body exists", "関数本体存在"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_SET_COMPLETE)] = {
        "Function body set complete", "関数本体設定完了"};
    messages[static_cast<int>(DebugMsgId::FUNC_BODY_NONE)] = {
        "No function body", "関数本体なし"};
    messages[static_cast<int>(DebugMsgId::FUNC_DEF_COMPLETE)] = {
        "Function definition complete", "関数定義完了"};

    // 配列関連の詳細メッセージ
    messages[static_cast<int>(DebugMsgId::ARRAY_DECL_DEBUG)] = {
        "Array declaration debug: %s", "配列宣言デバッグ: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_DIMENSIONS_COUNT)] = {
        "Array dimensions count: %d", "配列次元数: %d"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_PROCESSING)] = {
        "Multidimensional array processing", "多次元配列処理"};
    messages[static_cast<int>(DebugMsgId::SINGLE_DIM_ARRAY_PROCESSING)] = {
        "Single dimension array processing", "一次元配列処理"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_DETECTED)] =
        {"Multidimensional array assignment detected", "多次元配列代入検出"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_ACCESS_INFO)] = {
        "Multidimensional array access info", "多次元配列アクセス情報"};
    messages[static_cast<int>(DebugMsgId::FLAT_INDEX_CALCULATED)] = {
        "Flat index calculated: %lld", "フラットインデックス計算: %lld"};
    messages[static_cast<int>(
        DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_COMPLETED)] = {
        "Multidimensional array assignment completed", "多次元配列代入完了"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INFO)] = {"Array info: %s",
                                                          "配列情報: %s"};
    messages[static_cast<int>(DebugMsgId::ARRAY_INDEX_OUT_OF_BOUNDS)] = {
        "Array index out of bounds", "配列インデックス範囲外"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_START)] = {
        "Array element assignment start", "配列要素代入開始"};
    messages[static_cast<int>(DebugMsgId::ARRAY_ELEMENT_ASSIGN_SUCCESS)] = {
        "Array element assignment success", "配列要素代入成功"};
    messages[static_cast<int>(DebugMsgId::MULTIDIM_ARRAY_DECL_INFO)] = {
        "Multidimensional array declaration info", "多次元配列宣言情報"};

    // エラーメッセージ
    messages[static_cast<int>(DebugMsgId::PARSER_ERROR)] = {
        "Parser error: %s", "パーサーエラー: %s"};
    messages[static_cast<int>(DebugMsgId::TYPE_MISMATCH_ERROR)] = {
        "Type mismatch error: %s", "型不一致エラー: %s"};
    messages[static_cast<int>(DebugMsgId::VAR_REDECLARE_ERROR)] = {
        "Variable redeclaration error: %s", "変数再宣言エラー: %s"};
    messages[static_cast<int>(DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR)] = {
        "Negative array size error", "負の配列サイズエラー"};
    messages[static_cast<int>(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED)] = {
        "Dynamic array not supported", "動的配列はサポートされていません"};
    messages[static_cast<int>(DebugMsgId::MAIN_FUNC_NOT_FOUND_ERROR)] = {
        "Main function not found error", "main関数が見つからないエラー"};
    messages[static_cast<int>(DebugMsgId::UNDEFINED_VAR_ERROR)] = {
        "Undefined variable error: %s", "未定義変数エラー: %s"};
    messages[static_cast<int>(DebugMsgId::DIRECT_ARRAY_REF_ERROR)] = {
        "Direct array reference error", "直接配列参照エラー"};
    messages[static_cast<int>(DebugMsgId::UNDEFINED_ARRAY_ERROR)] = {
        "Undefined array error: %s", "未定義配列エラー: %s"};
    messages[static_cast<int>(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR)] = {
        "String index out of bounds error", "文字列インデックス範囲外エラー"};
    messages[static_cast<int>(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR)] = {
        "Array index out of bounds error", "配列インデックス範囲外エラー"};
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
        "Printf offset called", "printfオフセット呼び出し"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARG_LIST_INFO)] = {
        "Printf arg list info: %d args", "printf引数リスト情報: %d個"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARG_PROCESSING)] = {
        "Printf arg processing", "printf引数処理"};
    messages[static_cast<int>(DebugMsgId::PRINTF_ARRAY_REF_DEBUG)] = {
        "Printf array reference debug", "printf配列参照デバッグ"};

    // 他の未設定のメッセージにはデフォルト値を設定
    for (size_t i = 0; i < messages.size(); ++i) {
        if (messages[i].en == nullptr) {
            messages[i] = {"Debug message", "デバッグメッセージ"};
        }
    }

    return messages;
}

// グローバルなアクセス関数
const DebugMessageTemplate &get_debug_message(DebugMsgId msg_id) {
    static const auto messages = init_debug_messages();
    int index = static_cast<int>(msg_id);
    if (index >= 0 && index < static_cast<int>(messages.size())) {
        return messages[index];
    }
    static const DebugMessageTemplate fallback = {"Unknown debug message",
                                                  "不明なデバッグメッセージ"};
    return fallback;
}

// 後方互換性のための配列サイズ定義
const int debug_messages_size = 200;
