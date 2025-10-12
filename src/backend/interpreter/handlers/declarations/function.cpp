#include "function.h"
#include "../../../../common/ast.h"
#include "../../core/interpreter.h"

FunctionDeclarationHandler::FunctionDeclarationHandler(Interpreter *interp)
    : interpreter_(interp) {}

void FunctionDeclarationHandler::handle_function_declaration(
    const ASTNode *node) {
    // v0.11.0: namespace内の関数をNamespaceRegistryに登録
    auto *registry = interpreter_->get_namespace_registry();
    std::string function_key = node->name; // デフォルトは関数名のみ

    if (registry) {
        std::string current_ns = registry->getCurrentNamespace();

        if (interpreter_->debug_mode) {
            std::cerr << "[FUNC_DECL] Registering function: " << node->name
                      << ", current_namespace: '" << current_ns << "'"
                      << std::endl;
        }

        if (!current_ns.empty()) {
            // namespace内にいる場合、完全修飾名で登録
            function_key = current_ns + "::" + node->name;

            if (interpreter_->debug_mode) {
                std::cerr << "[FUNC_DECL] Using qualified name: "
                          << function_key << std::endl;
            }

            // NamespaceRegistryにもシンボルとして登録
            registry->registerSymbol(node->name, const_cast<ASTNode *>(node));
        }
    }

    // 実行時の関数定義をグローバルスコープに登録
    // namespace内の関数は完全修飾名で登録
    // const_castを使用してmapに格納（関数定義は後で変更される可能性があるため）
    interpreter_->global_scope.functions[function_key] =
        const_cast<ASTNode *>(node);

    if (interpreter_->debug_mode) {
        std::cerr << "[FUNC_DECL] Registered function with key: "
                  << function_key << std::endl;
    }
}
