#include "variable_manager.h"
#include "../common/debug_messages.h"
#include "array_manager.h"
#include "evaluator/expression_evaluator.h"
#include "interpreter.h"
#include "type_manager.h"

void VariableManager::push_scope() {
    // std::cerr << "DEBUG: push_scope called, stack size: " <<
    // interpreter_->scope_stack.size() << " -> " <<
    // (interpreter_->scope_stack.size() + 1) << std::endl;
    interpreter_->scope_stack.push_back(Scope{});
}

void VariableManager::pop_scope() {
    if (interpreter_->scope_stack.size() > 1) {
        // std::cerr << "DEBUG: pop_scope called, stack size: " <<
        // interpreter_->scope_stack.size() << " -> " <<
        // (interpreter_->scope_stack.size() - 1) << std::endl;
        interpreter_->scope_stack.pop_back();
    } else {
        // std::cerr << "DEBUG: pop_scope called but size is " <<
        // interpreter_->scope_stack.size() << ", not popping" << std::endl;
    }
}

Scope &VariableManager::current_scope() {
    return interpreter_->scope_stack.back();
}

Variable *VariableManager::find_variable(const std::string &name) {
    // std::cerr << "DEBUG: find_variable called for: " << name << std::endl;

    // ローカルスコープから検索
    // std::cerr << "DEBUG: Searching in " << interpreter_->scope_stack.size()
    // << " local scopes" << std::endl;
    for (auto it = interpreter_->scope_stack.rbegin();
         it != interpreter_->scope_stack.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            // std::cerr << "DEBUG: Found " << name << " in local scope" <<
            // std::endl;
            return &var_it->second;
        }
    }

    // グローバルスコープから検索
    // std::cerr << "DEBUG: Searching in global scope" << std::endl;
    auto global_var_it = interpreter_->global_scope.variables.find(name);
    if (global_var_it != interpreter_->global_scope.variables.end()) {
        // std::cerr << "DEBUG: Found " << name << " in global scope" <<
        // std::endl;
        return &global_var_it->second;
    }

    // static変数から検索
    Variable *static_var = interpreter_->find_static_variable(name);
    if (static_var) {
        return static_var;
    }

    // std::cerr << "DEBUG: Variable " << name << " not found anywhere" <<
    // std::endl;

    return nullptr;
}

void VariableManager::assign_variable(const std::string &name, int64_t value,
                                      TypeInfo type, bool is_const) {
    debug_msg(DebugMsgId::VAR_ASSIGN_READABLE, name.c_str(), value, "type",
              is_const ? "true" : "false");
    Variable *var = find_variable(name);
    if (!var) {
        debug_msg(DebugMsgId::VAR_CREATE_NEW);
        // 新しい変数を作成
        Variable new_var;
        new_var.type = type;
        new_var.value = value;
        new_var.is_assigned = true;
        new_var.is_const = is_const;
        interpreter_->type_manager_->check_type_range(type, value, name);
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
        interpreter_->type_manager_->check_type_range(var->type, value, name);
        var->value = value;
        var->is_assigned = true;
    }
}

void VariableManager::assign_function_parameter(const std::string &name,
                                                int64_t value, TypeInfo type) {
    // 関数パラメータは常に現在のスコープに新しい変数として作成
    Variable new_var;
    new_var.type = type;
    new_var.value = value;
    new_var.is_assigned = true;
    new_var.is_const = false;
    interpreter_->type_manager_->check_type_range(type, value, name);
    current_scope().variables[name] = new_var;
}

void VariableManager::assign_array_parameter(const std::string &name,
                                             const Variable &source_array,
                                             TypeInfo type) {
    (void)
        type; // パラメータは型チェック用に渡されるが、現在の実装ではsource_arrayの型を使用

    // 配列パラメータは新しい変数としてコピーを作成
    Variable param_var;

    // パラメータ変数の基本属性を設定
    param_var.is_array = true;
    param_var.is_assigned = true;
    param_var.is_const = false; // パラメータは基本的にconst扱いしない
    param_var.type = source_array.type; // 元の配列と同じ型を使用

    // source_arrayと同じ次元・サイズの配列を作成
    if (source_array.is_multidimensional) {
        param_var.is_multidimensional = true;
        param_var.array_type_info = source_array.array_type_info;
        // array_managerのcopyArrayを使用してコピー
        interpreter_->array_manager_->copyArray(param_var, source_array);
    } else {
        param_var.is_multidimensional = false;
        param_var.array_size = source_array.array_size;
        param_var.array_dimensions =
            source_array.array_dimensions; // 次元情報をコピー
        param_var.array_values = source_array.array_values;
        param_var.array_strings = source_array.array_strings;
    }

    current_scope().variables[name] = param_var;
}

void VariableManager::assign_variable(const std::string &name,
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

void VariableManager::assign_array_element(const std::string &name,
                                           int64_t index, int64_t value) {
    Variable *var = find_variable(name);
    if (var && var->is_array) {
        // const配列への書き込みチェック
        if (var->is_const) {
            std::cerr << "エラー: const配列 '" << name
                      << "' への書き込みはできません" << std::endl;
            throw std::runtime_error("Cannot assign to const array");
        }

        if (index < 0 || index >= var->array_size) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
            throw std::runtime_error("Array index out of bounds");
        }

        // 型チェック
        TypeInfo elem_type = static_cast<TypeInfo>(var->type - TYPE_ARRAY_BASE);
        interpreter_->check_type_range(elem_type, value, name);

        var->array_values[index] = value;
    } else {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("Variable not found or not an array");
    }
}

void VariableManager::assign_string_element(const std::string &name,
                                            int64_t index, char value) {
    Variable *var = find_variable(name);
    if (!var || var->type != TYPE_STRING) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("Variable not found or not a string");
    }

    if (var->is_const) {
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
        throw std::runtime_error("Cannot modify const string");
    }

    if (index < 0 || static_cast<size_t>(index) >= var->str_value.length()) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("String index out of bounds");
    }

    var->str_value[index] = value;
}

void VariableManager::declare_global_variable(const ASTNode *node) {

    // グローバル変数の重複宣言チェック
    if (interpreter_->global_scope.variables.find(node->name) !=
        interpreter_->global_scope.variables.end()) {
        error_msg(DebugMsgId::VAR_REDECLARE_ERROR, node->name.c_str());
        throw std::runtime_error("Variable redeclaration error");
    }

    Variable var;

    // typedef解決
    if (node->type_info == TYPE_UNKNOWN && !node->type_name.empty()) {
        std::string resolved_type =
            interpreter_->type_manager_->resolve_typedef(node->type_name);

        // 配列typedefの場合
        if (resolved_type.find("[") != std::string::npos) {
            std::string base = resolved_type.substr(0, resolved_type.find("["));
            std::string array_part =
                resolved_type.substr(resolved_type.find("["));

            TypeInfo base_type =
                interpreter_->type_manager_->string_to_type_info(base);
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
            var.is_array = true;

            // 配列サイズを解析 [3] -> 3
            if (array_part.length() > 2 && array_part[0] == '[' &&
                array_part[array_part.length() - 1] == ']') {
                std::string size_str =
                    array_part.substr(1, array_part.length() - 2);

                if (size_str.empty()) {
                    // 動的配列（TYPE[]）はサポートされていない
                    error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                              node->name.c_str());
                    throw std::runtime_error(
                        "Dynamic arrays are not supported yet");
                }

                var.array_size = std::stoi(size_str);

                // array_dimensionsを設定
                var.array_dimensions.clear();
                var.array_dimensions.push_back(var.array_size);

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
            var.type = interpreter_->type_manager_->string_to_type_info(
                node->type_name);
        }
    } else if (!node->type_name.empty() &&
               node->type_name.find("[") != std::string::npos) {
        // 直接配列宣言の場合（例：int[5] global_array）
        std::string base = node->type_name.substr(0, node->type_name.find("["));
        std::string array_part =
            node->type_name.substr(node->type_name.find("["));

        TypeInfo base_type =
            interpreter_->type_manager_->string_to_type_info(base);
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
        var.is_array = true;

        // 配列サイズを解析 [5] -> 5
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
        var.type = node->type_info;
    }

    var.is_const = node->is_const;
    var.is_assigned = false;

    interpreter_->global_scope.variables[node->name] = var;
}

void VariableManager::declare_local_variable(const ASTNode *node) {
    // ローカル変数の宣言
    Variable var;
    var.is_array = false;
    var.array_size = 0;

    // typedef解決
    if (node->type_info == TYPE_UNKNOWN && !node->type_name.empty()) {
        std::string resolved_type =
            interpreter_->type_manager_->resolve_typedef(node->type_name);

        std::cerr << "[DEBUG] Variable: " << node->name
                  << ", Type: " << node->type_name
                  << ", Resolved: " << resolved_type << std::endl;

        // 配列typedefの場合
        if (resolved_type.find("[") != std::string::npos) {
            std::string base = resolved_type.substr(0, resolved_type.find("["));
            std::string array_part =
                resolved_type.substr(resolved_type.find("["));

            TypeInfo base_type =
                interpreter_->type_manager_->string_to_type_info(base);
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
            var.is_array = true;

            // 配列サイズを解析 [3] -> 3
            if (array_part.length() > 2 && array_part[0] == '[' &&
                array_part[array_part.length() - 1] == ']') {
                std::string size_str =
                    array_part.substr(1, array_part.length() - 2);

                if (size_str.empty()) {
                    // 動的配列（TYPE[]）はサポートされていない
                    error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                              node->name.c_str());
                    throw std::runtime_error(
                        "Dynamic arrays are not supported yet");
                }

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
            var.type = interpreter_->type_manager_->string_to_type_info(
                node->type_name);
        }
    } else if (!node->type_name.empty() &&
               node->type_name.find("[") != std::string::npos) {
        // 直接配列宣言の場合（例：int[5] local_array）
        std::string base = node->type_name.substr(0, node->type_name.find("["));
        std::string array_part =
            node->type_name.substr(node->type_name.find("["));

        TypeInfo base_type =
            interpreter_->type_manager_->string_to_type_info(base);
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
        var.is_array = true;

        // 配列サイズを解析 [5] -> 5
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
        var.type = node->type_info != TYPE_VOID ? node->type_info : TYPE_INT;
    }

    var.is_const = node->is_const;
    var.is_assigned = false;

    // 初期値が指定されている場合は評価して設定
    if (node->children.size() > 0 && node->children[0]) {
        int64_t value = interpreter_->evaluate(node->children[0].get());
        var.value = value;
        var.is_assigned = true;

        // 型範囲チェック
        interpreter_->type_manager_->check_type_range(var.type, value,
                                                      node->name);
    }

    current_scope().variables[node->name] = var;
}

void VariableManager::assign_variable(const std::string &name,
                                      const std::string &value, bool is_const) {
    debug_msg(DebugMsgId::STRING_ASSIGN_READABLE, name.c_str(), value.c_str(),
              is_const ? "true" : "false");
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

void VariableManager::process_var_decl_or_assign(const ASTNode *node) {
    // std::cerr << "DEBUG: Creating variable: " << node->name << std::endl;
    if (node->node_type == ASTNodeType::AST_VAR_DECL) {
        // 変数宣言の処理
        Variable var;
        var.type = node->type_info;
        var.is_const = node->is_const;
        var.is_assigned = false;
        var.is_array = false;
        var.array_size = 0;

        // 新しいArrayTypeInfoが設定されている場合の処理
        if (node->array_type_info.base_type != TYPE_UNKNOWN) {
            // ArrayTypeInfoが設定されている場合は配列として処理
            var.is_array = true;
            var.type = node->array_type_info.base_type;
            var.array_type_info = node->array_type_info;

            // 配列サイズ情報をコピー
            if (!node->array_type_info.dimensions.empty()) {
                var.array_size = node->array_type_info.dimensions[0].size;
                var.array_dimensions.clear();
                for (const auto &dim : node->array_type_info.dimensions) {
                    var.array_dimensions.push_back(dim.size);
                }

                // 多次元配列かどうかをチェック
                if (var.array_dimensions.size() > 1) {
                    var.is_multidimensional = true;
                }

                // 配列初期化
                int total_size = 1;
                for (int dim : var.array_dimensions) {
                    total_size *= dim;
                }
                if (var.type == TYPE_STRING) {
                    if (var.is_multidimensional) {
                        var.multidim_array_strings.resize(total_size, "");
                    } else {
                        var.array_strings.resize(total_size, "");
                    }
                } else {
                    if (var.is_multidimensional) {
                        var.multidim_array_values.resize(total_size, 0);
                    } else {
                        var.array_values.resize(total_size, 0);
                    }
                }
            }
        }
        // 既存のtypedef解決処理（ArrayTypeInfoが設定されていない場合のみ）
        else if (node->type_info == TYPE_UNKNOWN && !node->type_name.empty()) {
            std::string resolved_type =
                interpreter_->type_manager_->resolve_typedef(node->type_name);

            // 配列typedefの場合
            if (resolved_type.find("[") != std::string::npos) {
                std::string base =
                    resolved_type.substr(0, resolved_type.find("["));
                std::string array_part =
                    resolved_type.substr(resolved_type.find("["));

                TypeInfo base_type =
                    interpreter_->type_manager_->string_to_type_info(base);
                var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
                var.is_array = true;

                // 配列サイズを解析 [3] -> 3
                if (array_part.length() >= 2 && array_part[0] == '[' &&
                    array_part[array_part.length() - 1] == ']') {
                    std::string size_str =
                        array_part.substr(1, array_part.length() - 2);

                    if (size_str.empty()) {
                        // 動的配列（TYPE[]）はサポートされていない
                        error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                                  node->name.c_str());
                        throw std::runtime_error(
                            "Dynamic arrays are not supported yet");
                    }

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
                var.type = interpreter_->type_manager_->string_to_type_info(
                    resolved_type);
            }
        }

        // 配列タイプチェック（直接配列宣言の場合）
        if (!var.is_array && node->type_name.find("[") != std::string::npos) {
            var.is_array = true;

            // 配列サイズを解析
            size_t bracket_pos = node->type_name.find("[");
            size_t close_bracket_pos = node->type_name.find("]");

            if (bracket_pos != std::string::npos &&
                close_bracket_pos != std::string::npos) {
                std::string size_str = node->type_name.substr(
                    bracket_pos + 1, close_bracket_pos - bracket_pos - 1);
                var.array_size = std::stoi(size_str);

                // array_dimensionsを設定
                var.array_dimensions.clear();
                var.array_dimensions.push_back(var.array_size);

                // 配列初期化
                if (var.type == TYPE_STRING) {
                    var.array_strings.resize(var.array_size, "");
                } else {
                    var.array_values.resize(var.array_size, 0);
                }
            }
        }

        // 初期化式がある場合
        if (node->init_expr) {
            if (var.is_array &&
                node->init_expr->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 配列スライス代入の処理
                std::string source_var_name = node->init_expr->name;
                Variable *source_var = find_variable(source_var_name);
                if (!source_var) {
                    throw std::runtime_error("Source variable not found: " +
                                             source_var_name);
                }

                // インデックスを評価
                std::vector<int64_t> indices;
                for (const auto &index_expr : node->init_expr->arguments) {
                    int64_t index = interpreter_->expression_evaluator_
                                        ->evaluate_expression(index_expr.get());
                    indices.push_back(index);
                }

                // 配列スライスをコピー
                interpreter_->array_manager_->copyArraySlice(var, *source_var,
                                                             indices);

            } else if (var.is_array && node->init_expr->node_type ==
                                           ASTNodeType::AST_ARRAY_LITERAL) {
                // 配列リテラル初期化の処理

                // まず変数を登録
                current_scope().variables[node->name] = var;

                // 配列リテラル代入を実行
                interpreter_->assign_array_literal(node->name,
                                                   node->init_expr.get());

                // 代入後に変数を再取得して更新
                current_scope().variables[node->name].is_assigned = true;

                return; // 配列リテラル処理完了後は早期リターン

            } else if (var.is_array && node->init_expr->node_type ==
                                           ASTNodeType::AST_VARIABLE) {
                // 配列全体のコピー
                std::string source_var_name = node->init_expr->name;
                Variable *source_var = find_variable(source_var_name);
                if (!source_var) {
                    throw std::runtime_error("Source variable not found: " +
                                             source_var_name);
                }

                // 配列をコピー
                interpreter_->array_manager_->copyArray(var, *source_var);

            } else if (var.type == TYPE_STRING &&
                       node->init_expr->node_type ==
                           ASTNodeType::AST_STRING_LITERAL) {
                // 文字列初期化の処理
                var.str_value = node->init_expr->str_value;
                var.value = 0; // プレースホルダー
                var.is_assigned = true;
            } else if (var.is_array && node->init_expr->node_type ==
                                           ASTNodeType::AST_FUNC_CALL) {
                // 配列を返す関数呼び出し
                try {
                    int64_t value =
                        interpreter_->expression_evaluator_
                            ->evaluate_expression(node->init_expr.get());
                    var.value = value;
                    var.is_assigned = true;
                } catch (const ReturnException &ret) {
                    if (ret.is_array) {
                        // 配列戻り値の場合
                        if (ret.type == TYPE_STRING) {
                            // 文字列配列
                            if (!ret.str_array_3d.empty() &&
                                !ret.str_array_3d[0].empty() &&
                                !ret.str_array_3d[0][0].empty()) {
                                var.array_strings = ret.str_array_3d[0][0];
                                var.array_size = var.array_strings.size();
                                var.type = static_cast<TypeInfo>(
                                    TYPE_ARRAY_BASE + TYPE_STRING);
                            }
                        } else {
                            // 数値配列
                            if (!ret.int_array_3d.empty() &&
                                !ret.int_array_3d[0].empty() &&
                                !ret.int_array_3d[0][0].empty()) {

                                // 多次元配列の場合
                                if (var.is_multidimensional &&
                                    var.array_type_info.dimensions.size() > 1) {
                                    var.multidim_array_values =
                                        ret.int_array_3d[0][0];
                                    var.array_size =
                                        var.multidim_array_values.size();
                                } else {
                                    var.array_values = ret.int_array_3d[0][0];
                                    var.array_size = var.array_values.size();
                                }
                                var.type = static_cast<TypeInfo>(
                                    TYPE_ARRAY_BASE + ret.type);
                            }
                        }
                        var.is_assigned = true;
                    } else {
                        // 非配列戻り値の場合
                        if (ret.type == TYPE_STRING) {
                            var.str_value = ret.str_value;
                        } else {
                            var.value = ret.value;
                        }
                        var.is_assigned = true;
                    }
                }
            } else {
                // 数値初期化の処理
                if (node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
                    // 関数呼び出しの場合、ReturnExceptionをキャッチ
                    try {
                        int64_t value =
                            interpreter_->expression_evaluator_
                                ->evaluate_expression(node->init_expr.get());
                        // ReturnExceptionがキャッチされない場合のみここに到達
                        var.value = value;
                        var.is_assigned = true;
                    } catch (const ReturnException &ret) {
                        if (ret.type == TYPE_STRING) {
                            var.str_value = ret.str_value;
                            var.type = TYPE_STRING;
                        } else {
                            var.value = ret.value;
                        }
                        var.is_assigned = true;
                    }

                    // 型チェック（ReturnExceptionがキャッチされた場合はスキップ）
                    if (!var.is_assigned && var.type == TYPE_STRING) {
                        throw std::runtime_error(
                            "Type mismatch: expected string but got numeric "
                            "value");
                    }
                } else {
                    int64_t value =
                        interpreter_->expression_evaluator_
                            ->evaluate_expression(node->init_expr.get());
                    var.value = value;
                    var.is_assigned = true;
                }

                // 型範囲チェック
                if (var.type != TYPE_STRING) {
                    interpreter_->type_manager_->check_type_range(
                        var.type, var.value, node->name);
                }
            }
        }

        // static変数の場合は特別処理
        if (node->is_static) {
            // static変数として登録
            Variable *existing_static =
                interpreter_->find_static_variable(node->name);
            if (existing_static) {
                // 既にstatic変数が存在する場合は何もしない（初期化は最初の1回のみ）
                return;
            } else {
                // 新しいstatic変数を作成
                interpreter_->create_static_variable(node->name, node);
                return;
            }
        }

        current_scope().variables[node->name] = var;
        // std::cerr << "DEBUG: Variable created: " << node->name << ",
        // is_array=" << var.is_array << std::endl;

    } else if (node->node_type == ASTNodeType::AST_ASSIGN) {
        // 変数代入の処理

        // 配列リテラル代入の特別処理
        if (node->right &&
            node->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            std::string var_name;
            if (node->left &&
                node->left->node_type == ASTNodeType::AST_VARIABLE) {
                var_name = node->left->name;
            } else if (!node->name.empty()) {
                var_name = node->name;
            } else {
                throw std::runtime_error(
                    "Array literal can only be assigned to simple variables");
            }

            interpreter_->assign_array_literal(var_name, node->right.get());
            return;
        }

        if (!node->name.empty() && node->right) {
            // node->nameを使った代入（通常の変数代入）
            std::string var_name = node->name;
            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->right.get());

            Variable *var = find_variable(var_name);
            if (!var) {
                throw std::runtime_error("Undefined variable: " + var_name);
            }

            if (var->is_const && var->is_assigned) {
                throw std::runtime_error("Cannot reassign const variable: " +
                                         var_name);
            }

            // 型範囲チェック（代入前に実行）
            interpreter_->type_manager_->check_type_range(var->type, value,
                                                          var_name);

            var->value = value;
            var->is_assigned = true;

        } else if (node->left &&
                   node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常の変数代入
            std::string var_name = node->left->name;
            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->right.get());

            Variable *var = find_variable(var_name);
            if (!var) {
                throw std::runtime_error("Undefined variable: " + var_name);
            }

            if (var->is_const && var->is_assigned) {
                throw std::runtime_error("Cannot reassign const variable: " +
                                         var_name);
            }

            // 型範囲チェック（代入前に実行）
            interpreter_->type_manager_->check_type_range(var->type, value,
                                                          var_name);

            var->value = value;
            var->is_assigned = true;
        } else if (node->left &&
                   node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // 配列要素代入の処理（N次元対応）
            std::string array_name = extract_array_name(node->left.get());
            if (array_name.empty()) {
                throw std::runtime_error("Cannot determine array name");
            }

            std::vector<int64_t> indices =
                extract_array_indices(node->left.get());
            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->right.get());

            Variable *var = find_variable(array_name);
            if (!var) {
                throw std::runtime_error("Undefined array: " + array_name);
            }

            // 文字列要素への代入の場合
            if (var->type == TYPE_STRING && !var->is_array) {
                if (indices.size() != 1) {
                    throw std::runtime_error("Invalid string element access");
                }

                if (var->is_const) {
                    throw std::runtime_error(
                        "Cannot assign to const string element: " + array_name);
                }

                int64_t index = indices[0];
                // 文字列要素代入は interpreter_.assign_string_element を使用
                interpreter_->assign_string_element(
                    array_name, index,
                    std::string(1, static_cast<char>(value)));
                return;
            }

            if (!var->is_array) {
                throw std::runtime_error("Not an array: " + array_name);
            }

            // 多次元配列の場合
            if (var->is_multidimensional && indices.size() > 1) {
                interpreter_->array_manager_->setMultidimensionalArrayElement(
                    *var, indices, value);
            } else if (indices.size() == 1) {
                // 1次元配列の場合
                // const配列への書き込みチェック
                if (var->is_const) {
                    throw std::runtime_error("Cannot assign to const array: " +
                                             array_name);
                }

                int64_t index = indices[0];
                if (index < 0 ||
                    index >= static_cast<int64_t>(var->array_values.size())) {
                    throw std::runtime_error("Array index out of bounds");
                }
                var->array_values[index] = value;
            } else {
                throw std::runtime_error("Invalid array access");
            }
        }
        // 他の複雑なケースは後で実装
    }
}

// N次元配列の配列名を抽出する汎用関数
std::string VariableManager::extract_array_name(const ASTNode *node) {
    if (!node)
        return "";

    if (node->node_type == ASTNodeType::AST_VARIABLE) {
        return node->name;
    } else if (node->node_type == ASTNodeType::AST_ARRAY_REF) {
        if (!node->name.empty()) {
            return node->name; // 直接名前がある場合
        } else if (node->left) {
            return extract_array_name(node->left.get()); // 再帰的に探索
        }
    }
    return "";
}

// N次元配列のインデックスを抽出する汎用関数
std::vector<int64_t>
VariableManager::extract_array_indices(const ASTNode *node) {
    std::vector<int64_t> indices;

    if (!node || node->node_type != ASTNodeType::AST_ARRAY_REF) {
        return indices;
    }

    // 現在のインデックスを評価
    if (node->array_index) {
        int64_t index =
            interpreter_->expression_evaluator_->evaluate_expression(
                node->array_index.get());
        indices.push_back(index);
    }

    // 左側に更なる配列アクセスがあるかチェック
    if (node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        std::vector<int64_t> left_indices =
            extract_array_indices(node->left.get());
        // 左側のインデックスを先頭に挿入
        indices.insert(indices.begin(), left_indices.begin(),
                       left_indices.end());
    }

    return indices;
}
