#include "handlers/assertion_handler.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "core/interpreter.h"
#include <cstdlib>
#include <iostream>

AssertionHandler::AssertionHandler(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void AssertionHandler::handle_assertion(const ASTNode *node) {
    debug_msg(DebugMsgId::ASSERT_CHECK_START);

    if (!node->left) {
        error_msg(DebugMsgId::ASSERT_FAILURE, node->location.line,
                  "Missing condition");
        std::exit(1);
    }

    // 条件を評価
    int64_t condition = interpreter_->evaluate(node->left.get());

    if (condition) {
        debug_msg(DebugMsgId::ASSERT_CONDITION_TRUE);
    } else {
        debug_msg(DebugMsgId::ASSERT_CONDITION_FALSE, node->location.line);
        error_msg(DebugMsgId::ASSERT_FAILURE, node->location.line,
                  "Assertion failed");
        std::cerr << "Assertion failed at line " << node->location.line
                  << std::endl;
        std::exit(1);
    }
}
