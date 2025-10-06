// Declaration Parser - 宣言解析を担当
// Phase 2-3: RecursiveParserへの委譲実装 + ドキュメント化
//
// このファイルは、変数、関数、typedefの宣言解析を担当します。
//
// 【サポートする宣言】:
// 1. 変数宣言: int x = 10;
// 2. 配列宣言: int[5] arr = [1, 2, 3, 4, 5];
// 3. 関数宣言: int add(int a, int b) { return a + b; }
// 4. typedef宣言: typedef MyInt = int;
// 5. 関数ポインタtypedef: typedef Callback = int(int, int);
//
#include "declaration_parser.h"
#include "../recursive_parser.h"

DeclarationParser::DeclarationParser(RecursiveParser* parser) 
    : parser_(parser) {
}

// ========================================
// 変数宣言
// ========================================

/**
 * @brief 変数宣言を解析
 * @return 解析されたAST変数宣言ノード
 * 
 * サポートする構文:
 * - 単純な変数: int x;
 * - 初期化付き: int x = 10;
 * - 複数宣言: int x = 1, y = 2, z = 3;
 * - 配列: int[5] arr;
 * - ポインタ: int* ptr;
 * - 参照: int& ref = x;
 * - const修飾子: const int x = 10;
 */
ASTNode* DeclarationParser::parseVariableDeclaration() {
    return parser_->parseVariableDeclaration();
}

/**
 * @brief typedef付き変数宣言を解析
 * @return 解析されたAST変数宣言ノード
 * 
 * 例: MyInt x = 10; (MyIntはtypedef)
 */
ASTNode* DeclarationParser::parseTypedefVariableDeclaration() {
    return parser_->parseTypedefVariableDeclaration();
}

// ========================================
// 関数宣言
// ========================================

/**
 * @brief 関数宣言を解析
 * @return 解析されたAST関数宣言ノード
 * 
 * 構文: return_type function_name(param1, param2, ...) { body }
 * 
 * サポートする機能:
 * - 戻り値の型指定（void含む）
 * - パラメータリスト（値渡し、参照渡し、ポインタ）
 * - 関数本体の解析
 * - 再帰関数
 */
ASTNode* DeclarationParser::parseFunctionDeclaration() {
    return parser_->parseFunctionDeclaration();
}

/**
 * @brief 関数名の後の部分を解析
 * @param return_type 戻り値の型
 * @param function_name 関数名
 * @return 解析されたAST関数宣言ノード
 * 
 * 既に関数名まで解析済みの場合に使用します。
 * パラメータリストと関数本体を解析します。
 */
ASTNode* DeclarationParser::parseFunctionDeclarationAfterName(
    const std::string& return_type,
    const std::string& function_name
) {
    return parser_->parseFunctionDeclarationAfterName(return_type, function_name);
}

// ========================================
// typedef宣言
// ========================================

/**
 * @brief typedef宣言を解析
 * @return 解析されたAST typedef宣言ノード
 * 
 * サポートする構文:
 * - 型エイリアス: typedef MyInt = int;
 * - 配列型エイリアス: typedef IntArray = int[10];
 * - Union型: typedef Status = 200 | 404 | 500;
 * - 構造体typedef: typedef struct Point { ... } Point;
 * - enum typedef: typedef enum Color { ... } Color;
 */
ASTNode* DeclarationParser::parseTypedefDeclaration() {
    return parser_->parseTypedefDeclaration();
}

/**
 * @brief 関数ポインタtypedef宣言を解析
 * @return 解析されたAST関数ポインタtypedef宣言ノード
 * 
 * 構文: typedef Callback = return_type(param1_type, param2_type, ...);
 * 
 * 例:
 * - typedef IntFunc = int(int, int);
 * - typedef VoidFunc = void();
 * 
 * 用途:
 * - コールバック関数の型定義
 * - 関数ポインタの型安全性向上
 */
ASTNode* DeclarationParser::parseFunctionPointerTypedefDeclaration() {
    return parser_->parseFunctionPointerTypedefDeclaration();
}
