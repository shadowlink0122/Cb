#include "function_declaration_handler.h"
#include "core/interpreter.h"
#include "../../../common/ast.h"

FunctionDeclarationHandler::FunctionDeclarationHandler(Interpreter* interp)
    : interpreter_(interp) {}

void FunctionDeclarationHandler::handle_function_declaration(const ASTNode* node) {
    // 実行時の関数定義をグローバルスコープに登録
    // const_castを使用してmapに格納（関数定義は後で変更される可能性があるため）
    interpreter_->global_scope.functions[node->name] = const_cast<ASTNode*>(node);
}
