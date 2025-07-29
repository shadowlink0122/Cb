#pragma once
#include <string>
#include <vector>

struct ASTNode {
    enum Type {
        AST_NUM,
        AST_VAR,
        AST_BINOP,
        AST_ASSIGN,
        AST_PRINT,
        AST_STMTLIST
    } type;
    int ival;
    std::string sval;
    std::string op;
    ASTNode *lhs, *rhs;
    std::vector<ASTNode *> stmts;
    ASTNode(Type t) : type(t), ival(0), lhs(nullptr), rhs(nullptr) {}
    ~ASTNode() {
        delete lhs;
        delete rhs;
        for (std::vector<ASTNode *>::iterator it = stmts.begin();
             it != stmts.end(); ++it)
            delete *it;
    }
};
