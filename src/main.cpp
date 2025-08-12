#include "ast/util.h"
#include "eval/eval.h"
#include "parser.h"
#include <map>
#include <stdio.h>
#include <string>
#include "ast/ast_debug.h"
// debug_printラッパー（eval.cppのdebug_printfを呼ぶ）
extern void debug_print(const char *fmt, ...);
// デバッグモードフラグ（eval.cppで定義）
extern bool debug_mode;

extern int yyparse();
extern FILE *yyin;
extern ASTNode *root;
extern int yylineno;

int main(int argc, char **argv) {
    if (argc > 1) {
        ASTNode *main_call = nullptr;
        ASTNode *ast = nullptr;
        // --debugオプションがあればデバッグモードON
        debug_mode = false;
        int file_arg = 1;
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--debug") {
                debug_mode = true;
            } else {
                file_arg = i;
            }
        }
        ast = parse_to_ast(argv[file_arg]);
        if (!ast)
            return 1;
        // 先にグローバルを登録
        register_globals(ast);
        // 関数定義ノードのみfunction_tableに登録（副作用のある実行はしない）
        if (ast && ast->type == ASTNode::AST_STMTLIST) {
            for (auto *stmt : ast->stmts) {
                if (stmt && stmt->type == ASTNode::AST_FUNCDEF) {
                    eval(stmt); // function_tableに登録のみ（既にregister_globalsで通っている可能性あり）
                }
            }
        } else if (ast && ast->type == ASTNode::AST_FUNCDEF) {
            eval(ast);
        }
        // main関数ASTのbodyをデバッグ出力（デバッグモード時のみ）
        if (debug_mode) {
            extern std::map<std::string, ASTNode*> function_table;
            if (function_table.find("main") != function_table.end()) {
                printf("[DEBUG] main関数AST body:\n");
                dump_ast(function_table["main"]->body, 2);
            }
        }
        // main関数呼び出しノードを生成
        main_call = new ASTNode(ASTNode::AST_FUNCCALL);
        main_call->sval = "main";
        main_call->params = std::vector<ASTNode*>();
        // main関数が存在しなければエラー
        extern std::map<std::string, ASTNode*> function_table;
        if (function_table.find("main") == function_table.end()) {
            fprintf(stderr, "Error: main関数が定義されていません\n");
            delete main_call;
            delete ast;
            return 1;
        }
        eval(main_call);
        delete main_call;
        delete ast;
        return 0;
    } else {
        fprintf(stderr, "Usage: %s <input.cb> [--debug]\n", argv[0]);
        return 1;
    }
}
