#include "output_manager.h"
#include "../interpreter.h"
#include "../../common/debug.h"
#include "../../common/debug_messages.h"
#include "../../common/utf8_utils.h"
#include "../../common/io_interface.h"
#include <cinttypes>
#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>

OutputManager::OutputManager(Interpreter* interpreter) 
    : interpreter_(interpreter), io_interface_(IOFactory::get_instance()) {}

Variable* OutputManager::find_variable(const std::string& name) {
    return interpreter_->get_variable(name);
}

int64_t OutputManager::evaluate_expression(const ASTNode* node) {
    return interpreter_->eval_expression(node);
}

const ASTNode* OutputManager::find_function(const std::string& name) {
    return interpreter_->get_function(name);
}

void OutputManager::print_value(const ASTNode *expr) {
    if (!expr) {
        io_interface_->write_string("(null)");
        return;
    }

    if (expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
        io_interface_->write_string(expr->str_value.c_str());
    } else if (expr->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *var = find_variable(expr->name);
        if (var && var->type == TYPE_STRING) {
            io_interface_->write_string(var->str_value.c_str());
        } else {
            int64_t value = evaluate_expression(expr);
            io_interface_->write_number(value);
        }
    } else if (expr->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列アクセスの特別処理（新旧構造対応）
        std::string var_name;
        
        // 新構造（expr->left）と旧構造（expr->name）の両方に対応
        if (expr->left && expr->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 新しい構造: expr->left が変数
            var_name = expr->left->name;
        } else if (!expr->name.empty()) {
            // 旧構造: expr->name が直接変数名を持つ
            var_name = expr->name;
        } else if (expr->left) {
            // 複雑な左側の式（多次元配列アクセスなど）
            int64_t value = evaluate_expression(expr);
            io_interface_->write_number(value);
            return;
        } else {
            io_interface_->write_string("(invalid array ref)");
            return;
        }
        
        Variable *var = find_variable(var_name);
        if (var && var->type == TYPE_STRING) {
            // 文字列要素アクセスの場合は文字として出力（UTF-8対応）
            int64_t index = evaluate_expression(expr->array_index.get());
            size_t utf8_length = utf8_utils::utf8_char_count(var->str_value);

            if (index >= 0 && index < static_cast<int64_t>(utf8_length)) {
                std::string utf8_char =
                    utf8_utils::utf8_char_at(var->str_value, static_cast<size_t>(index));
                io_interface_->write_string(utf8_char.c_str());
            } else {
                error_msg(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR,
                          var_name.c_str(), index, utf8_length);
                throw std::runtime_error("String out of bounds");
            }
        } else if (var && var->is_array) {
            // 配列アクセスの処理
            int64_t index = evaluate_expression(expr->array_index.get());

            if (index < 0 || index >= var->array_size) {
                error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR,
                          var_name.c_str());
                    throw std::runtime_error("Array out of bounds");
                }

                TypeInfo elem_type =
                    static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
                if (elem_type == TYPE_STRING) {
                    // 文字列配列の場合は文字列として出力
                    if (index < static_cast<int64_t>(var->array_strings.size())) {
                        io_interface_->write_string(var->array_strings[index].c_str());
                    } else {
                        io_interface_->write_string("");
                    }
                } else {
                    // 数値配列は数値として出力
                    int64_t value = var->array_values[index];
                    io_interface_->write_number(value);
                }
            } else {
                // 通常の配列アクセスは数値として出力
                int64_t value = evaluate_expression(expr);
                io_interface_->write_number(value);
            }
    } else if (expr->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出しの特別処理
        const ASTNode *func = find_function(expr->name);
        if (func && func->type_info == TYPE_STRING) {
            // 文字列を返す関数の場合
            interpreter_->push_interpreter_scope();

            // 引数を評価してパラメータに束縛
            for (size_t i = 0; i < func->parameters.size(); ++i) {
                int64_t arg_value =
                    evaluate_expression(expr->arguments[i].get());
                Variable param;
                param.type = func->parameters[i]->type_info;
                param.value = arg_value;
                param.is_assigned = true;
                interpreter_->get_current_scope().variables.insert_or_assign(func->parameters[i]->name, std::move(param));
            }

            try {
                interpreter_->exec_statement(func->body.get());
                interpreter_->pop_interpreter_scope();
                // void関数（空文字列）
            } catch (const ReturnException &e) {
                interpreter_->pop_interpreter_scope();
                if (e.type == TYPE_STRING) {
                    io_interface_->write_string(e.str_value.c_str());
                } else {
                    io_interface_->write_number(e.value);
                }
            }
        } else {
            // 通常の関数（数値を返す）
            int64_t value = evaluate_expression(expr);
            io_interface_->write_number(value);
        }
    } else {
        int64_t value = evaluate_expression(expr);
        io_interface_->write_number(value);
    }
}

void OutputManager::print_value_with_newline(const ASTNode *expr) {
    print_value(expr);
    io_interface_->write_newline();
}

void OutputManager::print_newline() {
    io_interface_->write_newline();
}

void OutputManager::print_multiple_with_newline(const ASTNode *arg_list) {
    print_multiple(arg_list);
    io_interface_->write_newline();
}

void OutputManager::print_formatted_with_newline(const ASTNode *format_str,
                                               const ASTNode *arg_list) {
    print_formatted(format_str, arg_list);
    io_interface_->write_newline();
}

void OutputManager::print_formatted(const ASTNode *format_str,
                                  const ASTNode *arg_list) {
    if (!format_str ||
        format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        io_interface_->write_string("(invalid format)");
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
        if (format[i] == '\\' && i + 1 < format.length() && format[i + 1] == '%') {
            // \% エスケープの処理
            result += '%';
            i++; // 次の文字をスキップ
        } else if (format[i] == '%' && i + 1 < format.length()) {
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
                    int64_t value;
                    // 文字列が渡された場合の型変換
                    if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                        try {
                            value = std::stoll(str_args[arg_index]);
                        } catch (const std::exception&) {
                            value = 0; // 変換できない場合は0
                        }
                    } else {
                        value = int_args[arg_index];
                    }
                    
                    std::string num_str = std::to_string(value);
                    if (width > 0 && zero_pad &&
                        num_str.length() < static_cast<size_t>(width)) {
                        // ゼロパディング（負の数の場合は符号を最初に出力）
                        if (int_args[arg_index] < 0) {
                            // 負の数の場合: -000123 の形式
                            std::string abs_str =
                                num_str.substr(1); // マイナス記号を除去
                            std::string padding(static_cast<size_t>(width) -
                                                    num_str.length(),
                                                '0');
                            result += "-" + padding + abs_str;
                        } else {
                            // 正の数の場合: 000123 の形式
                            std::string padding(static_cast<size_t>(width) -
                                                    num_str.length(),
                                                '0');
                            result += padding + num_str;
                        }
                    } else if (width > 0 &&
                               num_str.length() < static_cast<size_t>(width)) {
                        // スペースパディング
                        std::string padding(
                            static_cast<size_t>(width) - num_str.length(), ' ');
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
                        int64_t value;
                        // 文字列が渡された場合の型変換
                        if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                            try {
                                value = std::stoll(str_args[arg_index]);
                            } catch (const std::exception&) {
                                value = 0; // 変換できない場合は0
                            }
                        } else {
                            value = int_args[arg_index];
                        }
                        result += std::to_string(value);
                        spec_end += 2; // 追加の 'll' をスキップ
                    } else {
                        int64_t value;
                        // 文字列が渡された場合の型変換
                        if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                            try {
                                value = std::stoll(str_args[arg_index]);
                            } catch (const std::exception&) {
                                value = 0; // 変換できない場合は0
                            }
                        } else {
                            value = int_args[arg_index];
                        }
                        result += std::to_string(value);
                    }
                    break;
                case 's':
                    if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                        // 文字列が渡された場合
                        result += str_args[arg_index];
                    } else {
                        // 数値が渡された場合は文字列に変換
                        result += std::to_string(int_args[arg_index]);
                    }
                    break;
                case 'c':
                    if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                        // 文字列が渡された場合、最初の文字を使用
                        result += str_args[arg_index][0];
                    } else {
                        // 数値が渡された場合、ASCII文字として変換
                        char ch = static_cast<char>(int_args[arg_index]);
                        result += ch;
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
    std::string final_result = process_escape_sequences(result);

    io_interface_->write_string(final_result.c_str());
}

// オフセット付きprint_formatted（指定されたインデックスから引数を開始する）
void OutputManager::print_formatted(const ASTNode *format_str, const ASTNode *arg_list, size_t start_index) {
    debug_msg(DebugMsgId::PRINTF_OFFSET_CALLED, start_index);
    
    if (!format_str || format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        io_interface_->write_string("(invalid format)");
        return;
    }

    std::string format = format_str->str_value;
    
    std::vector<int64_t> int_args;
    std::vector<std::string> str_args;

    // 引数リストをstart_indexから評価
    if (arg_list && (arg_list->node_type == ASTNodeType::AST_STMT_LIST || 
                     arg_list->node_type == ASTNodeType::AST_PRINTLN_STMT ||
                     arg_list->node_type == ASTNodeType::AST_PRINT_STMT)) {
        debug_msg(DebugMsgId::PRINTF_ARG_LIST_INFO, arg_list->arguments.size(), start_index);
        
        for (size_t i = start_index; i < arg_list->arguments.size(); i++) {
            const auto &arg = arg_list->arguments[i];
            
            debug_msg(DebugMsgId::PRINTF_ARG_PROCESSING, i, static_cast<int>(arg->node_type));
            
            if (arg->node_type == ASTNodeType::AST_ARRAY_REF) {
                debug_msg(DebugMsgId::PRINTF_ARRAY_REF_DEBUG, 
                         arg->left.get(), arg->array_index.get());
            }
            
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

    // フォーマット文字列を処理（既存のロジックを再利用）
    std::string result;
    size_t arg_index = 0;
    for (size_t i = 0; i < format.length(); i++) {
        if (format[i] == '%' && i + 1 < format.length()) {
            if (format[i + 1] == '%') {
                result += '%';
                i++; // '%%' をスキップ
                continue;
            }

            if (arg_index >= int_args.size() && arg_index >= str_args.size()) {
                result += format[i];
                continue;
            }

            // フォーマット指定子を探す
            size_t spec_start = i + 1;
            size_t spec_end = spec_start;

            // 幅指定を探す
            int width = 0;
            bool zero_pad = false;
            if (spec_end < format.length() && format[spec_end] == '0') {
                zero_pad = true;
                spec_end++;
            }

            while (spec_end < format.length() && isdigit(format[spec_end])) {
                width = width * 10 + (format[spec_end] - '0');
                spec_end++;
            }

            if (spec_end < format.length()) {
                switch (format[spec_end]) {
                case 'd': {
                    int64_t value;
                    if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                        try {
                            value = std::stoll(str_args[arg_index]);
                        } catch (const std::exception&) {
                            value = 0;
                        }
                    } else {
                        value = int_args[arg_index];
                    }
                    std::string num_str = std::to_string(value);

                    if (zero_pad && width > 0 && num_str.length() < static_cast<size_t>(width)) {
                        std::string padding(static_cast<size_t>(width) - num_str.length(), '0');
                        result += padding + num_str;
                    } else if (width > 0 && num_str.length() < static_cast<size_t>(width)) {
                        std::string padding(static_cast<size_t>(width) - num_str.length(), ' ');
                        result += padding + num_str;
                    } else {
                        result += num_str;
                    }
                    break;
                }
                case 's':
                    if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                        result += str_args[arg_index];
                    } else if (arg_index < int_args.size()) {
                        result += std::to_string(int_args[arg_index]);
                    }
                    break;
                default:
                    result += format[i];
                    continue;
                }
                arg_index++;
                i = spec_end;
            } else {
                result += format[i];
            }
        } else {
            result += format[i];
        }
    }

    std::string final_result = process_escape_sequences(result);
    io_interface_->write_string(final_result.c_str());
}

std::string OutputManager::process_escape_sequences(const std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.length(); i++) {
        if (input[i] == '\\' && i + 1 < input.length()) {
            switch (input[i + 1]) {
            case 'n':
                result += '\n';
                i++;
                break;
            case 't':
                result += '\t';
                i++;
                break;
            case 'r':
                result += '\r';
                i++;
                break;
            case '\\':
                result += '\\';
                i++;
                break;
            case '"':
                result += '"';
                i++;
                break;
            case '%':
                result += '%';
                i++;
                break;
            default:
                result += input[i];
                break;
            }
        } else {
            result += input[i];
        }
    }
    return result;
}

bool OutputManager::has_unescaped_format_specifiers(const std::string& str) {
    if (debug_mode) {
        printf("[DEBUG] has_unescaped_format_specifiers: checking string '%s'\n", str.c_str());
    }
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            // \% でエスケープされているかチェック
            if (i > 0 && str[i-1] == '\\') {
                continue; // エスケープされている
            }
            // 次の文字がフォーマット指定子かチェック
            if (i + 1 < str.length()) {
                char next = str[i + 1];
                if (next == 'd' || next == 's' || next == 'c' || next == 'l' || next == '%') {
                    if (debug_mode) {
                        printf("[DEBUG] has_unescaped_format_specifiers: found specifier %%%c\n", next);
                    }
                    return true;
                }
            }
        }
    }
    if (debug_mode) {
        printf("[DEBUG] has_unescaped_format_specifiers: no format specifiers found\n");
    }
    return false;
}

size_t OutputManager::count_format_specifiers(const std::string& str) {
    size_t count = 0;
    if (debug_mode) {
        printf("[DEBUG] count_format_specifiers: counting in string '%s'\n", str.c_str());
    }
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            // \% でエスケープされているかチェック
            if (i > 0 && str[i-1] == '\\') {
                continue; // エスケープされている
            }
            if (i + 1 < str.length()) {
                char next = str[i + 1];
                if (next == 'd' || next == 's' || next == 'c') {
                    count++;
                    if (debug_mode) {
                        printf("[DEBUG] count_format_specifiers: found %c, count now %zu\n", next, count);
                    }
                } else if (next == 'l' && i + 3 < str.length() && 
                          str[i + 2] == 'l' && str[i + 3] == 'd') {
                    count++;
                    if (debug_mode) {
                        printf("[DEBUG] count_format_specifiers: found lld, count now %zu\n", count);
                    }
                    i += 3; // %lld をスキップ
                } else if (next == '%') {
                    // %% は引数を消費しないのでカウントしない
                    if (debug_mode) {
                        printf("[DEBUG] count_format_specifiers: found %%, not counting\n");
                    }
                }
                // %% は引数を消費しないのでカウントしない
            }
        }
    }
    if (debug_mode) {
        printf("[DEBUG] count_format_specifiers: final count %zu\n", count);
    }
    return count;
}

void OutputManager::print_multiple(const ASTNode *arg_list) {
    // AST_PRINT_STMTまたはAST_PRINTLN_STMTノードの場合、引数を直接処理
    if (arg_list && (arg_list->node_type == ASTNodeType::AST_PRINT_STMT || 
                     arg_list->node_type == ASTNodeType::AST_PRINTLN_STMT)) {
        if (debug_mode) {
            printf("[DEBUG] print_multiple: Processing %s with %zu arguments\n", 
                   (arg_list->node_type == ASTNodeType::AST_PRINT_STMT) ? "AST_PRINT_STMT" : "AST_PRINTLN_STMT",
                   arg_list->arguments.size());
        }
        
        // 引数がない場合は何もしない
        if (arg_list->arguments.empty()) {
            if (debug_mode) {
                printf("[DEBUG] print_multiple: No arguments in statement\n");
            }
            // 改行なし
            return;
        }

        // 引数が1つだけの場合の特別処理
        if (arg_list->arguments.size() == 1) {
            const auto &arg = arg_list->arguments[0];
            if (debug_mode) {
                printf("[DEBUG] print_multiple: Single argument in AST_PRINT_STMT, type: %d\n", (int)arg->node_type);
            }
            if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                // フォーマット指定子が含まれていても、引数が1つだけの場合はそのまま出力
                std::string output = process_escape_sequences(arg->str_value);
                io_interface_->write_string(output.c_str());
            } else {
                print_value(arg.get());
            }
            // 改行なし
            return;
        }
        
        // 複数引数の場合：フォーマット文字列を探す
        for (size_t i = 0; i < arg_list->arguments.size(); i++) {
            const auto &arg = arg_list->arguments[i];
            if (debug_mode) {
                printf("[DEBUG] print_multiple: checking argument %zu, type: %d\n", i, (int)arg->node_type);
            }
            if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                std::string str_val = arg->str_value;
                if (debug_mode) {
                    printf("[DEBUG] print_multiple: found string literal '%s'\n", str_val.c_str());
                }
                // \% エスケープされていない % を探す
                if (has_unescaped_format_specifiers(str_val)) {
                    if (debug_mode) {
                        printf("[DEBUG] print_multiple: format specifiers found, processing as printf\n");
                    }
                    // フォーマット指定子が見つかった場合

                    // 前の引数をスペース区切りで出力
                    for (size_t j = 0; j < i; j++) {
                        if (j > 0) io_interface_->write_char(' ');
                        print_value(arg_list->arguments[j].get());
                    }
                    if (i > 0) io_interface_->write_char(' ');

                    // printf形式の処理：元のarg_listを使用してコピーを避ける
                    print_formatted(arg.get(), arg_list, i + 1);
                    
                    // 余分な引数処理は不要 - print_formattedが全ての引数を処理する
                    // 改行なし
                    return;
                }
            }
        }
        
        // フォーマット指定子が見つからない場合は引数を順番に出力
        for (size_t i = 0; i < arg_list->arguments.size(); ++i) {
            if (i > 0) io_interface_->write_char(' '); // スペース区切りで出力
            print_value(arg_list->arguments[i].get());
        }
        // 改行なし
        return;
    }
    
    if (!arg_list || arg_list->node_type != ASTNodeType::AST_STMT_LIST) {
        if (debug_mode) {
            printf("[DEBUG] print_multiple: Invalid arg_list or not AST_STMT_LIST (type: %d)\n", 
                   arg_list ? (int)arg_list->node_type : -1);
        }
        return;
    }

    // 引数がない場合は何もしない
    if (arg_list->arguments.empty()) {
        if (debug_mode) {
            printf("[DEBUG] print_multiple: No arguments\n");
        }
        return;
    }

    if (debug_mode) {
        printf("[DEBUG] print_multiple: %zu arguments\n", arg_list->arguments.size());
    }

    // 引数が1つだけの場合の特別処理
    if (arg_list->arguments.size() == 1) {
        const auto &arg = arg_list->arguments[0];
        if (debug_mode) {
            printf("[DEBUG] print_multiple: Single argument, type: %d\n", (int)arg->node_type);
        }
        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
            // フォーマット指定子が含まれていても、引数が1つだけの場合はそのまま出力
            std::string output = process_escape_sequences(arg->str_value);
            io_interface_->write_string(output.c_str());
        } else {
            print_value(arg.get());
        }
        return;
    }

    // 複数引数の場合：フォーマット文字列を探す
    for (size_t i = 0; i < arg_list->arguments.size(); i++) {
        const auto &arg = arg_list->arguments[i];
        if (debug_mode) {
            printf("[DEBUG] print_multiple: checking argument %zu, type: %d\n", i, (int)arg->node_type);
        }
        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
            std::string str_val = arg->str_value;
            if (debug_mode) {
                printf("[DEBUG] print_multiple: found string literal '%s'\n", str_val.c_str());
            }
            // \% エスケープされていない % を探す
            if (has_unescaped_format_specifiers(str_val)) {
                if (debug_mode) {
                    printf("[DEBUG] print_multiple: format specifiers found, processing as printf\n");
                }
                // フォーマット指定子が見つかった場合

                // 前の引数をスペース区切りで出力
                for (size_t j = 0; j < i; j++) {
                    if (j > 0) io_interface_->write_char(' ');
                    print_value(arg_list->arguments[j].get());
                }
                if (i > 0) io_interface_->write_char(' ');

                // printf形式の処理：元のarg_listを使用してコピーを避ける
                print_formatted(arg.get(), arg_list, i + 1);
                
                // 余分な引数処理は不要 - print_formattedが全ての引数を処理する
                // 改行なし
                return;
            }
        }
    }

    // フォーマット指定子が見つからない場合、すべてスペース区切りで出力
    for (size_t i = 0; i < arg_list->arguments.size(); i++) {
        if (i > 0) {
            io_interface_->write_char(' ');
        }
        
        const auto &arg = arg_list->arguments[i];
        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
            std::string output = process_escape_sequences(arg->str_value);
            io_interface_->write_string(output.c_str());
        } else {
            print_value(arg.get());
        }
    }
    // 改行なし
}
