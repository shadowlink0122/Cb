#include "eval.h"

#include <cstdint>
#include <map>
#include <string>

struct Variable {
    int type;      // 1=tiny, 2=short, 3=int, 4=long
    int64_t value; // 値は常にint64_tで保持
};
static std::map<std::string, Variable> symbol_table;

void yyerror(const char *s, const char *error);

// 型情報: 1=tiny(int8_t), 2=short(int16_t), 3=int(int32_t), 4=long(int64_t)
int64_t eval_num(ASTNode *node) {
    // 型ごとのキャストはeval_assignで行う。ここでは常にlval64を返す。
    return node->lval64;
}

int64_t eval_var(ASTNode *node) {
    // シンボルテーブルから変数を取得
    auto it = symbol_table.find(node->sval);
    if (it == symbol_table.end()) {
        yyerror("未定義の変数です", node->sval.c_str());
        return 0;
    }
    const Variable &var = it->second;
    // 型に応じてキャストして返す
    switch (var.type) {
    case 1:
        return (int8_t)var.value;
    case 2:
        return (int16_t)var.value;
    case 3:
        return (int32_t)var.value;
    case 4:
        return (int64_t)var.value;
    default:
        return (int32_t)var.value;
    }
}

int64_t eval_binop(ASTNode *node) {
    int64_t l = eval(node->lhs);
    int64_t r = eval(node->rhs);
    // 型情報の大きい方を結果ノードに伝播
    int ltype = node->lhs ? node->lhs->type_info : 3;
    int rtype = node->rhs ? node->rhs->type_info : 3;
    int result_type = (ltype > rtype) ? ltype : rtype;
    node->type_info = result_type;
    int64_t result = 0;
    if (node->op == "+")
        result = l + r;
    else if (node->op == "-")
        result = l - r;
    else if (node->op == "*")
        result = l * r;
    else if (node->op == "/") {
        if (r == 0) {
            yyerror("Error", "0除算が発生しました");
        } else {
            result = l / r;
        }
    }
    // 結果はint64_tで返す
    return result;
}

int64_t eval_assign(ASTNode *node) {
    ASTNode *rhs = node->rhs;
    // 左辺の型情報を右辺に伝播
    rhs->type_info = node->type_info;
    int64_t value = eval(rhs);
    // 型ごとに範囲チェックし、範囲外ならエラー
    Variable var;
    var.type = node->type_info;
    bool out_of_range = false;
    switch (node->type_info) {
    case 1: // tiny(int8_t)
        if (value < -128 || value > 127)
            out_of_range = true;
        var.value = (int8_t)value;
        break;
    case 2: // short(int16_t)
        if (value < -32768 || value > 32767)
            out_of_range = true;
        var.value = (int16_t)value;
        break;
    case 3: // int(int32_t)
        if (value < -2147483648LL || value > 2147483647LL)
            out_of_range = true;
        var.value = (int32_t)value;
        break;
    case 4: // long(int64_t)
        // int64_tの範囲は十分広いのでチェック不要
        var.value = (int64_t)value;
        break;
    default:
        var.value = (int32_t)value;
        break;
    }
    if (out_of_range) {
        fprintf(stderr, "DEBUG: out_of_range for %s value=%lld type=%d\n",
                node->sval.c_str(), value, node->type_info);
        yyerror("型の範囲外の値を代入しようとしました", node->sval.c_str());
    }
    symbol_table[node->sval] = var;
    return var.value;
}

int eval_print(ASTNode *node) {
    int64_t value = eval(node->lhs);
    printf("%lld\n", value);
    return 0;
}

int eval_stmtlist(ASTNode *node) {
    for (std::vector<ASTNode *>::iterator it = node->stmts.begin();
         it != node->stmts.end(); ++it)
        eval(*it);
    return 0;
}

int64_t eval(ASTNode *node) {
    if (!node)
        return 0;
    switch (node->type) {
    case ASTNode::AST_NUM:
        return eval_num(node);
    case ASTNode::AST_VAR:
        return eval_var(node);
    case ASTNode::AST_BINOP:
        return eval_binop(node);
    case ASTNode::AST_ASSIGN:
        return eval_assign(node);
    case ASTNode::AST_PRINT:
        return eval_print(node);
    case ASTNode::AST_STMTLIST:
        return eval_stmtlist(node);
    }
    return 0;
}
