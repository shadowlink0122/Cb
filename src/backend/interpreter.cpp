#include "interpreter.h"
#include "../common/ast.h"
#include "../frontend/debug.h"
#include "evaluator/expression_evaluator.h"
#include "executor/statement_executor.h"
#include "output/output_manager.h"
#include <cctype>
#include <cinttypes>
#include <codecvt>
#include <cstdarg>
#include <cstdio>
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

    // OutputManagerを初期化
    output_manager_ = std::make_unique<OutputManager>(this);

    // ExpressionEvaluatorを初期化
    expression_evaluator_ = std::make_unique<ExpressionEvaluator>(*this);

    // StatementExecutorを初期化
    statement_executor_ = std::make_unique<StatementExecutor>(*this);
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
    statement_executor_->execute_statement(node);
}

int64_t Interpreter::evaluate_expression(const ASTNode *node) {
    return expression_evaluator_->evaluate_expression(node);
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
