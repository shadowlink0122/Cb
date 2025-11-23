#include "debug_parser_messages.h"

namespace DebugMessages {
namespace Parser {

void init_parser_messages(std::vector<DebugMessageTemplate> &messages) {
    // パーサ関連メッセージ
    messages[static_cast<int>(DebugMsgId::PARSER_ERROR)] = {
        "[PARSE_ERROR] Parser error", "[PARSE_ERROR] パーサーエラー"};

    messages[static_cast<int>(DebugMsgId::PARSING_START)] = {
        "[PARSE] Parsing started", "[PARSE] パース開始"};

    messages[static_cast<int>(DebugMsgId::AST_GENERATED)] = {
        "[PARSE] AST generation completed", "[PARSE] AST生成完了"};

    // ノード作成関連
    messages[static_cast<int>(DebugMsgId::NODE_CREATE_STMTLIST)] = {
        "[PARSE_NODE] Creating statement list node",
        "[PARSE_NODE] ステートメントリストノード作成"};

    messages[static_cast<int>(DebugMsgId::NODE_CREATE_TYPESPEC)] = {
        "[PARSE_NODE] Creating type specification node",
        "[PARSE_NODE] 型指定ノード作成"};

    messages[static_cast<int>(DebugMsgId::NODE_CREATE_VAR_DECL)] = {
        "[PARSE_NODE] Creating variable declaration node",
        "[PARSE_NODE] 変数宣言ノード作成"};

    messages[static_cast<int>(DebugMsgId::NODE_CREATE_ASSIGN)] = {
        "[PARSE_NODE] Creating assignment node", "[PARSE_NODE] 代入ノード作成"};

    messages[static_cast<int>(DebugMsgId::NODE_CREATE_ARRAY_DECL)] = {
        "[PARSE_NODE] Creating array declaration node",
        "[PARSE_NODE] 配列宣言ノード作成"};

    messages[static_cast<int>(DebugMsgId::NODE_CREATE_FUNC_DECL)] = {
        "[PARSE_NODE] Creating function declaration node",
        "[PARSE_NODE] 関数宣言ノード作成"};

    // 関数定義関連
    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER)] = {
        "[PARSE_FUNC] Registering function: %s", "[PARSE_FUNC] 関数登録: %s"};

    messages[static_cast<int>(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE)] = {
        "[PARSE_FUNC] Function registration complete",
        "[PARSE_FUNC] 関数登録完了"};

    messages[static_cast<int>(DebugMsgId::PARAM_LIST_START)] = {
        "[PARSE_FUNC] Processing parameter list",
        "[PARSE_FUNC] パラメータリスト処理中"};

    messages[static_cast<int>(DebugMsgId::PARAM_LIST_SIZE)] = {
        "[PARSE_FUNC] Parameter count: %d", "[PARSE_FUNC] パラメータ数: %d"};

    messages[static_cast<int>(DebugMsgId::PARAM_LIST_COMPLETE)] = {
        "[PARSE_FUNC] Parameter list processing complete",
        "[PARSE_FUNC] パラメータリスト処理完了"};

    messages[static_cast<int>(DebugMsgId::PARAM_LIST_DELETE)] = {
        "[PARSE_FUNC] Deleting parameter list",
        "[PARSE_FUNC] パラメータリスト削除"};

    messages[static_cast<int>(DebugMsgId::PARAM_LIST_NONE)] = {
        "[PARSE_FUNC] No parameters", "[PARSE_FUNC] パラメータなし"};

    messages[static_cast<int>(DebugMsgId::FUNC_BODY_START)] = {
        "[PARSE_FUNC] Processing function body", "[PARSE_FUNC] 関数本体処理中"};

    messages[static_cast<int>(DebugMsgId::FUNC_BODY_EXISTS)] = {
        "[PARSE_FUNC] Function body exists", "[PARSE_FUNC] 関数本体あり"};

    messages[static_cast<int>(DebugMsgId::FUNC_BODY_SET_COMPLETE)] = {
        "[PARSE_FUNC] Function body set complete",
        "[PARSE_FUNC] 関数本体設定完了"};

    messages[static_cast<int>(DebugMsgId::FUNC_BODY_NONE)] = {
        "[PARSE_FUNC] No function body", "[PARSE_FUNC] 関数本体なし"};

    messages[static_cast<int>(DebugMsgId::FUNC_DEF_COMPLETE)] = {
        "[PARSE_FUNC] Function definition complete",
        "[PARSE_FUNC] 関数定義完了"};
}

} // namespace Parser
} // namespace DebugMessages
