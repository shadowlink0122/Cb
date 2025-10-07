#ifndef FUNCTION_DECLARATION_HANDLER_H
#define FUNCTION_DECLARATION_HANDLER_H

struct ASTNode;
class Interpreter;

/**
 * FunctionDeclarationHandler
 * 関数宣言(AST_FUNC_DECL)の処理を担当
 */
class FunctionDeclarationHandler {
private:
    Interpreter* interpreter_;

public:
    explicit FunctionDeclarationHandler(Interpreter* interp);
    
    /**
     * 関数宣言を処理
     * @param node 関数宣言ASTノード
     */
    void handle_function_declaration(const ASTNode* node);
};

#endif // FUNCTION_DECLARATION_HANDLER_H
