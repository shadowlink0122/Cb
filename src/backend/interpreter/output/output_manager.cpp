#include "output/output_manager.h"
#include "core/interpreter.h"
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
// #include "services/expression_service.h" // DRY効率化:
// 循環依存解決まで一時コメントアウト
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../../../common/io_interface.h"
#include "../../../common/utf8_utils.h"
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

OutputManager::OutputManager(Interpreter *interpreter)
    : interpreter_(interpreter), io_interface_(IOFactory::get_instance()),
      expression_service_(
          nullptr) { // DRY効率化: 初期化時にnullptr、後でInterpreterから取得
}

// OutputManager::~OutputManager() {
//     // スマートポインタが自動的にリソース解放
// }

Variable *OutputManager::find_variable(const std::string &name) {
    return interpreter_->get_variable(name);
}

int64_t OutputManager::evaluate_expression(const ASTNode *node) {
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

const ASTNode *OutputManager::find_function(const std::string &name) {
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

            // ポインタ型の場合、16進数で表示
            int64_t numeric_val = typed.as_numeric();
            if (value_type == TYPE_POINTER) {
                // タグビットが設定されている場合は除去
                uint64_t clean_value = static_cast<uint64_t>(numeric_val);
                if (numeric_val & (1LL << 63)) {
                    clean_value &= ~(1ULL << 63);
                }
                std::ostringstream oss;
                oss << "0x" << std::hex << clean_value;

                io_interface_->write_string(oss.str().c_str());
                return;
            }

            write_numeric_value(io_interface_, value_type, numeric_val,
                                typed.as_double(), typed.as_quad());
            return;
        }

        io_interface_->write_number(0);
    };

    auto evaluate_numeric_and_write = [&](const ASTNode *node) {
        int64_t value = evaluate_expression(node);
        // ポインタ値の場合、タグビットを除去して16進数で表示
        if (value & (1LL << 63)) {
            // タグビットが設定されている場合、ポインタメタデータと判断
            uint64_t clean_value = static_cast<uint64_t>(value) & ~(1ULL << 63);
            std::ostringstream oss;
            oss << "0x" << std::hex << clean_value;
            io_interface_->write_string(oss.str().c_str());
        } else {
            io_interface_->write_number(value);
        }
    };

    auto print_string_array_element = [&](Variable *var, int64_t index) {
        if (!var) {
            io_interface_->write_string("");
            return;
        }

        if (index >= 0 &&
            index < static_cast<int64_t>(var->array_strings.size())) {
            io_interface_->write_string(
                var->array_strings[static_cast<size_t>(index)].c_str());
        } else {
            io_interface_->write_string("");
        }
    };

    auto print_numeric_array_element = [&](Variable *var, int64_t index) {
        if (!var) {
            io_interface_->write_number(0);
            return;
        }

        if (index >= 0 &&
            index < static_cast<int64_t>(var->array_values.size())) {
            io_interface_->write_number(
                var->array_values[static_cast<size_t>(index)]);
        } else {
            io_interface_->write_number(0);
        }
    };

    auto print_array_literal = [&](const ASTNode *array_node,
                                   const auto &self_ref) -> void {
        if (!array_node) {
            io_interface_->write_string("[]");
            return;
        }

        io_interface_->write_char('[');
        for (size_t i = 0; i < array_node->arguments.size(); ++i) {
            if (i > 0) {
                io_interface_->write_string(", ");
            }
            const ASTNode *child = array_node->arguments[i].get();
            if (child && child->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                self_ref(child, self_ref);
            } else {
                print_value(child);
            }
        }
        io_interface_->write_char(']');
    };

    auto determine_dimensions = [](const std::string &type_name) -> int {
        return static_cast<int>(
            std::count(type_name.begin(), type_name.end(), '['));
    };

    auto render_int_vector = [](const std::vector<int64_t> &vec) {
        std::ostringstream oss;
        oss << '[';
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << vec[i];
        }
        oss << ']';
        return oss.str();
    };

    auto render_string_vector = [](const std::vector<std::string> &vec) {
        std::ostringstream oss;
        oss << '[';
        for (size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            oss << '"' << vec[i] << '"';
        }
        oss << ']';
        return oss.str();
    };

    auto render_int_array = [&](const ReturnException &ret) {
        if (ret.int_array_3d.empty()) {
            return std::string("[]");
        }

        int dims = determine_dimensions(ret.array_type_name);
        if (dims <= 1) {
            return render_int_vector(ret.int_array_3d.front().front());
        }

        std::ostringstream oss;
        oss << '[';
        for (size_t i = 0; i < ret.int_array_3d.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            const auto &matrix = ret.int_array_3d[i];
            if (dims == 2) {
                if (matrix.empty()) {
                    oss << "[]";
                    continue;
                }
                oss << '[';
                for (size_t j = 0; j < matrix.size(); ++j) {
                    if (j > 0) {
                        oss << ", ";
                    }
                    oss << render_int_vector(matrix[j]);
                }
                oss << ']';
            } else {
                oss << '[';
                for (size_t j = 0; j < matrix.size(); ++j) {
                    if (j > 0) {
                        oss << ", ";
                    }
                    oss << '[';
                    const auto &row = matrix[j];
                    for (size_t k = 0; k < row.size(); ++k) {
                        if (k > 0) {
                            oss << ", ";
                        }
                        oss << row[k];
                    }
                    oss << ']';
                }
                oss << ']';
            }
        }
        oss << ']';
        return oss.str();
    };

    auto render_string_array = [&](const ReturnException &ret) {
        if (ret.str_array_3d.empty()) {
            return std::string("[]");
        }

        int dims = determine_dimensions(ret.array_type_name);
        if (dims <= 1) {
            return render_string_vector(ret.str_array_3d.front().front());
        }

        std::ostringstream oss;
        oss << '[';
        for (size_t i = 0; i < ret.str_array_3d.size(); ++i) {
            if (i > 0) {
                oss << ", ";
            }
            const auto &matrix = ret.str_array_3d[i];
            if (dims == 2) {
                if (matrix.empty()) {
                    oss << "[]";
                    continue;
                }
                oss << '[';
                for (size_t j = 0; j < matrix.size(); ++j) {
                    if (j > 0) {
                        oss << ", ";
                    }
                    oss << render_string_vector(matrix[j]);
                }
                oss << ']';
            } else {
                oss << '[';
                for (size_t j = 0; j < matrix.size(); ++j) {
                    if (j > 0) {
                        oss << ", ";
                    }
                    oss << '[';
                    const auto &row = matrix[j];
                    for (size_t k = 0; k < row.size(); ++k) {
                        if (k > 0) {
                            oss << ", ";
                        }
                        oss << '"' << row[k] << '"';
                    }
                    oss << ']';
                }
                oss << ']';
            }
        }
        oss << ']';
        return oss.str();
    };

    try {
        TypedValue typed = interpreter_->evaluate_typed_expression(expr);
        if (!typed.needs_deferred_evaluation()) {
            if (typed.is_struct()) {
                io_interface_->write_string("(struct)");
                return;
            }

            write_typed_value(typed);
            return;
        }
    } catch (const ReturnException &ret) {
        if (ret.is_array) {
            std::string rendered = !ret.str_array_3d.empty()
                                       ? render_string_array(ret)
                                       : render_int_array(ret);
            io_interface_->write_string(rendered.c_str());
            return;
        }

        if (ret.type == TYPE_STRING) {
            io_interface_->write_string(ret.str_value.c_str());
            return;
        }

        write_numeric_value(io_interface_, ret.type, ret.value,
                            ret.double_value, ret.quad_value);
        return;
    } catch (const std::exception &) {
        // フォールバック処理へ移行
    }

    if (expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
        io_interface_->write_string(expr->str_value.c_str());
        return;
    }

    if (expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        print_array_literal(expr, print_array_literal);
        return;
    }

    if (expr->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *var = find_variable(expr->name);

        // 参照型変数の場合、参照先変数を取得
        if (var && var->is_reference) {
            var = reinterpret_cast<Variable *>(var->value);
            if (!var) {
                io_interface_->write_string("(invalid reference)");
                return;
            }
        }

        if (var && var->type == TYPE_STRING) {
            io_interface_->write_string(var->str_value.c_str());
            return;
        }

        if (!var) {
            // 変数が見つからない場合、式として評価
            evaluate_numeric_and_write(expr);
            return;
        }

        // 変数の値を直接出力
        if (var) {
            // ポインタ型または関数ポインタの場合
            if (var->type == TYPE_POINTER) {
                // ポインタ値を表示
                uint64_t clean_value = static_cast<uint64_t>(var->value);
                if (var->value & (1LL << 63)) {
                    // タグビットが設定されている場合は除去
                    clean_value &= ~(1ULL << 63);
                }

                std::ostringstream oss;
                oss << "0x" << std::hex << clean_value;

                io_interface_->write_string(oss.str().c_str());
            } else {
                write_numeric_value(io_interface_, var->type, var->value,
                                    var->double_value, var->quad_value);
            }
            return;
        }

        evaluate_numeric_and_write(expr);
        return;
    }

    if (expr->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        std::string struct_name;
        std::string member_name = expr->name;

        if (expr->left && expr->left->node_type == ASTNodeType::AST_VARIABLE) {
            struct_name = expr->left->name;
        } else if (expr->left &&
                   expr->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列要素のメンバーアクセス: array[index].member または
            // func()[index].member
            if (!expr->left->left) {
                io_interface_->write_string("(null array reference)");
                return;
            }

            // 通常の配列要素のメンバーアクセス: array[index].member
            std::string array_name = expr->left->left->name;
            int64_t index = evaluate_expression(expr->left->array_index.get());
            struct_name = array_name + "[" + std::to_string(index) + "]";
        } else if (expr->left &&
                   expr->left->node_type == ASTNodeType::AST_UNARY_OP &&
                   expr->left->op == "DEREFERENCE") {
            // デリファレンスされたポインタからのメンバーアクセス: (*pp).x
            // デリファレンスを評価して構造体を取得
            try {
                int64_t ptr_value = evaluate_expression(expr->left.get());
                // ポインタ値から構造体変数を取得
                Variable *struct_var = reinterpret_cast<Variable *>(ptr_value);
                if (!struct_var) {
                    io_interface_->write_string("(null pointer dereference)");
                    return;
                }

                // メンバーにアクセス
                auto member_it = struct_var->struct_members.find(member_name);
                if (member_it != struct_var->struct_members.end()) {
                    if (member_it->second.type == TYPE_STRING) {
                        io_interface_->write_string(
                            member_it->second.str_value.c_str());
                    } else {
                        io_interface_->write_number(member_it->second.value);
                    }
                    return;
                } else {
                    io_interface_->write_string("(member not found)");
                    return;
                }
            } catch (const std::exception &) {
                io_interface_->write_string("(deref member access error)");
                return;
            }
        } else {
            io_interface_->write_string("(invalid member access)");
            return;
        }

        try {
            Variable *member_var =
                interpreter_->get_struct_member(struct_name, member_name);
            if (member_var->type == TYPE_STRING) {
                io_interface_->write_string(member_var->str_value.c_str());
            } else {
                io_interface_->write_number(member_var->value);
            }
        } catch (const std::exception &) {
            io_interface_->write_string("(member access error)");
        }
        return;
    }

    if (expr->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        std::string obj_name;
        if (expr->left && expr->left->node_type == ASTNodeType::AST_VARIABLE) {
            obj_name = expr->left->name;
        } else {
            io_interface_->write_string("(invalid member array access)");
            return;
        }

        std::string member_name = expr->name;
        int64_t index = evaluate_expression(expr->right.get());

        try {
            Variable *member_var =
                interpreter_->get_struct_member(obj_name, member_name);
            if (member_var && member_var->is_array) {
                if (member_var->type == TYPE_STRING) {
                    print_string_array_element(member_var, index);
                } else {
                    int64_t value =
                        interpreter_->get_struct_member_array_element(
                            obj_name, member_name, static_cast<int>(index));
                    io_interface_->write_number(value);
                }
            } else {
                io_interface_->write_string("(not an array member)");
            }
        } catch (const std::exception &) {
            io_interface_->write_string("(member array access error)");
        }
        return;
    }

    if (expr->node_type == ASTNodeType::AST_ARRAY_REF) {
        std::string var_name;

        if (expr->left && expr->left->node_type == ASTNodeType::AST_VARIABLE) {
            var_name = expr->left->name;
        } else if (!expr->name.empty()) {
            var_name = expr->name;
        } else if (expr->left) {
            if (expr->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                ASTNode *base_node = expr->left.get();
                while (base_node &&
                       base_node->node_type == ASTNodeType::AST_ARRAY_REF &&
                       base_node->left) {
                    base_node = base_node->left.get();
                }

                if (base_node &&
                    base_node->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable *var = find_variable(base_node->name);
                    if (var && var->is_multidimensional &&
                        var->array_type_info.base_type == TYPE_STRING) {
                        std::vector<int64_t> indices;
                        int64_t outer_index =
                            evaluate_expression(expr->array_index.get());
                        int64_t inner_index =
                            evaluate_expression(expr->left->array_index.get());

                        indices.push_back(inner_index);
                        indices.push_back(outer_index);

                        try {
                            std::string result =
                                interpreter_
                                    ->getMultidimensionalStringArrayElement(
                                        *var, indices);
                            io_interface_->write_string(result.c_str());
                            return;
                        } catch (const std::exception &) {
                            io_interface_->write_string(
                                "(string array access error)");
                            return;
                        }
                    }
                }
            }

            evaluate_numeric_and_write(expr);
            return;
        } else {
            io_interface_->write_string("(invalid array ref)");
            return;
        }

        Variable *var = find_variable(var_name);
        if (var && var->is_array) {
            if (!expression_service_) {
                expression_service_ = interpreter_->get_expression_service();
            }

            int64_t index = 0;
            if (expression_service_) {
                index = expression_service_->evaluate_array_index(
                    expr->array_index.get(), var->array_size, var_name);
            } else {
                index = evaluate_expression(expr->array_index.get());
                if (index < 0 || index >= var->array_size) {
                    error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR,
                              var_name.c_str());
                    throw std::runtime_error("Array out of bounds");
                }
            }

            if (var->type >= TYPE_ARRAY_BASE) {
                TypeInfo elem_type =
                    static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
                if (elem_type == TYPE_STRING) {
                    print_string_array_element(var, index);
                } else {
                    print_numeric_array_element(var, index);
                }
            } else if (var->type == TYPE_STRING && var->is_array) {
                print_string_array_element(var, index);
            } else {
                print_numeric_array_element(var, index);
            }
            return;
        }

        if (var && var->type == TYPE_STRING && !var->is_array) {
            int64_t index = evaluate_expression(expr->array_index.get());
            size_t utf8_length = utf8_utils::utf8_char_count(var->str_value);

            if (index >= 0 && index < static_cast<int64_t>(utf8_length)) {
                std::string utf8_char = utf8_utils::utf8_char_at(
                    var->str_value, static_cast<size_t>(index));
                io_interface_->write_string(utf8_char.c_str());
            } else {
                error_msg(DebugMsgId::STRING_OUT_OF_BOUNDS_ERROR,
                          var_name.c_str(), index, utf8_length);
                throw std::runtime_error("String out of bounds");
            }
            return;
        }

        evaluate_numeric_and_write(expr);
        return;
    }

    if (expr->node_type == ASTNodeType::AST_FUNC_CALL) {
        const ASTNode *func = find_function(expr->name);
        if (func && func->type_info == TYPE_STRING) {
            interpreter_->push_interpreter_scope();

            for (size_t i = 0; i < func->parameters.size(); ++i) {
                int64_t arg_value =
                    evaluate_expression(expr->arguments[i].get());
                Variable param;
                param.type = func->parameters[i]->type_info;
                param.value = arg_value;
                param.is_assigned = true;
                interpreter_->get_current_scope().variables.insert_or_assign(
                    func->parameters[i]->name, std::move(param));
            }

            try {
                interpreter_->exec_statement(func->body.get());
                interpreter_->pop_interpreter_scope();
            } catch (const ReturnException &e) {
                interpreter_->pop_interpreter_scope();
                if (e.type == TYPE_STRING) {
                    io_interface_->write_string(e.str_value.c_str());
                } else {
                    write_numeric_value(io_interface_, e.type, e.value,
                                        e.double_value, e.quad_value);
                }
            }
            return;
        }

        evaluate_numeric_and_write(expr);
        return;
    }

    evaluate_numeric_and_write(expr);
}

void OutputManager::print_value_with_newline(const ASTNode *expr) {
    print_value(expr);
    io_interface_->write_newline();
}

void OutputManager::print_newline() { io_interface_->write_newline(); }

void OutputManager::print_multiple_with_newline(const ASTNode *arg_list) {
    print_multiple(arg_list);
    io_interface_->write_newline();
}

void OutputManager::print_formatted(const ASTNode *format_str,
                                    const ASTNode *arg_list) {
    print_formatted(format_str, arg_list, 0);
}

void OutputManager::print_formatted_with_newline(const ASTNode *format_str,
                                                 const ASTNode *arg_list) {
    print_formatted(format_str, arg_list);
    io_interface_->write_newline();
}
// オフセット付きprint_formatted（指定されたインデックスから引数を開始する）
void OutputManager::print_formatted(const ASTNode *format_str,
                                    const ASTNode *arg_list,
                                    size_t start_index) {
    debug_msg(DebugMsgId::PRINTF_OFFSET_CALLED, start_index);

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

    bool allow_print_nodes = start_index > 0;
    collect_formatted_arguments(arg_list, start_index, allow_print_nodes,
                                int_args, str_args, double_args, quad_args,
                                type_args);

    std::string rendered =
        render_formatted_string(format_str->str_value, int_args, str_args,
                                double_args, quad_args, type_args, true);

    io_interface_->write_string(rendered.c_str());
}
std::string OutputManager::process_escape_sequences(const std::string &input) {
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

bool OutputManager::has_unescaped_format_specifiers(const std::string &str) {
    debug_msg(DebugMsgId::PRINT_FORMAT_SPEC_CHECKING, str.c_str());
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            // \% でエスケープされているかチェック
            if (i > 0 && str[i - 1] == '\\') {
                continue; // エスケープされている
            }
            // 次の文字がフォーマット指定子かチェック
            if (i + 1 < str.length()) {
                char next = str[i + 1];
                if (next == 'd' or next == 's' or next == 'c' or next == 'p' or
                    next == 'f' or next == '%') {
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_SPEC_FOUND,
                              std::string(1, next).c_str());
                    return true;
                }
                // %lld のチェック
                if (next == 'l' && i + 3 < str.length() && str[i + 2] == 'l' &&
                    str[i + 3] == 'd') {
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_SPEC_FOUND, "lld");
                    return true;
                }
            }
        }
    }
    debug_msg(DebugMsgId::PRINT_NO_FORMAT_SPECIFIERS);
    return false;
}

size_t OutputManager::count_format_specifiers(const std::string &str) {
    size_t count = 0;
    debug_msg(DebugMsgId::OUTPUT_FORMAT_COUNT, str.c_str());
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            // \% でエスケープされているかチェック
            if (i > 0 && str[i - 1] == '\\') {
                continue; // エスケープされている
            }
            if (i + 1 < str.length()) {
                char next = str[i + 1];
                if (next == 'd' or next == 's' or next == 'c' or next == 'p' or
                    next == 'f') {
                    count++;
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_COUNT,
                              std::to_string(count).c_str());
                } else if (next == 'l' && i + 3 < str.length() &&
                           str[i + 2] == 'l' && str[i + 3] == 'd') {
                    count++;
                    debug_msg(DebugMsgId::OUTPUT_FORMAT_COUNT,
                              std::to_string(count).c_str());
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
                  (arg_list->node_type == ASTNodeType::AST_PRINT_STMT)
                      ? "AST_PRINT_STMT"
                      : "AST_PRINTLN_STMT",
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
            debug_msg(DebugMsgId::PRINT_SINGLE_ARG_DEBUG, "AST_PRINT_STMT",
                      (int)arg->node_type);
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
            debug_msg(DebugMsgId::PRINT_CHECKING_ARGUMENT, (int)i,
                      (int)arg->node_type);
            if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                std::string str_val = arg->str_value;
                debug_msg(DebugMsgId::PRINT_FOUND_STRING_LITERAL,
                          str_val.c_str());
                // \% エスケープされていない % を探す
                if (has_unescaped_format_specifiers(str_val)) {
                    debug_msg(DebugMsgId::PRINT_PRINTF_FORMAT_FOUND);
                    // フォーマット指定子が見つかった場合

                    // 前の引数をスペース区切りで出力
                    for (size_t j = 0; j < i; j++) {
                        if (j > 0)
                            io_interface_->write_char(' ');
                        print_value(arg_list->arguments[j].get());
                    }
                    if (i > 0)
                        io_interface_->write_char(' ');

                    // printf形式の処理：元のarg_listを使用してコピーを避ける
                    print_formatted(arg.get(), arg_list, i + 1);

                    // 余分な引数処理は不要 -
                    // print_formattedが全ての引数を処理する 改行なし
                    return;
                }
            }
        }

        // フォーマット指定子が見つからない場合は引数を順番に出力
        for (size_t i = 0; i < arg_list->arguments.size(); ++i) {
            if (i > 0)
                io_interface_->write_char(' '); // スペース区切りで出力
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
        debug_msg(DebugMsgId::PRINT_CHECKING_ARGUMENT, (int)i,
                  (int)arg->node_type);
        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
            std::string str_val = arg->str_value;
            debug_msg(DebugMsgId::PRINT_FOUND_STRING_LITERAL, str_val.c_str());
            // \% エスケープされていない % を探す
            if (has_unescaped_format_specifiers(str_val)) {
                debug_msg(DebugMsgId::PRINT_PRINTF_FORMAT_FOUND);
                // フォーマット指定子が見つかった場合

                // 前の引数をスペース区切りで出力
                for (size_t j = 0; j < i; j++) {
                    if (j > 0)
                        io_interface_->write_char(' ');
                    print_value(arg_list->arguments[j].get());
                }
                if (i > 0)
                    io_interface_->write_char(' ');

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
std::string OutputManager::format_string(const ASTNode *format_str,
                                         const ASTNode *arg_list) {
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
                            resolved =
                                typed.is_floating() ? TYPE_DOUBLE : TYPE_INT;
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

                if (type_args[index] == TYPE_UNKNOWN &&
                    hinted_type != TYPE_UNKNOWN) {
                    type_args[index] = hinted_type;
                    if (hinted_type == TYPE_STRING && str_args[index].empty()) {
                        if (auto *var = find_variable(current->name)) {
                            str_args[index] = var->str_value;
                        }
                    }
                    if ((hinted_type == TYPE_FLOAT ||
                         hinted_type == TYPE_DOUBLE ||
                         hinted_type == TYPE_QUAD) &&
                        quad_args[index] == 0.0L) {
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
        int written =
            std::snprintf(buffer.data(), buffer.size(), fmt.c_str(), value);
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
        case 'p': {
            // ポインタのアドレスを16進数で表示（0x前綴り）
            fmt += "p";
            void *ptr_value = reinterpret_cast<void *>(
                static_cast<uintptr_t>(int_args[arg_index]));
            formatted = format_with_snprintf(fmt, ptr_value);
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
