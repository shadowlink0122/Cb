#include "statement_executor.h"
#include "../interpreter.h"
#include "../error_handler.h"
#include "../../common/debug.h"
#include "../../common/type_alias.h"

StatementExecutor::StatementExecutor(Interpreter& interpreter) : interpreter_(interpreter) {}

void StatementExecutor::execute_statement(const ASTNode *node) {
    execute(node);
}

void StatementExecutor::execute(const ASTNode *node) {
    if (!node) return;

    switch (node->node_type) {
        case ASTNodeType::AST_ASSIGN: {
            execute_assignment(node);
            break;
        }
        case ASTNodeType::AST_VAR_DECL: {
            execute_variable_declaration(node);
            break;
        }
        // 他のノードタイプは後で実装
        default:
            // TODO: 他のstatement types
            break;
    }
}

void StatementExecutor::execute_assignment(const ASTNode *node) {
    if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列要素への代入
        int64_t rvalue = interpreter_.evaluate_expression(node->right.get());
        
        // 多次元配列アクセスかチェック
        if (node->left->left && node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 多次元配列要素への代入
            throw std::runtime_error("Multidimensional array assignment not yet implemented");
        } else {
            // 単一次元配列要素への代入
            int64_t index_value = interpreter_.evaluate_expression(node->left->array_index.get());
            int index = static_cast<int>(index_value);
            
            std::string var_name;
            if (node->left->left && node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->left->name;
            } else if (!node->left->name.empty()) {
                var_name = node->left->name;
            } else {
                throw std::runtime_error("Invalid array reference in assignment");
            }
            
            Variable *var = interpreter_.find_variable(var_name);
            if (!var) {
                // 詳細なエラー表示
                print_error_with_ast_location(
                    "Undefined variable '" + var_name + "'", 
                    node);
                    
                throw DetailedErrorException("Undefined variable: " + var_name);
            }
            
            if (var->type == TYPE_STRING) {
                interpreter_.assign_string_element(var_name, index, 
                                                 std::string(1, static_cast<char>(rvalue)));
            } else {
                interpreter_.assign_array_element(var_name, index, rvalue);
            }
        }
    } else {
        // 通常の変数代入
        int64_t value = interpreter_.evaluate_expression(node->right.get());
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            interpreter_.assign_variable(node->name, node->right->str_value);
        } else {
            interpreter_.assign_variable(node->name, value, node->type_info);
        }
    }
}

void StatementExecutor::execute_variable_declaration(const ASTNode *node) {
    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_array = false;

    // 型を確定する
    if (node->type_info == TYPE_UNKNOWN && !node->str_value.empty()) {
        // 単純な型エイリアス解決
        var.type = TYPE_INT; // デフォルト
    } else {
        var.type = node->type_info;
    }

    // 初期化
    if (node->right) {
        int64_t value = interpreter_.evaluate_expression(node->right.get());
        if (var.type == TYPE_STRING) {
            var.str_value = node->right->str_value;
        } else {
            var.value = value;
            interpreter_.check_type_range(var.type, value, node->name);
        }
        var.is_assigned = true;
    }

    // 変数を現在のスコープに登録
    interpreter_.current_scope().variables[node->name] = var;
}
