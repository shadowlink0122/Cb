#include "output_manager.h"
#include "../interpreter.h"
#include "../../frontend/debug.h"
#include "../../common/utf8_utils.h"
#include <cinttypes>
#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>

OutputManager::OutputManager(Interpreter* interpreter) 
    : interpreter_(interpreter) {}

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
        printf("(null)");
        return;
    }

    if (expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
        printf("%s", expr->str_value.c_str());
    } else if (expr->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *var = find_variable(expr->name);
        if (var && var->type == TYPE_STRING) {
            printf("%s", var->str_value.c_str());
        } else {
            int64_t value = evaluate_expression(expr);
            printf("%" PRId64, value);
        }
    } else if (expr->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 配列アクセスの特別処理
        Variable *var = find_variable(expr->name);
        if (var && var->type == TYPE_STRING) {
            // 文字列要素アクセスの場合は文字として出力（UTF-8対応）
            int64_t index = evaluate_expression(expr->array_index.get());
            size_t utf8_length = utf8_utils::utf8_char_count(var->str_value);

            if (index >= 0 && index < static_cast<int64_t>(utf8_length)) {
                std::string utf8_char =
                    utf8_utils::utf8_char_at(var->str_value, static_cast<size_t>(index));
                printf("%s", utf8_char.c_str());
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
                    printf("%s", var->array_strings[index].c_str());
                } else {
                    printf("");
                }
            } else {
                // 数値配列は数値として出力
                int64_t value = var->array_values[index];
                printf("%" PRId64, value);
            }
        } else {
            // 通常の配列アクセスは数値として出力
            int64_t value = evaluate_expression(expr);
            printf("%" PRId64, value);
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
                interpreter_->get_current_scope().variables[func->parameters[i]->name] = param;
            }

            try {
                interpreter_->exec_statement(func->body.get());
                interpreter_->pop_interpreter_scope();
                printf(""); // void関数（空文字列）
            } catch (const ReturnException &e) {
                interpreter_->pop_interpreter_scope();
                if (e.type == TYPE_STRING) {
                    printf("%s", e.str_value.c_str());
                } else {
                    printf("%" PRId64, e.value);
                }
            }
        } else {
            // 通常の関数（数値を返す）
            int64_t value = evaluate_expression(expr);
            printf("%" PRId64, value);
        }
    } else {
        int64_t value = evaluate_expression(expr);
        printf("%" PRId64, value);
    }
}

void OutputManager::print_value_with_newline(const ASTNode *expr) {
    print_value(expr);
    printf("\n");
}

void OutputManager::print_multiple_with_newline(const ASTNode *arg_list) {
    print_multiple(arg_list);
    printf("\n");
}

void OutputManager::print_formatted_with_newline(const ASTNode *format_str,
                                               const ASTNode *arg_list) {
    print_formatted(format_str, arg_list);
    printf("\n");
}

void OutputManager::print_formatted(const ASTNode *format_str,
                                  const ASTNode *arg_list) {
    if (!format_str ||
        format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        printf("(invalid format)");
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

    printf("%s", final_result.c_str());
}

void OutputManager::print_multiple(const ASTNode *arg_list) {
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
                        printf(" ");
                    printf("%s", before_outputs[k].c_str());
                }
                if (!before_outputs.empty())
                    printf(" ");

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
            printf(" ");
        }
        printf("%s", outputs[i].c_str());
    }
}
