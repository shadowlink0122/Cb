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
    int type; // 0=void, 1=tiny, 2=short, 3=int, 4=long, 5=string, 6=bool(1bit)
    int64_t value;      // 整数値
    std::string svalue; // 文字列値（string型用）
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

// 関数呼び出し時のreturn値伝搬用例外（ASTNode*で値を伝搬）
struct ReturnException {
    ASTNode *value;
    ReturnException(ASTNode *v) : value(v) {}
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

// 型ごとの範囲チェック関数
void check_range(int type, int64_t value, const char *name) {
    switch (type) {
    case 1:
        if (value < -128 || value > 127)
            yyerror("tiny型の範囲外の値を代入しようとしました", name);
        break;
    case 2:
        if (value < -32768 || value > 32767)
            yyerror("short型の範囲外の値を代入しようとしました", name);
        break;
    case 3:
        if (value < -2147483648LL || value > 2147483647LL)
            yyerror("int型の範囲外の値を代入しようとしました", name);
        break;
    case 4:
        // long: チェック不要
        break;
    default:
        break;
    }
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
    case 6: // bool
        v = (v != 0) ? 1 : 0;
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
    if (var.type == 5) {
        // string型の場合はnode->svalに値をセットし、type_info/typeも更新
        node->type_info = 5;
        node->type = ASTNode::AST_STRING_LITERAL;
        node->sval = var.svalue;
        return 0; // print等でsvalを参照する
    }
    if (var.type == 6) {
        node->type_info = 6;
        return (var.value != 0) ? 1 : 0;
    }
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
    } else if (node->op == "%") {
        if (r == 0) {
            yyerror("Error", "0除算が発生しました");
        } else {
            result = l % r;
        }
    } else if (node->op == "==") {
        result = (l == r) ? 1 : 0;
        node->type_info = 6;
    } else if (node->op == "!=") {
        result = (l != r) ? 1 : 0;
        node->type_info = 6;
    } else if (node->op == ">") {
        result = (l > r) ? 1 : 0;
        node->type_info = 6;
    } else if (node->op == "<") {
        result = (l < r) ? 1 : 0;
        node->type_info = 6;
    } else if (node->op == ">=") {
        result = (l >= r) ? 1 : 0;
        node->type_info = 6;
    } else if (node->op == "<=") {
        result = (l <= r) ? 1 : 0;
        node->type_info = 6;
    } else if (node->op == "||") {
        l = (l != 0) ? 1 : 0;
        r = (r != 0) ? 1 : 0;
        result = (l || r) ? 1 : 0;
        node->type_info = 6;
    } else if (node->op == "&&") {
        l = (l != 0) ? 1 : 0;
        r = (r != 0) ? 1 : 0;
        result = (l && r) ? 1 : 0;
        node->type_info = 6;
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
    Variable var;
    var.type = lhs_type;
    bool out_of_range = false;
    if (lhs_type == 5) {
        // string型: rhsの内容をデバッグ出力
        debug_printf("DEBUG: assign string rhs type=%d type_info=%d sval=%s\n",
                     rhs->type, rhs->type_info, rhs->sval.c_str());
        var.svalue = rhs->sval;
        var.value = 0;
    } else if (lhs_type == 6) {
        // bool型: 1bitに正規化
        var.value = (value != 0) ? 1 : 0;
        var.svalue = "";
    } else {
        // デバッグ出力: 代入時の型・値
        debug_printf("DEBUG: assign %s value=%lld lhs_type=%d rhs_type=%d\n",
                     node->sval.c_str(), value, lhs_type, rhs->type_info);
        fflush(stderr);
        // 型ごとに範囲チェックし、範囲外ならエラー（キャスト前に必ずチェック）
        if (lhs_type != 0) {
            check_range(lhs_type, value, node->sval.c_str());
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
        var.svalue = "";
    }
    symbol_table[node->sval] = var;
    return var.value;
}

int eval_print(ASTNode *node) {
    if (!node->lhs) {
        printf("(null)\n");
        return 0;
    }
    debug_printf("DEBUG: eval_print lhs type=%d type_info=%d sval=%s\n",
                 node->lhs->type, node->lhs->type_info,
                 node->lhs->sval.c_str());
    ASTNode *result = node->lhs;
    // すべてのprint対象で必ずevalを呼ぶ（変数参照時も値をセット）
    int64_t value = eval(result);
    if (result->type == ASTNode::AST_STRING_LITERAL || result->type_info == 5) {
        if (result->sval.empty()) {
            printf("\n");
        } else {
            printf("%s\n", result->sval.c_str());
        }
        return 0;
    }
    // それ以外は数値として評価
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
    ASTNode *ret_node = nullptr;
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
            ret_node = nullptr; // void型は値なし
        }
    } catch (const ReturnException &e) {
        debug_printf("DEBUG: ReturnException caught in %s, node type=%d\n",
                     func->sval.c_str(), e.value ? e.value->type : -1);
        if (is_void) {
            yyerror("void型関数で値を返すことはできません", func->sval.c_str());
            ret_node = nullptr;
        } else {
            ret_node = e.value;
        }
    }
    symbol_table = old_symbol_table; // スコープ復元
    if (!ret_node)
        return 0;
    int rettype = func->rettypes[0] ? func->rettypes[0]->type_info : 3;
    if (rettype == 5 && ret_node->type == ASTNode::AST_STRING_LITERAL) {
        node->sval = ret_node->sval; // 呼び出しノードに文字列をセット
        node->type_info = 5;         // ここでtype_infoも必ずセット
        return 0;                    // print等でsvalを参照
    } else {
        return eval(ret_node);
    }
}

// return文
int64_t eval_return(ASTNode *node) {
    if (!node->lhs) {
        debug_printf("DEBUG: eval_return void\n");
        throw ReturnException(nullptr); // void型return
    } else if (node->lhs->type == ASTNode::AST_STRING_LITERAL) {
        debug_printf("DEBUG: eval_return string node type=%d type_info=%d\n",
                     node->lhs->type, node->lhs->type_info);
        throw ReturnException(node->lhs);
    } else {
        int64_t val = eval(node->lhs);
        debug_printf("DEBUG: eval_return value=%lld\n", val);
        ASTNode *num = new ASTNode(ASTNode::AST_NUM);
        num->lval64 = val;
        num->type_info = node->lhs->type_info;
        throw ReturnException(num);
    }
}

// break文用例外
struct BreakException {
    BreakException() {}
};

int64_t eval(ASTNode *node) {
    if (!node)
        return 0;
    switch (node->type) {
    case ASTNode::AST_PRE_INCDEC: {
        // ++a, --a
        if (!node->lhs || node->lhs->type != ASTNode::AST_VAR)
            yyerror("インクリメント/デクリメントの対象が変数ではありません",
                    "");
        auto it = symbol_table.find(node->lhs->sval);
        if (it == symbol_table.end())
            yyerror("未定義の変数です", node->lhs->sval.c_str());
        Variable &var = it->second;
        if (node->op == "++") {
            var.value += 1;
        } else if (node->op == "--") {
            var.value -= 1;
        } else {
            yyerror("未知のインクリメント/デクリメント演算子です",
                    node->op.c_str());
        }
        check_range(var.type, var.value, node->lhs->sval.c_str());
        return var.value;
    }
    case ASTNode::AST_POST_INCDEC: {
        // a++, a--
        if (!node->lhs || node->lhs->type != ASTNode::AST_VAR)
            yyerror("インクリメント/デクリメントの対象が変数ではありません",
                    "");
        auto it = symbol_table.find(node->lhs->sval);
        if (it == symbol_table.end())
            yyerror("未定義の変数です", node->lhs->sval.c_str());
        Variable &var = it->second;
        int64_t old = var.value;
        if (node->op == "++") {
            var.value += 1;
        } else if (node->op == "--") {
            var.value -= 1;
        } else {
            yyerror("未知のインクリメント/デクリメント演算子です",
                    node->op.c_str());
        }
        check_range(var.type, var.value, node->lhs->sval.c_str());
        return old;
    }
    case ASTNode::AST_NUM:
        return eval_num(node);
    case ASTNode::AST_VAR:
        return eval_var(node);
    case ASTNode::AST_BINOP:
        return eval_binop(node);
    case ASTNode::AST_UNARYOP:
        if (node->op == "!") {
            int64_t v = eval(node->lhs);
            return (v == 0) ? 1 : 0;
        }
        return 0;
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
    case ASTNode::AST_STRING_LITERAL:
        // 文字列リテラルは値としては0、svalは文字列本体
        return 0;
    case ASTNode::AST_WHILE: {
        // while(cond) { body }
        try {
            while (true) {
                if (node->for_cond) {
                    int64_t cond = eval(node->for_cond);
                    if (!cond)
                        break;
                }
                if (node->for_body)
                    eval(node->for_body);
            }
        } catch (const BreakException &) {
            // break文でループ脱出
        }
        return 0;
    }
    case ASTNode::AST_FOR: {
        // for(init; cond; update) { body }
        if (node->for_init)
            eval(node->for_init);
        try {
            while (true) {
                if (node->for_cond) {
                    int64_t cond = eval(node->for_cond);
                    if (!cond)
                        break; // 条件がfalseなら終了
                }
                if (node->for_body)
                    eval(node->for_body);
                if (node->for_update)
                    eval(node->for_update);
            }
        } catch (const BreakException &) {
            // break文でループ脱出
        }
        return 0;
    }
    case ASTNode::AST_BREAK: {
        // break; または break expr;
        if (!node->lhs) {
            throw BreakException();
        } else {
            int64_t cond = eval(node->lhs);
            if (cond) {
                throw BreakException();
            }
        }
        return 0;
    }
    case ASTNode::AST_IF: {
        int64_t cond = eval(node->if_cond);
        cond = (cond != 0) ? 1 : 0; // 0以外はtrueとして扱う
        if (cond) {
            if (node->if_then)
                eval(node->if_then);
        } else {
            if (node->if_else)
                eval(node->if_else);
        }
        return 0;
    }
    default:
        return 0;
    }
}
