
#include <cassert>
#include "src/ast/ast.h"
#include "src/eval/eval.h"

void test_assign_tiny() {
    // tiny型: a = 42
    ASTNode assign(ASTNode::AST_ASSIGN);
    assign.sval = "a";
    assign.type_info = 1; // tiny
    ASTNode rhs(ASTNode::AST_NUM);
    rhs.type_info = 1;
    rhs.lval64 = 42;
    assign.rhs = &rhs;
    eval_assign(&assign);
    ASTNode var(ASTNode::AST_VAR);
    var.sval = "a";
    var.type_info = 1;
    int64_t v = eval_var(&var);
    assert(v == 42);
    assign.rhs = nullptr;
}

extern "C" void test_unit_assign_tiny() {
    test_assign_tiny();
}
