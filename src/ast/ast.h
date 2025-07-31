#pragma once
#include <cstdint>
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
    int32_t type_info = 0; // 型情報: 1=tiny, 2=short, 3=int, 4=long
    int64_t lval64 = 0;    // 整数値（常にint64_tで保持）
    std::string sval;
    std::string op;
    ASTNode *lhs, *rhs;
    std::vector<ASTNode *> stmts;
    ASTNode()
        : type(AST_VAR), type_info(0), lval64(0), lhs(nullptr), rhs(nullptr) {}
    ASTNode(Type t)
        : type(t), type_info(0), lval64(0), lhs(nullptr), rhs(nullptr) {}
    ~ASTNode() {
        delete lhs;
        delete rhs;
        for (std::vector<ASTNode *>::iterator it = stmts.begin();
             it != stmts.end(); ++it)
            delete *it;
    }
};

// CbソースファイルからASTを生成する関数
ASTNode *parse_to_ast(const std::string &filename);
