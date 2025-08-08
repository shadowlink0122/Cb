#pragma once
#include "../ast/ast.h"
#include <map>
#include <string>

// テスト用バイナリでもリンクできるようにエラー関数extern宣言

// 関数定義テーブル（main.cpp等から参照する場合用）
extern std::map<std::string, ASTNode *> function_table;

struct Variable {
    int type; // 0=void, 1=tiny, 2=short, 3=int, 4=long, 5=string, 6=bool(1bit),
              // 100+で配列
    int64_t value;      // 整数値
    std::string svalue; // 文字列値（string型用）
    // 配列用
    std::vector<int64_t> arr_value;      // 整数・bool配列
    std::vector<std::string> arr_svalue; // 文字列配列
    int array_size = -1;                 // 配列サイズ（1次元互換性）
    std::vector<int> array_sizes;        // 多次元配列のサイズ（各次元のサイズ）
    int elem_type = 0;                   // 配列要素型
    bool is_array = false;
    bool is_const = false;    // const変数かどうか
    bool is_assigned = false; // 初回代入済みかどうか
};

int64_t eval(ASTNode *node);
int64_t eval_num(ASTNode *node);
int64_t eval_var(ASTNode *node);
int64_t eval_binop(ASTNode *node);
int64_t eval_assign(ASTNode *node);
int eval_print(ASTNode *node);
int eval_stmtlist(ASTNode *node);
