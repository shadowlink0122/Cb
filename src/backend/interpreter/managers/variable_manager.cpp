#include "managers/variable_manager.h"
#include "../../../common/debug_messages.h"
#include "managers/array_manager.h"
#include "managers/common_operations.h"
#include "evaluator/expression_evaluator.h"
#include "core/interpreter.h"
#include "managers/type_manager.h"
#include <algorithm>

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

bool VariableManager::is_global_variable(const std::string &name) {
    // グローバルスコープに存在するかチェック
    auto global_var_it = interpreter_->global_scope.variables.find(name);
    return (global_var_it != interpreter_->global_scope.variables.end());
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
            std::cerr << "Cannot reassign const variable: " << name
                      << std::endl;
            std::exit(1);
        }
        var->str_value = value;
        var->is_assigned = true;
    }
}

void VariableManager::assign_array_element(const std::string &name,
                                           int64_t index, int64_t value) {
    Variable *var = find_variable(name);
    if (!var) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("Variable not found");
    }

    // 共通実装を使用
    try {
        interpreter_->get_common_operations()->assign_array_element_safe(
            var, index, value, name);
    } catch (const std::exception &e) {
        error_msg(DebugMsgId::UNDEFINED_VAR_ERROR, name.c_str());
        throw std::runtime_error("Array element assignment failed: " +
                                 std::string(e.what()));
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

        debug_msg(DebugMsgId::VAR_MANAGER_TYPE_RESOLVED, 
                  node->name.c_str(), node->type_name.c_str(), resolved_type.c_str());

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
    // debug_msg(DebugMsgId::VAR_MANAGER_PROCESS, (int)node->node_type, node->name.c_str());
    if (interpreter_->debug_mode) {
        debug_print("VAR_DEBUG: process_var_decl_or_assign called for %s, node_type=%d\n", 
                   node->name.c_str(), static_cast<int>(node->node_type));
        debug_print("VAR_DEBUG: type_info=%d, type_name='%s'\n", 
                   static_cast<int>(node->type_info), node->type_name.c_str());
        
        std::string resolved = interpreter_->type_manager_->resolve_typedef(node->type_name);
        debug_print("VAR_DEBUG: resolve_typedef('%s') = '%s'\n", 
                   node->type_name.c_str(), resolved.c_str());
        debug_print("VAR_DEBUG: condition check: !empty=%d, resolved!=original=%d\n", 
                   !node->type_name.empty(), resolved != node->type_name);
    }
    if (node->node_type == ASTNodeType::AST_VAR_DECL) {
        // 変数宣言の処理
        Variable var;
        var.type = node->type_info;
        var.is_const = node->is_const;
        var.is_assigned = false;
        var.is_array = false;
        var.array_size = 0;
        
        // struct変数の場合の追加設定
        if (node->type_info == TYPE_STRUCT && !node->type_name.empty()) {
            var.is_struct = true;
            var.struct_type_name = node->type_name;
        }

        // 新しいArrayTypeInfoが設定されている場合の処理
        if (node->array_type_info.base_type != TYPE_UNKNOWN) {
            debug_print("VAR_DEBUG: Taking ArrayTypeInfo branch (base_type=%d)\n", static_cast<int>(node->array_type_info.base_type));
            
            // ArrayTypeInfoが設定されている場合は配列として処理
            var.is_array = true;
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->array_type_info.base_type);
            var.array_type_info = node->array_type_info;

            // 配列サイズ情報をコピーし、動的サイズを解決
            if (!node->array_type_info.dimensions.empty()) {
                var.array_dimensions.clear();
                for (const auto &dim : node->array_type_info.dimensions) {
                    int resolved_size = dim.size;
                    
                    // 動的サイズ（定数識別子）を解決
                    if (dim.is_dynamic && !dim.size_expr.empty()) {
                        Variable *const_var = find_variable(dim.size_expr);
                        if (const_var && const_var->is_const && const_var->type == TYPE_INT) {
                            resolved_size = static_cast<int>(const_var->value);
                        } else {
                            throw std::runtime_error("Array size must be a constant integer: " + dim.size_expr);
                        }
                    }
                    
                    var.array_dimensions.push_back(resolved_size);
                }
                
                // 多次元配列かどうかをチェック
                if (var.array_dimensions.size() > 1) {
                    var.is_multidimensional = true;
                }

                // 配列サイズを計算（多次元の場合は総サイズ、1次元の場合は第一次元のサイズ）
                int total_size = 1;
                for (int dim : var.array_dimensions) {
                    total_size *= dim;
                }
                var.array_size = total_size;  // 総サイズを設定
                
                if (debug_mode) {
                    debug_print("VAR_DEBUG: ArrayTypeInfo - dimensions=%zu, total_size=%d\n", 
                                var.array_dimensions.size(), total_size);
                }

                // 配列初期化
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
        // typedef解決処理（ArrayTypeInfoが設定されていない場合）
        // type_infoが基本型でも、type_nameがtypedef名の場合は処理する
        else if (!node->type_name.empty() && 
                 interpreter_->type_manager_->resolve_typedef(node->type_name) != node->type_name) {
            if (debug_mode) {
                debug_print("TYPEDEF_DEBUG: Entering typedef resolution branch\n");
            }
            std::string resolved_type =
                interpreter_->type_manager_->resolve_typedef(node->type_name);

            if (debug_mode) {
                debug_print("TYPEDEF_DEBUG: Resolving typedef '%s' -> '%s' (type_info=%d)\n", 
                            node->type_name.c_str(), resolved_type.c_str(), static_cast<int>(node->type_info));
            }

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

                // 多次元配列の次元解析 [2][3] -> {2, 3}
                std::vector<int> dimensions;
                std::string remaining = array_part;
                
                if (debug_mode) {
                    debug_print("TYPEDEF_DEBUG: Processing typedef array: %s (array_part: %s)\n", 
                                node->type_name.c_str(), array_part.c_str());
                }
                
                while (!remaining.empty() && remaining[0] == '[') {
                    size_t close_bracket = remaining.find(']');
                    if (close_bracket == std::string::npos) {
                        throw std::runtime_error("Invalid array syntax: missing ']'");
                    }
                    
                    std::string size_str = remaining.substr(1, close_bracket - 1);
                    if (size_str.empty()) {
                        // 動的配列（TYPE[]）はサポートされていない
                        error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                                  node->name.c_str());
                        throw std::runtime_error(
                            "Dynamic arrays are not supported yet");
                    }

                    int dimension_size;
                    // 数値か定数識別子かを判定
                    if (std::all_of(size_str.begin(), size_str.end(), ::isdigit)) {
                        dimension_size = std::stoi(size_str);
                    } else {
                        // 定数識別子の場合は値を取得
                        Variable *const_var = find_variable(size_str);
                        if (const_var && const_var->is_const && const_var->type == TYPE_INT) {
                            dimension_size = static_cast<int>(const_var->value);
                        } else {
                            throw std::runtime_error("Array size must be a constant integer: " + size_str);
                        }
                    }
                    
                    dimensions.push_back(dimension_size);
                    remaining = remaining.substr(close_bracket + 1);
                }
                
                if (dimensions.empty()) {
                    var.array_size = 0; // 動的配列
                } else if (dimensions.size() == 1) {
                    // 1次元配列
                    var.array_size = dimensions[0];
                    var.is_multidimensional = false;
                } else {
                    // 多次元配列
                    var.is_multidimensional = true;
                    var.array_dimensions = dimensions;
                    
                    // 総サイズを計算
                    int total_size = 1;
                    for (int dim : dimensions) {
                        total_size *= dim;
                    }
                    var.array_size = total_size;
                    
                    if (debug_mode) {
                        debug_print("TYPEDEF_DEBUG: Multidim array created: dimensions=%zu, total_size=%d\n", 
                                    dimensions.size(), total_size);
                    }
                }

                // 配列初期化
                if (base_type == TYPE_STRING) {
                    if (var.is_multidimensional) {
                        var.multidim_array_strings.resize(var.array_size, "");
                    } else {
                        var.array_strings.resize(var.array_size, "");
                    }
                } else {
                    if (var.is_multidimensional) {
                        var.multidim_array_values.resize(var.array_size, 0);
                    } else {
                        var.array_values.resize(var.array_size, 0);
                    }
                }

            } else {
                // 構造体typedefかチェック
                const StructDefinition* struct_def = interpreter_->find_struct_definition(resolved_type);
                if (struct_def) {
                    if (debug_mode) {
                        debug_print("TYPEDEF_DEBUG: Resolving struct typedef '%s' -> '%s'\n", 
                                    node->type_name.c_str(), resolved_type.c_str());
                    }
                    var.type = TYPE_STRUCT;
                    var.is_struct = true;
                    var.struct_type_name = resolved_type;
                    
                    // struct_membersを初期化
                    for (const auto &member : struct_def->members) {
                        Variable member_var;
                        member_var.type = member.type;
                        if (member.type == TYPE_STRING) {
                            member_var.str_value = "";
                        } else {
                            member_var.value = 0;
                        }
                        member_var.is_assigned = false;
                        var.struct_members[member.name] = member_var;
                        
                        // 個別メンバー変数も作成
                        std::string member_path = node->name + "." + member.name;
                        current_scope().variables[member_path] = member_var;
                        
                        if (debug_mode) {
                            debug_print("TYPEDEF_DEBUG: Added struct member: %s (type: %d)\n",
                                       member.name.c_str(), (int)member.type);
                        }
                    }
                } else {
                    var.type = interpreter_->type_manager_->string_to_type_info(
                        resolved_type);
                }
            }
        }
        // struct型の場合の処理
        else if (node->type_info == TYPE_STRUCT ||
                 (node->type_info == TYPE_UNKNOWN &&
                  (interpreter_->find_struct_definition(node->type_name) != nullptr ||
                   interpreter_->find_struct_definition(interpreter_->type_manager_->resolve_typedef(node->type_name)) != nullptr))) {
            if (debug_mode) {
                debug_print("VAR_DEBUG: Taking STRUCT branch (type_info=%d, TYPE_STRUCT=%d)\n", 
                           (int)node->type_info, (int)TYPE_STRUCT);
            }
            debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE, node->name.c_str(), node->type_name.c_str());
            var.type = TYPE_STRUCT;
            var.is_struct = true;
            
            // typedef名を実際のstruct名に解決
            std::string resolved_struct_type = interpreter_->type_manager_->resolve_typedef(node->type_name);
            var.struct_type_name = resolved_struct_type;

            // 構造体配列かどうかをチェック（型名に[サイズ]が含まれている場合）
            std::string base_struct_type = resolved_struct_type;
            bool is_struct_array = false;
            int struct_array_size = 0;

            size_t bracket_pos = resolved_struct_type.find("[");
            if (bracket_pos != std::string::npos) {
                is_struct_array = true;
                base_struct_type = resolved_struct_type.substr(0, bracket_pos);

                size_t close_bracket_pos = resolved_struct_type.find("]");
                if (close_bracket_pos != std::string::npos) {
                    std::string size_str = resolved_struct_type.substr(
                        bracket_pos + 1, close_bracket_pos - bracket_pos - 1);
                    struct_array_size = std::stoi(size_str);
                }

                var.is_array = true;
                var.array_size = struct_array_size;
                var.array_dimensions.push_back(struct_array_size);
            }

            // struct定義を取得してメンバ変数を初期化
            const StructDefinition *struct_def =
                interpreter_->find_struct_definition(interpreter_->type_manager_->resolve_typedef(base_struct_type));
            if (struct_def) {
                if (interpreter_->debug_mode) {
                    debug_print(
                        "Initializing struct %s with %zu members (array: %s, "
                        "size: %d)\n",
                        base_struct_type.c_str(), struct_def->members.size(),
                        is_struct_array ? "yes" : "no", struct_array_size);
                }

                if (is_struct_array) {
                    // 構造体配列の場合：各配列要素を独立した構造体変数として作成
                    for (int array_idx = 0; array_idx < struct_array_size;
                         array_idx++) {
                        std::string element_name =
                            node->name + "[" + std::to_string(array_idx) + "]";

                        Variable element_var;
                        element_var.type = TYPE_STRUCT;
                        element_var.is_struct = true;
                        element_var.struct_type_name = base_struct_type;

                        // 各構造体要素にメンバ変数を作成
                        for (const auto &member : struct_def->members) {
                            std::string member_name =
                                element_name + "." + member.name;
                            Variable member_var;
                            member_var.type = member.type;

                            // デフォルト値を設定
                            if (member_var.type == TYPE_STRING) {
                                member_var.str_value = "";
                            } else {
                                member_var.value = 0;
                            }
                            member_var.is_assigned = false;

                            // メンバ配列の場合の処理も追加可能だが、今回は基本メンバのみ
                            current_scope().variables[member_name] = member_var;
                        }

                        // 構造体要素自体も変数として登録
                        current_scope().variables[element_name] = element_var;
                    }
                } else {
                    // 通常の構造体の場合：既存の処理
                    for (const auto &member : struct_def->members) {
                        Variable member_var;
                        member_var.type = member.type;

                        // 配列メンバーの場合
                        if (member.array_info.is_array()) {
                            member_var.is_array = true;

                            // 多次元配列の総サイズを計算（定数解決を含む）
                            int total_size = 1;
                            for (const auto &dim :
                                 member.array_info.dimensions) {
                                int resolved_size = dim.size;

                                // 動的サイズの場合は定数識別子を解決
                                if (resolved_size == -1 && dim.is_dynamic &&
                                    !dim.size_expr.empty()) {
                                    Variable *const_var =
                                        interpreter_->find_variable(
                                            dim.size_expr);
                                    if (const_var && const_var->is_assigned) {
                                        // const変数または初期化済み変数を許可
                                        resolved_size =
                                            static_cast<int>(const_var->value);
                                        if (interpreter_->debug_mode) {
                                            debug_print(
                                                "Resolved constant %s to %d "
                                                "for struct member %s\n",
                                                dim.size_expr.c_str(),
                                                resolved_size,
                                                member.name.c_str());
                                        }
                                    } else {
                                        throw std::runtime_error(
                                            "Cannot resolve constant '" +
                                            dim.size_expr +
                                            "' for struct member array size");
                                    }
                                }

                                if (resolved_size <= 0) {
                                    throw std::runtime_error(
                                        "Invalid array size for struct "
                                        "member " +
                                        member.name);
                                }

                                total_size *= resolved_size;
                            }
                            member_var.array_size = total_size;

                            // array_dimensionsを設定（定数解決済み）
                            member_var.array_dimensions.clear();
                            for (const auto &dim :
                                 member.array_info.dimensions) {
                                int resolved_size = dim.size;

                                // 動的サイズの場合は定数識別子を解決
                                if (resolved_size == -1 && dim.is_dynamic &&
                                    !dim.size_expr.empty()) {
                                    Variable *const_var =
                                        interpreter_->find_variable(
                                            dim.size_expr);
                                    if (const_var && const_var->is_assigned) {
                                        // const変数または初期化済み変数を許可
                                        resolved_size =
                                            static_cast<int>(const_var->value);
                                    } else {
                                        throw std::runtime_error(
                                            "Cannot resolve constant '" +
                                            dim.size_expr +
                                            "' for struct member array size");
                                    }
                                }

                                member_var.array_dimensions.push_back(
                                    resolved_size);
                            }

                            // 多次元配列のフラグを設定
                            if (member_var.array_dimensions.size() > 1) {
                                member_var.is_multidimensional = true;
                                
                                // array_type_info.dimensionsを設定
                                member_var.array_type_info.dimensions.clear();
                                for (const auto& dim_size : member_var.array_dimensions) {
                                    ArrayDimension dimension(dim_size, false); // 構造体メンバーは静的サイズ
                                    member_var.array_type_info.dimensions.push_back(dimension);
                                }
                                member_var.array_type_info.base_type = member.type;
                                
                                debug_msg(DebugMsgId::VAR_MANAGER_MULTIDIM_FLAG, member.name.c_str(), member_var.array_dimensions.size());
                                if (interpreter_->debug_mode) {
                                    debug_print("Set multidimensional flag for struct member: %s (dimensions: %zu)\n",
                                        member.name.c_str(), member_var.array_dimensions.size());
                                }
                            }

                            if (interpreter_->debug_mode) {
                                debug_print(
                                    "Creating array member: %s with total size "
                                    "%d (dims: %zu)\n",
                                    member.name.c_str(), total_size,
                                    member.array_info.dimensions.size());
                            }

                            // 配列の各要素を個別の変数として作成（多次元対応）
                            for (int i = 0; i < total_size; i++) {
                                std::string element_name =
                                    node->name + "." + member.name + "[" +
                                    std::to_string(i) + "]";
                                Variable element_var;
                                element_var.type = member.type;
                                element_var.value = 0;
                                element_var.str_value = "";
                                element_var.is_assigned = false;
                                this->current_scope().variables[element_name] =
                                    element_var;

                                if (interpreter_->debug_mode) {
                                    debug_print("Created struct member array "
                                                "element: %s\n",
                                                element_name.c_str());
                                }
                            }

                            // 配列メンバー自体もstruct_membersに追加
                            member_var.array_values.resize(total_size, 0);
                            if (member.type == TYPE_STRING) {
                                member_var.array_strings.resize(total_size, "");
                            }
                            
                            // 多次元配列の場合は適切なストレージを使用
                            if (member_var.is_multidimensional) {
                                if (member.type == TYPE_STRING) {
                                    member_var.multidim_array_strings.resize(total_size, "");
                                } else {
                                    member_var.multidim_array_values.resize(total_size, 0);
                                }
                            }
                            
                            var.struct_members[member.name] = member_var;
                            
                            if (interpreter_->debug_mode) {
                                debug_print("Added to struct_members[%s]: is_multidimensional=%s, array_dimensions.size()=%zu\n",
                                    member.name.c_str(),
                                    member_var.is_multidimensional ? "true" : "false",
                                    member_var.array_dimensions.size());
                            }
                        } else {
                            // 通常のメンバーの場合

                            // デフォルト値を設定
                            if (member_var.type == TYPE_STRING) {
                                member_var.str_value = "";
                            } else {
                                member_var.value = 0;
                            }
                            member_var.is_assigned = false;

                            var.struct_members[member.name] = member_var;
                        }

                        // 通常のstruct変数でもメンバー直接アクセス変数を作成
                        std::string member_path =
                            node->name + "." + member.name;
                        Variable member_direct_var = member_var;
                        current_scope().variables[member_path] =
                            member_direct_var;

                        if (interpreter_->debug_mode) {
                            debug_print(
                                "Added member: %s (type: %d, is_array: %s)\n",
                                member.name.c_str(), (int)member.type,
                                member.array_info.is_array() ? "true"
                                                             : "false");
                        }
                    }
                }
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
            if (var.is_struct &&
                node->init_expr->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
                // struct literal初期化の処理: Person p = {25, "Bob"};

                // まず変数を登録
                current_scope().variables[node->name] = var;

                // struct literal代入を実行
                interpreter_->assign_struct_literal(node->name,
                                                    node->init_expr.get());

                // 代入完了
                current_scope().variables[node->name].is_assigned = true;

                return; // struct literal処理完了後は早期リターン

            } else if (var.is_struct && node->init_expr->node_type ==
                                            ASTNodeType::AST_VARIABLE) {
                // struct to struct代入の処理: Person p2 = p1;
                std::string source_var_name = node->init_expr->name;
                Variable *source_var = find_variable(source_var_name);
                if (!source_var) {
                    throw std::runtime_error("Source variable not found: " +
                                             source_var_name);
                }

                if (!source_var->is_struct) {
                    throw std::runtime_error(
                        "Cannot assign non-struct to struct variable");
                }

                if (source_var->struct_type_name != var.struct_type_name) {
                    throw std::runtime_error(
                        "Cannot assign struct of different type");
                }

                // まず変数を登録
                current_scope().variables[node->name] = var;

                // 全メンバをコピー
                for (const auto &member : source_var->struct_members) {
                    current_scope()
                        .variables[node->name]
                        .struct_members[member.first] = member.second;
                    
                    // 直接アクセス変数もコピー
                    std::string source_member_name = source_var_name + "." + member.first;
                    std::string dest_member_name = node->name + "." + member.first;
                    Variable *source_member_var = find_variable(source_member_name);
                    if (source_member_var) {
                        Variable member_copy = *source_member_var;
                        current_scope().variables[dest_member_name] = member_copy;
                        
                        // 配列メンバの場合、個別要素変数もコピー
                        if (source_member_var->is_array) {
                            for (int i = 0; i < source_member_var->array_size; i++) {
                                std::string source_element_name = source_member_name + "[" + std::to_string(i) + "]";
                                std::string dest_element_name = dest_member_name + "[" + std::to_string(i) + "]";
                                Variable *source_element_var = find_variable(source_element_name);
                                if (source_element_var) {
                                    Variable element_copy = *source_element_var;
                                    current_scope().variables[dest_element_name] = element_copy;
                                    
                                    if (interpreter_->debug_mode) {
                                        if (source_element_var->type == TYPE_STRING) {
                                            debug_print("STRUCT_COPY: Copied array element %s = '%s' to %s\n",
                                                      source_element_name.c_str(), 
                                                      source_element_var->str_value.c_str(),
                                                      dest_element_name.c_str());
                                        } else {
                                            debug_print("STRUCT_COPY: Copied array element %s = %lld to %s\n",
                                                      source_element_name.c_str(), 
                                                      (long long)source_element_var->value,
                                                      dest_element_name.c_str());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 代入完了
                current_scope().variables[node->name].is_assigned = true;

                return; // struct代入処理完了後は早期リターン

            } else if (var.is_array && node->init_expr->node_type ==
                                           ASTNodeType::AST_ARRAY_REF) {
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
                debug_print("DEBUG_BRANCH: Array function call for %s\n", node->name.c_str());
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
                    } else if (ret.is_struct) {
                        // struct戻り値の場合
                        debug_print("STRUCT_RETURN_DEBUG: Processing struct return value for %s\n", node->name.c_str());
                        var = ret.struct_value;
                        var.is_assigned = true;
                        
                        // struct変数を登録
                        current_scope().variables[node->name] = var;
                        
                        // 構造体定義を取得して配列メンバの個別要素変数を作成
                        const StructDefinition *struct_def = interpreter_->find_struct_definition(interpreter_->type_manager_->resolve_typedef(var.struct_type_name));
                        if (struct_def) {
                            for (const auto &member_def : struct_def->members) {
                                // 直接アクセス変数を作成
                                std::string member_name = node->name + "." + member_def.name;
                                Variable member_var;
                                
                                auto struct_member_it = var.struct_members.find(member_def.name);
                                if (struct_member_it != var.struct_members.end()) {
                                    member_var = struct_member_it->second;
                                    current_scope().variables[member_name] = member_var;
                                    
                                    // 配列メンバの場合、個別要素変数も作成
                                    if (member_var.is_array) {
                                        for (int i = 0; i < member_var.array_size; i++) {
                                            std::string element_name = member_name + "[" + std::to_string(i) + "]";
                                            Variable element_var;
                                            element_var.type = member_def.array_info.base_type;
                                            element_var.is_assigned = true;
                                            
                                            if (element_var.type == TYPE_STRING) {
                                                if (i < static_cast<int>(member_var.array_strings.size())) {
                                                    element_var.str_value = member_var.array_strings[i];
                                                }
                                            } else {
                                                if (i < static_cast<int>(member_var.array_values.size())) {
                                                    element_var.value = member_var.array_values[i];
                                                }
                                            }
                                            
                                            current_scope().variables[element_name] = element_var;
                                            
                                            if (interpreter_->debug_mode) {
                                                if (element_var.type == TYPE_STRING) {
                                                    debug_print("STRUCT_RETURN: Created array element %s = '%s'\n",
                                                              element_name.c_str(), element_var.str_value.c_str());
                                                } else {
                                                    debug_print("STRUCT_RETURN: Created array element %s = %lld\n",
                                                              element_name.c_str(), (long long)element_var.value);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        
                        return; // struct戻り値処理完了後は早期リターン
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
                        if (ret.is_struct) {
                            debug_print("STRUCT_RETURN_DEBUG_2: Processing struct return value for %s\n", node->name.c_str());
                            var = ret.struct_value;
                            var.is_assigned = true;
                            
                            // 構造体の場合、直接アクセス変数も作成
                            current_scope().variables[node->name] = var;
                            
                            // 構造体定義を取得して配列メンバの個別要素変数を作成
                            const StructDefinition *struct_def = interpreter_->find_struct_definition(interpreter_->type_manager_->resolve_typedef(var.struct_type_name));
                            if (struct_def) {
                                for (const auto &member_def : struct_def->members) {
                                    std::string member_name = node->name + "." + member_def.name;
                                    
                                    auto struct_member_it = var.struct_members.find(member_def.name);
                                    if (struct_member_it != var.struct_members.end()) {
                                        Variable member_var = struct_member_it->second;
                                        current_scope().variables[member_name] = member_var;
                                        
                                        // 配列メンバの場合、個別要素変数も作成
                                        if (member_var.is_array) {
                                            for (int i = 0; i < member_var.array_size; i++) {
                                                std::string element_name = member_name + "[" + std::to_string(i) + "]";
                                                Variable element_var;
                                                
                                                if (member_var.type == TYPE_STRING) {
                                                    element_var.type = TYPE_STRING;
                                                    if (i < static_cast<int>(member_var.array_strings.size())) {
                                                        element_var.str_value = member_var.array_strings[i];
                                                    } else {
                                                        element_var.str_value = "";
                                                    }
                                                } else {
                                                    element_var.type = member_var.type;
                                                    if (i < static_cast<int>(member_var.array_values.size())) {
                                                        element_var.value = member_var.array_values[i];
                                                    } else {
                                                        element_var.value = 0;
                                                    }
                                                }
                                                element_var.is_assigned = true;
                                                current_scope().variables[element_name] = element_var;
                                                
                                                if (interpreter_->debug_mode) {
                                                    if (element_var.type == TYPE_STRING) {
                                                        debug_print("STRUCT_RETURN_2: Created array element %s = '%s'\n",
                                                                  element_name.c_str(), element_var.str_value.c_str());
                                                    } else {
                                                        debug_print("STRUCT_RETURN_2: Created array element %s = %lld\n",
                                                                  element_name.c_str(), (long long)element_var.value);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            return; // 構造体処理完了後は早期リターン
                        } else if (ret.type == TYPE_STRING) {
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
        } else if (node->left &&
                   node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // struct メンバー代入の処理: obj.member = value または
            // array[index].member = value
            std::string member_name = node->left->name;
            Variable *struct_var = nullptr;
            std::string struct_name;

            if (node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
                // 通常のstruct変数: obj.member = value
                struct_name = node->left->left->name;
                struct_var = find_variable(struct_name);
            } else if (node->left->left->node_type ==
                       ASTNodeType::AST_ARRAY_REF) {
                // struct配列要素: array[index].member = value
                std::string array_name = node->left->left->left->name;
                int64_t index =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        node->left->left->array_index.get());
                struct_name = array_name + "[" + std::to_string(index) + "]";
                struct_var = find_variable(struct_name);
            }

            if (!struct_var) {
                throw std::runtime_error("Undefined struct variable: " +
                                         struct_name);
            }

            if (!struct_var->is_struct) {
                throw std::runtime_error(struct_name + " is not a struct");
            }

            // メンバーが存在するかチェック
            auto member_it = struct_var->struct_members.find(member_name);
            if (member_it == struct_var->struct_members.end()) {
                throw std::runtime_error("Struct " + struct_name +
                                         " has no member: " + member_name);
            }

            // 右辺の値を評価
            Variable &member = member_it->second;

            if (member.type == TYPE_STRING) {
                // 文字列型メンバーの場合
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    member.str_value = node->right->str_value;
                } else {
                    // 数値を文字列に変換
                    int64_t value =
                        interpreter_->expression_evaluator_
                            ->evaluate_expression(node->right.get());
                    member.str_value = std::to_string(value);
                }
            } else {
                // 数値型メンバーの場合
                int64_t value =
                    interpreter_->expression_evaluator_->evaluate_expression(
                        node->right.get());
                member.value = value;
            }
            member.is_assigned = true;
        } else if (node->left && node->left->node_type ==
                                     ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
            // struct メンバー配列要素代入の処理: obj.member[index] = value または obj.member[i][j] = value
            std::string member_name = node->left->name;

            if (!node->left->left ||
                node->left->left->node_type != ASTNodeType::AST_VARIABLE) {
                throw std::runtime_error("Invalid struct member array access");
            }

            std::string struct_name = node->left->left->name;
            Variable *struct_var = find_variable(struct_name);

            if (!struct_var) {
                throw std::runtime_error("Undefined struct variable: " +
                                         struct_name);
            }

            if (!struct_var->is_struct) {
                throw std::runtime_error(struct_name + " is not a struct");
            }

            // インデックスを評価（多次元対応）
            std::vector<int64_t> indices;
            if (node->left->right) {
                // 1次元の場合（従来通り）
                int64_t index = interpreter_->expression_evaluator_->evaluate_expression(
                    node->left->right.get());
                indices.push_back(index);
            } else if (!node->left->arguments.empty()) {
                // 多次元の場合
                for (const auto& arg : node->left->arguments) {
                    int64_t index = interpreter_->expression_evaluator_->evaluate_expression(
                        arg.get());
                    indices.push_back(index);
                }
            } else {
                throw std::runtime_error("No indices found for array access");
            }

            // 構造体メンバー変数を取得
            Variable *member_var = interpreter_->get_struct_member(struct_name, member_name);
            if (!member_var) {
                throw std::runtime_error("Struct member not found: " + member_name);
            }

            // 多次元配列の場合の処理
            if (member_var->is_multidimensional && indices.size() > 1) {
                // 多次元配列の要素への代入
                if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    std::string value = node->right->str_value;
                    interpreter_->setMultidimensionalStringArrayElement(*member_var, indices, value);
                } else {
                    int64_t value = interpreter_->expression_evaluator_->evaluate_expression(
                        node->right.get());
                    interpreter_->setMultidimensionalArrayElement(*member_var, indices, value);
                }
                return;
            }

            // 1次元配列または多次元配列の1次元アクセスの場合（従来処理）
            int64_t index = indices[0];

            // メンバー配列要素の変数名を生成: s.grades[0]
            std::string element_name = struct_name + "." + member_name + "[" +
                                       std::to_string(index) + "]";
            Variable *element_var = find_variable(element_name);

            if (!element_var) {
                throw std::runtime_error("Member array element not found: " +
                                         element_name);
            }

            // 右辺の値を評価
            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->right.get());

            // 型範囲チェック
            interpreter_->type_manager_->check_type_range(element_var->type,
                                                          value, element_name);

            // 値を代入
            element_var->value = value;
            element_var->is_assigned = true;

            if (interpreter_->debug_mode) {
                debug_print(
                    "Assigned %lld to struct member array element: %s\n",
                    (long long)value, element_name.c_str());
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
    } else if (node->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // メンバアクセスの場合: obj.member
        std::string obj_name;
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            obj_name = node->left->name;
        } else {
            return "";
        }
        std::string member_name = node->name;
        return obj_name + "." + member_name;
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

// Priority 3: 変数ポインターから名前を検索
std::string VariableManager::find_variable_name(const Variable* target_var) {
    if (!target_var) return "";
    
    // 実装を簡素化：変数名の逆引きは複雑なので、
    // フォールバック戦略として空文字列を返す
    // これにより、呼び出し元は従来の方法にフォールバックする
    return "";
}
