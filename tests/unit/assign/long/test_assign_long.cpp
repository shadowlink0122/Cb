
#include <cassert>
#include "src/ast/ast.h"
#include "src/eval/eval.h"

void test_assign_long() {
    // long long型: d = 9876543210
    ASTNode assign(ASTNode::AST_ASSIGN);
    assign.sval = "d";
    assign.type_info = 4; // long
    ASTNode rhs(ASTNode::AST_NUM);
    rhs.type_info = 4;
    rhs.lval64 = 9876543210LL;
    assign.rhs = &rhs;
    eval_assign(&assign);
    ASTNode var(ASTNode::AST_VAR);
    var.sval = "d";
    var.type_info = 4;
    int64_t v = eval_var(&var);
    assert(v == 9876543210LL);
    assign.rhs = nullptr;
}

extern "C" void test_unit_assign_long() {
    test_assign_long();
}
