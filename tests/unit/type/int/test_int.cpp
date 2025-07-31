#include <cassert>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "src/eval/eval.h"

void test_unit_int() {
    // int型の範囲内
    ASTNode n1; n1.type_info = 3; n1.lval64 = 2147483647; n1.type = ASTNode::AST_NUM; n1.lhs = nullptr; n1.rhs = nullptr;
    assert(eval_num(&n1) == 2147483647);
    ASTNode n2; n2.type_info = 3; n2.lval64 = -2147483648LL; n2.type = ASTNode::AST_NUM; n2.lhs = nullptr; n2.rhs = nullptr;
    assert(eval_num(&n2) == -2147483648LL);
    // 範囲外リテラルはexit(1)を期待
    if (fork() == 0) {
        FILE* tmp = tmpfile();
        if (tmp) {
            int fd = fileno(tmp);
            dup2(fd, STDERR_FILENO);
        }
        ASTNode n3; n3.type_info = 3; n3.lval64 = 2147483648LL; n3.type = ASTNode::AST_NUM; n3.lhs = nullptr; n3.rhs = nullptr;
        eval_num(&n3);
        _exit(0);
    } else {
        int status = 0;
        wait(&status);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);
    }
    // 範囲外代入もexit(1)を期待
    if (fork() == 0) {
        FILE* tmp = tmpfile();
        if (tmp) {
            int fd = fileno(tmp);
            dup2(fd, STDERR_FILENO);
        }
        ASTNode* lhs = new ASTNode(); lhs->type_info = 3; lhs->sval = "i"; lhs->type = ASTNode::AST_VAR; lhs->lval64 = 0;
        ASTNode* rhs = new ASTNode(); rhs->type_info = 3; rhs->lval64 = 2147483648LL; rhs->type = ASTNode::AST_NUM;
        ASTNode* assign = new ASTNode(); assign->type_info = 3; assign->lhs = lhs; assign->rhs = rhs; assign->type = ASTNode::AST_ASSIGN; assign->sval = "i";
        eval_assign(assign);
        delete assign;
        _exit(0);
    } else {
        int status = 0;
        wait(&status);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);
    }
    std::cout << "[unit] int test passed\n";
}
