#include <cstdio>
#include <string>
#include "util.h"
#include "ast.h"
#include <cstdio>
#include <stdio.h>
#include <string>

extern int yyparse();
extern FILE *yyin;
extern ASTNode *root;
extern int yylineno;
extern char *yyfilename;
static std::string yyfilename_holder;

ASTNode *parse_to_ast(const std::string &filename) {
    root = nullptr; // パース前に必ず初期化
    yyin = fopen(filename.c_str(), "r");
    yyfilename_holder = filename;
    yyfilename = (char *)yyfilename_holder.c_str();
    if (!yyin) {
        perror(filename.c_str());
        return nullptr;
    }
    int parse_result = yyparse();
    fclose(yyin);
    // yyfilenameのdangling pointer防止
    yyfilename_holder.clear();
    yyfilename = (char *)yyfilename_holder.c_str();
    if (parse_result != 0 || root == nullptr) {
        root = nullptr;
        return nullptr;
    }
    ASTNode *result = root;
    root = nullptr; // 呼び出し側でdeleteすること
    return result;
}

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

