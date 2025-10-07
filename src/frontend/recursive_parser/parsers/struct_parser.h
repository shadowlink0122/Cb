// 構造体パーサー - 構造体とUnionの宣言を解析
// Struct Parser - Parse struct and union declarations

#ifndef STRUCT_PARSER_H
#define STRUCT_PARSER_H

#include "../../common/ast.h"
#include <string>

class RecursiveParser;

// 構造体解析を担当するクラス
class StructParser {
  public:
    explicit StructParser(RecursiveParser *parser);

    // 構造体宣言
    ASTNode *parseStructDeclaration();
    ASTNode *parseStructTypedefDeclaration();

    // 前方宣言
    ASTNode *parseForwardDeclaration();

    // Union宣言
    ASTNode *parseUnionDeclaration();
    ASTNode *parseUnionTypedefDeclaration();

    // 列挙型宣言
    ASTNode *parseEnumDeclaration();
    ASTNode *parseEnumTypedefDeclaration();

    // 構造体メンバーの解析
    void parseStructMembers(StructDefinition *struct_def);
    void parseUnionMembers(UnionDefinition *union_def);

    // 循環参照の検出
    void detectCircularReference(const std::string &struct_name,
                                 const std::string &member_type,
                                 int pointer_level);

  private:
    RecursiveParser *parser_;
};

#endif // STRUCT_PARSER_H
