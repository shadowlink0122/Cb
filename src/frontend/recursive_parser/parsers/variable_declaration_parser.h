// Variable Declaration Parser - 変数宣言解析を担当
// Phase 5-3-5: 変数宣言の専用パーサー
//
// このファイルは、変数宣言の解析を専門に担当します。
// DeclarationParserから分離して、ファイルサイズを管理します。
//
// 【サポートする宣言】:
// 1. 単純な変数: int x;
// 2. 初期化付き: int x = 10;
// 3. 複数宣言: int x = 1, y = 2, z = 3;
// 4. 配列: int[5] arr;
// 5. ポインタ: int* ptr;
// 6. 参照: int& ref = x;
// 7. const修飾子: const int x = 10;
// 8. static修飾子: static int x = 10;
// 9. private修飾子: private int x = 10;
//
#ifndef VARIABLE_DECLARATION_PARSER_H
#define VARIABLE_DECLARATION_PARSER_H

#include "src/common/ast.h"

class RecursiveParser;

class VariableDeclarationParser {
public:
    explicit VariableDeclarationParser(RecursiveParser* parser);
    
    // 変数宣言解析のメインメソッド
    ASTNode* parseVariableDeclaration();
    
private:
    RecursiveParser* parser_;
};

#endif // VARIABLE_DECLARATION_PARSER_H
