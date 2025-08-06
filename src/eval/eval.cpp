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

#include "../ast/util.h"
#include <cstdint>
#include <map>
#include <string>

// struct Variableはeval.hで定義

// 配列のデフォルト値を型ごとに返す
static int64_t default_int_value(int type) {
    if (type == 6)
        return 0; // bool
    return 0;     // int系
}
static std::string default_str_value(int type) { return ""; }

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
    // 配列型(100+)なら要素型で伝播
    if (type_info >= 100) {
        node->type_info = type_info - 100;
    } else {
        node->type_info = type_info;
    }
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
    }
    const Variable &var = it->second;
    if (var.type == 5) {
        // string型の場合のみtype/svalをセット
        node->type_info = 5;
        node->type = ASTNode::AST_STRING_LITERAL;
        node->sval = var.svalue;
        return 0;
    }
    // string型以外はtype_infoを上書きしない
    if (var.type == 6) {
        return (var.value != 0) ? 1 : 0;
    }
    // int型などは値をそのまま返す
    return var.value;
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
    debug_printf("DEBUG: Variable %s is %s\n", node->sval.c_str(),
                 node->is_const ? "const" : "not const");
    // 変数テーブルから該当変数を検索
    auto it = symbol_table.find(
        node->sval); // 既に宣言済みの場合はsymbol_tableから取得
    if (it != symbol_table.end()) {
        Variable &var = it->second;
        // const変数の再代入禁止（宣言時はOK、2回目以降はNG）
        debug_printf("DEBUG: assign check %s is_const=%d is_assigned=%d\n",
                     node->sval.c_str(), var.is_const, var.is_assigned);
        if (var.is_const && var.is_assigned) {
            yyerror("constで定義された変数は再代入できません",
                    node->sval.c_str());
        }
    }
    // 配列要素代入: lhsがAST_ARRAY_REFの場合
    if (node->lhs && node->lhs->type == ASTNode::AST_ARRAY_REF) {
        ASTNode *arr_ref = node->lhs;
        auto it = symbol_table.find(arr_ref->sval);
        if (it == symbol_table.end()) {
            yyerror("未定義の配列または変数です", arr_ref->sval.c_str());
        }
        Variable &var = it->second;
        // const配列/const stringの要素変更禁止
        if (var.is_const) {
            yyerror("constで定義された配列・stringの要素は変更できません",
                    arr_ref->sval.c_str());
        }
        int64_t idx = eval(arr_ref->array_index);
        // string型の要素代入
        if (var.type == 5) {
            if (idx < 0 || idx >= (int64_t)var.svalue.size()) {
                yyerror("stringの範囲外アクセスです", arr_ref->sval.c_str());
            }
            propagate_type_info(node->rhs, 5);
            eval(node->rhs);
            if (node->rhs->sval.size() != 1) {
                yyerror("string要素代入は1文字のみ可能です",
                        arr_ref->sval.c_str());
            }
            var.svalue[idx] = node->rhs->sval[0];
            debug_printf("DEBUG: string assign %s[%lld] = %c\n",
                         arr_ref->sval.c_str(), idx, node->rhs->sval[0]);
            return 0;
        }
        // 配列の場合
        if (!var.is_array) {
            yyerror("配列またはstring以外の要素代入はできません",
                    arr_ref->sval.c_str());
        }
        if (idx < 0 || idx >= var.array_size) {
            yyerror("配列の範囲外アクセスです", arr_ref->sval.c_str());
        }
        int elem_type = var.elem_type;
        propagate_type_info(node->rhs, elem_type);
        int64_t value = eval(node->rhs);
        debug_printf("DEBUG: array assign %s[%lld] = %lld (elem_type=%d)\n",
                     arr_ref->sval.c_str(), idx, value, elem_type);
        if (elem_type == 5) {
            debug_printf("DEBUG: array assign string %s[%lld] = %s\n",
                         arr_ref->sval.c_str(), idx, node->rhs->sval.c_str());
            var.arr_svalue[idx] = node->rhs->sval;
        } else if (elem_type == 6) {
            var.arr_value[idx] = (value != 0) ? 1 : 0;
        } else {
            check_range(elem_type, value, arr_ref->sval.c_str());
            switch (elem_type) {
            case 1:
                var.arr_value[idx] = (int8_t)value;
                break;
            case 2:
                var.arr_value[idx] = (int16_t)value;
                break;
            case 3:
                var.arr_value[idx] = (int32_t)value;
                break;
            case 4:
                var.arr_value[idx] = (int64_t)value;
                break;
            default:
                var.arr_value[idx] = (int32_t)value;
                break;
            }
        }
        return value;
    }
    // 通常の変数代入
    ASTNode *rhs = node->rhs;
    // 左辺変数の型情報はsymbol_tableから取得（未定義ならnode->type_infoを使う）
    int lhs_type = node->type_info;
    if (it != symbol_table.end()) {
        // 配列変数名への直接代入はエラー
        if (it->second.is_array) {
            yyerror("配列変数名への直接代入はできません。要素指定してください",
                    node->sval.c_str());
        } else {
            lhs_type = it->second.type;
        }
    }
    // lhs_typeが未設定（0）の場合はrhsのtype_infoを使う（int型デフォルト）
    if (lhs_type == 0) {
        lhs_type = rhs->type_info ? rhs->type_info : 3;
    }
    propagate_type_info(rhs, lhs_type);
    int64_t value = eval(rhs);
    if (it != symbol_table.end()) {
        Variable &var = it->second;
        if (var.type == 0) {
            var.type = lhs_type;
        }
        if (lhs_type == 5) {
            debug_printf(
                "DEBUG: assign string rhs type=%d type_info=%d sval=%s\n",
                rhs->type, rhs->type_info, rhs->sval.c_str());
            var.svalue = rhs->sval;
            var.value = 0;
        } else if (lhs_type == 6) {
            var.value = (value != 0) ? 1 : 0;
            var.svalue = "";
        } else {
            debug_printf(
                "DEBUG: assign %s value=%lld lhs_type=%d rhs_type=%d\n",
                node->sval.c_str(), value, lhs_type, rhs->type_info);
            fflush(stderr);
            if (lhs_type != 0) {
                check_range(lhs_type, value, node->sval.c_str());
            }
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
        var.is_assigned = true;
        return var.value;
    } else {
        Variable var;
        var.type = lhs_type;
        // 既存変数があればis_constを維持
        if (it != symbol_table.end()) {
            var.is_const = it->second.is_const;
        } else {
            var.is_const = node->is_const;
        }
        if (lhs_type == 5) {
            debug_printf(
                "DEBUG: assign string rhs type=%d type_info=%d sval=%s\n",
                rhs->type, rhs->type_info, rhs->sval.c_str());
            var.svalue = rhs->sval;
            var.value = 0;
        } else if (lhs_type == 6) {
            var.value = (value != 0) ? 1 : 0;
            var.svalue = "";
        } else {
            debug_printf(
                "DEBUG: assign %s value=%lld lhs_type=%d rhs_type=%d\n",
                node->sval.c_str(), value, lhs_type, rhs->type_info);
            fflush(stderr);
            if (lhs_type != 0) {
                check_range(lhs_type, value, node->sval.c_str());
            }
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
        var.is_assigned = true;
        symbol_table[node->sval] = var;
        return var.value;
    }
}

int eval_print(ASTNode *node) {
    if (!node->lhs) {
        printf("(null)\n");
        return 0;
    }
    debug_printf("DEBUG: eval_print lhs type=%d type_info=%d sval=%s\n",
                 node->lhs->type, node->lhs->type_info,
                 node->lhs->sval.c_str());
    if (node->lhs->type == ASTNode::AST_ARRAY_REF) {
        debug_printf("DEBUG: print array ref %s, array_index type=%d\n",
                     node->lhs->sval.c_str(),
                     node->lhs->array_index ? node->lhs->array_index->type
                                            : -1);
    }
    ASTNode *result = node->lhs;
    // すべてのprint対象で必ずevalを呼ぶ（変数参照時も値をセット）
    int64_t value = eval(result);
    if (result->type == ASTNode::AST_ARRAY_REF) {
        debug_printf("DEBUG: after eval array ref %s, type_info=%d, sval=%s, "
                     "value=%lld\n",
                     result->sval.c_str(), result->type_info,
                     result->sval.c_str(), value);
        // string要素アクセス時はeval側で直接出力済みなので何も出力しない
        if (value == INT64_MIN) {
            return 0;
        }
        // 配列要素参照はevalの戻り値を出力
        // string型要素なら文字列出力、それ以外は数値出力
        if (result->type_info == 5) {
            // 文字列型要素
            if (!result->sval.empty()) {
                printf("%s\n", result->sval.c_str());
            } else {
                printf("\n");
            }
        } else {
            printf("%lld\n", value);
        }
        return 0;
    }
    if (result->type == ASTNode::AST_STRING_LITERAL || result->type_info == 5) {
        if (result->sval.empty()) {
            printf("\n");
        } else {
            printf("%s\n", result->sval.c_str());
        }
        return 0;
    }
    if (result->type == ASTNode::AST_VAR) {
        // 変数参照は必ずeval_varの戻り値を出力
        printf("%lld\n", value);
        return 0;
    }
    // それ以外は数値として評価
    printf("%lld\n", value);
    return 0;
}

int eval_stmtlist(ASTNode *node) {
    // グローバルスコープ(root)かどうか判定
    extern ASTNode *root;
    bool is_global = (node == root);
    std::map<std::string, Variable> old_symbol_table;
    if (!is_global) {
        old_symbol_table = symbol_table;
    }
    try {
        for (std::vector<ASTNode *>::iterator it = node->stmts.begin();
             it != node->stmts.end(); ++it) {
            if (*it) {
                debug_printf("DEBUG: stmtlist node type=%d\n", (*it)->type);
            }
            eval(*it);
        }
    } catch (const ReturnException &e) {
        if (!is_global)
            symbol_table = old_symbol_table;
        throw; // return値を上位に伝搬
    }
    if (!is_global) {
        // スコープ復元時、すべての変数（配列・スカラ）で値をマージして失われないようにする
        for (auto &kv : symbol_table) {
            old_symbol_table[kv.first] = kv.second;
        }
        symbol_table = old_symbol_table;
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
    case ASTNode::AST_VAR_DECL: {
        // 型 変数 = 値; の宣言（初期化付き変数宣言）
        // 既に同名変数が存在する場合はエラー
        if (symbol_table.find(node->sval) != symbol_table.end()) {
            yyerror("変数の再宣言はできません", node->sval.c_str());
            exit(1);
        }
        debug_printf("DEBUG: AST_VAR_DECL %s node->is_const=%d\n",
                     node->sval.c_str(), node->is_const);
        Variable var;
        var.type = node->type_info;
        var.is_const = node->is_const;
        var.is_array = false;
        var.svalue = "";
        var.value = 0;
        // 初期値があれば評価し、is_assignedフラグもセット
        if (node->rhs) {
            propagate_type_info(node->rhs, node->type_info);
            int64_t v = eval(node->rhs);
            if (node->type_info == 5) {
                // string型
                var.svalue = node->rhs->sval;
            } else if (node->type_info == 6) {
                var.value = (v != 0) ? 1 : 0;
            } else {
                check_range(node->type_info, v, node->sval.c_str());
                switch (node->type_info) {
                case 1:
                    var.value = (int8_t)v;
                    break;
                case 2:
                    var.value = (int16_t)v;
                    break;
                case 3:
                    var.value = (int32_t)v;
                    break;
                case 4:
                    var.value = (int64_t)v;
                    break;
                default:
                    var.value = (int32_t)v;
                    break;
                }
            }
            var.is_assigned = true;
        }
        symbol_table[node->sval] = var;
        return 0;
    }
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
    case ASTNode::AST_ARRAY_LITERAL:
        // 配列リテラル自体は値を返さない
        return 0;
    case ASTNode::AST_ARRAY_DECL: {
        // 配列宣言: シンボルテーブルに登録
        Variable var;
        var.is_array = true;
        int arr_size = node->array_size;
        if (node->array_size_expr) {
            arr_size = (int)eval(node->array_size_expr);
        }
        if (arr_size < 0) {
            yyerror("配列サイズが負です", node->sval.c_str());
        }
        var.array_size = arr_size;
        var.elem_type = node->elem_type_info;
        var.type = 100 + node->elem_type_info;
        if (var.elem_type == 5) {
            var.arr_svalue.resize(var.array_size,
                                  default_str_value(var.elem_type));
        } else {
            var.arr_value.resize(var.array_size,
                                 default_int_value(var.elem_type));
        }
        // 初期化子があればセット
        if (!node->elements.empty()) {
            for (size_t i = 0;
                 i < node->elements.size() && i < (size_t)var.array_size; ++i) {
                ASTNode *elem = node->elements[i];
                if (var.elem_type == 5) {
                    eval(elem);
                    var.arr_svalue[i] = elem->sval;
                } else if (var.elem_type == 6) {
                    int64_t v = eval(elem);
                    var.arr_value[i] = (v != 0) ? 1 : 0;
                } else {
                    int64_t v = eval(elem);
                    check_range(var.elem_type, v, node->sval.c_str());
                    var.arr_value[i] = v;
                }
            }
        }
        symbol_table[node->sval] = var;
        return 0;
    }
    case ASTNode::AST_ARRAY_REF: {
        debug_printf("DEBUG: AST_ARRAY_REF node->sval=%s\n",
                     node->sval.c_str());
        auto it = symbol_table.find(node->sval);
        if (it == symbol_table.end()) {
            yyerror("未定義の配列または変数です", node->sval.c_str());
            return 0;
        }
        Variable &var = it->second;
        int64_t idx = eval(node->array_index);
        // string型の要素アクセス
        if (var.type == 5) {
            // UTF-8の1文字単位でアクセスできるよう分割
            std::vector<std::string> chars;
            const std::string &s = var.svalue;
            for (size_t i = 0; i < s.size();) {
                unsigned char c = s[i];
                size_t char_len = 1;
                if ((c & 0x80) == 0x00)
                    char_len = 1; // ASCII
                else if ((c & 0xE0) == 0xC0)
                    char_len = 2; // 2バイト
                else if ((c & 0xF0) == 0xE0)
                    char_len = 3; // 3バイト
                else if ((c & 0xF8) == 0xF0)
                    char_len = 4; // 4バイト
                else {
                    char_len = 1;
                } // 不正バイト
                chars.push_back(s.substr(i, char_len));
                i += char_len;
            }
            if (idx < 0 || idx >= (int64_t)chars.size()) {
                yyerror("stringの範囲外アクセスです", node->sval.c_str());
                return 0;
            }
            node->type_info = 5;
            // UTF-8文字列をそのまま出力
            debug_printf("DEBUG: eval string ref %s[%lld] = %s\n",
                         node->sval.c_str(), idx, chars[idx].c_str());
            printf("%s\n", chars[idx].c_str());
            // print文の戻り値出力を抑止するため、特別な値を返す
            return INT64_MIN; // sentinel value
        }
        // 配列の場合
        if (!var.is_array) {
            yyerror("配列またはstring以外の要素アクセスはできません",
                    node->sval.c_str());
            return 0;
        }
        debug_printf("DEBUG: eval array ref %s[%lld] (elem_type=%d, "
                     "arr_value.size=%zu)\n",
                     node->sval.c_str(), idx, var.elem_type,
                     var.arr_value.size());
        if (idx < 0 || idx >= var.array_size) {
            yyerror("配列の範囲外アクセスです", node->sval.c_str());
            return 0;
        }
        if (var.elem_type == 5) {
            node->type_info = 5;
            // node->svalは絶対に上書きしない
            debug_printf("DEBUG: eval array ref string value = %s\n",
                         var.arr_svalue[idx].c_str());
            return 0;
        } else if (var.elem_type == 6) {
            node->type_info = 6;
            debug_printf("DEBUG: eval array ref bool value = %lld\n",
                         (var.arr_value[idx] != 0) ? 1 : 0);
            return (var.arr_value[idx] != 0) ? 1 : 0;
        } else {
            node->type_info = var.elem_type;
            debug_printf("DEBUG: eval array ref int value = %lld\n",
                         var.arr_value[idx]);
            switch (var.elem_type) {
            case 1:
                return (int8_t)var.arr_value[idx];
            case 2:
                return (int16_t)var.arr_value[idx];
            case 3:
                return (int32_t)var.arr_value[idx];
            case 4:
                return (int64_t)var.arr_value[idx];
            default:
                return (int32_t)var.arr_value[idx];
            }
        }
    }
    default:
        return 0;
    }
}
