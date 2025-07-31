
#include <cassert>
#include <iostream>
#include <sstream>
#include <cstdio>
#include "src/eval/eval.h"

// printの出力を捕捉するためにstdoutを一時的に差し替える
std::string capture_eval_print(ASTNode* node) {
    fflush(stdout);
    FILE* old = stdout;
    FILE* temp = tmpfile();
    stdout = temp;
    eval_print(node);
    fflush(stdout);
    fseek(temp, 0, SEEK_SET);
    char buf[256];
    std::string out;
    while (fgets(buf, sizeof(buf), temp)) out += buf;
    fclose(temp);
    stdout = old;
    return out;
}

void test_unit_func() {
    // 関数: short f1() { return 10 + 10; }
    ASTNode* f1 = new ASTNode(ASTNode::AST_FUNCDEF);
    f1->sval = "f1";
    f1->rettypes.push_back(new ASTNode(ASTNode::AST_NUM));
    f1->rettypes[0]->type_info = 2; // short
    ASTNode* f1_body = new ASTNode(ASTNode::AST_RETURN);
    ASTNode* f1_ret = new ASTNode(ASTNode::AST_BINOP);
    f1_ret->op = "+";
    f1_ret->lhs = new ASTNode(ASTNode::AST_NUM); f1_ret->lhs->lval64 = 10; f1_ret->lhs->type_info = 2;
    f1_ret->rhs = new ASTNode(ASTNode::AST_NUM); f1_ret->rhs->lval64 = 10; f1_ret->rhs->type_info = 2;
    f1_body->lhs = f1_ret;
    f1->body = f1_body;
    eval(f1);

    // 関数: short f2() { return 10 + 20; }
    ASTNode* f2 = new ASTNode(ASTNode::AST_FUNCDEF);
    f2->sval = "f2";
    f2->rettypes.push_back(new ASTNode(ASTNode::AST_NUM));
    f2->rettypes[0]->type_info = 2; // short
    ASTNode* f2_body = new ASTNode(ASTNode::AST_RETURN);
    ASTNode* f2_ret = new ASTNode(ASTNode::AST_BINOP);
    f2_ret->op = "+";
    f2_ret->lhs = new ASTNode(ASTNode::AST_NUM); f2_ret->lhs->lval64 = 10; f2_ret->lhs->type_info = 2;
    f2_ret->rhs = new ASTNode(ASTNode::AST_NUM); f2_ret->rhs->lval64 = 20; f2_ret->rhs->type_info = 2;
    f2_body->lhs = f2_ret;
    f2->body = f2_body;
    eval(f2);

    // 関数: void v1() { print f1(); print f2(); }
    ASTNode* v1 = new ASTNode(ASTNode::AST_FUNCDEF);
    v1->sval = "v1";
    v1->rettypes.push_back(new ASTNode(ASTNode::AST_NUM));
    v1->rettypes[0]->type_info = 0; // void
    ASTNode* v1_body = new ASTNode(ASTNode::AST_STMTLIST);
    // print f1();
    ASTNode* call_f1 = new ASTNode(ASTNode::AST_FUNCCALL); call_f1->sval = "f1";
    ASTNode* print1 = new ASTNode(ASTNode::AST_PRINT); print1->lhs = call_f1;
    // print f2();
    ASTNode* call_f2 = new ASTNode(ASTNode::AST_FUNCCALL); call_f2->sval = "f2";
    ASTNode* print2 = new ASTNode(ASTNode::AST_PRINT); print2->lhs = call_f2;
    v1_body->stmts.push_back(print1);
    v1_body->stmts.push_back(print2);
    v1->body = v1_body;
    eval(v1);

    // main関数: print 1; v1();
    ASTNode* mainf = new ASTNode(ASTNode::AST_FUNCDEF);
    mainf->sval = "main";
    mainf->rettypes.push_back(new ASTNode(ASTNode::AST_NUM));
    mainf->rettypes[0]->type_info = 3; // int
    ASTNode* main_body = new ASTNode(ASTNode::AST_STMTLIST);
    ASTNode* print_num = new ASTNode(ASTNode::AST_PRINT); print_num->lhs = new ASTNode(ASTNode::AST_NUM); print_num->lhs->lval64 = 1; print_num->lhs->type_info = 3;
    ASTNode* call_v1 = new ASTNode(ASTNode::AST_FUNCCALL); call_v1->sval = "v1";
    main_body->stmts.push_back(print_num);
    main_body->stmts.push_back(call_v1);
    mainf->body = main_body;
    eval(mainf);

    // main呼び出しノード
    ASTNode* main_call = new ASTNode(ASTNode::AST_FUNCCALL); main_call->sval = "main";
    // print出力を捕捉
    std::string out1 = capture_eval_print(print_num);
    assert(out1 == "1\n");
    // v1()のprint出力を捕捉
    ASTNode* call_f1_2 = new ASTNode(ASTNode::AST_FUNCCALL); call_f1_2->sval = "f1";
    ASTNode* print_f1 = new ASTNode(ASTNode::AST_PRINT); print_f1->lhs = call_f1_2;
    std::string out2 = capture_eval_print(print_f1);
    assert(out2 == "20\n");
    ASTNode* call_f2_2 = new ASTNode(ASTNode::AST_FUNCCALL); call_f2_2->sval = "f2";
    ASTNode* print_f2 = new ASTNode(ASTNode::AST_PRINT); print_f2->lhs = call_f2_2;
    std::string out3 = capture_eval_print(print_f2);
    assert(out3 == "30\n");

    std::cout << "[unit] func test passed\n";
}
