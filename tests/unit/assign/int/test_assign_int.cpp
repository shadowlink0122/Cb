
#include <cassert>
#include "src/ast/ast.h"
#include "src/eval/eval.h"

void test_assign_int() {
    // int型: c = 123456
    ASTNode assign(ASTNode::AST_ASSIGN);
    assign.sval = "c";
    assign.type_info = 3; // int
    ASTNode rhs(ASTNode::AST_NUM);
    rhs.type_info = 3;
    rhs.lval64 = 123456;
    assign.rhs = &rhs;
    eval_assign(&assign);
    ASTNode var(ASTNode::AST_VAR);
    var.sval = "c";
    var.type_info = 3;
    int64_t v = eval_var(&var);
    assert(v == 123456);
    assign.rhs = nullptr;
}

extern "C" void test_unit_assign_int() {
    test_assign_int();
}
