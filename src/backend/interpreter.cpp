#include "interpreter.h"
#include "../frontend/debug.h"
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
                throw std::runtime_error("変数 '" + node->name +
                                         "' の再宣言はできません");
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
                throw std::runtime_error("変数 '" + node->name +
                                         "' の再宣言はできません");
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
            throw std::runtime_error("配列サイズが負です: " + node->name);
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

    case ASTNodeType::AST_FUNC_DECL:
        debug_print("関数宣言を登録: %s\n", node->name.c_str());
        global_scope.functions[node->name] = node;
        debug_print("関数宣言登録完了: %s\n", node->name.c_str());
        break;

    default:
        break;
    }
}

void Interpreter::process(const ASTNode *ast) {
    debug_print("Interpreter::process() 開始\n");
    if (!ast) {
        debug_print("ASTがnullです\n");
        return;
    }

    debug_print("グローバル宣言の登録を開始\n");
    // まずグローバル宣言を登録
    register_global_declarations(ast);
    debug_print("グローバル宣言の登録が完了\n");

    debug_print("main関数を検索中\n");
    // main関数を探して実行
    const ASTNode *main_func = find_function("main");
    if (!main_func) {
        throw std::runtime_error("main関数が見つかりません");
    }
    debug_print("main関数が見つかりました\n");

    try {
        push_scope();
        execute_statement(main_func->body.get());
        pop_scope();
    } catch (const ReturnException &e) {
        debug_print("main関数が値 %lld で終了しました\n", e.value);
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
                        throw std::runtime_error(
                            "文字列要素には文字列リテラルのみ代入可能");
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
            throw std::runtime_error("配列サイズが負です: " + node->name);
        }

        // 配列初期化
        TypeInfo elem_type = node->type_info;
        if (elem_type == TYPE_STRING) {
            var.array_strings.resize(var.array_size);
        } else {
            var.array_values.resize(var.array_size, 0);
        }

        current_scope().variables[node->name] = var;
    } break;

    case ASTNodeType::AST_PRINT_STMT:
        print_value(node->left.get());
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
        debug_print("式評価: 数値リテラル = %lld\n", node->int_value);
        return node->int_value;

    case ASTNodeType::AST_STRING_LITERAL:
        debug_print("式評価: 文字列リテラル = \"%s\"\n",
                    node->str_value.c_str());
        // 文字列は特別な値として0を返す
        return 0;

    case ASTNodeType::AST_VARIABLE: {
        debug_print("式評価: 変数参照 = %s\n", node->name.c_str());
        Variable *var = find_variable(node->name);
        if (!var) {
            throw std::runtime_error("未定義の変数です: " + node->name);
        }
        if (var->is_array) {
            throw std::runtime_error("配列変数への直接参照はできません: " +
                                     node->name);
        }
        debug_print("  変数値 = %lld\n", var->value);
        return var->value;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        debug_print("式評価: 配列参照 = %s[...]\n", node->name.c_str());
        Variable *var = find_variable(node->name);
        if (!var) {
            throw std::runtime_error("未定義の配列です: " + node->name);
        }

        int64_t index = evaluate_expression(node->array_index.get());
        debug_print("  配列インデックス = %lld\n", index);

        if (var->type == TYPE_STRING) {
            // 文字列の個別文字アクセス（UTF-8対応）
            debug_print("  文字列要素アクセス (UTF-8対応)\n");

            // UTF-8文字数で範囲チェック
            size_t utf8_length = utf8_char_count(var->str_value);
            debug_print("  文字列長（UTF-8文字数）= %zu\n", utf8_length);

            if (index < 0 || index >= static_cast<int64_t>(utf8_length)) {
                throw std::runtime_error(
                    "文字列の範囲外アクセス: " + node->name +
                    " (index=" + std::to_string(index) +
                    ", length=" + std::to_string(utf8_length) + ")");
            }

            // UTF-8文字を取得
            std::string utf8_char =
                utf8_char_at(var->str_value, static_cast<size_t>(index));
            int64_t result = utf8_char_to_int(utf8_char);

            debug_print("  文字列要素値 = %lld (UTF-8文字: \"%s\")\n", result,
                        utf8_char.c_str());
            return result;
        } else if (var->is_array) {
            // 通常の配列アクセス
            debug_print("  配列要素アクセス\n");
            if (index < 0 || index >= var->array_size) {
                throw std::runtime_error("配列の範囲外アクセス: " + node->name);
            }

            TypeInfo elem_type =
                static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
            if (elem_type == TYPE_STRING) {
                // 文字列配列の場合、特別な処理が必要
                return 0; // 仮の実装
            } else {
                int64_t result = var->array_values[index];
                debug_print("  配列要素値 = %lld\n", result);
                return result;
            }
        } else {
            throw std::runtime_error("配列以外への配列参照です: " + node->name);
        }
    }

    case ASTNodeType::AST_BINARY_OP: {
        debug_print("式評価: 二項演算 = %s\n", node->op.c_str());
        int64_t left = evaluate_expression(node->left.get());
        int64_t right = evaluate_expression(node->right.get());
        debug_print("  左辺 = %lld, 右辺 = %lld\n", left, right);

        int64_t result = 0;
        if (node->op == "+")
            result = left + right;
        else if (node->op == "-")
            result = left - right;
        else if (node->op == "*")
            result = left * right;
        else if (node->op == "/") {
            if (right == 0) {
                throw std::runtime_error("ゼロ除算エラー");
            }
            result = left / right;
        } else if (node->op == "%") {
            if (right == 0) {
                throw std::runtime_error("ゼロ除算エラー");
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
            throw std::runtime_error("未知の二項演算子: " + node->op);
        }

        debug_print("  演算結果 = %lld\n", result);
        return result;
    }

    case ASTNodeType::AST_UNARY_OP: {
        debug_print("式評価: 単項演算 = %s\n", node->op.c_str());
        int64_t operand = evaluate_expression(node->left.get());
        debug_print("  オペランド = %lld\n", operand);

        int64_t result = 0;
        if (node->op == "!")
            result = (operand == 0) ? 1 : 0;
        else if (node->op == "-")
            result = -operand;
        else {
            throw std::runtime_error("未知の単項演算子: " + node->op);
        }

        debug_print("  単項演算結果 = %lld\n", result);
        return result;
    }

    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        Variable *var = find_variable(node->name);
        if (!var) {
            throw std::runtime_error("未定義の変数です: " + node->name);
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
            throw std::runtime_error("未定義の関数です: " + node->name);
        }

        // 引数の数チェック
        if (node->arguments.size() != func->parameters.size()) {
            throw std::runtime_error("引数の数が一致しません: " + node->name);
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
        debug_print(
            "AST_ARRAY_DECL が evaluate_expression で呼び出されました: %s\n",
            node->name.c_str());
        throw std::runtime_error("配列宣言は式として評価できません: " +
                                 node->name);

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
        throw std::runtime_error("未対応の式ノード: " + node_type_str);
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
            throw std::runtime_error("配列変数への直接代入: " + name);
        }
        check_type_range(var->type, value, name);
        var->value = value;
        var->is_assigned = true;
    }
}

void Interpreter::assign_variable(const std::string &name, int64_t value,
                                  TypeInfo type, bool is_const) {
    debug_print("変数代入: %s = %lld (type=%d, const=%d)\n", name.c_str(),
                value, type, is_const);
    Variable *var = find_variable(name);
    if (!var) {
        debug_print("  新規変数を作成\n");
        // 新しい変数を作成
        Variable new_var;
        new_var.type = type;
        new_var.value = value;
        new_var.is_assigned = true;
        new_var.is_const = is_const;
        check_type_range(type, value, name);
        current_scope().variables[name] = new_var;
    } else {
        debug_print("  既存変数に代入\n");
        if (var->is_const && var->is_assigned) {
            std::cerr << "再代入できません: " << name << std::endl;
            std::exit(1);
        }
        if (var->is_array) {
            throw std::runtime_error("配列変数への直接代入: " + name);
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
    debug_print("文字列代入: %s = \"%s\" (const=%d)\n", name.c_str(),
                value.c_str(), is_const);
    Variable *var = find_variable(name);
    if (!var) {
        debug_print("  新規文字列変数を作成\n");
        Variable new_var;
        new_var.type = TYPE_STRING;
        new_var.str_value = value;
        new_var.is_assigned = true;
        new_var.is_const = is_const;
        current_scope().variables[name] = new_var;
    } else {
        debug_print("  既存文字列変数に代入\n");
        if (var->is_const && var->is_assigned) {
            std::cerr << "再代入できません: " << name << std::endl;
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
        throw std::runtime_error("未定義の配列: " + name);
    }
    if (!var->is_array) {
        throw std::runtime_error("配列以外への配列代入: " + name);
    }
    if (var->is_const) {
        throw std::runtime_error("const配列への代入: " + name);
    }
    if (index < 0 || index >= var->array_size) {
        throw std::runtime_error("配列の範囲外アクセス: " + name);
    }

    TypeInfo elem_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
    check_type_range(elem_type, value, name);
    var->array_values[index] = value;
}

void Interpreter::assign_string_element(const std::string &name, int64_t index,
                                        const std::string &value) {
    debug_print("文字列要素代入: %s[%lld] = \"%s\" (UTF-8対応)\n", name.c_str(),
                index, value.c_str());

    Variable *var = find_variable(name);
    if (!var) {
        throw std::runtime_error("未定義の変数: " + name);
    }
    if (var->type != TYPE_STRING) {
        throw std::runtime_error("文字列以外への文字代入: " + name);
    }
    if (var->is_const) {
        std::cerr << "要素は変更できません: " << name << std::endl;
        std::exit(1);
    }

    // UTF-8文字数で範囲チェック
    size_t utf8_length = utf8_char_count(var->str_value);
    debug_print("  元の文字列長（UTF-8文字数）= %zu\n", utf8_length);

    if (index < 0 || index >= static_cast<int64_t>(utf8_length)) {
        throw std::runtime_error("文字列の範囲外アクセス: " + name +
                                 " (index=" + std::to_string(index) +
                                 ", length=" + std::to_string(utf8_length) +
                                 ")");
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
            debug_print("  位置 %lld の文字を \"%s\" に置換\n", index,
                        value.c_str());
        } else {
            // 既存の文字をコピー
            new_string += var->str_value.substr(i, len);
        }

        i += len;
        current_index++;
    }

    var->str_value = new_string;
    debug_print("  置換後の文字列: \"%s\"\n", var->str_value.c_str());
}

void Interpreter::print_value(const ASTNode *expr) {
    if (!expr) {
        std::cout << "(null)" << std::endl;
        return;
    }

    if (expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
        std::cout << expr->str_value << std::endl;
    } else if (expr->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *var = find_variable(expr->name);
        if (var && var->type == TYPE_STRING) {
            std::cout << var->str_value << std::endl;
        } else {
            int64_t value = evaluate_expression(expr);
            std::cout << value << std::endl;
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
                std::cout << utf8_char << std::endl;
            } else {
                throw std::runtime_error(
                    "文字列の範囲外アクセス: " + expr->name +
                    " (index=" + std::to_string(index) +
                    ", length=" + std::to_string(utf8_length) + ")");
            }
        } else {
            // 通常の配列アクセスは数値として出力
            int64_t value = evaluate_expression(expr);
            std::cout << value << std::endl;
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
                std::cout << "" << std::endl; // void関数（空文字列）
            } catch (const ReturnException &e) {
                pop_scope();
                if (e.type == TYPE_STRING) {
                    std::cout << e.str_value << std::endl;
                } else {
                    std::cout << e.value << std::endl;
                }
            }
        } else {
            // 通常の関数（数値を返す）
            int64_t value = evaluate_expression(expr);
            std::cout << value << std::endl;
        }
    } else {
        int64_t value = evaluate_expression(expr);
        std::cout << value << std::endl;
    }
}

void Interpreter::check_type_range(TypeInfo type, int64_t value,
                                   const std::string &name) {
    switch (type) {
    case TYPE_TINY:
        if (value < -128 || value > 127) {
            throw std::runtime_error("型の範囲外の値を代入しようとしました");
        }
        break;
    case TYPE_SHORT:
        if (value < -32768 || value > 32767) {
            throw std::runtime_error("型の範囲外の値を代入しようとしました");
        }
        break;
    case TYPE_INT:
        if (value < -2147483648LL || value > 2147483647LL) {
            throw std::runtime_error("型の範囲外の値を代入しようとしました");
        }
        break;
    case TYPE_BOOL:
        // bool型は0/1に正規化
        break;
    default:
        break;
    }
}
