#include "impl.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../core/interpreter.h"

ImplDeclarationHandler::ImplDeclarationHandler(Interpreter *interpreter) {
    (void)interpreter; // 未使用パラメータの警告抑制
}

void ImplDeclarationHandler::handle_impl_declaration(const ASTNode *node) {
    // impl宣言は register_global_declarations() で既に処理済み
    // 実行時には何もしない
    debug_msg(DebugMsgId::PARSE_STRUCT_DEF,
              "Skipping impl declaration in execute_statement (already "
              "registered)");
}
