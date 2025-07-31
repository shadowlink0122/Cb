// 右辺AST全体のtype_infoを再帰的に上書き
#include <vector>
#ifdef __cplusplus
extern "C" {
#endif
extern char *yyfilename;
extern int yylineno;
#ifdef __cplusplus
}
#endif
#include <cstdarg>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>

// デバッグモードフラグ（CB_DEBUG_MODE=1なら有効化）
#include <cstdlib>
bool debug_mode = false;
void set_debug_mode_from_env() {
    const char *env = std::getenv("CB_DEBUG_MODE");
    if (env && env[0] == '1')
        debug_mode = true;
}

#include "eval.h"

#include <cstdint>
#include <map>
#include <string>

struct Variable {
    int type;      // 0=void, 1=tiny, 2=short, 3=int, 4=long
    int64_t value; // 値は常にint64_tで保持
};

std::map<std::string, Variable> symbol_table;
// 関数定義用: 関数名→ASTNode*（関数定義ノード）
std::map<std::string, ASTNode *> function_table;

// va_list対応のデバッグ用printfラッパー
extern "C" void vdebug_printf(const char *fmt, va_list args) {
    if (!debug_mode)
        return;
    vfprintf(stderr, fmt, args);
}

// デバッグ用printfラッパー
void debug_printf(const char *fmt, ...) {
    if (!debug_mode)
        return;
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

// 右辺AST全体のtype_infoを再帰的に上書き
static void propagate_type_info(ASTNode *node, int type_info) {
    if (!node)
        return;
    node->type_info = type_info;
    switch (node->type) {
    case ASTNode::AST_BINOP:
        propagate_type_info(node->lhs, type_info);
        propagate_type_info(node->rhs, type_info);
        break;
    case ASTNode::AST_ASSIGN:
        propagate_type_info(node->rhs, type_info);
        break;
    case ASTNode::AST_PRINT:
        propagate_type_info(node->lhs, type_info);
        break;
    case ASTNode::AST_STMTLIST:
        for (auto *s : node->stmts)
            propagate_type_info(s, type_info);
        break;
    case ASTNode::AST_FUNCCALL:
        for (auto *p : node->params)
            propagate_type_info(p, type_info);
        break;
    case ASTNode::AST_RETURN:
        propagate_type_info(node->lhs, type_info);
        break;
    default:
        break;
    }
}

// 関数呼び出し時のreturn値伝搬用例外
struct ReturnException {
    int64_t value;
    ReturnException(int64_t v) : value(v) {}
};

// 1引数版（bison生成Cコード用）
void yyerror(const char *s) {
    yyerror(s, ""); // exit(1);
}

// 2引数版（C++評価系・型エラー用）
void yyerror(const char *s, const char *error) {
    fprintf(stderr, "%s: %s\n", s, error);
    fflush(stderr);
    if (yyfilename) {
        FILE *fp = fopen(yyfilename, "r");
        if (fp) {
            char buf[1024];
            int line = 1;
            while (fgets(buf, sizeof(buf), fp)) {
                if (line == yylineno) {
                    fprintf(stderr, "%s:%d\n>> %s", yyfilename, line, buf);
                    break;
                }
                line++;
            }
            fclose(fp);
        }
    }
    fflush(stderr);
    exit(1);
}

// 型情報: 0=void, 1=tiny(int8_t), 2=short(int16_t), 3=int(int32_t),
// 4=long(int64_t)
int64_t eval_num(ASTNode *node) {
    // 型ごとに範囲外リテラルは例外を投げる
    int64_t v = node->lval64;
    switch (node->type_info) {
    case 1: // tiny
        if (v < -128 || v > 127)
            yyerror("tiny型の範囲外の値を代入しようとしました", "");
        break;
    case 2: // short
        if (v < -32768 || v > 32767)
            yyerror("short型の範囲外の値を代入しようとしました", "");
        break;
    case 3: // int
        if (v < -2147483648LL || v > 2147483647LL)
            yyerror("int型の範囲外の値を代入しようとしました", "");
        break;
    case 4: // long
        // int64_tの範囲は十分広いのでチェック不要
        break;
    default:
        break;
    }
    return v;
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
    case 0:
        yyerror("void型の値は参照できません", node->sval.c_str());
        return 0;
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
    // 左辺変数の型情報はsymbol_tableから取得（未定義ならnode->type_infoを使う）
    int lhs_type = node->type_info;
    auto it = symbol_table.find(node->sval);
    if (it != symbol_table.end()) {
        lhs_type = it->second.type;
    }
    propagate_type_info(rhs, lhs_type);
    int64_t value = eval(rhs);
    // デバッグ出力: 代入時の型・値
    debug_printf("DEBUG: assign %s value=%lld lhs_type=%d rhs_type=%d\n",
                 node->sval.c_str(), value, lhs_type, rhs->type_info);
    // fprintf(stderr, "DBG_ASSIGN: %s value=%lld lhs_type=%d rhs_type=%d\n",
    //         node->sval.c_str(), value, lhs_type, rhs->type_info);
    fflush(stderr);
    // 型ごとに範囲チェックし、範囲外ならエラー（キャスト前に必ずチェック）
    Variable var;
    var.type = lhs_type;
    bool out_of_range = false;
    switch (lhs_type) {
    case 0: // void
        var.value = 0;
        break;
    case 1: // tiny(int8_t)
        if (value < -128 || value > 127)
            out_of_range = true;
        break;
    case 2: // short(int16_t)
        if (value < -32768 || value > 32767)
            out_of_range = true;
        break;
    case 3: // int(int32_t)
        if (value < -2147483648LL || value > 2147483647LL)
            out_of_range = true;
        break;
    case 4: // long(int64_t)
        // int64_tの範囲は十分広いのでチェック不要
        break;
    default:
        break;
    }
    debug_printf("DEBUG: out_of_range=%d\n", out_of_range);
    if (out_of_range) {
        debug_printf("DEBUG: out_of_range for %s value=%lld type=%d\n",
                     node->sval.c_str(), value, lhs_type);
        yyerror("型の範囲外の値を代入しようとしました", node->sval.c_str());
    }
    // 範囲内ならキャストして代入
    switch (lhs_type) {
    case 0:
        var.value = 0;
        break;
    case 1:
        var.value = (int8_t)value;
        break;
    case 2:
        var.value = (int16_t)value;
        break;
    case 3:
        var.value = (int32_t)value;
        break;
    case 4:
        var.value = (int64_t)value;
        break;
    default:
        var.value = (int32_t)value;
        break;
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
    try {
        for (std::vector<ASTNode *>::iterator it = node->stmts.begin();
             it != node->stmts.end(); ++it) {
            if (*it) {
                debug_printf("DEBUG: stmtlist node type=%d\n", (*it)->type);
            }
            eval(*it);
        }
    } catch (const ReturnException &e) {
        throw; // return値を上位に伝搬
    }
    return 0;
}

// 関数呼び出し
int64_t eval_funccall(ASTNode *node) {
    // node->sval: 関数名, node->params: 引数式ノード
    if (!node) {
        yyerror("関数呼び出しノードが不正です", "");
        return 0;
    }
    auto it = function_table.find(node->sval);
    if (it == function_table.end()) {
        yyerror("未定義の関数です", node->sval.c_str());
        return 0;
    }
    ASTNode *func = it->second;
    debug_printf("DEBUG: funccall %s, rettypes.size=%zu\n", node->sval.c_str(),
                 func->rettypes.size());
    if (!func->rettypes.empty() && func->rettypes[0]) {
        debug_printf("DEBUG: funccall %s, rettypes[0]->type_info=%d\n",
                     node->sval.c_str(), func->rettypes[0]->type_info);
        // main関数の型チェック
        if (node->sval == "main" && func->rettypes[0]->type_info != 3) {
            fprintf(stderr, "Error: main関数はint型で定義してください\n");
            exit(1);
        }
    }
    if (!func) {
        return 0;
    }
    // 仮引数と実引数の数チェック
    size_t param_size = node->params.size();
    size_t func_param_size = func->params.size();
    if (func_param_size != param_size) {
        yyerror("引数の数が一致しません", node->sval.c_str());
        return 0;
    }
    // ローカルスコープ作成
    std::map<std::string, Variable> old_symbol_table = symbol_table;
    // 仮引数に実引数を束縛
    for (size_t i = 0; i < func_param_size; ++i) {
        ASTNode *param = func->params[i];
        if (!param) {
            yyerror("仮引数ノードが不正です", node->sval.c_str());
            continue;
        }
        int64_t argval = 0;
        if (i < param_size && node->params[i]) {
            argval = eval(node->params[i]);
        }
        Variable var;
        var.type = param->type_info;
        var.value = argval;
        symbol_table[param->sval] = var;
    }
    int64_t ret = 0;
    bool is_void = false;
    if (!func->rettypes.empty() && func->rettypes[0] &&
        func->rettypes[0]->type_info == 0) {
        is_void = true;
    }
    debug_printf("DEBUG: funccall %s, is_void=%d\n", node->sval.c_str(),
                 (int)is_void);
    try {
        eval(func->body);
        if (is_void) {
            ret = 0; // void型は値なし
        }
    } catch (const ReturnException &e) {
        debug_printf("DEBUG: ReturnException caught in %s, value=%lld\n",
                     func->sval.c_str(), e.value);
        if (is_void) {
            yyerror("void型関数で値を返すことはできません", func->sval.c_str());
            ret = 0;
        } else {
            ret = e.value;
            // 戻り値型に従い型変換・範囲チェック
            int rettype = func->rettypes[0] ? func->rettypes[0]->type_info : 3;
            bool out_of_range = false;
            switch (rettype) {
            case 0: // void
                ret = 0;
                break;
            case 1: // tiny(int8_t)
                if (ret < -128 || ret > 127)
                    out_of_range = true;
                ret = (int8_t)ret;
                break;
            case 2: // short(int16_t)
                if (ret < -32768 || ret > 32767)
                    out_of_range = true;
                ret = (int16_t)ret;
                break;
            case 3: // int(int32_t)
                if (ret < -2147483648LL || ret > 2147483647LL)
                    out_of_range = true;
                ret = (int32_t)ret;
                break;
            case 4: // long(int64_t)
                // int64_tの範囲は十分広いのでチェック不要
                ret = (int64_t)ret;
                break;
            default:
                ret = (int32_t)ret;
                break;
            }
            if (out_of_range) {
                debug_printf("DEBUG: return value out_of_range for %s "
                             "value=%lld type=%d\n",
                             func->sval.c_str(), e.value, rettype);
                yyerror("関数戻り値が型の範囲外です", func->sval.c_str());
            }
        }
    }
    symbol_table = old_symbol_table; // スコープ復元
    return ret;
}

// return文
int64_t eval_return(ASTNode *node) {
    // 呼び出し中の関数の型情報を取得するには、AST的にはfunc->rettypes[0]->type_infoを参照する必要があるが、
    // ここではreturn文のlhsがnullptrならvoid型とみなす
    if (!node->lhs) {
        debug_printf("DEBUG: eval_return void\n");
        throw ReturnException(0); // void型return
    } else {
        int64_t val = eval(node->lhs);
        debug_printf("DEBUG: eval_return value=%lld\n", val);
        throw ReturnException(val);
    }
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
    case ASTNode::AST_FUNCDEF:
        // 関数定義: function_tableに登録
        if (!node->sval.empty()) {
            function_table[node->sval] = node;
        }
        return 0;
    case ASTNode::AST_FUNCPARAM:
        return 0; // 仮引数ノードは評価しない
    case ASTNode::AST_FUNCCALL:
        return eval_funccall(node);
    case ASTNode::AST_RETURN:
        return eval_return(node);
    default:
        return 0;
    }
}
