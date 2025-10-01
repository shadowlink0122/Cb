#include "output/output_manager.h"
#include "core/interpreter.h"
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
// #include "services/expression_service.h" // DRY効率化: 循環依存解決まで一時コメントアウト
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../../../common/utf8_utils.h"
#include "../../../common/io_interface.h"
#include <algorithm>
#include <cctype>
#include <cinttypes>
#include <cstdio>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

std::string numeric_to_string(TypeInfo type, int64_t int_value,
                              double double_value, long double quad_value) {
    std::ostringstream oss;
    switch (type) {
    case TYPE_FLOAT:
    case TYPE_DOUBLE: {
        oss << std::setprecision(15) << std::defaultfloat << double_value;
        break;
    }
    case TYPE_QUAD: {
        oss << std::setprecision(std::numeric_limits<long double>::digits10)
            << std::defaultfloat << quad_value;
        break;
    }
    default:
        oss << int_value;
        break;
    }
    return oss.str();
}

void write_numeric_value(IOInterface *io_interface, TypeInfo type,
                         int64_t int_value, double double_value,
                         long double quad_value) {
    if (!io_interface) {
        return;
    }

    switch (type) {
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
        io_interface->write_float(double_value);
        break;
    case TYPE_QUAD: {
        std::ostringstream oss;
        oss << std::setprecision(std::numeric_limits<long double>::digits10)
            << std::defaultfloat << quad_value;
        io_interface->write_string(oss.str().c_str());
        break;
    }
    default:
        io_interface->write_number(int_value);
        break;
    }
}

} // namespace

OutputManager::OutputManager(Interpreter* interpreter) 
    : interpreter_(interpreter), 
      io_interface_(IOFactory::get_instance()),
      expression_service_(nullptr) { // DRY効率化: 初期化時にnullptr、後でInterpreterから取得
}

// OutputManager::~OutputManager() {
//     // スマートポインタが自動的にリソース解放
// }

Variable* OutputManager::find_variable(const std::string& name) {
    return interpreter_->get_variable(name);
}

int64_t OutputManager::evaluate_expression(const ASTNode* node) {
    // DRY効率化: 統一式評価サービスを遅延初期化して使用
    if (!expression_service_) {
        expression_service_ = interpreter_->get_expression_service();
    }
    
    if (expression_service_) {
        return expression_service_->evaluate_safe(node, "OutputManager");
    } else {
        // フォールバック: 従来方式
        return interpreter_->eval_expression(node);
    }
}

const ASTNode* OutputManager::find_function(const std::string& name) {
    return interpreter_->get_function(name);
}

void OutputManager::print_value(const ASTNode *expr) {
    if (!expr) {
        io_interface_->write_string("(null)");
        return;
    }

    auto write_typed_value = [&](const TypedValue &typed) {
        if (typed.is_string()) {
            io_interface_->write_string(typed.as_string().c_str());
            return;
        }

        if (typed.is_numeric()) {
            TypeInfo value_type = typed.numeric_type != TYPE_UNKNOWN
                                      ? typed.numeric_type
                                      : typed.type.type_info;
            if (value_type == TYPE_UNKNOWN) {
                value_type = typed.is_floating() ? TYPE_DOUBLE : TYPE_INT;
            }
            write_numeric_value(io_interface_, value_type, typed.as_numeric(),
                                typed.as_double(), typed.as_quad());
            return;
        }

        io_interface_->write_number(0);
    };

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

    std::vector<int64_t> int_args;
    std::vector<std::string> str_args;
    std::vector<double> double_args;
    std::vector<long double> quad_args;
    std::vector<TypeInfo> type_args;

    collect_formatted_arguments(arg_list, 0, true, int_args, str_args,
                                double_args, quad_args, type_args);

    std::string rendered = render_formatted_string(
        format_str->str_value, int_args, str_args, double_args, quad_args,
        type_args, true);

    io_interface_->write_string(rendered.c_str());
}

// オフセット付きprint_formatted（指定されたインデックスから引数を開始する）
void OutputManager::print_formatted(const ASTNode *format_str, const ASTNode *arg_list, size_t start_index) {
    debug_msg(DebugMsgId::PRINTF_OFFSET_CALLED, start_index);
    
    if (!format_str or format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
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
                if (var) {
                    if (var->type == TYPE_STRING) {
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
            } else if (arg->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                // struct メンバーアクセス: obj.member または array[idx].member
                std::string member_name = arg->name;
                std::string struct_name;
                
                if (arg->left && arg->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // 配列要素のメンバアクセス: team[0].name
                    struct_name = interpreter_->extract_array_element_name(arg->left.get());
                } else if (arg->left && arg->left->node_type == ASTNodeType::AST_VARIABLE) {
                    // 通常の構造体メンバアクセス: obj.member
                    struct_name = arg->left->name;
                } else {
                    // その他の場合は通常の式評価
                    int64_t value = evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                    continue;
                }
                
                try {
                    Variable* member_var = interpreter_->get_struct_member(struct_name, member_name);
                    if (member_var && member_var->type == TYPE_STRING) {
                        str_args.push_back(member_var->str_value);
                        int_args.push_back(0); // プレースホルダー
                    } else if (member_var) {
                        int_args.push_back(member_var->value);
                        str_args.push_back(""); // プレースホルダー
                    } else {
                        // メンバ変数が見つからない場合は通常の式評価
                        int64_t value = evaluate_expression(arg.get());
                        int_args.push_back(value);
                        str_args.push_back(""); // プレースホルダー
                    }
                } catch (const std::exception& e) {
                    std::cerr << "DEBUG: Exception in member access: " << e.what() << std::endl;
                    int64_t value = evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                }
            } else if (arg->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 配列要素の処理
                
                // まず、構造体メンバー配列アクセスかどうかチェック
                if (arg->left && arg->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                    // 構造体メンバー配列アクセス: obj.member[index]
                    std::string obj_name = arg->left->left->name;
                    std::string member_name = arg->left->name;
                    int64_t index = evaluate_expression(arg->array_index.get());
                    
                    try {
                        int64_t value = interpreter_->get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
                        int_args.push_back(value);
                        str_args.push_back("");
                    } catch (const std::exception& e) {
                        int_args.push_back(0);
                        str_args.push_back("");
                    }
                } else {
                    // 通常の配列アクセス
                    Variable *var = find_variable(arg->left->name);
                    if (var && var->is_array) {
                        int64_t index = evaluate_expression(arg->array_index.get());
                        // 文字列配列かどうかの判定を修正
                        if (!var->array_strings.empty()) {
                            if (index >= 0 && index < static_cast<int64_t>(var->array_strings.size())) {
                                str_args.push_back(var->array_strings[index]);
                                int_args.push_back(0); // プレースホルダー
                            } else {
                                str_args.push_back("");
                                int_args.push_back(0);
                            }
                        } else {
                            if (index >= 0 && index < static_cast<int64_t>(var->array_values.size())) {
                                int_args.push_back(var->array_values[index]);
                                str_args.push_back("");
                            } else {
                                int_args.push_back(0);
                                str_args.push_back("");
                            }
                        }
                    } else {
                        int_args.push_back(0);
                        str_args.push_back("");
                    }
                }
            } else if (arg->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
                // 構造体メンバー配列アクセス: obj.member[index]
                std::string obj_name;
                if (arg->left && arg->left->node_type == ASTNodeType::AST_VARIABLE) {
                    obj_name = arg->left->name;
                } else {
                    int_args.push_back(0);
                    str_args.push_back("");
                    continue;
                }
                
                std::string member_name = arg->name;
                int64_t index = evaluate_expression(arg->right.get());
                
                try {
                    int64_t value = interpreter_->get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
                    int_args.push_back(value);
                    str_args.push_back("");
                } catch (const std::exception& e) {
                    int_args.push_back(0);
                    str_args.push_back("");
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
                case 'c':
                    if (arg_index < str_args.size() && !str_args[arg_index].empty()) {
                        // 文字列が渡された場合、最初の文字を使用
                        result += str_args[arg_index][0];
                    } else if (arg_index < int_args.size()) {
                        // 数値が渡された場合、ASCII文字として変換
                        char ch = static_cast<char>(int_args[arg_index]);
                        result += ch;
                    }
                    break;
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
                        } else if (arg_index < int_args.size()) {
                            value = int_args[arg_index];
                        } else {
                            value = 0;
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
                        } else if (arg_index < int_args.size()) {
                            value = int_args[arg_index];
                        } else {
                            value = 0;
                        }
                        result += std::to_string(value);
                    }
                    break;
                default:
                    result += format[i];
                    continue;
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
    debug_msg(DebugMsgId::PRINT_FORMAT_SPEC_CHECKING, str.c_str());
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            // \% でエスケープされているかチェック
            if (i > 0 && str[i-1] == '\\') {
                continue; // エスケープされている
            }
            // 次の文字がフォーマット指定子かチェック
            if (i + 1 < str.length()) {
                char next = str[i + 1];
                if (next == 'd' or next == 's' or next == 'c' or next == '%') {
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_SPEC_FOUND, std::string(1, next).c_str());
                    return true;
                }
                // %lld のチェック
                if (next == 'l' && i + 3 < str.length() && str[i + 2] == 'l' && str[i + 3] == 'd') {
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_SPEC_FOUND, "lld");
                    return true;
                }
            }
        }
    }
    debug_msg(DebugMsgId::PRINT_NO_FORMAT_SPECIFIERS);
    return false;
}

size_t OutputManager::count_format_specifiers(const std::string& str) {
    size_t count = 0;
    debug_msg(DebugMsgId::OUTPUT_FORMAT_COUNT, str.c_str());
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            // \% でエスケープされているかチェック
            if (i > 0 && str[i-1] == '\\') {
                continue; // エスケープされている
            }
            if (i + 1 < str.length()) {
                char next = str[i + 1];
                if (next == 'd' or next == 's' or next == 'c') {
                    count++;
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_COUNT, std::to_string(count).c_str());
                } else if (next == 'l' && i + 3 < str.length() && 
                          str[i + 2] == 'l' && str[i + 3] == 'd') {
                    count++;
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_COUNT, std::to_string(count).c_str());
                    i += 3; // %lld をスキップ
                } else if (next == '%') {
                    // %% は引数を消費しないのでカウントしない
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_SPEC_FOUND, "%%");
                }
                // %% は引数を消費しないのでカウントしない
            }
        }
    }
    debug_msg(DebugMsgId::OUTPUT_FORMAT_COUNT, std::to_string(count).c_str());
    return count;
}

void OutputManager::print_multiple(const ASTNode *arg_list) {
    // AST_PRINT_STMTまたはAST_PRINTLN_STMTノードの場合、引数を直接処理
    if (arg_list && (arg_list->node_type == ASTNodeType::AST_PRINT_STMT || 
                     arg_list->node_type == ASTNodeType::AST_PRINTLN_STMT)) {
        
        debug_msg(DebugMsgId::PRINT_MULTIPLE_PROCESSING,
                    (arg_list->node_type == ASTNodeType::AST_PRINT_STMT) ? "AST_PRINT_STMT" : "AST_PRINTLN_STMT",
                    (int)arg_list->arguments.size());
        
        // 引数がない場合は何もしない
        if (arg_list->arguments.empty()) {
            debug_msg(DebugMsgId::PRINT_NO_ARGUMENTS_DEBUG);
            // 改行なし
            return;
        }

        // 引数が1つだけの場合の特別処理
        if (arg_list->arguments.size() == 1) {
            const auto &arg = arg_list->arguments[0];
            debug_msg(DebugMsgId::PRINT_SINGLE_ARG_DEBUG,
                "AST_PRINT_STMT", (int)arg->node_type);
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
            debug_msg(DebugMsgId::PRINT_CHECKING_ARGUMENT, (int)i, (int)arg->node_type);
            if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                std::string str_val = arg->str_value;
                debug_msg(DebugMsgId::PRINT_FOUND_STRING_LITERAL, str_val.c_str());
                // \% エスケープされていない % を探す
                if (has_unescaped_format_specifiers(str_val)) {
                    debug_msg(DebugMsgId::PRINT_PRINTF_FORMAT_FOUND);
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
        debug_msg(DebugMsgId::PRINT_NO_ARGUMENTS_DEBUG);
        return;
    }

    // 引数がない場合は何もしない
    if (arg_list->arguments.empty()) {
        debug_msg(DebugMsgId::PRINT_NO_ARGUMENTS_DEBUG);
        return;
    }

    debug_msg(DebugMsgId::PRINT_MULTIPLE_PROCESSING);

    // 引数が1つだけの場合の特別処理
    if (arg_list->arguments.size() == 1) {
        const auto &arg = arg_list->arguments[0];
        debug_msg(DebugMsgId::PRINT_SINGLE_ARG_DEBUG);
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
        debug_msg(DebugMsgId::PRINT_CHECKING_ARGUMENT, (int)i, (int)arg->node_type);
        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
            std::string str_val = arg->str_value;
            debug_msg(DebugMsgId::PRINT_FOUND_STRING_LITERAL, str_val.c_str());
            // \% エスケープされていない % を探す
            if (has_unescaped_format_specifiers(str_val)) {
                debug_msg(DebugMsgId::PRINT_PRINTF_FORMAT_FOUND);
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

// 文字列フォーマット機能（戻り値として返す）
std::string OutputManager::format_string(const ASTNode *format_str, const ASTNode *arg_list) {
    if (!format_str ||
        format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        return "(invalid format)";
    }

    std::vector<int64_t> int_args;
    std::vector<std::string> str_args;
    std::vector<double> double_args;
    std::vector<long double> quad_args;
    std::vector<TypeInfo> type_args;

    collect_formatted_arguments(arg_list, 0, true, int_args, str_args,
                                double_args, quad_args, type_args);

    return render_formatted_string(format_str->str_value, int_args, str_args,
                                   double_args, quad_args, type_args, false);
}

void OutputManager::collect_formatted_arguments(
    const ASTNode *arg_list, size_t start_index, bool allow_print_nodes,
    std::vector<int64_t> &int_args, std::vector<std::string> &str_args,
    std::vector<double> &double_args, std::vector<long double> &quad_args,
    std::vector<TypeInfo> &type_args) {

    auto push_defaults = [&]() -> size_t {
        int_args.emplace_back(0);
        str_args.emplace_back();
        double_args.emplace_back(0.0);
        quad_args.emplace_back(0.0L);
        type_args.emplace_back(TYPE_UNKNOWN);
        return int_args.size() - 1;
    };

    auto should_skip = [&](const ASTNode *node) {
        if (allow_print_nodes || !node) {
            return false;
        }
        switch (node->node_type) {
        case ASTNodeType::AST_PRINT_STMT:
        case ASTNodeType::AST_PRINTLN_STMT:
        case ASTNodeType::AST_PRINTF_STMT:
        case ASTNodeType::AST_PRINTLNF_STMT:
            return true;
        default:
            return false;
        }
    };

    std::function<void(const ASTNode *, size_t)> append_nodes =
        [&](const ASTNode *node, size_t offset) {
            if (!node) {
                return;
            }

            auto process_single = [&](const ASTNode *current) {
                if (!current || should_skip(current)) {
                    return;
                }

                size_t index = push_defaults();

                auto set_numeric = [&](TypeInfo value_type, long double quad_value) {
                    quad_args[index] = quad_value;
                    double_args[index] = static_cast<double>(quad_value);
                    int_args[index] = static_cast<int64_t>(quad_value);
                    type_args[index] = value_type;
                };

                auto fallback_numeric = [&]() {
                    int64_t value = evaluate_expression(current);
                    quad_args[index] = static_cast<long double>(value);
                    double_args[index] = static_cast<double>(value);
                    int_args[index] = value;
                    if (type_args[index] == TYPE_UNKNOWN) {
                        type_args[index] = TYPE_INT;
                    }
                };

                if (current->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    str_args[index] = current->str_value;
                    type_args[index] = TYPE_STRING;
                    return;
                }

                TypeInfo hinted_type = TYPE_UNKNOWN;
                if (current->node_type == ASTNodeType::AST_VARIABLE) {
                    if (auto *var = find_variable(current->name)) {
                        hinted_type = var->type;
                    }
                }

                try {
                    TypedValue typed =
                        interpreter_->evaluate_typed_expression(current);

                    if (typed.needs_deferred_evaluation()) {
                        fallback_numeric();
                        return;
                    }

                    if (typed.is_string()) {
                        str_args[index] = typed.as_string();
                        type_args[index] = TYPE_STRING;
                        return;
                    }

                    if (typed.is_numeric()) {
                        long double quad_value = typed.as_quad();
                        quad_args[index] = quad_value;
                        double_args[index] = typed.as_double();
                        int_args[index] = typed.as_numeric();

                        TypeInfo resolved = typed.numeric_type != TYPE_UNKNOWN
                                                ? typed.numeric_type
                                                : typed.type.type_info;
                        if (resolved == TYPE_UNKNOWN) {
                            resolved = typed.is_floating() ? TYPE_DOUBLE
                                                            : TYPE_INT;
                        }
                        type_args[index] = resolved;
                        return;
                    }

                    if (typed.is_struct()) {
                        str_args[index] = "(struct)";
                        type_args[index] = TYPE_STRUCT;
                        return;
                    }

                    fallback_numeric();
                } catch (const ReturnException &ret) {
                    if (ret.type == TYPE_STRING) {
                        str_args[index] = ret.str_value;
                        type_args[index] = TYPE_STRING;
                    } else {
                        quad_args[index] = ret.quad_value;
                        double_args[index] = ret.double_value;
                        int_args[index] = ret.value;
                        type_args[index] = ret.type;
                    }
                } catch (const std::exception &) {
                    fallback_numeric();
                }

                if (type_args[index] == TYPE_UNKNOWN && hinted_type != TYPE_UNKNOWN) {
                    type_args[index] = hinted_type;
                    if (hinted_type == TYPE_STRING && str_args[index].empty()) {
                        if (auto *var = find_variable(current->name)) {
                            str_args[index] = var->str_value;
                        }
                    }
                    if ((hinted_type == TYPE_FLOAT || hinted_type == TYPE_DOUBLE ||
                         hinted_type == TYPE_QUAD) && quad_args[index] == 0.0L) {
                        if (auto *var = find_variable(current->name)) {
                            quad_args[index] = var->quad_value;
                            double_args[index] = var->double_value;
                            int_args[index] = var->value;
                        }
                    }
                }

                if (type_args[index] == TYPE_UNKNOWN) {
                    type_args[index] = TYPE_INT;
                }
            };

            switch (node->node_type) {
            case ASTNodeType::AST_STMT_LIST:
            case ASTNodeType::AST_PRINT_STMT:
            case ASTNodeType::AST_PRINTLN_STMT:
            case ASTNodeType::AST_PRINTF_STMT:
            case ASTNodeType::AST_PRINTLNF_STMT: {
                if (offset >= node->arguments.size()) {
                    return;
                }
                for (size_t i = offset; i < node->arguments.size(); ++i) {
                    process_single(node->arguments[i].get());
                }
                break;
            }
            default:
                if (offset == 0) {
                    process_single(node);
                }
                break;
            }
        };

    append_nodes(arg_list, start_index);
}

std::string OutputManager::render_formatted_string(
    const std::string &format, const std::vector<int64_t> &int_args,
    const std::vector<std::string> &str_args,
    const std::vector<double> &double_args,
    const std::vector<long double> &quad_args,
    const std::vector<TypeInfo> &type_args, bool append_extra_args) {

    auto format_with_snprintf = [&](const std::string &fmt, auto value) {
        std::vector<char> buffer(256);
        int written = std::snprintf(buffer.data(), buffer.size(), fmt.c_str(),
                                    value);
        if (written < 0) {
            return std::string{};
        }
        if (written >= static_cast<int>(buffer.size())) {
            buffer.resize(static_cast<size_t>(written) + 1);
            std::snprintf(buffer.data(), buffer.size(), fmt.c_str(), value);
        }
        return std::string(buffer.data());
    };

    auto argument_to_string = [&](size_t index) {
        if (index >= type_args.size()) {
            return std::string{};
        }
        if (type_args[index] == TYPE_STRING) {
            return str_args[index];
        }
        return numeric_to_string(type_args[index], int_args[index],
                                 double_args[index], quad_args[index]);
    };

    std::string result;
    size_t arg_index = 0;
    const size_t total_args = type_args.size();
    const std::string flag_chars = "-+ 0#";

    for (size_t i = 0; i < format.length(); ++i) {
        char ch = format[i];

        if (ch == '\\' && i + 1 < format.length() && format[i + 1] == '%') {
            result += '%';
            ++i;
            continue;
        }

        if (ch != '%') {
            result += ch;
            continue;
        }

        if (i + 1 < format.length() && format[i + 1] == '%') {
            result += '%';
            ++i;
            continue;
        }

        size_t pos = i + 1;
        std::string flags;
        while (pos < format.length() &&
               flag_chars.find(format[pos]) != std::string::npos) {
            flags += format[pos++];
        }

        std::string width;
        while (pos < format.length() &&
               std::isdigit(static_cast<unsigned char>(format[pos]))) {
            width += format[pos++];
        }

        std::string precision;
        if (pos < format.length() && format[pos] == '.') {
            precision += '.';
            ++pos;
            while (pos < format.length() &&
                   std::isdigit(static_cast<unsigned char>(format[pos]))) {
                precision += format[pos++];
            }
        }

        std::string length_mod;
        if (pos < format.length()) {
            if (format[pos] == 'l') {
                length_mod += 'l';
                ++pos;
                if (pos < format.length() && format[pos] == 'l') {
                    length_mod += 'l';
                    ++pos;
                }
            } else if (format[pos] == 'L') {
                length_mod += 'L';
                ++pos;
            }
        }

        if (pos >= format.length()) {
            result += '%';
            result += flags;
            result += width;
            result += precision;
            result += length_mod;
            break;
        }

        char spec = format[pos];
        i = pos;

        if (arg_index >= total_args) {
            result += '%';
            result += spec;
            continue;
        }

        std::string fmt = "%" + flags + width + precision;
        std::string formatted;

        switch (spec) {
        case 'd':
        case 'i': {
            fmt += "lld";
            long long value = static_cast<long long>(int_args[arg_index]);
            formatted = format_with_snprintf(fmt, value);
            break;
        }
        case 'u': {
            fmt += "llu";
            unsigned long long value =
                static_cast<unsigned long long>(int_args[arg_index]);
            formatted = format_with_snprintf(fmt, value);
            break;
        }
        case 'o': {
            fmt += "llo";
            unsigned long long value =
                static_cast<unsigned long long>(int_args[arg_index]);
            formatted = format_with_snprintf(fmt, value);
            break;
        }
        case 'x': {
            fmt += "llx";
            unsigned long long value =
                static_cast<unsigned long long>(int_args[arg_index]);
            formatted = format_with_snprintf(fmt, value);
            break;
        }
        case 'X': {
            fmt += "llX";
            unsigned long long value =
                static_cast<unsigned long long>(int_args[arg_index]);
            formatted = format_with_snprintf(fmt, value);
            break;
        }
        case 'c': {
            fmt += 'c';
            int value = 0;
            if (type_args[arg_index] == TYPE_STRING &&
                !str_args[arg_index].empty()) {
                value = static_cast<unsigned char>(str_args[arg_index][0]);
            } else {
                value = static_cast<int>(int_args[arg_index]);
            }
            formatted = format_with_snprintf(fmt, value);
            break;
        }
        case 's': {
            fmt += 's';
            std::string string_value = str_args[arg_index];
            if (string_value.empty() && type_args[arg_index] != TYPE_STRING) {
                string_value = argument_to_string(arg_index);
            }
            formatted = format_with_snprintf(fmt, string_value.c_str());
            break;
        }
        case 'f':
        case 'F':
        case 'e':
        case 'E':
        case 'g':
        case 'G':
        case 'a':
        case 'A': {
            bool use_long_double =
                length_mod == "L" || type_args[arg_index] == TYPE_QUAD;
            if (use_long_double) {
                fmt += 'L';
            }
            fmt.push_back(spec);
            if (use_long_double) {
                formatted = format_with_snprintf(fmt, quad_args[arg_index]);
            } else {
                formatted = format_with_snprintf(fmt, double_args[arg_index]);
            }
            break;
        }
        default:
            result += '%';
            result += spec;
            ++arg_index;
            continue;
        }

        if (formatted.empty()) {
            formatted = argument_to_string(arg_index);
        }

        result += formatted;
        ++arg_index;
    }

    if (append_extra_args && arg_index < total_args) {
        for (size_t i = arg_index; i < total_args; ++i) {
            if (!result.empty()) {
                result += ' ';
            }
            result += argument_to_string(i);
        }
    }

    return process_escape_sequences(result);
}
