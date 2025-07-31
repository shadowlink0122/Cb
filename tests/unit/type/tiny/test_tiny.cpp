
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include "src/eval/eval.h"

void test_unit_tiny() {
    // tiny型の範囲内（AST直接テスト）
    ASTNode n1; n1.type_info = 1; n1.lval64 = 127; n1.type = ASTNode::AST_NUM; n1.lhs = nullptr; n1.rhs = nullptr;
    assert(eval_num(&n1) == 127);
    ASTNode n2; n2.type_info = 1; n2.lval64 = -128; n2.type = ASTNode::AST_NUM; n2.lhs = nullptr; n2.rhs = nullptr;
    assert(eval_num(&n2) == -128);


    // 異常系（範囲外リテラル）: eval_numでexit(1)が呼ばれることを検証
    {
        if (fork() == 0) {
            FILE* tmp = tmpfile();
            if (tmp) {
                int fd = fileno(tmp);
                dup2(fd, STDERR_FILENO);
            }
            ASTNode n3; n3.type_info = 1; n3.lval64 = 128; n3.type = ASTNode::AST_NUM; n3.lhs = nullptr; n3.rhs = nullptr;
            eval_num(&n3);
            _exit(0); // ここに来たら失敗
        } else {
            int status = 0;
            wait(&status);
            assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);
        }
    }

    // 異常系（範囲外代入）: eval_assignでexit(1)が呼ばれることを検証
    {
        if (fork() == 0) {
            FILE* tmp = tmpfile();
            if (tmp) {
                int fd = fileno(tmp);
                dup2(fd, STDERR_FILENO);
            }
            ASTNode* lhs = new ASTNode(); lhs->type_info = 1; lhs->type = ASTNode::AST_VAR; lhs->sval = "a";
            ASTNode* rhs = new ASTNode(); rhs->type_info = 1; rhs->type = ASTNode::AST_NUM; rhs->lval64 = 200;
            ASTNode* assign = new ASTNode(); assign->type = ASTNode::AST_ASSIGN; assign->sval = "a"; assign->lhs = lhs; assign->rhs = rhs; assign->type_info = 1;
            eval_assign(assign);
            delete assign;
            _exit(0);
        } else {
            int status = 0;
            wait(&status);
            assert(WIFEXITED(status) && WEXITSTATUS(status) == 1);
        }
    }

    std::cout << "[unit] tiny test passed\n";
}
