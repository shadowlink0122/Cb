#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct ASTNode {
    enum Type {
        AST_NUM,
        AST_VAR,
        AST_BINOP,
        AST_UNARYOP, // 単項演算子（NOT, MINUS など）
        AST_ASSIGN,
        AST_PRINT,
        AST_STMTLIST,
        AST_FUNCDEF,        // 関数定義
        AST_FUNCPARAM,      // 引数リスト
        AST_TYPELIST,       // 型リスト（タプル型）
        AST_RETURN,         // return文
        AST_FUNCCALL,       // 関数呼び出し
        AST_STRING_LITERAL, // 文字列リテラル
        AST_FOR,            // for文
        AST_WHILE,          // while文
        AST_BREAK,          // break文
        AST_IF,             // if文
        AST_PRE_INCDEC,     // 前置インクリメント/デクリメント
        AST_POST_INCDEC,    // 後置インクリメント/デクリメント
        AST_ARRAY_DECL,     // 配列宣言
        AST_ARRAY_LITERAL,  // 配列リテラル
        AST_ARRAY_REF       // 配列要素アクセス a[n]
    } type;
    // 配列用
    int array_size = -1; // 宣言時サイズ（-1は未指定）
    ASTNode *array_size_expr = nullptr; // 配列サイズ式（int a[expr]; 用）
    std::vector<ASTNode *> elements;    // 配列リテラル・初期化子
    ASTNode *array_index = nullptr;     // 要素アクセス用
    int elem_type_info = 0;             // 配列要素の型情報
    // if文用: 条件式、then, else
    ASTNode *if_cond = nullptr;
    ASTNode *if_then = nullptr;
    ASTNode *if_else = nullptr;
    // 型情報: 1=tiny, 2=short, 3=int, 4=long, 5=string, 6=bool(1bit)
    int32_t type_info = 0;
    int64_t lval64 = 0; // 整数値（常にint64_tで保持）
    std::string sval;   // 変数名・関数名など
    std::string op;
    ASTNode *lhs, *rhs;
    std::vector<ASTNode *> stmts;
    // 関数定義用
    std::vector<ASTNode *> params;   // 引数リスト
    std::vector<ASTNode *> rettypes; // 戻り値型リスト
    ASTNode *body = nullptr;         // 関数本体
    ASTNode()
        : type(AST_VAR), type_info(0), lval64(0), lhs(nullptr), rhs(nullptr) {}
    ASTNode(Type t)
        : type(t), type_info(0), lval64(0), lhs(nullptr), rhs(nullptr) {}
    ~ASTNode() {
        delete lhs;
        delete rhs;
        for (auto it = stmts.begin(); it != stmts.end(); ++it)
            delete *it;
        for (auto it = params.begin(); it != params.end(); ++it)
            delete *it;
        for (auto it = rettypes.begin(); it != rettypes.end(); ++it)
            delete *it;
        delete body;
    }
    // for文用: for(init; cond; update) { body }
    ASTNode *for_init = nullptr;   // 初期化文
    ASTNode *for_cond = nullptr;   // 条件式
    ASTNode *for_update = nullptr; // 更新式
    ASTNode *for_body = nullptr;   // 本体
    // return文用: lhsに返す式を格納
};

// CbソースファイルからASTを生成する関数
ASTNode *parse_to_ast(const std::string &filename);
