#include "output/output_manager.h"
#include "core/interpreter.h"
#include "evaluator/expression_evaluator.h"
#include "managers/type_manager.h"
#include "services/expression_service.h" // DRY効率化: 統一式評価サービス
// #include "services/expression_service.h" // DRY効率化: 循環依存解決まで一時コメントアウト
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../../../common/utf8_utils.h"
#include "../../../common/io_interface.h"
#include <cinttypes>
#include <cstdio>
#include <cctype>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

    if (expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
        io_interface_->write_string(expr->str_value.c_str());
    } else if (expr->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *var = find_variable(expr->name);
        if (var && var->type == TYPE_UNION) {
            // union型変数の場合、current_typeに基づいて出力
            if (var->current_type == TYPE_STRING) {
                io_interface_->write_string(var->str_value.c_str());
            } else {
                io_interface_->write_number(var->value);
            }
        } else if (var && var->type == TYPE_STRING) {
            io_interface_->write_string(var->str_value.c_str());
        } else {
            // 他の式の場合は評価を試行
            try {
                int64_t value = evaluate_expression(expr);
                io_interface_->write_number(value);
            } catch (const ReturnException& ret) {
                if (ret.type == TYPE_STRING) {
                    io_interface_->write_string(ret.str_value.c_str());
                } else {
                    io_interface_->write_number(ret.value);
                }
            }
        }
    } else if (expr->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数・メソッド呼び出しの戻り値処理
        try {
            int64_t value = evaluate_expression(expr);
            io_interface_->write_number(value);
        } catch (const ReturnException& ret) {
            if (ret.type == TYPE_STRING) {
                io_interface_->write_string(ret.str_value.c_str());
            } else {
                io_interface_->write_number(ret.value);
            }
        }
    } else if (expr->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // struct メンバーアクセス: obj.member または array[index].member
        std::string struct_name;
        std::string member_name = expr->name;
        
        if (expr->left && expr->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常のstruct変数: obj.member
            struct_name = expr->left->name;
        } else if (expr->left && expr->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素: array[index].member
            if (!expr->left->left) {
                io_interface_->write_string("(null array reference)");
                return;
            }
            std::string array_name = expr->left->left->name;
            int64_t index = evaluate_expression(expr->left->array_index.get());
            struct_name = array_name + "[" + std::to_string(index) + "]";
        } else {
            io_interface_->write_string("(invalid member access)");
            return;
        }
        
        try {
            Variable* member_var = interpreter_->get_struct_member(struct_name, member_name);
            

            
            // Union型の場合は実際の値の型を確認
            if (member_var->type == TYPE_STRING || 
                (!member_var->type_name.empty() && 
                 interpreter_->get_type_manager()->is_union_type(member_var->type_name) &&
                 !member_var->str_value.empty() && member_var->is_assigned)) {
                io_interface_->write_string(member_var->str_value.c_str());
            } else {
                io_interface_->write_number(member_var->value);
            }
        } catch (const std::exception& e) {
            io_interface_->write_string("(member access error)");
        }
    } else if (expr->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // メンバの配列アクセス: obj.member[index]
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
            // 構造体メンバー変数を取得して型を確認
            Variable *member_var = interpreter_->get_struct_member(obj_name, member_name);
            if (member_var && member_var->is_array) {
                if (member_var->type == TYPE_STRING || 
                    (!member_var->type_name.empty() && interpreter_->get_type_manager()->is_union_type(member_var->type_name) && !member_var->str_value.empty() && member_var->is_assigned)) {
                    // 文字列配列の場合
                    if (index >= 0 && index < static_cast<int64_t>(member_var->array_strings.size())) {
                        io_interface_->write_string(member_var->array_strings[index].c_str());
                    } else {
                        io_interface_->write_string("");
                    }
                } else {
                    // 数値配列の場合
                    int64_t value = interpreter_->get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
                    io_interface_->write_number(value);
                }
            } else {
                io_interface_->write_string("(not an array member)");
            }
        } catch (const std::exception& e) {
            io_interface_->write_string("(member array access error)");
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
        } else if (!expr->name.empty()) {
            // 旧構造: expr->name が直接変数名を持つ
            var_name = expr->name;
        } else if (expr->left) {
            // 複雑な左側の式（多次元配列アクセスなど）
            // まず構造体メンバーの多次元配列アクセスをチェック
            if (expr->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                ASTNode* base_node = expr->left.get();
                while (base_node && base_node->node_type == ASTNodeType::AST_ARRAY_REF && base_node->left) {
                    base_node = base_node->left.get();
                }
                
                // 構造体メンバーの多次元配列アクセス: test.data[0][0]
                if (base_node && base_node->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                    debug_msg(DebugMsgId::PRINTF_ARRAY_REF_DEBUG, "struct member multidim access");
                    
                    std::string obj_name = base_node->left->name;
                    std::string member_name = base_node->name;
                    
                    // 多次元インデックスを収集
                    std::vector<int64_t> indices;
                    const ASTNode* current_node = expr;
                    while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                        int64_t index = evaluate_expression(current_node->array_index.get());
                        indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                        current_node = current_node->left.get();
                    }
                    
                    try {
                        int64_t value = interpreter_->get_struct_member_multidim_array_element(obj_name, member_name, indices);
                        io_interface_->write_number(value);
                        return;
                    } catch (const std::exception& e) {
                        io_interface_->write_string("(struct member multidim access error)");
                        return;
                    }
                }
                // 通常の多次元配列の文字列アクセスをチェック
                else if (base_node && base_node->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable* var = find_variable(base_node->name);
                    if (var && var->is_multidimensional && var->array_type_info.base_type == TYPE_STRING) {
                        // 汎用的な多次元文字列配列処理
                        debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_ACCESS, base_node->name.c_str());
                        
                        std::vector<int64_t> indices;
                        const ASTNode* current = expr;
                        
                        // 再帰的にインデックスを収集（右から左へ）
                        while (current && current->node_type == ASTNodeType::AST_ARRAY_REF) {
                            int64_t index = evaluate_expression(current->array_index.get());
                            indices.insert(indices.begin(), index); // 先頭に挿入
                            current = current->left.get();
                        }
                        
                        // インデックスの情報をデバッグ出力
                        std::string indices_str;
                        for (size_t i = 0; i < indices.size(); ++i) {
                            if (i > 0) indices_str += ", ";
                            indices_str += std::to_string(indices[i]);
                        }
                        debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_INDICES, indices_str.c_str());
                        
                        try {
                            std::string result = interpreter_->getMultidimensionalStringArrayElement(*var, indices);
                            debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_VALUE, result.c_str());
                            io_interface_->write_string(result.c_str());
                            return;
                        } catch (const std::exception& e) {
                            io_interface_->write_string("(string array access error)");
                            return;
                        }
                    }
                }
            }
            
            // 通常の数値型多次元配列アクセス
            int64_t value = evaluate_expression(expr);
            io_interface_->write_number(value);
            return;
        } else {
            io_interface_->write_string("(invalid array ref)");
            return;
        }
        
        Variable *var = find_variable(var_name);
        if (var && var->is_array) {
            // DRY効率化: 境界チェック付き配列インデックス評価を統一サービスで処理
            if (!expression_service_) {
                expression_service_ = interpreter_->get_expression_service();
            }
            
            int64_t index;
            if (expression_service_) {
                index = expression_service_->evaluate_array_index(
                    expr->array_index.get(), var->array_size, var_name);
            } else {
                // フォールバック: 従来の境界チェック
                index = evaluate_expression(expr->array_index.get());
                if (index < 0 || index >= var->array_size) {
                    error_msg(DebugMsgId::ARRAY_OUT_OF_BOUNDS_ERROR, var_name.c_str());
                    throw std::runtime_error("Array out of bounds");
                }
            }

            // 配列型を確認
            if (var->type >= TYPE_ARRAY_BASE) {
                TypeInfo elem_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
                if (elem_type == TYPE_STRING) {
                    // 文字列配列の場合は文字列として出力
                    if (!var->array_strings.empty() && index < static_cast<int64_t>(var->array_strings.size())) {
                        io_interface_->write_string(var->array_strings[index].c_str());
                    } else {
                        io_interface_->write_string("");
                    }
                } else {
                    // 数値配列は数値として出力
                    if (!var->array_values.empty() && index < static_cast<int64_t>(var->array_values.size())) {
                        int64_t value = var->array_values[index];
                        io_interface_->write_number(value);
                    } else {
                        io_interface_->write_number(0);
                    }
                }
            } else if (var->type == TYPE_STRING && var->is_array) {
                // typedef文字列配列の特別ケース（type=TYPE_STRINGだがis_array=true）
                if (!var->array_strings.empty() && index < static_cast<int64_t>(var->array_strings.size())) {
                    io_interface_->write_string(var->array_strings[index].c_str());
                } else {
                    io_interface_->write_string("");
                }
            } else {
                // その他の配列型
                if (!var->array_values.empty() && index < static_cast<int64_t>(var->array_values.size())) {
                    int64_t value = var->array_values[index];
                    io_interface_->write_number(value);
                } else {
                    io_interface_->write_number(0);
                }
            }
        } else if (var && var->type == TYPE_STRING && !var->is_array) {
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
        // その他の式の評価
        try {
            int64_t value = evaluate_expression(expr);
            io_interface_->write_number(value);
        } catch (const ReturnException& ret) {
            if (ret.type == TYPE_STRING) {
                io_interface_->write_string(ret.str_value.c_str());
            } else {
                io_interface_->write_number(ret.value);
            }
        }
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
            printf("[DEBUG] Processing argument with node_type: %d\n", static_cast<int>(arg->node_type));
            
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
            } else if (arg->node_type == ASTNodeType::AST_FUNC_CALL) {
                // 関数呼び出しの処理
                try {
                    int64_t value = evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                } catch (const ReturnException& ret) {
                    if (ret.type == TYPE_STRING) {
                        // 文字列戻り値の関数
                        str_args.push_back(ret.str_value);
                        int_args.push_back(0); // プレースホルダー
                    } else {
                        // 数値戻り値の関数
                        int_args.push_back(ret.value);
                        str_args.push_back(""); // プレースホルダー
                    }
                }
            } else if (arg->node_type == ASTNodeType::AST_TERNARY_OP) {
                // 三項演算子の場合は型推論対応評価を使用
                auto* expr_evaluator = interpreter_->get_expression_evaluator();
                
                printf("[DEBUG] Processing ternary operator\n");
                
                // 三項演算子を評価（結果はキャッシュされる）
                int64_t dummy_result = evaluate_expression(arg.get());
                
                printf("[DEBUG] Ternary evaluation completed, dummy_result: %lld\n", (long long)dummy_result);
                
                // 型推論結果を取得
                const auto& typed_result = expr_evaluator->get_last_typed_result();
                
                printf("[DEBUG] Typed result - is_string: %d, as_string: %s, as_numeric: %lld\n", 
                       typed_result.is_string(), typed_result.as_string().c_str(), 
                       (long long)typed_result.as_numeric());
                
                if (typed_result.is_string()) {
                    str_args.push_back(typed_result.as_string());
                    int_args.push_back(0); // プレースホルダー
                    printf("[DEBUG] Added string result: %s\n", typed_result.as_string().c_str());
                } else {
                    int_args.push_back(typed_result.as_numeric());
                    str_args.push_back(""); // プレースホルダー
                    printf("[DEBUG] Added numeric result: %lld\n", (long long)typed_result.as_numeric());
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
                    if (arg_index < str_args.size()) {
                        // 文字列引数がある場合（空文字列も有効）
                        result += str_args[arg_index];
                    } else if (arg_index < int_args.size()) {
                        // 数値が渡された場合は文字列に変換
                        result += std::to_string(int_args[arg_index]);
                    } else {
                        // 引数がない場合
                        result += "(null)";
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
                // 引数が不足している場合、フォーマット指定子をそのまま出力
                result += '%';
                result += specifier;
                i = spec_end; // specifierの位置に移動
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
                
                debug_msg(DebugMsgId::PRINTF_PROCESSING_ARRAY_REF);
                
                // 文字列配列の場合の特別処理
                std::string array_name;
                const ASTNode* base_node = arg.get();
                while (base_node && base_node->node_type == ASTNodeType::AST_ARRAY_REF && base_node->left) {
                    base_node = base_node->left.get();
                }
                if (base_node && base_node->node_type == ASTNodeType::AST_VARIABLE) {
                    array_name = base_node->name;
                }
                
                Variable* var = find_variable(array_name);
                
                debug_msg(DebugMsgId::PRINTF_ARRAY_NAME_FOUND, array_name.c_str());
                debug_msg(DebugMsgId::PRINTF_VARIABLE_FOUND, var ? "true" : "false");
                
                if (var && var->is_array && var->array_type_info.base_type == TYPE_STRING) {
                    debug_msg(DebugMsgId::PRINTF_STRING_MULTIDIM_PROCESSING);
                    
                    // 多次元インデックスを収集
                    std::vector<int64_t> indices;
                    const ASTNode* current_node = arg.get();
                    while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                        int64_t index = evaluate_expression(current_node->array_index.get());
                        indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                        current_node = current_node->left.get();
                    }
                    
                    try {
                        std::string str_value = interpreter_->getMultidimensionalStringArrayElement(*var, indices);
                        debug_msg(DebugMsgId::PRINTF_STRING_VALUE_RETRIEVED, str_value.c_str());
                        str_args.push_back(str_value);
                        int_args.push_back(0); // プレースホルダー
                        continue; // 次の引数へ
                    } catch (const std::exception& e) {
                        str_args.push_back("");
                        int_args.push_back(0);
                        continue;
                    }
                }
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
                    } else if (var->type == TYPE_UNION) {
                        // ユニオン型変数の場合、current_typeに基づいて処理

                        if (var->current_type == TYPE_STRING) {
                            str_args.push_back(var->str_value);
                            int_args.push_back(0); // プレースホルダー
                        } else {
                            int_args.push_back(var->value);
                            str_args.push_back(""); // プレースホルダー
                        }
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
            } else if (arg->node_type == ASTNodeType::AST_TERNARY_OP) {
                // 三項演算子の処理 - 型推論エンジンを使用
                auto* expression_evaluator = interpreter_->get_expression_evaluator();
                TypedValue typed_result = expression_evaluator->evaluate_typed_expression(arg.get());
                
                if (typed_result.type.type_info == TYPE_STRING) {
                    str_args.push_back(typed_result.string_value);
                    int_args.push_back(0); // プレースホルダー
                } else {
                    int_args.push_back(typed_result.numeric_value);
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
                    if (member_var && (member_var->type == TYPE_STRING || 
                        (!member_var->type_name.empty() && interpreter_->get_type_manager()->is_union_type(member_var->type_name) && !member_var->str_value.empty() && member_var->is_assigned))) {
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
                    int64_t value = evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                }
            } else if (arg->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 配列要素の処理
                
                // まず、多次元構造体メンバー配列アクセスかどうかチェック（test.data[i][j]）
                if (arg->left && arg->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // 基底ノードを探して、構造体メンバーアクセスかチェック
                    ASTNode* base_node = arg->left.get();
                    while (base_node && base_node->node_type == ASTNodeType::AST_ARRAY_REF && base_node->left) {
                        base_node = base_node->left.get();
                    }
                    
                    if (base_node && base_node->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                        // 構造体メンバーの多次元配列アクセス: test.data[i][j]
                        std::string obj_name = base_node->left->name;
                        std::string member_name = base_node->name;
                        
                        // 多次元インデックスを収集
                        std::vector<int64_t> indices;
                        const ASTNode* current_node = arg.get();
                        while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                            int64_t index = evaluate_expression(current_node->array_index.get());
                            indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                            current_node = current_node->left.get();
                        }
                        
                        try {
                            int64_t value = interpreter_->get_struct_member_multidim_array_element(obj_name, member_name, indices);
                            int_args.push_back(value);
                            str_args.push_back(""); // プレースホルダー
                        } catch (const std::exception& e) {
                            int_args.push_back(0);
                            str_args.push_back("");
                        }
                    } else {
                        // 通常の多次元配列アクセス
                        int64_t value = evaluate_expression(arg.get());
                        int_args.push_back(value);
                        str_args.push_back(""); // プレースホルダー
                    }
                }
                // 構造体メンバー配列アクセスかどうかチェック（obj.member[index]）
                else if (arg->left && arg->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
                    // 構造体メンバー配列アクセス: obj.member[index]
                    std::string obj_name = arg->left->left->name;
                    std::string member_name = arg->left->name;
                    int64_t index = evaluate_expression(arg->array_index.get());
                    
                    try {
                        // 文字列配列かどうかを判定
                        Variable* member_var = interpreter_->get_struct_member(obj_name, member_name);
                        if (member_var->type == TYPE_STRING || 
                            (!member_var->type_name.empty() && interpreter_->get_type_manager()->is_union_type(member_var->type_name) && !member_var->str_value.empty() && member_var->is_assigned)) {
                            // 文字列配列の場合
                            std::string str_value = interpreter_->get_struct_member_array_string_element(
                                obj_name, member_name, static_cast<int>(index));
                            str_args.push_back(str_value);
                            int_args.push_back(0); // プレースホルダー
                        } else {
                            // 数値配列の場合
                            int64_t value = interpreter_->get_struct_member_array_element(obj_name, member_name, static_cast<int>(index));
                            int_args.push_back(value);
                            str_args.push_back(""); // プレースホルダー
                        }
                    } catch (const std::exception& e) {
                        int_args.push_back(0);
                        str_args.push_back("");
                    }
                } else {
                    // 通常の配列アクセス（1次元と多次元の両方に対応）
                    Variable *var = find_variable(arg->left->name);
                    if (var && var->is_array) {
                        if (var->is_multidimensional) {
                            // 多次元配列のアクセス
                            // 多次元インデックスを収集
                            std::vector<int64_t> indices;
                            const ASTNode* current_node = arg.get();
                            while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                                int64_t index = evaluate_expression(current_node->array_index.get());
                                indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                                current_node = current_node->left.get();
                            }
                            
                            try {
                                if (var->array_type_info.base_type == TYPE_STRING) {
                                    // 文字列多次元配列の場合
                                    std::string str_value = interpreter_->getMultidimensionalStringArrayElement(*var, indices);
                                    str_args.push_back(str_value);
                                    int_args.push_back(0); // プレースホルダー
                                } else {
                                    // 数値多次元配列の場合
                                    int64_t value = interpreter_->getMultidimensionalArrayElement(*var, indices);
                                    int_args.push_back(value);
                                    str_args.push_back(""); // プレースホルダー
                                }
                            } catch (const std::exception& e) {
                                int_args.push_back(0);
                                str_args.push_back("");
                            }
                        } else {
                            // 1次元配列のアクセス
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
                // その他の配列アクセスや式
                // 配列アクセスで文字列型の場合の特別処理
                if (arg->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // ベース変数名を取得
                    std::string base_var_name;
                    if (arg->left && arg->left->node_type == ASTNodeType::AST_VARIABLE) {
                        base_var_name = arg->left->name;
                    } else if (arg->left && arg->left->node_type == ASTNodeType::AST_ARRAY_REF) {
                        // 多次元配列の場合、再帰的にベース名を取得
                        const ASTNode* current = arg.get();
                        while (current && current->node_type == ASTNodeType::AST_ARRAY_REF) {
                            if (current->left && current->left->node_type == ASTNodeType::AST_VARIABLE) {
                                base_var_name = current->left->name;
                                break;
                            }
                            current = current->left.get();
                        }
                    }
                    
                    Variable* var = find_variable(base_var_name);
                    if (var) {
                    }
                    
                    if (var && var->is_array && var->array_type_info.base_type == TYPE_STRING) {
                        
                        // 多次元インデックスを収集
                        std::vector<int64_t> indices;
                        const ASTNode* current_node = arg.get();
                        while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                            int64_t index = evaluate_expression(current_node->array_index.get());
                            indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                            current_node = current_node->left.get();
                        }
                        
                        try {
                            if (var->is_multidimensional) {
                                std::string str_value = interpreter_->getMultidimensionalStringArrayElement(*var, indices);
                                str_args.push_back(str_value);
                                int_args.push_back(0); // プレースホルダー
                            } else {
                                // 1次元配列
                                if (indices.size() == 1 && indices[0] < static_cast<int64_t>(var->array_strings.size())) {
                                    str_args.push_back(var->array_strings[indices[0]]);
                                    int_args.push_back(0); // プレースホルダー
                                } else {
                                    str_args.push_back("");
                                    int_args.push_back(0);
                                }
                            }
                        } catch (const std::exception& e) {
                            str_args.push_back("");
                            int_args.push_back(0);
                        }
                        continue; // 次の引数へ
                    }
                }
                
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
                if (next == 'd' || next == 's' || next == 'c' || next == '%') {
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
                if (next == 'd' || next == 's' || next == 'c') {
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
    if (!format_str || format_str->node_type != ASTNodeType::AST_STRING_LITERAL) {
        return "(invalid format)";
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
            } else if (arg->node_type == ASTNodeType::AST_FUNC_CALL) {
                // 関数呼び出しの処理
                try {
                    int64_t value = evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                } catch (const ReturnException& ret) {
                    if (ret.type == TYPE_STRING) {
                        // 文字列戻り値の関数
                        str_args.push_back(ret.str_value);
                        int_args.push_back(0); // プレースホルダー
                    } else {
                        // 数値戻り値の関数
                        int_args.push_back(ret.value);
                        str_args.push_back(""); // プレースホルダー
                    }
                }
            } else {
                // その他の式（AST_MEMBER_ARRAY_ACCESSなども含む）
                if (arg->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
                    // 構造体メンバーの配列アクセス: obj.member[index]
                    std::string obj_name;
                    if (arg->left && arg->left->node_type == ASTNodeType::AST_VARIABLE) {
                        obj_name = arg->left->name;
                        std::string member_name = arg->name;
                        int64_t index = evaluate_expression(arg->right.get());
                        
                        try {
                            Variable *member_var = interpreter_->get_struct_member(obj_name, member_name);
                            if (member_var && member_var->is_array && (member_var->type == TYPE_STRING || 
                                (!member_var->type_name.empty() && interpreter_->get_type_manager()->is_union_type(member_var->type_name) && !member_var->str_value.empty() && member_var->is_assigned))) {
                                // 文字列配列の場合は文字列を取得
                                std::string str_value = interpreter_->get_struct_member_array_string_element(obj_name, member_name, static_cast<int>(index));
                                str_args.push_back(str_value);
                                int_args.push_back(0); // プレースホルダー
                            } else {
                                // 数値配列の場合は通常の評価
                                int64_t value = evaluate_expression(arg.get());
                                int_args.push_back(value);
                                str_args.push_back(""); // プレースホルダー
                            }
                        } catch (const std::exception& e) {
                            // エラーの場合は空文字列
                            str_args.push_back("");
                            int_args.push_back(0);
                        }
                    } else {
                        // 通常の評価へフォールバック
                        int64_t value = evaluate_expression(arg.get());
                        int_args.push_back(value);
                        str_args.push_back(""); // プレースホルダー
                    }
                } else {
                    // その他の式は通常の評価
                    int64_t value = evaluate_expression(arg.get());
                    int_args.push_back(value);
                    str_args.push_back(""); // プレースホルダー
                }
            }        }
    }

    // フォーマット文字列を処理
    std::string result;
    size_t arg_index = 0;
    for (size_t i = 0; i < format.length(); i++) {
        if (format[i] == '\\' && i + 1 < format.length() && format[i + 1] == '%') {
            result += '%';
            i++;
        } else if (format[i] == '%' && i + 1 < format.length()) {
            size_t spec_start = i + 1;
            size_t spec_end = spec_start;
            int width = 0;
            bool zero_pad = false;

            if (spec_end < format.length() && format[spec_end] == '0') {
                zero_pad = true;
                spec_end++;
            }

            while (spec_end < format.length() && std::isdigit(format[spec_end])) {
                width = width * 10 + (format[spec_end] - '0');
                spec_end++;
            }

            if (spec_end >= format.length()) {
                result += format[i];
                continue;
            }

            char specifier = format[spec_end];

            if (specifier == '%') {
                result += '%';
                i = spec_end;
                continue;
            }

            if (arg_index < int_args.size()) {
                switch (specifier) {
                case 'd':
                case 'i': {
                    int64_t value = int_args[arg_index];
                    std::string num_str = std::to_string(value);
                    if (width > 0 && num_str.length() < static_cast<size_t>(width)) {
                        char pad_char = zero_pad ? '0' : ' ';
                        num_str = std::string(width - num_str.length(), pad_char) + num_str;
                    }
                    result += num_str;
                    break;
                }
                case 's': {
                    std::string str_value = str_args[arg_index];
                    if (width > 0 && str_value.length() < static_cast<size_t>(width)) {
                        str_value = std::string(width - str_value.length(), ' ') + str_value;
                    }
                    result += str_value;
                    break;
                }
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
    
    return result;
}
