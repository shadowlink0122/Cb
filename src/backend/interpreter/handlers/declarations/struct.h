#ifndef CB_INTERPRETER_STRUCT_DECLARATION_HANDLER_H
#define CB_INTERPRETER_STRUCT_DECLARATION_HANDLER_H

struct ASTNode;
class Interpreter;

/**
 * @brief 構造体宣言の処理を管理するクラス
 *
 * AST_STRUCT_DECL と AST_STRUCT_TYPEDEF_DECL の処理を担当。
 * execute_statement から分離して単一責任原則を実現。
 */
class StructDeclarationHandler {
  public:
    explicit StructDeclarationHandler(Interpreter *interpreter);

    /**
     * @brief 構造体宣言(AST_STRUCT_DECL, AST_STRUCT_TYPEDEF_DECL)を処理
     * @param node AST_STRUCT_DECL または AST_STRUCT_TYPEDEF_DECL ノード
     *
     * 構造体定義をグローバルスコープに登録する。
     */
    void handle_struct_declaration(const ASTNode *node);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_STRUCT_DECLARATION_HANDLER_H
