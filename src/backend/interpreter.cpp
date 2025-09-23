#include "interpreter.h"
#include "../common/ast.h"
#include "../common/debug.h"
#include "error_handler.h"
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

Interpreter::Interpreter(bool debug)
    : debug_mode(debug),
      output_manager_(std::make_unique<OutputManager>(this)) {
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
    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
    case ASTNodeType::AST_ASSIGN:
        if (node->node_type == ASTNodeType::AST_MULTIPLE_VAR_DECL) {
            // 複数変数宣言の場合、各子ノードを処理
            for (const auto &child : node->children) {
                if (child->node_type == ASTNodeType::AST_VAR_DECL) {
                    register_global_declarations(child.get());
                }
            }
        } else if (node->node_type == ASTNodeType::AST_ASSIGN) {
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

            // typedef解決
            if (node->type_info == TYPE_UNKNOWN && !node->type_name.empty()) {
                std::string resolved_type = resolve_typedef(node->type_name);

                // 配列typedefの場合
                if (resolved_type.find("[") != std::string::npos) {
                    std::string base =
                        resolved_type.substr(0, resolved_type.find("["));
                    std::string array_part =
                        resolved_type.substr(resolved_type.find("["));

                    TypeInfo base_type = string_to_type_info(base);
                    var.type =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
                    var.is_array = true;

                    // 配列サイズを解析 [3] -> 3
                    if (array_part.length() > 2 && array_part[0] == '[' &&
                        array_part[array_part.length() - 1] == ']') {
                        std::string size_str =
                            array_part.substr(1, array_part.length() - 2);
                        var.array_size = std::stoi(size_str);
                    } else {
                        var.array_size = 0; // 動的配列
                    }

                    // 配列初期化
                    if (base_type == TYPE_STRING) {
                        var.array_strings.resize(var.array_size, "");
                    } else {
                        var.array_values.resize(var.array_size, 0);
                    }
                } else {
                    var.type = string_to_type_info(node->type_name);
                }
            } else {
                var.type = node->type_info;
            }

            var.is_const = node->is_const;
            var.is_assigned = false;
            global_scope.variables[node->name] = var;
        }
        break;

    case ASTNodeType::AST_ARRAY_DECL: {
        Variable var;

        debug_msg(DebugMsgId::ARRAY_DECL_START, node->name.c_str());

        // 多次元配列かどうかを確認
        if (node->array_type_info.dimensions.size() > 1) {
            debug_msg(
                DebugMsgId::MULTIDIM_ARRAY_DECL_INFO,
                static_cast<int>(node->array_type_info.dimensions.size()));

            // 多次元配列の場合
            var.is_array = true;
            var.is_multidimensional = true;
            var.array_type_info = node->array_type_info;
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                             node->array_type_info.base_type);
            var.is_const = node->is_const;
            var.is_assigned = false;

            // 全次元のサイズを計算して平坦化された配列を作成
            int total_size = 1;
            for (const ArrayDimension &dim : node->array_type_info.dimensions) {
                total_size *= dim.size;
            }

            debug_msg(DebugMsgId::ARRAY_TOTAL_SIZE, total_size);

            // 多次元配列用のストレージを初期化
            if (node->array_type_info.base_type == TYPE_STRING) {
                var.multidim_array_strings.resize(total_size, "");
            } else {
                var.multidim_array_values.resize(total_size, 0);
            }

            current_scope().variables[node->name] = var;
            debug_msg(DebugMsgId::MULTIDIM_ARRAY_DECL_SUCCESS,
                      node->name.c_str());
        } else {
            // 単一次元配列の場合（既存のロジック）
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
            var.is_const = node->is_const;
            var.is_array = true;
            var.is_assigned = false;

            // デバッグ: 配列サイズ情報を出力
            debug_msg(DebugMsgId::ARRAY_DIMENSIONS_COUNT,
                      node->array_dimensions.size());
            if (!node->array_dimensions.empty() && node->array_dimensions[0]) {
                debug_msg(DebugMsgId::ARRAY_ELEMENT_ACCESS,
                          "Array dimension exists");
            }

            // 配列サイズ決定
            if (node->array_size_expr) {
                var.array_size = static_cast<int>(
                    evaluate_expression(node->array_size_expr.get()));
            } else if (!node->array_dimensions.empty() &&
                       node->array_dimensions[0]) {
                // 配列次元から評価
                try {
                    var.array_size = static_cast<int>(
                        evaluate_expression(node->array_dimensions[0].get()));
                } catch (const std::exception &e) {
                    error_msg(DebugMsgId::NEGATIVE_ARRAY_SIZE_ERROR,
                              ("Failed to evaluate array size: " +
                               std::string(e.what()))
                                  .c_str());
                    throw std::runtime_error("Failed to evaluate array size: " +
                                             std::string(e.what()));
                }
            } else {
                var.array_size = node->array_size;
                debug_msg(DebugMsgId::ARRAY_ELEMENT_ACCESS,
                          ("Using node->array_size: " +
                           std::to_string(node->array_size))
                              .c_str());
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
            debug_msg(DebugMsgId::ARRAY_DECL_SUCCESS, node->name.c_str());
        }
    } break;

    case ASTNodeType::AST_FUNC_DECL:
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER, node->name.c_str());
        global_scope.functions[node->name] = node;
        debug_msg(DebugMsgId::FUNC_DECL_REGISTER_COMPLETE, node->name.c_str());
        break;

    case ASTNodeType::AST_TYPEDEF_DECL:
        // typedef宣言をtypedef_mapに登録
        // 重複チェック
        if (typedef_map.find(node->name) != typedef_map.end()) {
            error_msg(DebugMsgId::VAR_REDECLARE_ERROR, node->name.c_str());
            throw std::runtime_error("Typedef redefinition error: " +
                                     node->name);
        }
        typedef_map[node->name] = node->type_name;
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
        debug_msg(DebugMsgId::MAIN_FUNC_FOUND, "main function execute");

        if (main_func->body) {
            debug_msg(DebugMsgId::MAIN_FUNC_FOUND, "main function body exists");
        } else {
            debug_msg(DebugMsgId::MAIN_FUNC_FOUND,
                      "main function body is null");
        }

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

    if (debug_mode) {
        const char *node_type_name = "UNKNOWN";
        switch (node->node_type) {
        case ASTNodeType::AST_PRINT_STMT:
            node_type_name = "AST_PRINT_STMT";
            break;
        case ASTNodeType::AST_PRINTLN_STMT:
            node_type_name = "AST_PRINTLN_STMT";
            break;
        case ASTNodeType::AST_STMT_LIST:
            node_type_name = "AST_STMT_LIST";
            break;
        case ASTNodeType::AST_VAR_DECL:
            node_type_name = "AST_VAR_DECL";
            break;
        case ASTNodeType::AST_MULTIPLE_VAR_DECL:
            node_type_name = "AST_MULTIPLE_VAR_DECL";
            break;
        case ASTNodeType::AST_ASSIGN:
            node_type_name = "AST_ASSIGN";
            break;
        case ASTNodeType::AST_ARRAY_DECL:
            node_type_name = "AST_ARRAY_DECL";
            break;
        case ASTNodeType::AST_FOR_STMT:
            node_type_name = "AST_FOR_STMT";
            break;
        case ASTNodeType::AST_COMPOUND_STMT:
            node_type_name = "AST_COMPOUND_STMT";
            break;
        default:
            break;
        }
        debug_msg(DebugMsgId::VAR_DECLARATION_DEBUG, node_type_name);
    }

    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
        for (const auto &stmt : node->statements) {
            execute_statement(stmt.get());
        }
        break;

    case ASTNodeType::AST_COMPOUND_STMT:
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

                // 多次元配列アクセスかチェック
                if (node->left->left &&
                    node->left->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                    debug_msg(DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_DETECTED);

                    // 多次元配列の場合、まず変数名とインデックスを収集
                    std::vector<int64_t> indices;
                    std::string var_name;

                    // ネストしたAST_ARRAY_REFを辿って変数名とインデックスを収集
                    const ASTNode *current = node->left.get();
                    while (current &&
                           current->node_type == ASTNodeType::AST_ARRAY_REF) {
                        int64_t index =
                            evaluate_expression(current->array_index.get());
                        indices.insert(
                            indices.begin(),
                            index); // 逆順で挿入（後から追加されるのが外側のインデックス）

                        if (current->left->node_type ==
                            ASTNodeType::AST_VARIABLE) {
                            var_name = current->left->name;
                            break;
                        } else if (current->left->node_type ==
                                   ASTNodeType::AST_ARRAY_REF) {
                            current = current->left.get();
                        } else {
                            throw std::runtime_error("Invalid multidimensional "
                                                     "array access structure");
                        }
                    }

                    debug_msg(DebugMsgId::MULTIDIM_ARRAY_ACCESS_INFO,
                              var_name.c_str());

                    // 多次元配列の代入を実装
                    Variable *var = find_variable(var_name);
                    if (!var) {
                        throw std::runtime_error("Undefined variable: " +
                                                 var_name);
                    }

                    // インデックスをintベクターに変換
                    std::vector<int> int_indices;
                    for (int64_t idx : indices) {
                        int_indices.push_back(static_cast<int>(idx));
                    }

                    // フラットインデックスを計算
                    int flat_index = var->calculate_flat_index(int_indices);
                    debug_msg(DebugMsgId::FLAT_INDEX_CALCULATED, flat_index);

                    // 値を評価
                    int64_t value = evaluate_expression(node->right.get());

                    // 多次元配列に値を代入
                    if (var->array_type_info.base_type == TYPE_STRING) {
                        if (flat_index >= 0 &&
                            flat_index <
                                static_cast<int>(
                                    var->multidim_array_strings.size())) {
                            var->multidim_array_strings[flat_index] =
                                std::to_string(value);
                        } else {
                            throw std::runtime_error(
                                "Multidimensional array index out of bounds");
                        }
                    } else {
                        if (flat_index >= 0 &&
                            flat_index <
                                static_cast<int>(
                                    var->multidim_array_values.size())) {
                            var->multidim_array_values[flat_index] = value;
                        } else {
                            throw std::runtime_error(
                                "Multidimensional array index out of bounds");
                        }
                    }

                    debug_msg(DebugMsgId::MULTIDIM_ARRAY_ASSIGNMENT_COMPLETED);
                } else {
                    // 単一次元配列の場合
                    int64_t index =
                        evaluate_expression(node->left->array_index.get());

                    // 変数名を取得（新構造と旧構造の両方に対応）
                    std::string var_name;
                    if (node->left->left && node->left->left->node_type ==
                                                ASTNodeType::AST_VARIABLE) {
                        var_name = node->left->left->name;
                    } else if (!node->left->name.empty()) {
                        var_name = node->left->name;
                    } else {
                        throw std::runtime_error(
                            "Invalid array reference in assignment");
                    }

                    // 変数の型を確認して文字列か配列かを判断
                    Variable *var = find_variable(var_name);
                    if (var && var->type == TYPE_STRING) {
                        // 文字列要素への代入
                        if (node->right->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            assign_string_element(var_name, index,
                                                  node->right->str_value);
                        } else {
                            error_msg(DebugMsgId::NON_STRING_CHAR_ASSIGN_ERROR);
                            throw std::runtime_error(
                                "Non-string character assignment error");
                        }
                    } else {
                        // 通常の配列要素への代入
                        int64_t value = evaluate_expression(node->right.get());
                        assign_array_element(var_name, index, value);
                    }
                }
            } else {
                // 通常の代入
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    assign_variable(node->name, node->right->str_value,
                                    node->is_const);
                } else if (node->right->node_type ==
                           ASTNodeType::AST_ARRAY_LITERAL) {
                    // 配列リテラル代入
                    Variable *var = find_variable(node->name);
                    if (var && var->is_array) {
                        TypeInfo elem_type =
                            static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
                        for (size_t i = 0;
                             i < node->right->arguments.size() &&
                             i < static_cast<size_t>(var->array_size);
                             ++i) {
                            if (elem_type == TYPE_STRING) {
                                var->array_strings[i] =
                                    node->right->arguments[i]->str_value;
                            } else {
                                int64_t val = evaluate_expression(
                                    node->right->arguments[i].get());
                                var->array_values[i] = val;
                            }
                        }
                        var->is_assigned = true;
                    } else {
                        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR,
                                  node->name.c_str());
                        throw std::runtime_error(
                            "Variable not found for array assignment");
                    }
                } else {
                    int64_t value = evaluate_expression(node->right.get());
                    assign_variable(node->name, value, node->type_info,
                                    node->is_const);
                }
            }
        } else {
            // 変数宣言
            Variable var;

            // typedef解決
            if (node->type_info == TYPE_UNKNOWN && !node->type_name.empty()) {
                std::string resolved_type = resolve_typedef(node->type_name);

                // 配列typedefの場合
                if (resolved_type.find("[") != std::string::npos) {
                    std::string base =
                        resolved_type.substr(0, resolved_type.find("["));
                    std::string array_part =
                        resolved_type.substr(resolved_type.find("["));

                    TypeInfo base_type = string_to_type_info(base);
                    var.type =
                        static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
                    var.is_array = true;

                    // 配列サイズを解析 [3] -> 3
                    if (array_part.length() > 2 && array_part[0] == '[' &&
                        array_part[array_part.length() - 1] == ']') {
                        std::string size_str =
                            array_part.substr(1, array_part.length() - 2);
                        var.array_size = std::stoi(size_str);
                    } else {
                        var.array_size = 0; // 動的配列
                    }

                    // 配列初期化
                    if (base_type == TYPE_STRING) {
                        var.array_strings.resize(var.array_size, "");
                    } else {
                        var.array_values.resize(var.array_size, 0);
                    }
                } else {
                    var.type = string_to_type_info(node->type_name);
                }
            } else {
                var.type = node->type_info;
            }

            var.is_const = node->is_const;
            var.is_assigned = false;

            // 初期化式がある場合は評価して値を設定
            if (node->init_expr) {
                if (debug_mode) {
                    printf("[DEBUG] Variable declaration with initialization: "
                           "%s\n",
                           node->name.c_str());
                }
                if (node->init_expr->node_type ==
                    ASTNodeType::AST_STRING_LITERAL) {
                    var.str_value = node->init_expr->str_value;
                    var.type = TYPE_STRING;
                    var.is_assigned = true;
                } else if (node->init_expr->node_type ==
                           ASTNodeType::AST_ARRAY_LITERAL) {
                    // 配列リテラル代入
                    if (var.is_array) {
                        TypeInfo elem_type =
                            static_cast<TypeInfo>(var.type - TYPE_ARRAY_BASE);
                        for (size_t i = 0;
                             i < node->init_expr->arguments.size() &&
                             i < static_cast<size_t>(var.array_size);
                             ++i) {
                            if (elem_type == TYPE_STRING) {
                                var.array_strings[i] =
                                    node->init_expr->arguments[i]->str_value;
                            } else {
                                int64_t val = evaluate_expression(
                                    node->init_expr->arguments[i].get());
                                var.array_values[i] = val;
                            }
                        }
                        var.is_assigned = true;
                    }
                } else {
                    int64_t value = evaluate_expression(node->init_expr.get());
                    var.value = value;
                    var.is_assigned = true;

                    // 型範囲チェック
                    check_type_range(var.type, value, node->name);

                    if (debug_mode) {
                        printf(
                            "[DEBUG] Initialized variable %s with value %lld\n",
                            node->name.c_str(), value);
                    }
                }
            }

            current_scope().variables[node->name] = var;
        }
        break;

    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
        // 複数変数宣言の処理
        for (const auto &child : node->children) {
            if (child->node_type == ASTNodeType::AST_VAR_DECL) {
                execute_statement(child.get());
            }
        }
        break;

    case ASTNodeType::AST_ARRAY_DECL: {
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG, node->name.c_str());
        debug_msg(DebugMsgId::ARRAY_DIMENSIONS_COUNT,
                  node->array_dimensions.size());

        Variable var;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->type_info);
        var.is_const = node->is_const;
        var.is_array = true;
        var.is_assigned = false;

        // 多次元配列かどうかチェック
        if (node->array_dimensions.size() > 1) {
            debug_msg(DebugMsgId::MULTIDIM_ARRAY_PROCESSING);
            // 多次元配列の場合
            // 各次元のサイズを評価して整数配列に変換
            std::vector<ArrayDimension> dimensions;
            for (const auto &dim_expr : node->array_dimensions) {
                int dim_size =
                    static_cast<int>(evaluate_expression(dim_expr.get()));
                var.array_dimensions.push_back(dim_size);
                dimensions.push_back(ArrayDimension(dim_size, false));
            }

            // ArrayTypeInfoを作成
            var.array_type_info = ArrayTypeInfo(node->type_info, dimensions);

            // 総要素数を計算
            int total_size = 1;
            for (int dim : var.array_dimensions) {
                total_size *= dim;
            }
            var.array_size = total_size;

            // 要素の型
            TypeInfo elem_type = node->type_info;
            if (elem_type == TYPE_STRING) {
                var.multidim_array_strings.resize(total_size, "");
            } else {
                var.multidim_array_values.resize(total_size, 0);
            }

            // 多次元配列リテラル初期化
            if (node->init_expr &&
                node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                debug_msg(DebugMsgId::PRINTF_OFFSET_CALLED, 0);
                debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                          "processing initialization");

                // 多次元配列リテラルの処理を改善
                if (node->init_expr->arguments.empty()) {
                    // 空の配列リテラル
                    debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                              "empty array literal");
                } else {
                    // 最初の要素が配列リテラルかチェック
                    bool is_multidim =
                        node->init_expr->arguments[0]->node_type ==
                        ASTNodeType::AST_ARRAY_LITERAL;

                    if (is_multidim) {
                        debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                                  "processing 2D literal");
                        // 2次元配列リテラルの処理
                        int flat_index = 0;

                        for (size_t row = 0;
                             row < node->init_expr->arguments.size(); ++row) {
                            const auto &row_element =
                                node->init_expr->arguments[row];

                            if (row_element->node_type ==
                                ASTNodeType::AST_ARRAY_LITERAL) {
                                // 行要素が配列リテラルの場合
                                for (size_t col = 0;
                                     col < row_element->arguments.size();
                                     ++col) {
                                    if (flat_index >= total_size)
                                        break;

                                    const auto &element =
                                        row_element->arguments[col];
                                    if (elem_type == TYPE_STRING) {
                                        if (element->node_type ==
                                            ASTNodeType::AST_STRING_LITERAL) {
                                            var.multidim_array_strings
                                                [flat_index] =
                                                element->str_value;
                                        }
                                    } else {
                                        int64_t val =
                                            evaluate_expression(element.get());
                                        var.multidim_array_values[flat_index] =
                                            val;

                                        debug_msg(
                                            DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                                            ("Set element[" +
                                             std::to_string(flat_index) +
                                             "] = " + std::to_string(val))
                                                .c_str());
                                    }
                                    flat_index++;
                                }
                            } else {
                                // 行要素が単一値の場合（エラー）
                                error_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                                          "Expected nested array literal for "
                                          "2D array");
                                throw std::runtime_error(
                                    "Inconsistent array literal structure");
                            }
                        }
                    } else {
                        if (debug_mode) {
                            debug_msg(DebugMsgId::ARRAY_DECL_EVAL_DEBUG,
                                      "Processing 1D array literal in multidim "
                                      "context");
                        }
                        // 1次元配列リテラル（エラーの可能性があるが処理を試行）
                        for (size_t i = 0;
                             i < node->init_expr->arguments.size() &&
                             i < static_cast<size_t>(total_size);
                             ++i) {
                            const auto &element = node->init_expr->arguments[i];
                            if (elem_type == TYPE_STRING) {
                                if (element->node_type ==
                                    ASTNodeType::AST_STRING_LITERAL) {
                                    var.multidim_array_strings[i] =
                                        element->str_value;
                                }
                            } else {
                                int64_t val =
                                    evaluate_expression(element.get());
                                var.multidim_array_values[i] = val;
                            }
                        }
                    }
                }
            }
        } else {
            debug_msg(DebugMsgId::SINGLE_DIM_ARRAY_PROCESSING);
            // 単一次元配列の処理（既存のロジック）
            // 配列サイズ決定
            if (node->array_size_expr) {
                var.array_size = static_cast<int>(
                    evaluate_expression(node->array_size_expr.get()));
            } else if (!node->array_dimensions.empty() &&
                       node->array_dimensions[0]) {
                // 配列次元から評価（変数サイズ対応）
                var.array_size = static_cast<int>(
                    evaluate_expression(node->array_dimensions[0].get()));
            } else if (node->init_expr && node->init_expr->node_type ==
                                              ASTNodeType::AST_ARRAY_LITERAL) {
                // 配列リテラルからサイズを推測
                var.array_size =
                    static_cast<int>(node->init_expr->arguments.size());
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
        }

        // 初期化リストがある場合（新しい形式：配列リテラル）
        // 多次元配列の場合は既に初期化済みなのでスキップ
        if (node->init_expr &&
            node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL &&
            node->array_dimensions.size() <= 1) {
            debug_msg(DebugMsgId::PRINTF_OFFSET_CALLED,
                      0); // デバッグ用メッセージ
            std::cerr << "[DEBUG] Processing array literal initialization"
                      << std::endl;

            TypeInfo elem_type = node->type_info; // ここで再定義
            for (size_t i = 0; i < node->init_expr->arguments.size() &&
                               i < static_cast<size_t>(var.array_size);
                 ++i) {
                const auto &element = node->init_expr->arguments[i];
                std::cerr << "[DEBUG] Processing element " << i
                          << ", type: " << static_cast<int>(element->node_type)
                          << std::endl;

                if (elem_type == TYPE_STRING) {
                    // string配列の場合、要素はstring型である必要がある
                    if (element->node_type != ASTNodeType::AST_STRING_LITERAL) {
                        std::cerr << "[DEBUG] Type mismatch: expected string "
                                     "literal in string array"
                                  << std::endl;
                        error_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                                  "Type mismatch: string array cannot contain "
                                  "non-string elements");
                        throw std::runtime_error(
                            "Type mismatch in string array initialization");
                    }
                    var.array_strings[i] = element->str_value;
                } else {
                    // 数値配列の場合、要素は数値型である必要がある
                    if (element->node_type == ASTNodeType::AST_STRING_LITERAL) {
                        std::cerr << "[DEBUG] Type mismatch: found string "
                                     "literal in integer array"
                                  << std::endl;
                        error_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                                  "Type mismatch: integer array cannot contain "
                                  "string elements");
                        throw std::runtime_error(
                            "Type mismatch in integer array initialization");
                    }
                    std::cerr
                        << "[DEBUG] About to evaluate expression for element "
                        << i << std::endl;
                    int64_t val = evaluate_expression(element.get());
                    std::cerr << "[DEBUG] Evaluated value: " << val
                              << std::endl;
                    check_type_range(elem_type, val, node->name);
                    var.array_values[i] = val;
                }
            }
        }
        // 初期化リストがある場合（既存の形式）
        else {
            for (size_t i = 0; i < node->children.size() &&
                               i < static_cast<size_t>(var.array_size);
                 ++i) {
                const auto &child = node->children[i];
                if (child->node_type == ASTNodeType::AST_STMT_LIST) {
                    // 配列リテラル [1,2,3,...] の場合
                    TypeInfo elem_type = node->type_info; // ここで再定義
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
                    TypeInfo elem_type = node->type_info; // ここで再定義
                    if (elem_type == TYPE_STRING) {
                        var.array_strings[i] = child->str_value;
                    } else {
                        int64_t val = evaluate_expression(child.get());
                        check_type_range(elem_type, val, node->name);
                        var.array_values[i] = val;
                    }
                }
            }
        }

        current_scope().variables[node->name] = var;
    } break;

    case ASTNodeType::AST_PRINT_STMT:
        if (debug_mode) {
            printf("[DEBUG] Executing print statement\n");
        }
        if (!node->arguments.empty()) {
            // 複数引数のprint文（再帰下降パーサー対応）
            if (debug_mode) {
                printf("[DEBUG] Print statement has arguments\n");
            }
            output_manager_->print_multiple(node);
        } else if (node->left) {
            // 単一引数のprint文
            if (debug_mode) {
                printf("[DEBUG] Print statement has left node\n");
            }
            print_value(node->left.get());
        } else {
            if (debug_mode) {
                printf("[DEBUG] Print statement has no arguments\n");
            }
        }
        break;

    case ASTNodeType::AST_PRINTLN_STMT:
        if (node->left) {
            // 単一引数のprintln文
            output_manager_->print_value_with_newline(node->left.get());
        } else if (!node->arguments.empty()) {
            // 複数引数のprintln文（再帰下降パーサー対応）
            output_manager_->print_multiple_with_newline(node);
        } else {
            // 引数なしのprintln（改行のみ）
            output_manager_->print_newline();
        }
        break;

    case ASTNodeType::AST_PRINTLN_EMPTY:
        output_manager_->print_newline();
        break;

    case ASTNodeType::AST_PRINTF_STMT:
        output_manager_->print_formatted(node->left.get(), node->right.get());
        break;

    case ASTNodeType::AST_PRINTLNF_STMT:
        output_manager_->print_formatted_with_newline(node->left.get(),
                                                      node->right.get());
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
                try {
                    execute_statement(node->body.get());
                } catch (const ContinueException &e) {
                    // continue文でループ継続
                    continue;
                }
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
                try {
                    execute_statement(node->body.get());
                } catch (const ContinueException &e) {
                    // continue文でループ継続、update部分だけ実行
                }
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

    case ASTNodeType::AST_CONTINUE_STMT: {
        int64_t cond = 1;
        if (node->left) {
            cond = evaluate_expression(node->left.get());
        }
        if (cond) {
            throw ContinueException(cond);
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
            // 詳細なエラー表示（debug messageは重複するため削除）
            print_error_with_ast_location(
                "Undefined variable '" + node->name + "'", node);

            throw DetailedErrorException("Undefined variable");
        }
        if (var->is_array) {
            error_msg(DebugMsgId::DIRECT_ARRAY_REF_ERROR, node->name.c_str());
            throw std::runtime_error("Direct array reference error");
        }
        debug_msg(DebugMsgId::VAR_VALUE, var->value);
        return var->value;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        // デバッグ: ノードの内容を詳細にチェック
        if (debug_mode) {
            std::cerr << "[DEBUG] AST_ARRAY_REF evaluation started"
                      << std::endl;
            std::cerr << "[DEBUG] node pointer: " << node << std::endl;
            std::cerr << "[DEBUG] node->left pointer: " << node->left.get()
                      << std::endl;
            std::cerr << "[DEBUG] node->name: '" << node->name << "'"
                      << std::endl;
            std::cerr << "[DEBUG] node->array_index pointer: "
                      << node->array_index.get() << std::endl;
        }

        // 多次元配列アクセスかチェック
        if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            debug_msg(DebugMsgId::MULTIDIM_ARRAY_ACCESS_INFO, "reading");

            // 多次元配列の場合、まず変数名とインデックスを収集
            std::vector<int64_t> indices;
            std::string var_name;

            // ネストしたAST_ARRAY_REFを辿って変数名とインデックスを収集
            const ASTNode *current = node;
            while (current &&
                   current->node_type == ASTNodeType::AST_ARRAY_REF) {
                int64_t index = evaluate_expression(current->array_index.get());
                indices.insert(indices.begin(), index); // 逆順で挿入

                if (current->left->node_type == ASTNodeType::AST_VARIABLE) {
                    var_name = current->left->name;
                    break;
                } else if (current->left->node_type ==
                           ASTNodeType::AST_ARRAY_REF) {
                    current = current->left.get();
                } else {
                    throw std::runtime_error(
                        "Invalid multidimensional array access structure");
                }
            }

            // 変数を取得
            Variable *var = find_variable(var_name);
            if (!var) {
                error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, var_name.c_str());
                throw std::runtime_error("Undefined variable: " + var_name);
            }

            // インデックスをintベクターに変換
            std::vector<int> int_indices;
            for (int64_t idx : indices) {
                int_indices.push_back(static_cast<int>(idx));
            }

            // フラットインデックスを計算
            int flat_index = var->calculate_flat_index(int_indices);
            debug_msg(DebugMsgId::FLAT_INDEX_CALCULATED, flat_index);

            // 多次元配列から値を読み取り
            if (var->array_type_info.base_type == TYPE_STRING) {
                if (flat_index >= 0 &&
                    flat_index <
                        static_cast<int>(var->multidim_array_strings.size())) {
                    // 文字列の場合、最初の文字のコードを返す（互換性のため）
                    const std::string &str =
                        var->multidim_array_strings[flat_index];
                    return str.empty() ? 0 : static_cast<int64_t>(str[0]);
                } else {
                    throw std::runtime_error(
                        "Multidimensional array index out of bounds");
                }
            } else {
                if (flat_index >= 0 &&
                    flat_index <
                        static_cast<int>(var->multidim_array_values.size())) {
                    return var->multidim_array_values[flat_index];
                } else {
                    throw std::runtime_error(
                        "Multidimensional array index out of bounds");
                }
            }
        } else {
            // 単一次元配列アクセス（既存のロジック）
            std::string var_name;

            debug_msg(DebugMsgId::ARRAY_ELEMENT_ACCESS);

            // より堅牢なnullチェック
            if (node->left != nullptr) {
                if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
                    // 新しい構造: node->left が変数
                    var_name = node->left->name;
                    debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF,
                              var_name.c_str());
                } else {
                    // node->leftがあるが変数ではない場合
                    if (!node->name.empty()) {
                        var_name = node->name;
                        debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF,
                                  var_name.c_str());
                    } else {
                        error_msg(DebugMsgId::NON_ARRAY_REF_ERROR,
                                  "node->left exists but is not variable and "
                                  "no name");
                        throw std::runtime_error("Invalid array reference: "
                                                 "node->left is not variable");
                    }
                }
            } else if (!node->name.empty()) {
                // 旧構造: node->name が直接変数名を持つ
                var_name = node->name;
                debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF, var_name.c_str());
            } else {
                // ノードが完全に破損している可能性
                error_msg(DebugMsgId::NON_ARRAY_REF_ERROR,
                          "Both node->left and node->name are empty");
                throw std::runtime_error(
                    "Invalid array reference structure: complete null");
            }

            debug_msg(DebugMsgId::EXPR_EVAL_ARRAY_REF, var_name.c_str());
            Variable *var = find_variable(var_name);
            if (!var) {
                error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, var_name.c_str());
                throw std::runtime_error("Undefined variable: " + var_name);
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
                              var_name.c_str(), index, utf8_length);
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
                              var_name.c_str());
                    throw std::runtime_error("Array out of bounds access");
                }

                TypeInfo elem_type =
                    static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
                if (elem_type == TYPE_STRING) {
                    // 文字列配列の場合、文字列をint値として返す（仮実装）
                    // 実際には文字列を返すべきだが、print関数が文字列を処理できるように修正が必要
                    if (index >= 0 && index < static_cast<int64_t>(
                                                  var->array_strings.size())) {
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
                error_msg(DebugMsgId::NON_ARRAY_REF_ERROR, var_name.c_str());
                throw std::runtime_error("Non-array reference error");
            }
        }
    }

    case ASTNodeType::AST_BINARY_OP: {
        if (debug_mode) {
            printf("[DEBUG] Binary operation: %s\n", node->op.c_str());
        }

        int64_t left = evaluate_expression(node->left.get());
        int64_t right = evaluate_expression(node->right.get());
        if (debug_mode) {
            printf("[DEBUG] Operands: left=%lld, right=%lld\n", left, right);
        }

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
            if (debug_mode) {
                printf("[DEBUG] Unknown binary operator: %s\n",
                       node->op.c_str());
            }
            error_msg(DebugMsgId::UNKNOWN_BINARY_OP_ERROR, node->op.c_str());
            throw std::runtime_error("Unknown binary operator");
        }

        if (debug_mode) {
            printf("[DEBUG] Binary operation result: %lld\n", result);
        }
        return result;
    }

    case ASTNodeType::AST_UNARY_OP: {
        debug_msg(DebugMsgId::UNARY_OP_DEBUG, node->op.c_str());

        // ポストフィックス演算子の場合
        if (node->op == "++_post" || node->op == "--_post") {
            if (!node->left ||
                node->left->node_type != ASTNodeType::AST_VARIABLE) {
                error_msg(DebugMsgId::DIRECT_ARRAY_REF_ERROR, node->op.c_str());
                throw std::runtime_error("Invalid postfix operation");
            }

            Variable *var = find_variable(node->left->name);
            if (!var) {
                error_msg(DebugMsgId::UNDEFINED_VAR_ERROR,
                          node->left->name.c_str());
                throw std::runtime_error("Undefined variable");
            }

            int64_t old_value = var->value;
            if (node->op == "++_post") {
                var->value += 1;
            } else if (node->op == "--_post") {
                var->value -= 1;
            }

            check_type_range(var->type, var->value, node->left->name);
            return old_value; // ポストフィックスは古い値を返す
        }

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

    case ASTNodeType::AST_ARRAY_LITERAL:
        // 配列リテラル処理 [1,2,3,...] または [[1,2],[3,4],...] (多次元)
        // 注意: 配列リテラルは通常、配列宣言や代入の文脈で処理されるべきです
        // ここでは基本的な処理のみ行い、適切な値を返します
        {
            // 配列リテラルが独立して評価される場合は、通常エラーまたは特別な処理が必要
            // 今回は最初の要素の値を返すか、配列の要素数を返す
            if (!node->arguments.empty()) {
                // 最初の要素を評価して返す（暫定処理）
                if (node->arguments[0]->node_type ==
                    ASTNodeType::AST_ARRAY_LITERAL) {
                    // 多次元配列の場合は0を返す（適切な処理は配列宣言で行われる）
                    return 0;
                } else {
                    // 単次元配列の場合は最初の要素を返す
                    return evaluate_expression(node->arguments[0].get());
                }
            }
            return 0;
        }

    case ASTNodeType::AST_STMT_LIST:
        // 文リストの処理（通常は配列リテラルではない）
        {
            // これは文リストとして扱う
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
    debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_DEBUG, name.c_str(), index,
              value);

    Variable *var = find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::VARIABLE_NOT_FOUND, name.c_str());
        error_msg(DebugMsgId::UNDEFINED_ARRAY_ERROR, name.c_str());
        throw std::runtime_error("Undefined array");
    }

    debug_msg(DebugMsgId::ARRAY_INFO, var->is_array, var->array_size,
              var->array_values.size());

    if (!var->is_array) {
        error_msg(DebugMsgId::NON_ARRAY_REF_ERROR, name.c_str());
        throw std::runtime_error("Non-array reference");
    }
    if (var->is_const) {
        error_msg(DebugMsgId::CONST_ARRAY_ASSIGN_ERROR, name.c_str());
        throw std::runtime_error("Assignment to const array");
    }
    if (index < 0 || index >= var->array_size) {
        debug_msg(DebugMsgId::ARRAY_INDEX_OUT_OF_BOUNDS, index,
                  var->array_size);
        error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, name.c_str());
        throw std::runtime_error("Array out of bounds");
    }

    debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_START, index);
    TypeInfo elem_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
    check_type_range(elem_type, value, name);
    var->array_values[index] = value;
    debug_msg(DebugMsgId::ARRAY_ELEMENT_ASSIGN_SUCCESS);
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
    output_manager_->print_value(expr);
}

void Interpreter::print_formatted(const ASTNode *format_str,
                                  const ASTNode *arg_list) {
    if (!format_str ||
        format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        std::cout << "(invalid format)" << std::endl;
        return;
    }

    std::string format = format_str->str_value;
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
            char specifier = format[i + 1];

            if (specifier == '%') {
                // %% の場合は常に % を追加（引数不要）
                result += '%';
                i++; // %% をスキップ
            } else if (arg_index < int_args.size()) {
                switch (specifier) {
                case 'd':
                case 'i':
                    result += std::to_string(int_args[arg_index]);
                    break;
                case 'l':
                    // %lld の処理
                    if (i + 3 < format.length() && format[i + 2] == 'l' &&
                        format[i + 3] == 'd') {
                        result += std::to_string(int_args[arg_index]);
                        i += 2; // 追加の 'll' をスキップ
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
                    result += static_cast<char>(int_args[arg_index]);
                    break;
                default:
                    result += '%';
                    result += specifier;
                    break;
                }
                arg_index++;
                i++; // specifier をスキップ
            } else {
                result += format[i];
            }
        } else {
            result += format[i];
        }
    }

    std::cout << result << std::endl;
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
    case TYPE_CHAR:
        if (value < 0 || value > 255) {
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

std::string Interpreter::resolve_typedef(const std::string &type_name) {
    auto it = typedef_map.find(type_name);
    if (it != typedef_map.end()) {
        // さらに別のtypedefの可能性があるので再帰的に解決
        return resolve_typedef(it->second);
    }
    return type_name; // typedef aliasでない場合はそのまま返す
}

TypeInfo Interpreter::string_to_type_info(const std::string &type_str) {
    std::string resolved = resolve_typedef(type_str);

    if (resolved == "int")
        return TYPE_INT;
    if (resolved == "long")
        return TYPE_LONG;
    if (resolved == "short")
        return TYPE_SHORT;
    if (resolved == "tiny")
        return TYPE_TINY;
    if (resolved == "bool")
        return TYPE_BOOL;
    if (resolved == "string")
        return TYPE_STRING;
    if (resolved == "char")
        return TYPE_CHAR;
    if (resolved == "void")
        return TYPE_VOID;

    // 配列型の処理 (例: "int[5]")
    if (resolved.find("[") != std::string::npos) {
        std::string base = resolved.substr(0, resolved.find("["));
        TypeInfo base_type = string_to_type_info(base);
        return static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
    }

    return TYPE_UNKNOWN;
}
