#include "debug_hir_messages.h"

namespace DebugMessages {
namespace HIR {

void init_hir_messages(std::vector<DebugMessageTemplate> &messages) {
    // HIR生成全般
    messages[static_cast<int>(DebugMsgId::HIR_GENERATION_START)] = {
        "[HIR] === HIR Generation Started ===", "[HIR] === HIR生成開始 ==="};

    messages[static_cast<int>(DebugMsgId::HIR_GENERATION_COMPLETE)] = {
        "[HIR] === HIR Generation Completed ===", "[HIR] === HIR生成完了 ==="};

    messages[static_cast<int>(DebugMsgId::HIR_PROCESSING_NODE)] = {
        "[HIR] Processing AST node: %s", "[HIR] ASTノード処理中: %s"};

    // 関数処理
    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_PROCESSING)] = {
        "[HIR_FUNC] Processing function: %s", "[HIR_FUNC] 関数処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_ADDED)] = {
        "[HIR_FUNC] ✓ Function added: %s (params: %d, return: %s)",
        "[HIR_FUNC] ✓ 関数追加: %s (パラメータ: %d, 戻り値: %s)"};

    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_BODY_START)] = {
        "[HIR_FUNC]   Converting function body...",
        "[HIR_FUNC]   関数本体変換中..."};

    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_BODY_COMPLETE)] = {
        "[HIR_FUNC]   Function body converted (statements: %d)",
        "[HIR_FUNC]   関数本体変換完了 (文: %d)"};

    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_PARAM_PROCESSING)] = {
        "[HIR_FUNC]   Processing parameter: %s",
        "[HIR_FUNC]   パラメータ処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_FUNCTION_PARAM_ADDED)] = {
        "[HIR_FUNC]   Parameter added: %s (type: %s)",
        "[HIR_FUNC]   パラメータ追加: %s (型: %s)"};

    // 構造体処理
    messages[static_cast<int>(DebugMsgId::HIR_STRUCT_PROCESSING)] = {
        "[HIR_STRUCT] Processing struct: %s", "[HIR_STRUCT] 構造体処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_STRUCT_ADDED)] = {
        "[HIR_STRUCT] ✓ Struct added: %s (fields: %d)",
        "[HIR_STRUCT] ✓ 構造体追加: %s (フィールド: %d)"};

    messages[static_cast<int>(DebugMsgId::HIR_STRUCT_FIELD_PROCESSING)] = {
        "[HIR_STRUCT]   Processing field: %s",
        "[HIR_STRUCT]   フィールド処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_STRUCT_FIELD_ADDED)] = {
        "[HIR_STRUCT]   Field added: %s (type: %s)",
        "[HIR_STRUCT]   フィールド追加: %s (型: %s)"};

    // 列挙型処理
    messages[static_cast<int>(DebugMsgId::HIR_ENUM_PROCESSING)] = {
        "[HIR_ENUM] Processing enum: %s", "[HIR_ENUM] 列挙型処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_ENUM_ADDED)] = {
        "[HIR_ENUM] ✓ Enum added: %s (values: %d)",
        "[HIR_ENUM] ✓ 列挙型追加: %s (値: %d)"};

    messages[static_cast<int>(DebugMsgId::HIR_ENUM_VALUE_PROCESSING)] = {
        "[HIR_ENUM]   Processing enum value: %s",
        "[HIR_ENUM]   列挙値処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_ENUM_VALUE_ADDED)] = {
        "[HIR_ENUM]   Enum value added: %s = %d",
        "[HIR_ENUM]   列挙値追加: %s = %d"};

    // インターフェース処理
    messages[static_cast<int>(DebugMsgId::HIR_INTERFACE_PROCESSING)] = {
        "[HIR_INTERFACE] Processing interface: %s",
        "[HIR_INTERFACE] インターフェース処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_INTERFACE_ADDED)] = {
        "[HIR_INTERFACE] ✓ Interface added: %s (methods: %d)",
        "[HIR_INTERFACE] ✓ インターフェース追加: %s (メソッド: %d)"};

    messages[static_cast<int>(DebugMsgId::HIR_INTERFACE_METHOD_PROCESSING)] = {
        "[HIR_INTERFACE]   Processing method: %s",
        "[HIR_INTERFACE]   メソッド処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_INTERFACE_METHOD_ADDED)] = {
        "[HIR_INTERFACE]   Method signature added: %s",
        "[HIR_INTERFACE]   メソッドシグネチャ追加: %s"};

    // 実装処理
    messages[static_cast<int>(DebugMsgId::HIR_IMPL_PROCESSING)] = {
        "[HIR_IMPL] Processing impl for: %s", "[HIR_IMPL] 実装処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_IMPL_ADDED)] = {
        "[HIR_IMPL] ✓ Impl added: %s (methods: %d)",
        "[HIR_IMPL] ✓ 実装追加: %s (メソッド: %d)"};

    messages[static_cast<int>(DebugMsgId::HIR_IMPL_METHOD_PROCESSING)] = {
        "[HIR_IMPL]   Processing method: %s",
        "[HIR_IMPL]   メソッド処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_IMPL_METHOD_ADDED)] = {
        "[HIR_IMPL]   Method added: %s", "[HIR_IMPL]   メソッド追加: %s"};

    // グローバル変数処理
    messages[static_cast<int>(DebugMsgId::HIR_GLOBAL_VAR_PROCESSING)] = {
        "[HIR_VAR] Processing global variable: %s",
        "[HIR_VAR] グローバル変数処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_GLOBAL_VAR_ADDED)] = {
        "[HIR_VAR] ✓ Global variable added: %s (type: %s)",
        "[HIR_VAR] ✓ グローバル変数追加: %s (型: %s)"};

    // FFI関数処理
    messages[static_cast<int>(DebugMsgId::HIR_FFI_FUNCTION_PROCESSING)] = {
        "[HIR_FFI] Processing FFI function: %s", "[HIR_FFI] FFI関数処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_FFI_FUNCTION_ADDED)] = {
        "[HIR_FFI] ✓ FFI function added: %s (lib: %s)",
        "[HIR_FFI] ✓ FFI関数追加: %s (ライブラリ: %s)"};

    // ステートメント処理
    messages[static_cast<int>(DebugMsgId::HIR_STATEMENT_PROCESSING)] = {
        "[HIR_STMT] Converting statement: %s",
        "[HIR_STMT] ステートメント変換中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_STATEMENT_CONVERTED)] = {
        "[HIR_STMT] ✓ Statement converted: %s",
        "[HIR_STMT] ✓ ステートメント変換完了: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_STMT_RETURN)] = {
        "[HIR_STMT]   Return statement", "[HIR_STMT]   return文"};

    messages[static_cast<int>(DebugMsgId::HIR_STMT_IF)] = {
        "[HIR_STMT]   If statement", "[HIR_STMT]   if文"};

    messages[static_cast<int>(DebugMsgId::HIR_STMT_WHILE)] = {
        "[HIR_STMT]   While loop", "[HIR_STMT]   while文"};

    messages[static_cast<int>(DebugMsgId::HIR_STMT_FOR)] = {
        "[HIR_STMT]   For loop", "[HIR_STMT]   for文"};

    messages[static_cast<int>(DebugMsgId::HIR_STMT_EXPR)] = {
        "[HIR_STMT]   Expression statement", "[HIR_STMT]   式文"};

    messages[static_cast<int>(DebugMsgId::HIR_STMT_VAR_DECL)] = {
        "[HIR_STMT]   Variable declaration: %s", "[HIR_STMT]   変数宣言: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_STMT_BLOCK)] = {
        "[HIR_STMT]   Block statement (%d statements)",
        "[HIR_STMT]   ブロック文 (%d文)"};

    // 式処理
    messages[static_cast<int>(DebugMsgId::HIR_EXPRESSION_PROCESSING)] = {
        "[HIR_EXPR] Converting expression: %s", "[HIR_EXPR] 式変換中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPRESSION_CONVERTED)] = {
        "[HIR_EXPR] ✓ Expression converted: %s", "[HIR_EXPR] ✓ 式変換完了: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_LITERAL)] = {
        "[HIR_EXPR]   Literal: %s", "[HIR_EXPR]   リテラル: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_VARIABLE)] = {
        "[HIR_EXPR]   Variable reference: %s", "[HIR_EXPR]   変数参照: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_BINARY_OP)] = {
        "[HIR_EXPR]   Binary operation: %s", "[HIR_EXPR]   二項演算: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_UNARY_OP)] = {
        "[HIR_EXPR]   Unary operation: %s", "[HIR_EXPR]   単項演算: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_FUNC_CALL)] = {
        "[HIR_EXPR]   Function call: %s", "[HIR_EXPR]   関数呼び出し: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_MEMBER_ACCESS)] = {
        "[HIR_EXPR]   Member access: .%s", "[HIR_EXPR]   メンバアクセス: .%s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_ARRAY_ACCESS)] = {
        "[HIR_EXPR]   Array access", "[HIR_EXPR]   配列アクセス"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_CAST)] = {
        "[HIR_EXPR]   Type cast: %s", "[HIR_EXPR]   型キャスト: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_EXPR_TERNARY)] = {
        "[HIR_EXPR]   Ternary operator", "[HIR_EXPR]   三項演算子"};

    // 型処理
    messages[static_cast<int>(DebugMsgId::HIR_TYPE_RESOLUTION)] = {
        "[HIR_TYPE] Resolving type: %s", "[HIR_TYPE] 型解決中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_TYPE_RESOLVED)] = {
        "[HIR_TYPE] ✓ Type resolved: %s", "[HIR_TYPE] ✓ 型解決完了: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_TYPE_PRIMITIVE)] = {
        "[HIR_TYPE]   Primitive type: %s", "[HIR_TYPE]   プリミティブ型: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_TYPE_STRUCT)] = {
        "[HIR_TYPE]   Struct type: %s", "[HIR_TYPE]   構造体型: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_TYPE_ARRAY)] = {
        "[HIR_TYPE]   Array type: %s[]", "[HIR_TYPE]   配列型: %s[]"};

    messages[static_cast<int>(DebugMsgId::HIR_TYPE_POINTER)] = {
        "[HIR_TYPE]   Pointer type: %s*", "[HIR_TYPE]   ポインタ型: %s*"};

    messages[static_cast<int>(DebugMsgId::HIR_TYPE_FUNCTION)] = {
        "[HIR_TYPE]   Function type", "[HIR_TYPE]   関数型"};

    // ジェネリック処理
    messages[static_cast<int>(DebugMsgId::HIR_GENERIC_INSTANTIATION)] = {
        "[HIR_GENERIC] Instantiating generic: %s with types: %s",
        "[HIR_GENERIC] ジェネリック具体化: %s 型: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_GENERIC_PARAM_PROCESSING)] = {
        "[HIR_GENERIC] Processing generic parameter: %s",
        "[HIR_GENERIC] ジェネリックパラメータ処理中: %s"};

    messages[static_cast<int>(DebugMsgId::HIR_GENERIC_CONSTRAINT)] = {
        "[HIR_GENERIC] Processing constraint: %s",
        "[HIR_GENERIC] 制約処理中: %s"};
}

} // namespace HIR
} // namespace DebugMessages
