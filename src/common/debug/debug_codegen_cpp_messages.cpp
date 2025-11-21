#include "debug_codegen_cpp_messages.h"

namespace DebugMessages {
namespace CodegenCpp {

void init_codegen_cpp_messages(std::vector<DebugMessageTemplate> &messages) {
    // C++コード生成全般
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_START)] = {
        "[CODEGEN_CPP] === C++ Code Generation Started ===",
        "[CODEGEN_CPP] === C++コード生成開始 ==="};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_COMPLETE)] = {
        "[CODEGEN_CPP] === C++ Code Generation Completed (lines: %d) ===",
        "[CODEGEN_CPP] === C++コード生成完了 (行数: %d) ==="};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_PROGRAM)] = {
        "[CODEGEN_CPP] Generating program (structs: %d, functions: %d)",
        "[CODEGEN_CPP] プログラム生成中 (構造体: %d, 関数: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_HEADER)] = {
        "[CODEGEN_CPP] Generating C++ header and includes",
        "[CODEGEN_CPP] C++ヘッダーとインクルード生成中"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_IMPORTS)] = {
        "[CODEGEN_CPP] Generating imports (%d items)",
        "[CODEGEN_CPP] インポート生成中 (%d個)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_TYPEDEFS)] = {
        "[CODEGEN_CPP] Generating typedefs (%d items)",
        "[CODEGEN_CPP] typedef生成中 (%d個)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_FORWARD_DECL)] = {
        "[CODEGEN_CPP] Generating forward declarations",
        "[CODEGEN_CPP] 前方宣言生成中"};

    // 構造体生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STRUCT_START)] = {
        "[CODEGEN_CPP_STRUCT] Generating struct: %s (fields: %d)",
        "[CODEGEN_CPP_STRUCT] 構造体生成中: %s (フィールド: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STRUCT_COMPLETE)] = {
        "[CODEGEN_CPP_STRUCT] ✓ Struct generated: %s",
        "[CODEGEN_CPP_STRUCT] ✓ 構造体生成完了: %s"};

    // Enum生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_ENUM_START)] = {
        "[CODEGEN_CPP_ENUM] Generating enum: %s (values: %d)",
        "[CODEGEN_CPP_ENUM] enum生成中: %s (値: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_ENUM_COMPLETE)] = {
        "[CODEGEN_CPP_ENUM] ✓ Enum generated: %s",
        "[CODEGEN_CPP_ENUM] ✓ enum生成完了: %s"};

    // Union生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_UNION_START)] = {
        "[CODEGEN_CPP_UNION] Generating union: %s (variants: %d)",
        "[CODEGEN_CPP_UNION] union生成中: %s (バリアント: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_UNION_COMPLETE)] = {
        "[CODEGEN_CPP_UNION] ✓ Union generated: %s",
        "[CODEGEN_CPP_UNION] ✓ union生成完了: %s"};

    // Interface生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_INTERFACE_START)] = {
        "[CODEGEN_CPP_IFACE] Generating interface: %s (methods: %d)",
        "[CODEGEN_CPP_IFACE] interface生成中: %s (メソッド: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_INTERFACE_COMPLETE)] = {
        "[CODEGEN_CPP_IFACE] ✓ Interface generated: %s",
        "[CODEGEN_CPP_IFACE] ✓ interface生成完了: %s"};

    // 関数生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_FUNCTION_START)] = {
        "[CODEGEN_CPP_FUNC] Generating function: %s (params: %d)",
        "[CODEGEN_CPP_FUNC] 関数生成中: %s (パラメータ: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_FUNCTION_COMPLETE)] = {
        "[CODEGEN_CPP_FUNC] ✓ Function generated: %s (statements: %d)",
        "[CODEGEN_CPP_FUNC] ✓ 関数生成完了: %s (文: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_FUNCTION_SIGNATURE)] = {
        "[CODEGEN_CPP_FUNC]   Signature: %s",
        "[CODEGEN_CPP_FUNC]   シグネチャ: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_FUNCTION_BODY)] = {
        "[CODEGEN_CPP_FUNC]   Generating function body...",
        "[CODEGEN_CPP_FUNC]   関数本体生成中..."};

    // Impl生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_IMPL_START)] = {
        "[CODEGEN_CPP_IMPL] Generating impl for: %s (methods: %d)",
        "[CODEGEN_CPP_IMPL] impl生成中: %s (メソッド: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_IMPL_COMPLETE)] = {
        "[CODEGEN_CPP_IMPL] ✓ Impl generated: %s",
        "[CODEGEN_CPP_IMPL] ✓ impl生成完了: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_IMPL_METHOD)] = {
        "[CODEGEN_CPP_IMPL]   Method: %s", "[CODEGEN_CPP_IMPL]   メソッド: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_GLOBAL_VAR)] = {
        "[CODEGEN_CPP_VAR] Generating global variable: %s (type: %s)",
        "[CODEGEN_CPP_VAR] グローバル変数生成中: %s (型: %s)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_FFI_FUNC)] = {
        "[CODEGEN_CPP_FFI] Generating FFI function: %s",
        "[CODEGEN_CPP_FFI] FFI関数生成中: %s"};

    // ステートメント生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_START)] = {
        "[CODEGEN_CPP_STMT] Generating statement: %s",
        "[CODEGEN_CPP_STMT] ステートメント生成中: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_VAR_DECL)] = {
        "[CODEGEN_CPP_STMT]   Variable declaration: %s %s",
        "[CODEGEN_CPP_STMT]   変数宣言: %s %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_ASSIGNMENT)] = {
        "[CODEGEN_CPP_STMT]   Assignment: %s = ...",
        "[CODEGEN_CPP_STMT]   代入: %s = ..."};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_IF)] = {
        "[CODEGEN_CPP_STMT]   If statement (has else: %s)",
        "[CODEGEN_CPP_STMT]   if文 (else有: %s)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_WHILE)] = {
        "[CODEGEN_CPP_STMT]   While loop", "[CODEGEN_CPP_STMT]   whileループ"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_FOR)] = {
        "[CODEGEN_CPP_STMT]   For loop", "[CODEGEN_CPP_STMT]   forループ"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_RETURN)] = {
        "[CODEGEN_CPP_STMT]   Return statement (has value: %s)",
        "[CODEGEN_CPP_STMT]   return文 (値有: %s)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_BLOCK)] = {
        "[CODEGEN_CPP_STMT]   Block (%d statements)",
        "[CODEGEN_CPP_STMT]   ブロック (%d文)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_SWITCH)] = {
        "[CODEGEN_CPP_STMT]   Switch statement (%d cases)",
        "[CODEGEN_CPP_STMT]   switch文 (%dケース)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_DEFER)] = {
        "[CODEGEN_CPP_STMT]   Defer statement", "[CODEGEN_CPP_STMT]   defer文"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_DELETE)] = {
        "[CODEGEN_CPP_STMT]   Delete statement: %s",
        "[CODEGEN_CPP_STMT]   delete文: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_TRY_CATCH)] = {
        "[CODEGEN_CPP_STMT]   Try-catch statement",
        "[CODEGEN_CPP_STMT]   try-catch文"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_STMT_ASSERT)] = {
        "[CODEGEN_CPP_STMT]   Assert statement",
        "[CODEGEN_CPP_STMT]   assert文"};

    // 式生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_START)] = {
        "[CODEGEN_CPP_EXPR] Generating expression: %s",
        "[CODEGEN_CPP_EXPR] 式生成中: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_LITERAL)] = {
        "[CODEGEN_CPP_EXPR]   Literal: %s",
        "[CODEGEN_CPP_EXPR]   リテラル: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_VARIABLE)] = {
        "[CODEGEN_CPP_EXPR]   Variable: %s", "[CODEGEN_CPP_EXPR]   変数: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_BINARY_OP)] = {
        "[CODEGEN_CPP_EXPR]   Binary op: %s",
        "[CODEGEN_CPP_EXPR]   二項演算: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_UNARY_OP)] = {
        "[CODEGEN_CPP_EXPR]   Unary op: %s",
        "[CODEGEN_CPP_EXPR]   単項演算: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_FUNC_CALL)] = {
        "[CODEGEN_CPP_EXPR]   Function call: %s (args: %d)",
        "[CODEGEN_CPP_EXPR]   関数呼び出し: %s (引数: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_METHOD_CALL)] = {
        "[CODEGEN_CPP_EXPR]   Method call: %s.%s (args: %d)",
        "[CODEGEN_CPP_EXPR]   メソッド呼び出し: %s.%s (引数: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_MEMBER_ACCESS)] = {
        "[CODEGEN_CPP_EXPR]   Member access: .%s",
        "[CODEGEN_CPP_EXPR]   メンバアクセス: .%s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_ARRAY_ACCESS)] = {
        "[CODEGEN_CPP_EXPR]   Array access: [%s]",
        "[CODEGEN_CPP_EXPR]   配列アクセス: [%s]"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_CAST)] = {
        "[CODEGEN_CPP_EXPR]   Cast: (%s)",
        "[CODEGEN_CPP_EXPR]   キャスト: (%s)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_TERNARY)] = {
        "[CODEGEN_CPP_EXPR]   Ternary: ? :",
        "[CODEGEN_CPP_EXPR]   三項演算: ? :"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_LAMBDA)] = {
        "[CODEGEN_CPP_EXPR]   Lambda (params: %d)",
        "[CODEGEN_CPP_EXPR]   ラムダ式 (引数: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_STRUCT_LITERAL)] = {
        "[CODEGEN_CPP_EXPR]   Struct literal: %s (fields: %d)",
        "[CODEGEN_CPP_EXPR]   構造体リテラル: %s (フィールド: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_ARRAY_LITERAL)] = {
        "[CODEGEN_CPP_EXPR]   Array literal (elements: %d)",
        "[CODEGEN_CPP_EXPR]   配列リテラル (要素: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_NEW)] = {
        "[CODEGEN_CPP_EXPR]   New expression: new %s",
        "[CODEGEN_CPP_EXPR]   new式: new %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_EXPR_AWAIT)] = {
        "[CODEGEN_CPP_EXPR]   Await expression",
        "[CODEGEN_CPP_EXPR]   await式"};

    // ポインタ型生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_TYPE_START)] = {
        "[CODEGEN_CPP_PTR] Generating pointer type for: %s",
        "[CODEGEN_CPP_PTR] ポインタ型生成中: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_TYPE)] = {
        "[CODEGEN_CPP_PTR]   Generated pointer type: %s*",
        "[CODEGEN_CPP_PTR]   ポインタ型生成: %s*"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_CONST)] = {
        "[CODEGEN_CPP_PTR]   Const pointer: %s* const",
        "[CODEGEN_CPP_PTR]   constポインタ: %s* const"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_TO_CONST)] = {
        "[CODEGEN_CPP_PTR]   Pointer to const: const %s*",
        "[CODEGEN_CPP_PTR]   constへのポインタ: const %s*"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_ADDRESS_OF)] = {
        "[CODEGEN_CPP_PTR]   Address-of: &%s",
        "[CODEGEN_CPP_PTR]   アドレス演算子: &%s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_DEREF)] = {
        "[CODEGEN_CPP_PTR]   Dereference: *%s",
        "[CODEGEN_CPP_PTR]   デリファレンス: *%s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_ARROW)] = {
        "[CODEGEN_CPP_PTR]   Arrow operator: %s->%s",
        "[CODEGEN_CPP_PTR]   アロー演算子: %s->%s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_NULL)] = {
        "[CODEGEN_CPP_PTR]   Nullptr literal",
        "[CODEGEN_CPP_PTR]   nullptrリテラル"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_CAST)] = {
        "[CODEGEN_CPP_PTR]   Pointer cast: (%s*)%s",
        "[CODEGEN_CPP_PTR]   ポインタキャスト: (%s*)%s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_POINTER_ARITHMETIC)] = {
        "[CODEGEN_CPP_PTR]   Pointer arithmetic: %s %s %s",
        "[CODEGEN_CPP_PTR]   ポインタ算術演算: %s %s %s"};

    // 参照型生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_REFERENCE_TYPE)] = {
        "[CODEGEN_CPP_REF]   Reference type: %s&",
        "[CODEGEN_CPP_REF]   参照型: %s&"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_RVALUE_REF_TYPE)] = {
        "[CODEGEN_CPP_REF]   Rvalue reference type: %s&&",
        "[CODEGEN_CPP_REF]   右辺値参照型: %s&&"};

    // 型生成
    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_TYPE_START)] = {
        "[CODEGEN_CPP_TYPE] Generating type: %s",
        "[CODEGEN_CPP_TYPE] 型生成中: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_TYPE_BASIC)] = {
        "[CODEGEN_CPP_TYPE]   Basic type: %s",
        "[CODEGEN_CPP_TYPE]   基本型: %s"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_TYPE_ARRAY)] = {
        "[CODEGEN_CPP_TYPE]   Array type: %s[%d]",
        "[CODEGEN_CPP_TYPE]   配列型: %s[%d]"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_TYPE_FUNCTION)] = {
        "[CODEGEN_CPP_TYPE]   Function type (params: %d)",
        "[CODEGEN_CPP_TYPE]   関数型 (引数: %d)"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_TYPE_GENERIC)] = {
        "[CODEGEN_CPP_TYPE]   Generic type: %s<%s>",
        "[CODEGEN_CPP_TYPE]   ジェネリック型: %s<%s>"};

    messages[static_cast<int>(DebugMsgId::CODEGEN_CPP_TYPE_COMPLETE)] = {
        "[CODEGEN_CPP_TYPE] ✓ Type generated: %s",
        "[CODEGEN_CPP_TYPE] ✓ 型生成完了: %s"};

    // ポインタ実装デバッグ
    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_ALLOC)] = {
        "[POINTER_IMPL] Allocating memory for pointer: %s (size: %d bytes)",
        "[POINTER_IMPL] ポインタメモリ割り当て: %s (サイズ: %dバイト)"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_FREE)] = {
        "[POINTER_IMPL] Freeing pointer: %s (address: %p)",
        "[POINTER_IMPL] ポインタメモリ解放: %s (アドレス: %p)"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_COPY)] = {
        "[POINTER_IMPL] Copying pointer: %s -> %s",
        "[POINTER_IMPL] ポインタコピー: %s -> %s"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_ASSIGN)] = {
        "[POINTER_IMPL] Assigning pointer: %s = %s (address: %p)",
        "[POINTER_IMPL] ポインタ代入: %s = %s (アドレス: %p)"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_COMPARE)] = {
        "[POINTER_IMPL] Comparing pointers: %s %s %s",
        "[POINTER_IMPL] ポインタ比較: %s %s %s"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_NULL_CHECK)] = {
        "[POINTER_IMPL] NULL check for pointer: %s (is null: %s)",
        "[POINTER_IMPL] ポインタNULLチェック: %s (NULL: %s)"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_DEREF_CHECK)] = {
        "[POINTER_IMPL] Pre-dereference check: %s (safe: %s)",
        "[POINTER_IMPL] デリファレンス前チェック: %s (安全: %s)"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_BOUNDS_CHECK)] = {
        "[POINTER_IMPL] Bounds check: %s[%d] (valid: %s)",
        "[POINTER_IMPL] ポインタ境界チェック: %s[%d] (有効: %s)"};

    messages[static_cast<int>(DebugMsgId::POINTER_IMPL_TYPE_MISMATCH)] = {
        "[POINTER_IMPL] ⚠ Type mismatch: expected %s*, got %s*",
        "[POINTER_IMPL] ⚠ 型不一致: 期待 %s*, 実際 %s*"};
}

} // namespace CodegenCpp
} // namespace DebugMessages
