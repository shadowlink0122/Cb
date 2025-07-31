
#include <cassert>
#include "src/ast/ast.h"
#include "src/eval/eval.h"

void test_assign_short() {
    // short型: b = -1234
    ASTNode assign(ASTNode::AST_ASSIGN);
    assign.sval = "b";
    assign.type_info = 2; // short
    ASTNode rhs(ASTNode::AST_NUM);
    rhs.type_info = 2;
    rhs.lval64 = -1234;
    assign.rhs = &rhs;
    eval_assign(&assign);
    ASTNode var(ASTNode::AST_VAR);
    var.sval = "b";
    var.type_info = 2;
    int64_t v = eval_var(&var);
    assert(v == -1234);
    assign.rhs = nullptr;
}

extern "C" void test_unit_assign_short() {
    test_assign_short();
}
