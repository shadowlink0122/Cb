#include "interpreter.h"
#include "../common/ast.h"
#include "../frontend/debug.h"
#include <cctype>
#include <codecvt>
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <stdexcept>

// UTF-8文字列処理用のヘルパー関数
namespace {
// UTF-8バイト数を取得
int utf8_char_length(unsigned char byte) {
    if (byte < 0x80)
        return 1; // ASCII
    if ((byte >> 5) == 0x06)
        return 2; // 110xxxxx
    if ((byte >> 4) == 0x0E)
        return 3; // 1110xxxx
    if ((byte >> 3) == 0x1E)
        return 4; // 11110xxx
    return 1;     // 不正なバイトの場合は1バイトとして扱う
}

// UTF-8文字列の文字数をカウント
size_t utf8_char_count(const std::string &str) {
    size_t count = 0;
    for (size_t i = 0; i < str.size();) {
        int len = utf8_char_length(static_cast<unsigned char>(str[i]));
        i += len;
        count++;
    }
    return count;
}

// UTF-8文字列の指定位置の文字を取得
std::string utf8_char_at(const std::string &str, size_t index) {
    size_t current_index = 0;
    for (size_t i = 0; i < str.size();) {
        int len = utf8_char_length(static_cast<unsigned char>(str[i]));
        if (current_index == index) {
            return str.substr(i, len);
        }
        i += len;
        current_index++;
    }
    return ""; // 範囲外
}

// UTF-8文字の最初のバイトを整数として返す（従来の互換性のため）
int64_t utf8_char_to_int(const std::string &utf8_char) {
    if (utf8_char.empty())
        return 0;
    return static_cast<int64_t>(static_cast<unsigned char>(utf8_char[0]));
}
} // namespace

Interpreter::Interpreter(bool debug) : debug_mode(debug) {
    // 環境変数からデバッグモード設定
    const char *env_debug = std::getenv("CB_DEBUG_MODE");
    if (env_debug && env_debug[0] == '1') {
        debug_mode = true;
    }

    // グローバルスコープを初期化
    scope_stack.push_back(global_scope);
}

void Interpreter::push_scope() { scope_stack.push_back(Scope{}); }

void Interpreter::pop_scope() {
    if (scope_stack.size() > 1) {
        scope_stack.pop_back();
    }
}

Scope &Interpreter::current_scope() { return scope_stack.back(); }

Variable *Interpreter::find_variable(const std::string &name) {
    // ローカルスコープから検索
    for (auto it = scope_stack.rbegin(); it != scope_stack.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            return &var_it->second;
        }
    }

    // グローバルスコープから検索
    auto global_var_it = global_scope.variables.find(name);
    if (global_var_it != global_scope.variables.end()) {
        return &global_var_it->second;
    }

    return nullptr;
}

const ASTNode *Interpreter::find_function(const std::string &name) {
    // グローバルスコープの関数を検索
    auto func_it = global_scope.functions.find(name);
    if (func_it != global_scope.functions.end()) {
        return func_it->second;
    }
    return nullptr;
}

void Interpreter::register_global_declarations(const ASTNode *node) {
    if (!node)
        return;

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        for (const auto &stmt : node->statements) {
            register_global_declarations(stmt.get());
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        if (node->node_type == ASTNodeType::AST_ASSIGN) {
            // グローバル変数の重複宣言チェック
            if (global_scope.variables.find(node->name) !=
                global_scope.variables.end()) {
                error_msg(DebugMsgId::VAR_REDECLARE_ERROR, node->name.c_str());
                throw std::runtime_error("Variable redeclaration error");
            }

            // グローバル変数の初期化
            Variable var;
            var.type =
                node->type_info != TYPE_VOID ? node->type_info : TYPE_INT;
            var.is_const = node->is_const;
            var.is_assigned = false;

            if (node->right) {
                int64_t value = evaluate_expression(node->right.get());
                if (var.type == TYPE_STRING) {
                    var.str_value = node->right->str_value;
                } else {
                    var.value = value;
                    check_type_range(var.type, value, node->name);
                }
                var.is_assigned = true;
            }

            global_scope.variables[node->name] = var;
        } else if (node->node_type == ASTNodeType::AST_VAR_DECL) {
            // グローバル変数の重複宣言チェック
            if (global_scope.variables.find(node->name) !=
                global_scope.variables.end()) {
                error_msg(DebugMsgId::VAR_REDECLARE_ERROR, node->name.c_str());
                throw std::runtime_error("Variable redeclaration error");
            }

            Variable var;
            var.type = node->type_info;
            var.is_const = node->is_const;
            var.is_assigned = false;
            global_scope.variables[node->name] = var;
        }
        break;

    case ASTNodeType::AST_ARRAY_DECL: {
        Variable var;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
        var.is_const = node->is_const;
        var.is_array = true;
        var.is_assigned = false;

        // 配列サイズ決定
        if (node->array_size_expr) {
            var.array_size = static_cast<int>(
                evaluate_expression(node->array_size_expr.get()));
        } else {
            var.array_size = node->array_size;
        }

        if (var.array_size < 0) {
            error_msg(DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR,
                      node->name.c_str());
            throw std::runtime_error("Negative array size error");
        }

        // 配列初期化
        TypeInfo elem_type = node->type_info;
        if (elem_type == TYPE_STRING) {
            var.array_strings.resize(var.array_size, "");
        } else {
            var.array_values.resize(var.array_size, 0);
        }

        // 初期化リストがある場合
        for (size_t i = 0; i < node->children.size() &&
                           i < static_cast<size_t>(var.array_size);
             ++i) {
            const auto &child = node->children[i];
            if (child->node_type == ASTNodeType::AST_STMT_LIST) {
                // 配列リテラル [1,2,3,...] の場合
                size_t j = 0;
                for (const auto &element : child->children) {
                    if (j >= static_cast<size_t>(var.array_size))
                        break;
                    if (elem_type == TYPE_STRING) {
                        if (element->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            var.array_strings[j] = element->str_value;
                        } else {
                            var.array_strings[j] = ""; // デフォルト値
                        }
                    } else {
                        int64_t val = evaluate_expression(element.get());
                        check_type_range(elem_type, val, node->name);
                        var.array_values[j] = val;
                    }
                    j++;
                }
                break; // 配列リテラルは一つだけ
            } else {
                // 単一要素の初期化
                if (elem_type == TYPE_STRING) {
                    var.array_strings[i] = child->str_value;
                } else {
                    int64_t val = evaluate_expression(child.get());
                    check_type_range(elem_type, val, node->name);
                    var.array_values[i] = val;
                }
            }
        }

        current_scope().variables[node->name] = var;
    } break;

    case ASTNodeType::AST_FUNC_DECL:
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER, node->name.c_str());
        global_scope.functions[node->name] = node;
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE, node->name.c_str());
        break;

    default:
        break;
    }
}

void Interpreter::process(const ASTNode *ast) {
    debug_msg(DebugMsgId::INTERPRETER_START);
    if (!ast) {
        debug_msg(DebugMsgId::AST_IS_NULL);
        return;
    }

    debug_msg(DebugMsgId::GLOBAL_DECL_START);
    // まずグローバル宣言を登録
    register_global_declarations(ast);
    debug_msg(DebugMsgId::GLOBAL_DECL_COMPLETE);

    debug_msg(DebugMsgId::MAIN_FUNC_SEARCH);
    // main関数を探して実行
    const ASTNode *main_func = find_function("main");
    if (!main_func) {
        error_msg(DebugMsgId::MAIN_FUNC_NOT_FOUND_ERROR);
        throw std::runtime_error("Main function not found");
    }
    debug_msg(DebugMsgId::MAIN_FUNC_FOUND);

    try {
        push_scope();
        execute_statement(main_func->body.get());
        pop_scope();
    } catch (const ReturnException &e) {
        debug_msg(DebugMsgId::MAIN_FUNC_EXIT, e.value);
    }
}

int64_t Interpreter::evaluate(const ASTNode *node) {
    return evaluate_expression(node);
}

void Interpreter::execute_statement(const ASTNode *node) {
    if (!node)
        return;

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        for (const auto &stmt : node->statements) {
            execute_statement(stmt.get());
        }
        break;

    case ASTNodeType::AST_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        if (node->node_type == ASTNodeType::AST_ASSIGN) {
            if (node->left &&
                node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 配列要素への代入
                int64_t index =
                    evaluate_expression(node->left->array_index.get());

                // 変数の型を確認して文字列か配列かを判断
                Variable *var = find_variable(node->left->name);
                if (var && var->type == TYPE_STRING) {
                    // 文字列要素への代入
                    if (node->right->node_type ==
                        ASTNodeType::AST_STRING_LITERAL) {
                        assign_string_element(node->left->name, index,
                                              node->right->str_value);
                    } else {
                        error_msg(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR);
                        throw std::runtime_error(
                            "Non-string character assignment error");
                    }
                } else {
                    // 通常の配列要素への代入
                    int64_t value = evaluate_expression(node->right.get());
                    assign_array_element(node->left->name, index, value);
                }
            } else {
                // 通常の代入
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    assign_variable(node->name, node->right->str_value,
                                    node->is_const);
                } else {
                    int64_t value = evaluate_expression(node->right.get());
                    assign_variable(node->name, value, node->type_info,
                                    node->is_const);
                }
            }
        } else {
            // 変数宣言のみ
            Variable var;
            var.type = node->type_info;
            var.is_const = node->is_const;
            var.is_assigned = false;
            global_scope.variables[node->name] = var;
        }
        break;

    case ASTNodeType::AST_ARRAY_DECL: {
        Variable var;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
        var.is_const = node->is_const;
        var.is_array = true;
        var.is_assigned = false;

        // 配列サイズ決定
        if (node->array_size_expr) {
            var.array_size = static_cast<int>(
                evaluate_expression(node->array_size_expr.get()));
        } else {
            var.array_size = node->array_size;
        }

        if (var.array_size < 0) {
            error_msg(DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR,
                      node->name.c_str());
            throw std::runtime_error("Negative array size error");
        }

        // 配列初期化
        TypeInfo elem_type = node->type_info;
        if (elem_type == TYPE_STRING) {
            var.array_strings.resize(var.array_size, "");
        } else {
            var.array_values.resize(var.array_size, 0);
        }

        // 初期化リストがある場合
        for (size_t i = 0; i < node->children.size() &&
                           i < static_cast<size_t>(var.array_size);
             ++i) {
            const auto &child = node->children[i];
            if (child->node_type == ASTNodeType::AST_STMT_LIST) {
                // 配列リテラル [1,2,3,...] の場合
                size_t j = 0;
                for (const auto &element : child->children) {
                    if (j >= static_cast<size_t>(var.array_size))
                        break;
                    if (elem_type == TYPE_STRING) {
                        var.array_strings[j] = element->str_value;
                    } else {
                        int64_t val = evaluate_expression(element.get());
                        check_type_range(elem_type, val, node->name);
                        var.array_values[j] = val;
                    }
                    j++;
                }
                break; // 配列リテラルは一つだけ
            } else {
                // 単一要素の初期化
                if (elem_type == TYPE_STRING) {
                    var.array_strings[i] = child->str_value;
                } else {
                    int64_t val = evaluate_expression(child.get());
                    check_type_range(elem_type, val, node->name);
                    var.array_values[i] = val;
                }
            }
        }

        current_scope().variables[node->name] = var;
    } break;

    case ASTNodeType::AST_PRINT_STMT:
        print_value(node->left.get());
        break;

    case ASTNodeType::AST_PRINTLN_STMT:
        print_value_with_newline(node->left.get());
        break;

    case ASTNodeType::AST_PRINTLN_EMPTY:
        std::cout << std::endl;
        break;

    case ASTNodeType::AST_PRINTLN_MULTI_STMT:
        print_multiple_with_newline(node->right.get());
        break;

    case ASTNodeType::AST_PRINTLNF_STMT:
        print_formatted_with_newline(node->left.get(), node->right.get());
        break;

    case ASTNodeType::AST_PRINTF_STMT:
        print_formatted(node->left.get(), node->right.get());
        break;

    case ASTNodeType::AST_PRINT_MULTI_STMT:
        print_multiple(node->right.get());
        break;

    case ASTNodeType::AST_IF_STMT: {
        int64_t cond = evaluate_expression(node->condition.get());
        if (cond) {
            execute_statement(node->left.get());
        } else if (node->right) {
            execute_statement(node->right.get());
        }
    } break;

    case ASTNodeType::AST_WHILE_STMT:
        try {
            while (true) {
                int64_t cond = evaluate_expression(node->condition.get());
                if (!cond)
                    break;
                execute_statement(node->body.get());
            }
        } catch (const BreakException &e) {
            // break文でループ脱出
        }
        break;

    case ASTNodeType::AST_FOR_STMT:
        try {
            if (node->init_expr) {
                execute_statement(node->init_expr.get());
            }
            while (true) {
                if (node->condition) {
                    int64_t cond = evaluate_expression(node->condition.get());
                    if (!cond)
                        break;
                }
                execute_statement(node->body.get());
                if (node->update_expr) {
                    execute_statement(node->update_expr.get());
                }
            }
        } catch (const BreakException &e) {
            // break文でループ脱出
        }
        break;

    case ASTNodeType::AST_RETURN_STMT:
        if (node->left) {
            if (node->left->node_type == ASTNodeType::AST_STRING_LITERAL) {
                throw ReturnException(node->left->str_value);
            } else {
                int64_t value = evaluate_expression(node->left.get());
                throw ReturnException(value);
            }
        } else {
            throw ReturnException(0);
        }
        break;

    case ASTNodeType::AST_BREAK_STMT: {
        int64_t cond = 1;
        if (node->left) {
            cond = evaluate_expression(node->left.get());
        }
        if (cond) {
            throw BreakException(cond);
        }
    } break;

    case ASTNodeType::AST_FUNC_DECL:
        // 実行時の関数定義をグローバルスコープに登録
        global_scope.functions[node->name] = node;
        break;

    default:
        evaluate_expression(node); // 式文として評価
        break;
    }
}

int64_t Interpreter::evaluate_expression(const ASTNode *node) {
    if (!node)
        return 0;

    switch (node->node_type) {
    case ASTNodeType::AST_NUMBER:
        debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, node->int_value);
        return node->int_value;

    case ASTNodeType::AST_STRING_LITERAL:
        // 文字列リテラルのデバッグ出力
        debug_msg(DebugMsgId::STRING_LITERAL_DEBUG, node->str_value.c_str());
        // 文字列は特別な値として0を返す
        return 0;

    case ASTNodeType::AST_VARIABLE: {
        debug_msg(DebugMsgId::EXPR_EVAL_VAR_REF, node->name.c_str());
        Variable *var = find_variable(node->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined variable");
        }
        if (var->is_array) {
            error_msg(DebugMsgId::DIRECT_ARRAY_REF_ERROR, node->name.c_str());
            throw std::runtime_error("Direct array reference error");
        }
        debug_msg(DebugMsgId::VAR_VALUE, var->value);
        return var->value;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF, node->name.c_str());
        Variable *var = find_variable(node->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_ARRAY_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined array");
        }

        int64_t index = evaluate_expression(node->array_index.get());
        debug_msg(DebugMsgId::ARRAY_INDEX, index);

        if (var->type == TYPE_STRING) {
            // 文字列の個別文字アクセス（UTF-8対応）
            debug_msg(DebugMsgId::STRING_ELEMENT_ACCESS);

            // UTF-8文字数で範囲チェック
            size_t utf8_length = utf8_char_count(var->str_value);
            debug_msg(DebugMsgId::STRING_LENGTH_UTF8, utf8_length);

            if (index < 0 || index >= static_cast<int64_t>(utf8_length)) {
                error_msg(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR,
                          node->name.c_str(), index, utf8_length);
                throw std::runtime_error("String out of bounds access");
            }

            // UTF-8文字を取得
            std::string utf8_char =
                utf8_char_at(var->str_value, static_cast<size_t>(index));
            int64_t result = utf8_char_to_int(utf8_char);

            debug_msg(DebugMsgId::STRING_ELEMENT_VALUE, result,
                      utf8_char.c_str());
            return result;
        } else if (var->is_array) {
            // 通常の配列アクセス
            debug_msg(DebugMsgId::ARRAY_ELEMENT_ACCESS);
            if (index < 0 || index >= var->array_size) {
                error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR,
                          node->name.c_str());
                throw std::runtime_error("Array out of bounds access");
            }

            TypeInfo elem_type =
                static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
            if (elem_type == TYPE_STRING) {
                // 文字列配列の場合、文字列をint値として返す（仮実装）
                // 実際には文字列を返すべきだが、print関数が文字列を処理できるように修正が必要
                if (index >= 0 &&
                    index < static_cast<int64_t>(var->array_strings.size())) {
                    // 文字列の最初の文字のUTF-8コードポイントを返す（デモ用）
                    std::string &str = var->array_strings[index];
                    if (!str.empty()) {
                        return static_cast<int64_t>(str[0]);
                    } else {
                        return 0;
                    }
                } else {
                    return 0;
                }
            } else {
                int64_t result = var->array_values[index];
                debug_msg(DebugMsgId::ARRAY_ELEMENT_VALUE, result);
                return result;
            }
        } else {
            error_msg(DebugMsgId::NON_ARRAY_REF_ERROR, node->name.c_str());
            throw std::runtime_error("Non-array reference error");
        }
    }

    case ASTNodeType::AST_BINARY_OP: {
        debug_msg(DebugMsgId::EXPR_EVAL_BINARY_OP, node->op.c_str());
        int64_t left = evaluate_expression(node->left.get());
        int64_t right = evaluate_expression(node->right.get());
        debug_msg(DebugMsgId::BINARY_OP_VALUES, left, right);

        int64_t result = 0;
        if (node->op == "+")
            result = left + right;
        else if (node->op == "-")
            result = left - right;
        else if (node->op == "*")
            result = left * right;
        else if (node->op == "/") {
            if (right == 0) {
                error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
                throw std::runtime_error("Division by zero");
            }
            result = left / right;
        } else if (node->op == "%") {
            if (right == 0) {
                error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
                throw std::runtime_error("Division by zero");
            }
            result = left % right;
        } else if (node->op == "==")
            result = (left == right) ? 1 : 0;
        else if (node->op == "!=")
            result = (left != right) ? 1 : 0;
        else if (node->op == "<")
            result = (left < right) ? 1 : 0;
        else if (node->op == ">")
            result = (left > right) ? 1 : 0;
        else if (node->op == "<=")
            result = (left <= right) ? 1 : 0;
        else if (node->op == ">=")
            result = (left >= right) ? 1 : 0;
        else if (node->op == "&&")
            result = ((left != 0) && (right != 0)) ? 1 : 0;
        else if (node->op == "||")
            result = ((left != 0) || (right != 0)) ? 1 : 0;
        else {
            error_msg(DebugMsgId::UNKNOWN_BINARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown binary operator");
        }

        debug_msg(DebugMsgId::BINARY_OP_RESULT_DEBUG, result);
        return result;
    }

    case ASTNodeType::AST_UNARY_OP: {
        debug_msg(DebugMsgId::UNARY_OP_DEBUG, node->op.c_str());
        int64_t operand = evaluate_expression(node->left.get());
        debug_msg(DebugMsgId::UNARY_OP_OPERAND_DEBUG, operand);

        int64_t result = 0;
        if (node->op == "!")
            result = (operand == 0) ? 1 : 0;
        else if (node->op == "-")
            result = -operand;
        else {
            error_msg(DebugMsgId::UNKNOWN_UNARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown unary operator");
        }

        debug_msg(DebugMsgId::UNARY_OP_RESULT_DEBUG, result);
        return result;
    }

    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        Variable *var = find_variable(node->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined variable");
        }

        int64_t old_value = var->value;
        if (node->op == "++") {
            var->value += 1;
        } else if (node->op == "--") {
            var->value -= 1;
        }

        check_type_range(var->type, var->value, node->name);

        return (node->node_type == ASTNodeType::AST_PRE_INCDEC) ? var->value
                                                                : old_value;
    }

    case ASTNodeType::AST_FUNC_CALL: {
        const ASTNode *func = find_function(node->name);
        if (!func) {
            error_msg(DebugMsgId::UNDEFINED_FUNC_ERROR, node->name.c_str());
            throw std::runtime_error("Undefined function");
        }

        // 引数の数チェック
        if (node->arguments.size() != func->parameters.size()) {
            error_msg(DebugMsgId::ARG_COUNT_MISMATCH_ERROR, node->name.c_str());
            throw std::runtime_error("Argument count mismatch");
        }

        // ローカルスコープ作成
        push_scope();

        // 引数を評価してパラメータに束縛
        for (size_t i = 0; i < func->parameters.size(); ++i) {
            int64_t arg_value = evaluate_expression(node->arguments[i].get());
            Variable param;
            param.type = func->parameters[i]->type_info;
            param.value = arg_value;
            param.is_assigned = true;
            current_scope().variables[func->parameters[i]->name] = param;
        }

        try {
            execute_statement(func->body.get());
            pop_scope();
            return 0; // void関数
        } catch (const ReturnException &e) {
            pop_scope();
            return e.value;
        }
    }

    case ASTNodeType::AST_ARRAY_DECL:
        // 配列宣言は式として評価できない
        // デバッグ用：どこから呼び出されたかを調べる
        debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG, node->name.c_str());
        error_msg(DebugMsgId::ARRAY_DECL_AS_EXPR_ERROR, node->name.c_str());
        throw std::runtime_error("Array declaration as expression error");

    case ASTNodeType::AST_STMT_LIST:
        // 配列リテラル処理（パーサーがcreate_array_literalでAST_STMT_LISTを使用）
        {
            // これは配列リテラル [1,2,3,...] として扱う
            // node->childrenには各要素のノードが含まれている
            Variable result;
            result.array_values.clear();

            // 各要素を評価して配列に追加
            for (auto &child : node->children) {
                int64_t element_value = evaluate_expression(child.get());
                result.array_values.push_back(element_value);
            }

            // 配列として返す（値としては0）
            return 0;
        }

    default:
        // デバッグ用: どのノード型が未対応かを表示
        std::string node_type_str =
            "unknown(" + std::to_string(static_cast<int>(node->node_type)) +
            ")";
        error_msg(DebugMsgId::UNSUPPORTED_EXPR_NODE_ERROR,
                  node_type_str.c_str());
        throw std::runtime_error("Unsupported expression node");
    }
}

void Interpreter::assign_variable(const std::string &name, int64_t value,
                                  TypeInfo type) {
    Variable *var = find_variable(name);
    if (!var) {
        // 新しい変数を作成
        Variable new_var;
        new_var.type = type;
        new_var.value = value;
        new_var.is_assigned = true;
        new_var.is_const = false; // デフォルトはnon-const
        check_type_range(type, value, name);
        current_scope().variables[name] = new_var;
    } else {
        if (var->is_const && var->is_assigned) {
            std::cerr << "再代入できません: " << name << std::endl;
            std::exit(1);
        }
        if (var->is_array) {
            error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR, name.c_str());
            throw std::runtime_error("Direct array assignment error");
        }
        check_type_range(var->type, value, name);
        var->value = value;
        var->is_assigned = true;
    }
}

void Interpreter::assign_variable(const std::string &name, int64_t value,
                                  TypeInfo type, bool is_const) {
    debug_msg(DebugMsgId::VAR_ASSIGN_READABLE, name.c_str(), value,
              type_info_to_string(type), bool_to_string(is_const));
    Variable *var = find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::VAR_CREATE_NEW);
        // 新しい変数を作成
        Variable new_var;
        new_var.type = type;
        new_var.value = value;
        new_var.is_assigned = true;
        new_var.is_const = is_const;
        check_type_range(type, value, name);
        current_scope().variables[name] = new_var;
    } else {
        debug_msg(DebugMsgId::EXISTING_VAR_ASSIGN_DEBUG);
        if (var->is_const && var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
            std::exit(1);
        }
        if (var->is_array) {
            error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR, name.c_str());
            throw std::runtime_error("Direct array assignment error");
        }
        check_type_range(var->type, value, name);
        var->value = value;
        var->is_assigned = true;
    }
}

void Interpreter::assign_variable(const std::string &name,
                                  const std::string &value) {
    Variable *var = find_variable(name);
    if (!var) {
        Variable new_var;
        new_var.type = TYPE_STRING;
        new_var.str_value = value;
        new_var.is_assigned = true;
        current_scope().variables[name] = new_var;
    } else {
        if (var->is_const && var->is_assigned) {
            std::cerr << "再代入できません: " << name << std::endl;
            std::exit(1);
        }
        var->str_value = value;
        var->is_assigned = true;
    }
}

void Interpreter::assign_variable(const std::string &name,
                                  const std::string &value, bool is_const) {
    debug_msg(DebugMsgId::STRING_ASSIGN_READABLE, name.c_str(), value.c_str(),
              bool_to_string(is_const));
    Variable *var = find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::STRING_VAR_CREATE_NEW);
        Variable new_var;
        new_var.type = TYPE_STRING;
        new_var.str_value = value;
        new_var.is_assigned = true;
        new_var.is_const = is_const;
        current_scope().variables[name] = new_var;
    } else {
        debug_msg(DebugMsgId::EXISTING_STRING_VAR_ASSIGN_DEBUG);
        if (var->is_const && var->is_assigned) {
            error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
            std::exit(1);
        }
        var->str_value = value;
        var->is_assigned = true;
    }
}

void Interpreter::assign_array_element(const std::string &name, int64_t index,
                                       int64_t value) {
    Variable *var = find_variable(name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_ARRAY_ERROR, name.c_str());
        throw std::runtime_error("Undefined array");
    }
    if (!var->is_array) {
        error_msg(DebugMsgId::NON_ARRAY_REF_ERROR, name.c_str());
        throw std::runtime_error("Non-array reference");
    }
    if (var->is_const) {
        error_msg(DebugMsgId::CONST_ARRAY_ASSIGN_ERROR, name.c_str());
        throw std::runtime_error("Assignment to const array");
    }
    if (index < 0 || index >= var->array_size) {
        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, name.c_str());
        throw std::runtime_error("Array out of bounds");
    }

    TypeInfo elem_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
    check_type_range(elem_type, value, name);
    var->array_values[index] = value;
}

void Interpreter::assign_string_element(const std::string &name, int64_t index,
                                        const std::string &value) {
    debug_msg(DebugMsgId::STRING_ELEMENT_ASSIGN_DEBUG, name.c_str(), index,
              value.c_str());

    Variable *var = find_variable(name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("Undefined variable");
    }
    if (var->type != TYPE_STRING) {
        error_msg(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR);
        throw std::runtime_error("Non-string character assignment");
    }
    if (var->is_const) {
        error_msg(DebugMsgId::CONST_STRING_ELEMENT_ASSIGN_ERROR, name.c_str());
        std::exit(1);
    }

    // UTF-8文字数で範囲チェック
    size_t utf8_length = utf8_char_count(var->str_value);
    debug_msg(DebugMsgId::STRING_LENGTH_UTF8_DEBUG, utf8_length);

    if (index < 0 || index >= static_cast<int64_t>(utf8_length)) {
        error_msg(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR, name.c_str(), index,
                  utf8_length);
        throw std::runtime_error("String out of bounds");
    }

    // UTF-8文字列の指定位置の文字を置換
    // 新しい文字列を構築
    std::string new_string;
    size_t current_index = 0;
    for (size_t i = 0; i < var->str_value.size();) {
        int len =
            utf8_char_length(static_cast<unsigned char>(var->str_value[i]));

        if (current_index == static_cast<size_t>(index)) {
            // 置換対象の文字位置
            new_string += value;
            debug_msg(DebugMsgId::STRING_ELEMENT_REPLACE_DEBUG, index,
                      value.c_str());
        } else {
            // 既存の文字をコピー
            new_string += var->str_value.substr(i, len);
        }

        i += len;
        current_index++;
    }

    var->str_value = new_string;
    debug_msg(DebugMsgId::STRING_AFTER_REPLACE_DEBUG, var->str_value.c_str());
}

void Interpreter::print_value(const ASTNode *expr) {
    if (!expr) {
        std::cout << "(null)";
        return;
    }

    if (expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
        std::cout << expr->str_value;
    } else if (expr->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *var = find_variable(expr->name);
        if (var && var->type == TYPE_STRING) {
            std::cout << var->str_value;
        } else {
            int64_t value = evaluate_expression(expr);
            std::cout << value;
        }
    } else if (expr->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列アクセスの特別処理
        Variable *var = find_variable(expr->name);
        if (var && var->type == TYPE_STRING) {
            // 文字列要素アクセスの場合は文字として出力（UTF-8対応）
            int64_t index = evaluate_expression(expr->array_index.get());
            size_t utf8_length = utf8_char_count(var->str_value);

            if (index >= 0 && index < static_cast<int64_t>(utf8_length)) {
                std::string utf8_char =
                    utf8_char_at(var->str_value, static_cast<size_t>(index));
                std::cout << utf8_char;
            } else {
                error_msg(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR,
                          expr->name.c_str(), index, utf8_length);
                throw std::runtime_error("String out of bounds");
            }
        } else if (var && var->is_array) {
            // 配列アクセスの処理
            int64_t index = evaluate_expression(expr->array_index.get());

            if (index < 0 || index >= var->array_size) {
                error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR,
                          expr->name.c_str());
                throw std::runtime_error("Array out of bounds");
            }

            TypeInfo elem_type =
                static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
            if (elem_type == TYPE_STRING) {
                // 文字列配列の場合は文字列として出力
                if (index < static_cast<int64_t>(var->array_strings.size())) {
                    std::cout << var->array_strings[index];
                } else {
                    std::cout << "";
                }
            } else {
                // 数値配列は数値として出力
                int64_t value = var->array_values[index];
                std::cout << value;
            }
        } else {
            // 通常の配列アクセスは数値として出力
            int64_t value = evaluate_expression(expr);
            std::cout << value;
        }
    } else if (expr->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出しの特別処理
        const ASTNode *func = find_function(expr->name);
        if (func && func->type_info == TYPE_STRING) {
            // 文字列を返す関数の場合
            push_scope();

            // 引数を評価してパラメータに束縛
            for (size_t i = 0; i < func->parameters.size(); ++i) {
                int64_t arg_value =
                    evaluate_expression(expr->arguments[i].get());
                Variable param;
                param.type = func->parameters[i]->type_info;
                param.value = arg_value;
                param.is_assigned = true;
                current_scope().variables[func->parameters[i]->name] = param;
            }

            try {
                execute_statement(func->body.get());
                pop_scope();
                std::cout << ""; // void関数（空文字列）
            } catch (const ReturnException &e) {
                pop_scope();
                if (e.type == TYPE_STRING) {
                    std::cout << e.str_value;
                } else {
                    std::cout << e.value;
                }
            }
        } else {
            // 通常の関数（数値を返す）
            int64_t value = evaluate_expression(expr);
            std::cout << value;
        }
    } else {
        int64_t value = evaluate_expression(expr);
        std::cout << value;
    }
}

void Interpreter::print_value_with_newline(const ASTNode *expr) {
    print_value(expr);
    std::cout << std::endl;
}

void Interpreter::print_multiple_with_newline(const ASTNode *arg_list) {
    print_multiple(arg_list);
    std::cout << std::endl;
}

void Interpreter::print_formatted_with_newline(const ASTNode *format_str,
                                               const ASTNode *arg_list) {
    print_formatted(format_str, arg_list);
    std::cout << std::endl;
}

void Interpreter::print_formatted(const ASTNode *format_str,
                                  const ASTNode *arg_list) {
    if (!format_str ||
        format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        std::cout << "(invalid format)";
        return;
    }

    std::string format = format_str->str_value;
    // レキサーでエスケープ処理済みなので、ここでの処理は不要

    std::vector<int64_t> int_args;
    std::vector<std::string> str_args;

    // 引数リストを評価
    if (arg_list && arg_list->node_type == ASTNodeType::AST_STMT_LIST) {
        for (const auto &arg : arg_list->arguments) {
            if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                str_args.push_back(arg->str_value);
                int_args.push_back(0); // プレースホルダー
            } else if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                Variable *var = find_variable(arg->name);
                if (var && var->type == TYPE_STRING) {
                    str_args.push_back(var->str_value);
                    int_args.push_back(0); // プレースホルダー
                } else {
                    int64_t value = evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                }
            } else {
                int64_t value = evaluate_expression(arg.get());
                int_args.push_back(value);
                str_args.push_back(""); // プレースホルダー
            }
        }
    }

    // フォーマット文字列を処理
    std::string result;
    size_t arg_index = 0;
    for (size_t i = 0; i < format.length(); i++) {
        if (format[i] == '%' && i + 1 < format.length()) {
            // フォーマット指定子をパース
            size_t spec_start = i + 1;
            size_t spec_end = spec_start;
            int width = 0;
            bool zero_pad = false;

            // ゼロパディングをチェック
            if (spec_end < format.length() && format[spec_end] == '0') {
                zero_pad = true;
                spec_end++;
            }

            // 幅指定を読み取り
            while (spec_end < format.length() &&
                   std::isdigit(format[spec_end])) {
                width = width * 10 + (format[spec_end] - '0');
                spec_end++;
            }

            if (spec_end >= format.length()) {
                result += format[i];
                continue;
            }

            char specifier = format[spec_end];

            // %% の場合は特別処理（引数を消費しない）
            if (specifier == '%') {
                result += '%';
                i = spec_end; // specifierの位置に移動
                continue;
            }

            if (arg_index < int_args.size()) {
                switch (specifier) {
                case 'd':
                case 'i': {
                    std::string num_str = std::to_string(int_args[arg_index]);
                    if (width > 0 && zero_pad && num_str.length() < width) {
                        // ゼロパディング（負の数の場合は符号を最初に出力）
                        if (int_args[arg_index] < 0) {
                            // 負の数の場合: -000123 の形式
                            std::string abs_str =
                                num_str.substr(1); // マイナス記号を除去
                            std::string padding(width - num_str.length(), '0');
                            result += "-" + padding + abs_str;
                        } else {
                            // 正の数の場合: 000123 の形式
                            std::string padding(width - num_str.length(), '0');
                            result += padding + num_str;
                        }
                    } else if (width > 0 && num_str.length() < width) {
                        // スペースパディング
                        std::string padding(width - num_str.length(), ' ');
                        result += padding + num_str;
                    } else {
                        result += num_str;
                    }
                    break;
                }
                case 'l':
                    // %lld の処理
                    if (spec_end + 2 < format.length() &&
                        format[spec_end + 1] == 'l' &&
                        format[spec_end + 2] == 'd') {
                        result += std::to_string(int_args[arg_index]);
                        spec_end += 2; // 追加の 'll' をスキップ
                    } else {
                        result += std::to_string(int_args[arg_index]);
                    }
                    break;
                case 's':
                    if (arg_index < str_args.size() &&
                        !str_args[arg_index].empty()) {
                        result += str_args[arg_index];
                    } else {
                        result += std::to_string(int_args[arg_index]);
                    }
                    break;
                case 'c':
                    if (arg_index < str_args.size() &&
                        !str_args[arg_index].empty()) {
                        // 文字列の最初の文字を使用
                        result += str_args[arg_index][0];
                    } else {
                        // 整数値を文字として使用
                        result += static_cast<char>(int_args[arg_index]);
                    }
                    break;
                default:
                    result += '%';
                    result += specifier;
                    break;
                }
                arg_index++;
                i = spec_end; // specifierの位置に移動
            } else {
                result += format[i];
            }
        } else {
            result += format[i];
        }
    }

    // 余分な引数がある場合、スペース区切りで追加
    if (arg_index < int_args.size()) {
        for (size_t i = arg_index; i < int_args.size(); i++) {
            result += " ";
            if (i < str_args.size() && !str_args[i].empty()) {
                result += str_args[i];
            } else {
                result += std::to_string(int_args[i]);
            }
        }
    }

    // 最終結果にもエスケープシーケンスを適用
    std::string final_result;
    for (size_t i = 0; i < result.length(); i++) {
        if (result[i] == '\\' && i + 1 < result.length()) {
            switch (result[i + 1]) {
            case 'n':
                final_result += '\n';
                i++;
                break;
            case 't':
                final_result += '\t';
                i++;
                break;
            case 'r':
                final_result += '\r';
                i++;
                break;
            case '\\':
                final_result += '\\';
                i++;
                break;
            case '"':
                final_result += '"';
                i++;
                break;
            default:
                final_result += result[i];
                break;
            }
        } else {
            final_result += result[i];
        }
    }

    std::cout << final_result;
}

void Interpreter::print_multiple(const ASTNode *arg_list) {
    if (!arg_list || arg_list->node_type != ASTNodeType::AST_STMT_LIST) {
        return;
    }

    // 文字列リテラルにフォーマット指定子があるかチェック
    for (size_t i = 0; i < arg_list->arguments.size(); i++) {
        const auto &arg = arg_list->arguments[i];
        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
            std::string str_val = arg->str_value;
            if (str_val.find('%') != std::string::npos) {
                // フォーマット指定子が見つかった場合、printf形式として処理

                // 前の引数をまず処理
                std::vector<std::string> before_outputs;
                for (size_t j = 0; j < i; j++) {
                    const auto &before_arg = arg_list->arguments[j];
                    std::string output;

                    if (before_arg->node_type ==
                        ASTNodeType::AST_STRING_LITERAL) {
                        output = before_arg->str_value;
                    } else if (before_arg->node_type ==
                               ASTNodeType::AST_VARIABLE) {
                        Variable *var = find_variable(before_arg->name);
                        if (var && var->type == TYPE_STRING) {
                            output = var->str_value;
                        } else {
                            int64_t value =
                                evaluate_expression(before_arg.get());
                            output = std::to_string(value);
                        }
                    } else {
                        int64_t value = evaluate_expression(before_arg.get());
                        output = std::to_string(value);
                    }
                    before_outputs.push_back(output);
                }

                // 前の引数を出力
                for (size_t k = 0; k < before_outputs.size(); k++) {
                    if (k > 0)
                        std::cout << " ";
                    std::cout << before_outputs[k];
                }
                if (!before_outputs.empty())
                    std::cout << " ";

                // 残りの引数でprintf形式処理
                auto remaining_args =
                    std::make_unique<ASTNode>(ASTNodeType::AST_STMT_LIST);
                for (size_t j = i + 1; j < arg_list->arguments.size(); j++) {
                    // 新しいノードを作成してコピー
                    auto new_node = std::make_unique<ASTNode>(
                        arg_list->arguments[j]->node_type);
                    new_node->name = arg_list->arguments[j]->name;
                    new_node->str_value = arg_list->arguments[j]->str_value;
                    new_node->int_value = arg_list->arguments[j]->int_value;
                    remaining_args->arguments.push_back(std::move(new_node));
                }

                print_formatted(arg.get(), remaining_args.get());
                return;
            }
        }
    }

    // フォーマット指定子が見つからない場合、通常の複数引数処理
    std::vector<std::string> outputs;
    for (size_t i = 0; i < arg_list->arguments.size(); i++) {
        const auto &arg = arg_list->arguments[i];
        std::string output;

        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
            // エスケープシーケンスを処理
            std::string raw_output = arg->str_value;
            std::string processed_output;
            for (size_t j = 0; j < raw_output.length(); j++) {
                if (raw_output[j] == '\\' && j + 1 < raw_output.length()) {
                    switch (raw_output[j + 1]) {
                    case 'n':
                        processed_output += '\n';
                        j++;
                        break;
                    case 't':
                        processed_output += '\t';
                        j++;
                        break;
                    case 'r':
                        processed_output += '\r';
                        j++;
                        break;
                    case '\\':
                        processed_output += '\\';
                        j++;
                        break;
                    case '"':
                        processed_output += '"';
                        j++;
                        break;
                    default:
                        processed_output += raw_output[j];
                        break;
                    }
                } else {
                    processed_output += raw_output[j];
                }
            }
            output = processed_output;
        } else if (arg->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = find_variable(arg->name);
            if (var && var->type == TYPE_STRING) {
                output = var->str_value;
            } else {
                int64_t value = evaluate_expression(arg.get());
                output = std::to_string(value);
            }
        } else {
            int64_t value = evaluate_expression(arg.get());
            output = std::to_string(value);
        }

        outputs.push_back(output);
    }

    // スペース区切りで出力
    for (size_t i = 0; i < outputs.size(); i++) {
        if (i > 0) {
            std::cout << " ";
        }
        std::cout << outputs[i];
    }
}

void Interpreter::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &name) {
    switch (type) {
    case TYPE_TINY:
        if (value < -128 || value > 127) {
            error_msg(DebugMsgId::TYPE_RANGE_ERROR);
            throw std::runtime_error("Type range error");
        }
        break;
    case TYPE_SHORT:
        if (value < -32768 || value > 32767) {
            error_msg(DebugMsgId::TYPE_RANGE_ERROR);
            throw std::runtime_error("Type range error");
        }
        break;
    case TYPE_INT:
        if (value < -2147483648LL || value > 2147483647LL) {
            error_msg(DebugMsgId::TYPE_RANGE_ERROR);
            throw std::runtime_error("Type range error");
        }
        break;
    case TYPE_BOOL:
        // bool型は0/1に正規化
        break;
    default:
        break;
    }
}
