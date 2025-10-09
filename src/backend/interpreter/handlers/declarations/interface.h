#ifndef CB_INTERPRETER_INTERFACE_DECLARATION_HANDLER_H
#define CB_INTERPRETER_INTERFACE_DECLARATION_HANDLER_H

struct ASTNode;
class Interpreter;

/**
 * @brief インターフェース宣言の処理を管理するクラス
 *
 * AST_INTERFACE_DECL の処理を担当。
 * execute_statement から分離して単一責任原則を実現。
 */
class InterfaceDeclarationHandler {
  public:
    explicit InterfaceDeclarationHandler(Interpreter *interpreter);

    /**
     * @brief インターフェース宣言(AST_INTERFACE_DECL)を処理
     * @param node AST_INTERFACE_DECL ノード
     *
     * インターフェース定義をグローバルスコープに登録する。
     */
    void handle_interface_declaration(const ASTNode *node);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_INTERFACE_DECLARATION_HANDLER_H
