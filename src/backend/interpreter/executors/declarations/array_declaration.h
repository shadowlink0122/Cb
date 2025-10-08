#pragma once
#include "../../../../common/ast.h"

// Forward declarations
class Interpreter;
class StatementExecutor;

namespace DeclarationHandlers {

// 配列宣言を処理するヘルパー関数
void execute_array_decl(StatementExecutor *executor, Interpreter &interpreter,
                        const ASTNode *node);

void execute_struct_array_literal_init(Interpreter &interpreter,
                                       const std::string &array_name,
                                       const ASTNode *array_literal,
                                       const std::string &struct_type);

} // namespace DeclarationHandlers
