#include "return_handler.h"
#include "../core/interpreter.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../evaluator/expression_evaluator.h"
#include <stdexcept>

// return文の実行
void ReturnHandler::execute_return_statement(const ASTNode *node) {
    debug_msg(DebugMsgId::INTERPRETER_RETURN_STMT);
    
    if (!node->left) {
        // return値なし
        return;
    }
    
    debug_msg(DebugMsgId::INTERPRETER_RETURN_STMT);
    
    // ノードタイプに応じて処理を分岐
    switch (node->left->node_type) {
    case ASTNodeType::AST_ARRAY_LITERAL:
        handle_array_literal_return(node);
        break;
        
    case ASTNodeType::AST_STRING_LITERAL:
        throw ReturnException(node->left->str_value);
        break;
        
    case ASTNodeType::AST_IDENTIFIER:
        handle_identifier_return(node);
        break;
        
    case ASTNodeType::AST_VARIABLE:
        handle_variable_return(node);
        break;
        
    default:
        // その他の式を評価して返す
        {
            int64_t value = interpreter_->expression_evaluator_->evaluate_expression(
                node->left.get());
            throw ReturnException(value);
        }
        break;
    }
}

// 配列リテラルのreturn処理
void ReturnHandler::handle_array_literal_return(const ASTNode *node) {
    const std::vector<std::unique_ptr<ASTNode>> &elements =
        node->left->arguments;
    debug_msg(DebugMsgId::INTERPRETER_RETURN_ARRAY, elements.size());
    
    // 配列リテラルを処理
    std::vector<int64_t> array_values;
    std::vector<std::string> array_strings;
    bool is_string_array = false;

    // 最初の要素で型を判定
    if (!elements.empty()) {
        if (elements[0]->node_type == ASTNodeType::AST_STRING_LITERAL) {
            is_string_array = true;
        } else if (elements[0]->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 多次元配列リテラルの場合
            const auto &nested_elements = elements[0]->arguments;
            if (!nested_elements.empty() &&
                nested_elements[0]->node_type == ASTNodeType::AST_STRING_LITERAL) {
                is_string_array = true;
            }
        }
    }

    // 多次元配列リテラルの場合の特別処理
    if (!elements.empty() &&
        elements[0]->node_type == ASTNodeType::AST_ARRAY_LITERAL) {

        if (is_string_array) {
            // 多次元文字列配列を3D形式に変換
            std::vector<std::vector<std::vector<std::string>>> str_array_3d;
            std::vector<std::vector<std::string>> str_array_2d;

            for (const auto &row_element : elements) {
                if (row_element->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
                    throw std::runtime_error("Expected nested array literal");
                }

                std::vector<std::string> row;
                for (const auto &cell_element : row_element->arguments) {
                    if (cell_element->node_type != ASTNodeType::AST_STRING_LITERAL) {
                        throw std::runtime_error(
                            "Expected string literal in multidim array");
                    }
                    row.push_back(cell_element->str_value);
                }
                str_array_2d.push_back(row);
            }
            str_array_3d.push_back(str_array_2d);

            throw ReturnException(
                str_array_3d, "string[][]",
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING));
        } else {
            // 多次元整数配列を3D形式に変換
            std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
            std::vector<std::vector<int64_t>> int_array_2d;

            for (const auto &row_element : elements) {
                if (row_element->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
                    throw std::runtime_error("Expected nested array literal");
                }

                std::vector<int64_t> row;
                for (const auto &cell_element : row_element->arguments) {
                    int64_t value = interpreter_->expression_evaluator_->evaluate_expression(
                        cell_element.get());
                    row.push_back(value);
                }
                int_array_2d.push_back(row);
            }
            int_array_3d.push_back(int_array_2d);

            throw ReturnException(int_array_3d, "int[][]", TYPE_INT);
        }
    }

    // 全要素を評価
    for (size_t i = 0; i < elements.size(); i++) {
        const auto &element = elements[i];
        if (is_string_array) {
            if (element->node_type != ASTNodeType::AST_STRING_LITERAL) {
                throw std::runtime_error(
                    "Type mismatch in array literal return: expected string");
            }
            array_strings.push_back(element->str_value);
        } else {
            if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                throw std::runtime_error(
                    "Type mismatch in array literal return: expected number");
            }
            int64_t value = interpreter_->expression_evaluator_->evaluate_expression(
                element.get());
            array_values.push_back(value);
        }
    }

    // ReturnExceptionで配列を返す
    if (is_string_array) {
        std::vector<std::vector<std::vector<std::string>>> str_array_3d;
        std::vector<std::vector<std::string>> str_array_2d;
        str_array_2d.push_back(array_strings);
        str_array_3d.push_back(str_array_2d);
        throw ReturnException(str_array_3d, "string[]", TYPE_STRING);
    } else {
        std::vector<std::vector<std::vector<int64_t>>> int_array_3d;
        std::vector<std::vector<int64_t>> int_array_2d;
        int_array_2d.push_back(array_values);
        int_array_3d.push_back(int_array_2d);
        throw ReturnException(int_array_3d, "int[]", TYPE_INT);
    }
}

// 識別子のreturn処理（変数、self等）
void ReturnHandler::handle_identifier_return(const ASTNode *node) {
    // この部分は非常に大きいので、interpreter側で実装を続ける
    // 将来的にはこちらに完全移行すべき
    
    // 今回は単純な委譲に留める
    if (node->left->name == "self") {
        Variable *self_var = interpreter_->find_variable("self");
        if (self_var && self_var->is_struct) {
            interpreter_->sync_struct_members_from_direct_access("self");
            if (self_var->type != TYPE_INTERFACE) {
                self_var->type = TYPE_STRUCT;
            }
            throw ReturnException(*self_var);
        }
    } else {
        Variable *var = interpreter_->find_variable(node->left->name);
        if (var) {
            // 参照型チェック
            bool return_as_reference = false;
            if (!interpreter_->current_function_name.empty()) {
                const ASTNode *func =
                    interpreter_->global_scope.functions[interpreter_->current_function_name];
                if (func && func->return_type_name.find('&') != std::string::npos) {
                    return_as_reference = true;
                }
            }

            if (return_as_reference) {
                if (var->is_reference) {
                    Variable *target_var = reinterpret_cast<Variable *>(var->value);
                    throw ReturnException(target_var);
                } else {
                    throw ReturnException(var);
                }
            } else if (var->is_struct) {
                interpreter_->sync_struct_members_from_direct_access(node->left->name);
                if (var->type != TYPE_INTERFACE) {
                    var->type = TYPE_STRUCT;
                }
                throw ReturnException(*var);
            } else if (!var->interface_name.empty()) {
                Variable interface_copy = *var;
                interface_copy.type = TYPE_INTERFACE;
                throw ReturnException(interface_copy);
            } else if (var->type == TYPE_STRING) {
                throw ReturnException(var->str_value);
            } else if (var->type == TYPE_POINTER) {
                throw ReturnException(var->value);
            } else {
                throw ReturnException(var->value);
            }
        }
    }
}

// 変数のreturn処理（大部分はinterpreter側に残す）
void ReturnHandler::handle_variable_return(const ASTNode *node) {
    // この処理は非常に複雑なので、今回は元のinterpreterに委譲
    // 完全な移行は後のフェーズで実施
    throw std::runtime_error("Variable return handling not yet fully migrated");
}
