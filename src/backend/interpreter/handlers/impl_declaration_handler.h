#ifndef CB_INTERPRETER_IMPL_DECLARATION_HANDLER_H
#define CB_INTERPRETER_IMPL_DECLARATION_HANDLER_H

class ASTNode;
class Interpreter;

/**
 * @brief impl宣言の処理を管理するクラス
 *
 * AST_IMPL_DECL の処理を担当。
 * 実際には何もしない(register_global_declarations()で既に処理済み)。
 */
class ImplDeclarationHandler {
  public:
    explicit ImplDeclarationHandler(Interpreter *interpreter);

    /**
     * @brief impl宣言(AST_IMPL_DECL)を処理
     * @param node AST_IMPL_DECL ノード
     *
     * 実行時には何もしない(既に登録済み)。
     */
    void handle_impl_declaration(const ASTNode *node);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_IMPL_DECLARATION_HANDLER_H
