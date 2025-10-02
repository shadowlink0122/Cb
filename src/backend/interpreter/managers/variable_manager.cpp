#include "managers/variable_manager.h"
#include "../../../common/debug_messages.h"
#include "../../../common/debug.h"
#include "../services/debug_service.h"
#include "managers/array_manager.h"
#include "managers/common_operations.h"
#include "evaluator/expression_evaluator.h"
#include "core/interpreter.h"
#include "core/type_inference.h"
#include "managers/type_manager.h"
#include "managers/enum_manager.h"
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <utility>

namespace {

bool isPrimitiveType(const Variable *var) {
    if (!var) {
        return false;
    }

    switch (var->type) {
    case TYPE_BOOL:
    case TYPE_CHAR:
    case TYPE_INT:
    case TYPE_LONG:
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
    case TYPE_STRING:
        return true;
    default:
        break;
    }

    return false;
}

std::string getPrimitiveTypeNameForImpl(TypeInfo type) {
    return std::string(type_info_to_string(type));
}

void setNumericFields(Variable &var, long double quad_value) {
    var.quad_value = quad_value;
    var.double_value = static_cast<double>(quad_value);
    var.float_value = static_cast<float>(quad_value);
    var.value = static_cast<int64_t>(quad_value);
}

} // namespace

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
    // 一時変数のデバッグ出力
    if (name.substr(0, 12) == "__temp_chain") {
        std::cerr << "DEBUG: Searching for temp variable: " << name << std::endl;
        std::cerr << "DEBUG: Scope stack size: " << interpreter_->scope_stack.size() << std::endl;
    }

    // ローカルスコープから検索
    // std::cerr << "DEBUG: Searching in " << interpreter_->scope_stack.size()
    // << " local scopes" << std::endl;
    for (auto it = interpreter_->scope_stack.rbegin();
         it != interpreter_->scope_stack.rend(); ++it) {
        auto var_it = it->variables.find(name);
        if (var_it != it->variables.end()) {
            // std::cerr << "DEBUG: Found " << name << " in local scope" <<
            // std::endl;
            if (name.substr(0, 12) == "__temp_chain") {
                std::cerr << "DEBUG: Found temp variable in local scope" << std::endl;
            }
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

void VariableManager::assign_interface_view(const std::string &dest_name,
                                            Variable interface_var,
                                            const Variable &source_var,
                                            const std::string &source_var_name) {
    std::string source_type_name = resolve_interface_source_type(source_var);

    if (!interface_impl_exists(interface_var.interface_name, source_type_name)) {
        throw std::runtime_error("No impl found for interface '" + interface_var.interface_name +
                                 "' with type '" + source_type_name + "'");
    }

    if (!source_var_name.empty()) {
        interpreter_->sync_struct_members_from_direct_access(source_var_name);
    }

    Variable assigned_var = std::move(interface_var);
    assigned_var.struct_type_name = source_type_name;
    assigned_var.is_assigned = true;

    if (source_var.is_struct || (!source_var.struct_members.empty() && source_var.type == TYPE_INTERFACE)) {
        assigned_var.is_struct = true;
        assigned_var.struct_members.clear();
        for (const auto &member_pair : source_var.struct_members) {
            const std::string &member_name = member_pair.first;
            const Variable &source_member = member_pair.second;

            Variable dest_member = source_member;
            if (source_member.is_multidimensional) {
                dest_member.is_multidimensional = true;
                dest_member.array_dimensions = source_member.array_dimensions;
                dest_member.multidim_array_values = source_member.multidim_array_values;
                dest_member.multidim_array_strings = source_member.multidim_array_strings;
            }
            assigned_var.struct_members[member_name] = dest_member;
        }
    } else if (source_var.type >= TYPE_ARRAY_BASE) {
        assigned_var.is_struct = false;
        assigned_var.type = source_var.type;
        assigned_var.value = source_var.value;
        assigned_var.str_value = source_var.str_value;
        assigned_var.array_dimensions = source_var.array_dimensions;
        assigned_var.is_multidimensional = source_var.is_multidimensional;
        assigned_var.array_values = source_var.array_values;
        assigned_var.array_strings = source_var.array_strings;
        assigned_var.multidim_array_values = source_var.multidim_array_values;
        assigned_var.multidim_array_strings = source_var.multidim_array_strings;

        if (!source_var.struct_type_name.empty()) {
            assigned_var.struct_type_name = source_var.struct_type_name;
        } else {
            TypeInfo base_type = static_cast<TypeInfo>(source_var.type - TYPE_ARRAY_BASE);
            assigned_var.struct_type_name = getPrimitiveTypeNameForImpl(base_type) + "[]";
        }
    } else {
        assigned_var.is_struct = false;
        assigned_var.type = source_var.type;
        assigned_var.value = source_var.value;
        assigned_var.str_value = source_var.str_value;

        if (!source_var.struct_type_name.empty()) {
            assigned_var.struct_type_name = source_var.struct_type_name;
        } else {
            assigned_var.struct_type_name = getPrimitiveTypeNameForImpl(source_var.type);
        }
    }

    current_scope().variables[dest_name] = assigned_var;
    Variable &dest_var = current_scope().variables[dest_name];
    dest_var.is_assigned = true;
    dest_var.implementing_struct = source_type_name;

    for (const auto &member_pair : source_var.struct_members) {
        const std::string &member_name = member_pair.first;
        const Variable &member_var = member_pair.second;

        std::string dest_member_name = dest_name + "." + member_name;
        Variable dest_member_var = member_var;

        if (!source_var_name.empty()) {
            std::string source_member_name = source_var_name + "." + member_name;
            if (Variable *source_member_var = find_variable(source_member_name)) {
                dest_member_var = *source_member_var;
            }
        }

        dest_var.struct_members[member_name] = dest_member_var;
        current_scope().variables[dest_member_name] = dest_member_var;

        if (member_var.is_array) {
            int total_size = 1;
            for (const auto &dim : member_var.array_dimensions) {
                total_size *= dim;
            }

            for (int i = 0; i < total_size; ++i) {
                std::string dest_element_name = dest_member_name + "[" + std::to_string(i) + "]";
                Variable element_var;
                element_var.is_assigned = true;

                bool copied = false;
                if (!source_var_name.empty()) {
                    std::string source_element_name = source_var_name + "." + member_name + "[" + std::to_string(i) + "]";
                    if (Variable *source_element_var = find_variable(source_element_name)) {
                        element_var = *source_element_var;
                        copied = true;
                    }
                }

                if (!copied) {
                    if (member_var.type == TYPE_STRING) {
                        element_var.type = TYPE_STRING;
                        if (i < static_cast<int>(member_var.array_strings.size())) {
                            element_var.str_value = member_var.array_strings[i];
                        } else if (i < static_cast<int>(member_var.multidim_array_strings.size())) {
                            element_var.str_value = member_var.multidim_array_strings[i];
                        } else {
                            element_var.str_value = "";
                        }
                    } else {
                        element_var.type = member_var.type;
                        int64_t value = 0;
                        if (member_var.is_multidimensional &&
                            i < static_cast<int>(member_var.multidim_array_values.size())) {
                            value = member_var.multidim_array_values[i];
                        } else if (i < static_cast<int>(member_var.array_values.size())) {
                            value = member_var.array_values[i];
                        }
                        element_var.value = value;
                    }
                }

                current_scope().variables[dest_element_name] = element_var;
            }
        }
    }
}

bool VariableManager::interface_impl_exists(const std::string &interface_name,
                                            const std::string &struct_type_name) const {
    const auto &impls = interpreter_->get_impl_definitions();
    for (const auto &impl_def : impls) {
        if (impl_def.interface_name == interface_name && impl_def.struct_name == struct_type_name) {
            return true;
        }
    }
    return false;
}

std::string VariableManager::resolve_interface_source_type(const Variable &source_var) const {
    if (!source_var.struct_type_name.empty()) {
        return source_var.struct_type_name;
    }

    if (source_var.type == TYPE_INTERFACE && !source_var.implementing_struct.empty()) {
        return source_var.implementing_struct;
    }

    if (source_var.is_struct) {
        return source_var.struct_type_name;
    }

    if (source_var.type >= TYPE_ARRAY_BASE || source_var.is_array) {
        TypeInfo base_type = TYPE_UNKNOWN;
        if (source_var.type >= TYPE_ARRAY_BASE) {
            base_type = static_cast<TypeInfo>(source_var.type - TYPE_ARRAY_BASE);
        } else if (source_var.array_type_info.base_type != TYPE_UNKNOWN) {
            base_type = source_var.array_type_info.base_type;
        } else if (source_var.current_type != TYPE_UNKNOWN) {
            base_type = source_var.current_type;
        } else if (source_var.type != TYPE_INTERFACE) {
            base_type = source_var.type;
        }

        if (base_type == TYPE_UNKNOWN) {
            base_type = TYPE_INT;
        }
        return getPrimitiveTypeNameForImpl(base_type) + "[]";
    }

    return getPrimitiveTypeNameForImpl(source_var.type);
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

    if (node->is_pointer) {
        var.is_pointer = true;
        var.pointer_depth = node->pointer_depth;
        var.pointer_base_type_name = node->pointer_base_type_name;
        var.pointer_base_type = node->pointer_base_type;
        if (var.type != TYPE_POINTER) {
            var.type = TYPE_POINTER;
        }
        if (var.type_name.empty()) {
            var.type_name = node->type_name;
        }
    }

    var.is_reference = node->is_reference;
    var.is_unsigned = node->is_unsigned;

    var.is_unsigned = node->is_unsigned;
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
                resolved_type);
            // カスタム型名を保持（例：MyString -> string に解決されても MyString を記録）
            if (resolved_type != node->type_name) {
                var.type_name = node->type_name;
                // current_typeも設定（union代入で必要）
                var.current_type = var.type;
            }
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
    if (var.is_unsigned && value < 0) {
            DEBUG_WARN(VARIABLE,
                       "Unsigned variable %s initialized with negative literal (%lld); clamping to 0",
                       node->name.c_str(), static_cast<long long>(value));
            value = 0;
        }
        var.value = value;
        var.is_assigned = true;

        // 型範囲チェック
        interpreter_->type_manager_->check_type_range(var.type, value,
                                                      node->name,
                                                      var.is_unsigned);
    }

    current_scope().variables[node->name] = var;
}

void VariableManager::assign_variable(const std::string &name, int64_t value,
                                      TypeInfo type, bool is_const) {
    TypeInfo effective = (type != TYPE_UNKNOWN) ? type : TYPE_INT;
    InferredType inferred(effective, type_info_to_string(effective));
    TypedValue typed_value(value, inferred);
    assign_variable(name, typed_value, type, is_const);
}

void VariableManager::assign_variable(const std::string &name,
                                      const std::string &value, bool is_const) {
    InferredType inferred(TYPE_STRING, "string");
    TypedValue typed_value(value, inferred);
    assign_variable(name, typed_value, TYPE_STRING, is_const);
}

void VariableManager::assign_variable(const std::string &name,
                                      const std::string &value) {
    assign_variable(name, value, false);
}

void VariableManager::assign_variable(const std::string &name,
                                      const TypedValue &typed_value,
                                      TypeInfo type_hint, bool is_const) {
    debug_msg(DebugMsgId::VAR_ASSIGN_READABLE, name.c_str(),
              typed_value.is_numeric() ? typed_value.as_numeric() : 0, "type",
              is_const ? "true" : "false");

    auto apply_assignment = [&](Variable &target, bool allow_type_override) {
        auto clamp_unsigned = [&](int64_t &numeric_value) {
            if (!target.is_unsigned || numeric_value >= 0) {
                return;
            }
            DEBUG_WARN(VARIABLE,
                       "Unsigned variable %s received negative assignment (%lld); clamping to 0",
                       name.c_str(), static_cast<long long>(numeric_value));
            numeric_value = 0;
        };

        if (typed_value.is_struct()) {
            if (typed_value.struct_data) {
                bool was_const = target.is_const;
                bool was_unsigned = target.is_unsigned;
                target = *typed_value.struct_data;
                target.is_const = was_const;
                target.is_unsigned = was_unsigned;
                target.is_assigned = true;
                // 構造体戻り値や代入で生成された最新のメンバー状態を
                // ダイレクトアクセス変数にも反映させる
                interpreter_->sync_direct_access_from_struct_value(
                    name, target);
            }
            return;
        }

        if (typed_value.is_string()) {
            if (allow_type_override || target.type == TYPE_UNKNOWN ||
                target.type == TYPE_STRING) {
                target.type = TYPE_STRING;
            }
            target.str_value = typed_value.string_value;
            target.value = 0;
            target.float_value = 0.0f;
            target.double_value = 0.0;
            target.quad_value = 0.0L;
            target.big_value = 0;
            target.is_assigned = true;
            return;
        }

        // 非数値でもここまで来た場合は0として扱う
        if (!typed_value.is_numeric()) {
            setNumericFields(target, 0.0L);
            target.big_value = 0;
            target.str_value.clear();
            target.is_assigned = true;
            return;
        }

        TypeInfo resolved_type = type_hint;
        if (resolved_type == TYPE_UNKNOWN) {
            if (!allow_type_override && target.type != TYPE_UNKNOWN &&
                target.type != TYPE_UNION && target.type != TYPE_INTERFACE &&
                target.type != TYPE_STRUCT && target.type < TYPE_ARRAY_BASE) {
                resolved_type = target.type;
            } else if (typed_value.numeric_type != TYPE_UNKNOWN) {
                resolved_type = typed_value.numeric_type;
            } else if (typed_value.type.type_info != TYPE_UNKNOWN) {
                resolved_type = typed_value.type.type_info;
            }
        }
        if (resolved_type == TYPE_UNKNOWN) {
            resolved_type = (!allow_type_override && target.type != TYPE_UNKNOWN)
                                ? target.type
                                : TYPE_INT;
        }

        if ((allow_type_override || target.type == TYPE_UNKNOWN) &&
            target.type != TYPE_UNION) {
            target.type = resolved_type;
        }

        if (target.type == TYPE_UNION) {
            target.current_type = resolved_type;
        }

        target.str_value.clear();
        target.big_value = 0;

        if (resolved_type == TYPE_FLOAT) {
            long double quad_val = typed_value.as_quad();
            float f = static_cast<float>(quad_val);
            target.float_value = f;
            target.double_value = static_cast<double>(f);
            target.quad_value = static_cast<long double>(f);
            target.value = static_cast<int64_t>(f);
        } else if (resolved_type == TYPE_DOUBLE) {
            long double quad_val = typed_value.as_quad();
            double d = static_cast<double>(quad_val);
            target.float_value = static_cast<float>(d);
            target.double_value = d;
            target.quad_value = static_cast<long double>(d);
            target.value = static_cast<int64_t>(d);
        } else if (resolved_type == TYPE_QUAD) {
            long double q = typed_value.as_quad();
            target.float_value = static_cast<float>(q);
            target.double_value = static_cast<double>(q);
            target.quad_value = q;
            target.value = static_cast<int64_t>(q);
        } else if (resolved_type == TYPE_STRING) {
            target.type = TYPE_STRING;
            target.str_value = typed_value.as_string();
            target.value = 0;
            target.float_value = 0.0f;
            target.double_value = 0.0;
            target.quad_value = 0.0L;
        } else {
            int64_t numeric_value = typed_value.as_numeric();
            if (resolved_type == TYPE_BOOL) {
                numeric_value = (numeric_value != 0) ? 1 : 0;
            }
            clamp_unsigned(numeric_value);
            if (interpreter_->is_debug_mode()) {
                debug_print("ASSIGN_DEBUG: name=%s target_type=%d resolved_type=%d numeric_value=%lld allow_override=%d\n",
                            name.c_str(), static_cast<int>(target.type), static_cast<int>(resolved_type),
                            static_cast<long long>(numeric_value), allow_type_override ? 1 : 0);
            }
            TypeInfo range_check_type = resolved_type;
            if (target.type != TYPE_UNKNOWN && target.type != TYPE_UNION &&
                target.type != TYPE_INTERFACE && target.type != TYPE_STRUCT &&
                target.type < TYPE_ARRAY_BASE) {
                range_check_type = target.type;
            }

            interpreter_->type_manager_->check_type_range(range_check_type,
                                                          numeric_value, name,
                                                          target.is_unsigned);
            setNumericFields(target, static_cast<long double>(numeric_value));
        }

        target.is_assigned = true;
    };

    Variable *var = find_variable(name);
    if (!var) {
        Variable new_var;
        apply_assignment(new_var, true);
        new_var.is_const = is_const;
        current_scope().variables[name] = new_var;
        return;
    }

    if (var->is_const && var->is_assigned) {
        std::fprintf(stderr, "Cannot reassign const variable: %s\n",
                     name.c_str());
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
        std::exit(1);
    }

    apply_assignment(*var, false);
    if (is_const) {
        var->is_const = true;
    }
}

void VariableManager::assign_function_parameter(const std::string &name,
                                                int64_t value,
                                                TypeInfo type,
                                                bool is_unsigned) {
    TypeInfo effective = type != TYPE_UNKNOWN ? type : TYPE_INT;
    InferredType inferred(effective, type_info_to_string(effective));
    TypedValue typed_value(value, inferred);
    assign_function_parameter(name, typed_value, type, is_unsigned);
}

void VariableManager::assign_function_parameter(const std::string &name,
                                                const TypedValue &value,
                                                TypeInfo type,
                                                bool is_unsigned) {
    Scope &scope = current_scope();
    auto iter = scope.variables.find(name);
    if (iter == scope.variables.end()) {
        Variable placeholder;
        placeholder.type = TYPE_UNKNOWN;
        placeholder.is_unsigned = is_unsigned;
        placeholder.is_assigned = false;
        iter = scope.variables.emplace(name, placeholder).first;
    } else {
        iter->second.is_assigned = false;
        iter->second.is_unsigned = is_unsigned;
    }

    assign_variable(name, value, type, false);

    if (auto updated_iter = scope.variables.find(name); updated_iter != scope.variables.end()) {
        updated_iter->second.is_unsigned = is_unsigned;
    } else if (Variable *updated_var = find_variable(name)) {
        updated_var->is_unsigned = is_unsigned;
    }
}

void VariableManager::assign_array_parameter(const std::string &name,
                                             const Variable &source_array,
                                             TypeInfo type) {
    Variable array_param = source_array;
    array_param.type = type != TYPE_UNKNOWN ? type : source_array.type;
    array_param.is_assigned = true;
    current_scope().variables[name] = array_param;
}

void VariableManager::process_var_decl_or_assign(const ASTNode *node) {
    // debug_msg(DebugMsgId::VAR_MANAGER_PROCESS, (int)node->node_type, node->name.c_str());
    if (interpreter_->debug_mode) {
        debug_print("VAR_DEBUG: process_var_decl_or_assign called for %s, node_type=%d\n", 
                   node->name.c_str(), static_cast<int>(node->node_type));
        debug_print("VAR_DEBUG: type_info=%d, type_name='%s'\n", 
                   static_cast<int>(node->type_info), node->type_name.c_str());
        debug_print("VAR_DEBUG: node->is_unsigned=%d\n", node->is_unsigned ? 1 : 0);
        
        std::string resolved = interpreter_->type_manager_->resolve_typedef(node->type_name);
        debug_print("VAR_DEBUG: resolve_typedef('%s') = '%s'\n", 
                   node->type_name.c_str(), resolved.c_str());
        debug_print("VAR_DEBUG: condition check: !empty=%d, resolved!=original=%d\n", 
                   !node->type_name.empty(), resolved != node->type_name);
    }

    auto clamp_unsigned_initial = [&](Variable &target, int64_t &value,
                                      const char *context) {
        if (!target.is_unsigned || value >= 0) {
            return;
        }
        const char *var_name = node ? node->name.c_str() : "<anonymous>";
        DEBUG_WARN(VARIABLE,
                   "Unsigned variable %s %s negative value (%lld); clamping to 0",
                   var_name, context, static_cast<long long>(value));
        value = 0;
    };
    if (node->node_type == ASTNodeType::AST_VAR_DECL) {
        // 変数宣言の処理
        Variable var;
        var.type = node->type_info;
        var.is_const = node->is_const;
        var.is_assigned = false;
        var.is_array = false;
        var.array_size = 0;
    var.is_unsigned = node->is_unsigned;
        
        // struct変数の場合の追加設定
        if (node->type_info == TYPE_STRUCT && !node->type_name.empty()) {
            var.is_struct = true;
            var.struct_type_name = node->type_name;
        }
        
        // interface変数の場合の追加設定
        if (node->type_info == TYPE_INTERFACE && !node->type_name.empty()) {
            var.interface_name = node->type_name;
        }

        // 新しいArrayTypeInfoが設定されている場合の処理
        if (node->array_type_info.base_type != TYPE_UNKNOWN) {
            debug_print("VAR_DEBUG: Taking ArrayTypeInfo branch (base_type=%d)\n", static_cast<int>(node->array_type_info.base_type));
            
            // ArrayTypeInfoが設定されている場合は配列として処理
            var.is_array = true;
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + node->array_type_info.base_type);
            var.array_type_info = node->array_type_info;

            // typedef名を保存（interfaceでの型マッチングに使用）
            if (!node->type_name.empty()) {
                var.struct_type_name = node->type_name;
            }

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

                // 配列も符号無し指定を保持
                if (node->is_unsigned) {
                    var.is_unsigned = true;
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

            // union typedefの場合
            if (interpreter_->type_manager_->is_union_type(node->type_name)) {
                if (debug_mode) {
                    debug_print("TYPEDEF_DEBUG: Processing union typedef: %s\n", node->type_name.c_str());
                }
                var.type = TYPE_UNION;
                var.type_name = node->type_name;  // union型名を保存
                var.current_type = TYPE_UNKNOWN;  // まだ値が設定されていない

                // 初期化値がある場合は検証して代入
                if (node->right || node->init_expr) {
                    ASTNode* init_node = node->init_expr ? node->init_expr.get() : node->right.get();
                    assign_union_value(var, node->type_name, init_node);
                }
                
                // union型変数の処理完了後、変数を登録して終了
                interpreter_->current_scope().variables[node->name] = var;
                return;
            }
            // 配列typedefの場合
            else if (resolved_type.find("[") != std::string::npos) {
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
                    // プリミティブtypedefの場合
                    var.type = interpreter_->type_manager_->string_to_type_info(
                        resolved_type);
                    
                    // プリミティブtypedefでもimpl解決のためにstruct_type_nameを設定
                    var.struct_type_name = node->type_name;
                    
                    if (debug_mode) {
                        debug_print("TYPEDEF_DEBUG: Set primitive typedef '%s' with struct_type_name='%s'\n", 
                                    node->type_name.c_str(), node->type_name.c_str());
                    }
                }
            }
            
            // カスタム型の保存（union以外）
            if (var.type != TYPE_UNION) {
                var.type_name = node->type_name;
                var.current_type = var.type;
            }

            // 初期化処理
            if (node->right || node->init_expr) {
                ASTNode* init_node = node->init_expr ? node->init_expr.get() : node->right.get();
                
                // 三項演算子による初期化の新しい処理（型推論使用）
                if (init_node->node_type == ASTNodeType::AST_TERNARY_OP) {
                    TypedValue ternary_result = interpreter_->evaluate_ternary_typed(init_node);
                    
                    if (ternary_result.is_string()) {
                        var.str_value = ternary_result.string_value;
                        var.value = 0;
                    } else {
                        var.value = ternary_result.value;
                        var.str_value = "";
                    }
                    
                    interpreter_->current_scope().variables[node->name] = var;
                    return; // 早期リターン
                }
                
                // 型チェック: typedef変数の初期化値が適切な型かチェック
                if (var.type == TYPE_STRING && init_node->node_type == ASTNodeType::AST_NUMBER) {
                    throw std::runtime_error("Type mismatch: Cannot assign integer value " + 
                                           std::to_string(init_node->int_value) + " to string type '" + 
                                           node->type_name + "'");
                } else if ((var.type == TYPE_INT || var.type == TYPE_LONG || var.type == TYPE_SHORT || var.type == TYPE_TINY) 
                          && init_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    throw std::runtime_error("Type mismatch: Cannot assign string value '" + 
                                           init_node->str_value + "' to numeric type '" + 
                                           node->type_name + "'");
                } else if (var.type == TYPE_BOOL && init_node->node_type == ASTNodeType::AST_NUMBER && 
                          init_node->int_value != 0 && init_node->int_value != 1) {
                    throw std::runtime_error("Type mismatch: Cannot assign integer value " + 
                                           std::to_string(init_node->int_value) + " to boolean type '" + 
                                           node->type_name + "'");
                }
                
                // カスタム型（typedef）変数代入の型チェック
                if (init_node->node_type == ASTNodeType::AST_VARIABLE) {
                    Variable* source_var = find_variable(init_node->name);
                    if (source_var && !source_var->type_name.empty()) {
                        // 代入元がカスタム型を持つ場合、型名の整合性をチェック
                        std::string source_resolved = interpreter_->type_manager_->resolve_typedef(source_var->type_name);
                        std::string target_resolved = interpreter_->type_manager_->resolve_typedef(node->type_name);
                        
                        // 基本型は同じだが、カスタム型名が異なる場合
                        if (source_resolved == target_resolved && source_var->type_name != node->type_name) {
                            // 再帰的typedefでは同じ基本型に解決される場合は互換性がある
                            // 例: ID=int, UserID=ID の場合、IDとUserIDは互換性がある
                            // この場合は型チェックを通す（TypeScriptの型エイリアス的動作）
                            if (interpreter_->is_debug_mode()) {
                                debug_print("RECURSIVE_TYPEDEF_DEBUG: %s and %s both resolve to %s - allowing assignment\n",
                                           source_var->type_name.c_str(), node->type_name.c_str(), source_resolved.c_str());
                            }
                            // 互換性があるものとして処理を続行
                        }
                    }
                }
                
                if (var.type == TYPE_STRING && init_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
                    // 文字列リテラル初期化
                    var.str_value = init_node->str_value;
                    var.value = 0; // プレースホルダー
                    var.is_assigned = true;
                } else if (var.type == TYPE_STRING && init_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // 文字列配列アクセス初期化
                    // 配列名を取得
                    std::string array_name;
                    const ASTNode* base_node = init_node;
                    while (base_node && base_node->node_type == ASTNodeType::AST_ARRAY_REF && base_node->left) {
                        base_node = base_node->left.get();
                    }
                    if (base_node && base_node->node_type == ASTNodeType::AST_VARIABLE) {
                        array_name = base_node->name;
                    }
                    
                    Variable* array_var = find_variable(array_name);
                    if (array_var && array_var->is_array && array_var->array_type_info.base_type == TYPE_STRING) {
                        debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_ACCESS, array_name.c_str());
                        
                        // 多次元インデックスを収集
                        std::vector<int64_t> indices;
                        const ASTNode* current_node = init_node;
                        while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                            int64_t index = interpreter_->expression_evaluator_->evaluate_expression(current_node->array_index.get());
                            indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                            current_node = current_node->left.get();
                        }
                        
                        // インデックス情報をデバッグ出力
                        std::string indices_str;
                        for (size_t i = 0; i < indices.size(); ++i) {
                            if (i > 0) indices_str += ", ";
                            indices_str += std::to_string(indices[i]);
                        }
                        debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_INDICES, indices_str.c_str());
                        
                        try {
                            std::string str_value = interpreter_->getMultidimensionalStringArrayElement(*array_var, indices);
                            debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_VALUE, str_value.c_str());
                            var.str_value = str_value;
                            var.value = 0; // プレースホルダー
                            var.is_assigned = true;
                        } catch (const std::exception& e) {
                            var.str_value = "";
                            var.value = 0;
                            var.is_assigned = true;
                        }
                    } else {
                        // 配列アクセスではない場合は通常の処理にフォールバック
                        int64_t value = interpreter_->expression_evaluator_->evaluate_expression(init_node);
                        var.str_value = std::to_string(value);
                        var.value = value;
                        var.is_assigned = true;
                    }
                } else if (var.type == TYPE_STRING && init_node->node_type == ASTNodeType::AST_BINARY_OP && init_node->op == "+") {
                    // 文字列連結の処理
                    std::string left_str, right_str;
                    bool success = true;
                    
                    // 左オペランドを取得
                    if (init_node->left->node_type == ASTNodeType::AST_VARIABLE) {
                        Variable* left_var = find_variable(init_node->left->name);
                        if (left_var && (left_var->type == TYPE_STRING || left_var->current_type == TYPE_STRING)) {
                            left_str = left_var->str_value;
                        } else {
                            success = false;
                        }
                    } else if (init_node->left->node_type == ASTNodeType::AST_STRING_LITERAL) {
                        left_str = init_node->left->str_value;
                    } else {
                        success = false;
                    }
                    
                    // 右オペランドを取得
                    if (success) {
                        if (init_node->right->node_type == ASTNodeType::AST_VARIABLE) {
                            Variable* right_var = find_variable(init_node->right->name);
                            if (right_var && (right_var->type == TYPE_STRING || right_var->current_type == TYPE_STRING)) {
                                right_str = right_var->str_value;
                            } else {
                                success = false;
                            }
                        } else if (init_node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
                            right_str = init_node->right->str_value;
                        } else {
                            success = false;
                        }
                    }
                    
                    if (success) {
                        // 文字列連結を実行
                        var.str_value = left_str + right_str;
                        var.value = 0; // プレースホルダー
                        var.is_assigned = true;
                    } else {
                        // 文字列連結に失敗した場合は通常の処理にフォールバック
                        throw std::runtime_error("String concatenation failed for typedef variable '" + node->name + "'");
                    }
                } else {
                    // その他の初期化
                    try {
                        int64_t value = interpreter_->expression_evaluator_->evaluate_expression(init_node);
                        clamp_unsigned_initial(var, value,
                                               "initialized with expression");
                        var.value = value;
                        var.is_assigned = true;
                        
                        // 型範囲チェック
                        if (var.type != TYPE_STRING) {
                            interpreter_->type_manager_->check_type_range(
                                var.type, var.value, node->name,
                                var.is_unsigned);
                        }
                    } catch (const ReturnException &ret) {
                        // 関数戻り値の処理
                        if (var.type == TYPE_STRING && ret.type == TYPE_STRING) {
                            // 文字列戻り値の場合
                            var.str_value = ret.str_value;
                            var.is_assigned = true;
                        } else if (ret.is_struct && var.type == TYPE_STRUCT) {
                            // struct戻り値の場合
                            var = ret.struct_value;
                            var.is_assigned = true;
                        } else if (ret.is_struct && var.type == TYPE_UNION) {
                            // union型変数への構造体代入の場合
                            if (interpreter_->get_type_manager()->is_custom_type_allowed_for_union(var.type_name, ret.struct_value.struct_type_name)) {
                                var.value = ret.struct_value.value;
                                var.str_value = ret.struct_value.str_value;
                                var.current_type = TYPE_STRUCT;
                                var.is_struct = true;
                                var.struct_type_name = ret.struct_value.struct_type_name;
                                var.struct_members = ret.struct_value.struct_members;
                                var.is_assigned = true;
                            } else {
                                throw std::runtime_error("Struct type '" + ret.struct_value.struct_type_name + 
                                                       "' is not allowed for union type " + var.type_name);
                            }
                        } else if (!ret.is_array && !ret.is_struct) {
                            // 数値戻り値の場合
                            int64_t numeric_value = ret.value;
                            clamp_unsigned_initial(var, numeric_value,
                                                   "initialized with function return");
                            var.value = numeric_value;
                            var.is_assigned = true;
                            
                            // 型範囲チェック
                            if (var.type != TYPE_STRING) {
                                interpreter_->type_manager_->check_type_range(
                                    var.type, var.value, node->name,
                                    var.is_unsigned);
                            }
                        } else {
                            throw std::runtime_error("Incompatible return type for typedef variable '" + node->name + "'");
                        }
                    } catch (const std::exception& e) {
                        throw std::runtime_error("Failed to initialize typedef variable '" + node->name + "': " + e.what());
                    }
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
            
            // 構造体のtype_nameを設定
            var.type_name = node->type_name;
            
            // typedef名を実際のstruct名に解決
            std::string resolved_struct_type = interpreter_->type_manager_->resolve_typedef(node->type_name);
            var.struct_type_name = resolved_struct_type;

            // 構造体配列かどうかをチェック
            std::string base_struct_type = resolved_struct_type;
            bool is_struct_array = false;
            int struct_array_size = 0;
            std::vector<int> struct_array_dimensions;

            // まずASTの配列情報を優先的に確認
            if (node->array_type_info.is_array()) {
                is_struct_array = true;
                var.is_array = true;
                var.is_multidimensional =
                    node->array_type_info.dimensions.size() > 1;

                for (const auto &dim : node->array_type_info.dimensions) {
                    if (dim.is_dynamic || dim.size < 0) {
                        struct_array_dimensions.push_back(0);
                    } else {
                        struct_array_dimensions.push_back(dim.size);
                    }
                }

                if (!struct_array_dimensions.empty() &&
                    struct_array_dimensions[0] > 0) {
                    struct_array_size = struct_array_dimensions[0];
                }
            } else if (node->is_array || node->array_size >= 0 ||
                       !node->array_dimensions.empty()) {
                is_struct_array = true;
                var.is_array = true;

                int declared_size = node->array_size;
                if (declared_size < 0 && !node->array_dimensions.empty()) {
                    // array_dimensionsにはサイズ式が格納される場合があるが、
                    // 現状では定数サイズのみ対応
                    const ASTNode *size_node = node->array_dimensions[0].get();
                    if (size_node && size_node->node_type == ASTNodeType::AST_NUMBER) {
                        declared_size = static_cast<int>(size_node->int_value);
                    }
                }

                if (declared_size >= 0) {
                    struct_array_size = declared_size;
                    struct_array_dimensions.push_back(declared_size);
                }
            }

            // 互換性のため、型名に配列表記が含まれる場合も処理
            if (!is_struct_array) {
                size_t bracket_pos = resolved_struct_type.find("[");
                if (bracket_pos != std::string::npos) {
                    is_struct_array = true;
                    base_struct_type =
                        resolved_struct_type.substr(0, bracket_pos);

                    size_t close_bracket_pos =
                        resolved_struct_type.find("]", bracket_pos);
                    if (close_bracket_pos != std::string::npos) {
                        std::string size_str = resolved_struct_type.substr(
                            bracket_pos + 1,
                            close_bracket_pos - bracket_pos - 1);
                        if (!size_str.empty()) {
                            struct_array_size = std::stoi(size_str);
                            struct_array_dimensions.push_back(struct_array_size);
                        }
                    }

                    var.is_array = true;
                }
            }

            if (is_struct_array) {
                if (!struct_array_dimensions.empty()) {
                    var.array_dimensions = struct_array_dimensions;
                    if (!var.is_multidimensional &&
                        var.array_dimensions.size() > 1) {
                        var.is_multidimensional = true;
                    }
                }

                if (struct_array_size > 0) {
                    var.array_size = struct_array_size;
                }
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
                            member_var.type_name = member.type_alias;  // Union型名などを保持
                            member_var.is_pointer = member.is_pointer;
                            member_var.pointer_depth = member.pointer_depth;
                            member_var.pointer_base_type_name = member.pointer_base_type_name;
                            member_var.pointer_base_type = member.pointer_base_type;
                            member_var.is_reference = member.is_reference;
                            member_var.is_unsigned = member.is_unsigned;
                            member_var.is_private_member = member.is_private;

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
                        member_var.type_name = member.type_alias;  // Union型名などを保持
                        member_var.is_pointer = member.is_pointer;
                        member_var.pointer_depth = member.pointer_depth;
                        member_var.pointer_base_type_name = member.pointer_base_type_name;
                        member_var.pointer_base_type = member.pointer_base_type;
                        member_var.is_reference = member.is_reference;
                        member_var.is_unsigned = member.is_unsigned;
                        member_var.is_private_member = member.is_private;

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

            } else if (!var.interface_name.empty()) {
                auto assign_from_source = [&](const Variable& source, const std::string& source_name) {
                    assign_interface_view(node->name, var, source, source_name);
                };

                if (node->init_expr->node_type == ASTNodeType::AST_VARIABLE ||
                    node->init_expr->node_type == ASTNodeType::AST_IDENTIFIER) {
                    std::string source_var_name = node->init_expr->name;
                    Variable* source_var = find_variable(source_var_name);
                    if (!source_var) {
                        throw std::runtime_error("Source variable not found: " + source_var_name);
                    }
                    if (!source_var->is_struct && !isPrimitiveType(source_var) &&
                        source_var->type < TYPE_ARRAY_BASE && source_var->type != TYPE_INTERFACE) {
                        throw std::runtime_error("Cannot assign non-struct/non-primitive to interface variable");
                    }

                    debug_msg(DebugMsgId::INTERFACE_VARIABLE_ASSIGN, var.interface_name.c_str(), source_var_name.c_str());
                    assign_from_source(*source_var, source_var_name);
                    return;
                }

                auto create_temp_primitive = [&](TypeInfo value_type, int64_t numeric_value, const std::string& string_value) {
                    Variable temp;
                    temp.is_assigned = true;
                    temp.type = value_type;
                    if (value_type == TYPE_STRING) {
                        temp.str_value = string_value;
                    } else {
                        temp.value = numeric_value;
                    }
                    temp.struct_type_name = getPrimitiveTypeNameForImpl(value_type);
                    return temp;
                };

                try {
                    if (node->init_expr->node_type == ASTNodeType::AST_STRING_LITERAL) {
                        Variable temp = create_temp_primitive(TYPE_STRING, 0, node->init_expr->str_value);
                        assign_from_source(temp, "");
                        return;
                    }

                    int64_t numeric_value = interpreter_->evaluate(node->init_expr.get());
                    TypeInfo resolved_type = node->init_expr->type_info != TYPE_UNKNOWN
                                                  ? node->init_expr->type_info
                                                  : TYPE_INT;
                    Variable temp = create_temp_primitive(resolved_type, numeric_value, "");
                    assign_from_source(temp, "");
                    return;
                } catch (const ReturnException& ret) {
                    if (ret.is_array) {
                        throw std::runtime_error("Cannot assign array return value to interface variable '" + node->name + "'");
                    }

                    if (!ret.is_struct) {
                        if (ret.type == TYPE_STRING) {
                            Variable temp = create_temp_primitive(TYPE_STRING, 0, ret.str_value);
                            assign_from_source(temp, "");
                            return;
                        }

                        Variable temp = create_temp_primitive(ret.type, ret.value, ret.str_value);
                        assign_from_source(temp, "");
                        return;
                    }

                    assign_from_source(ret.struct_value, "");
                    return;
                }

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

            } else if (var.is_struct && node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
                // 構造体変数の関数呼び出し初期化: Calculator add_result = math.add(10, 5);
                
                try {
                    // 構造体戻り値を期待した関数実行（副作用のため実行）
                    (void)interpreter_->expression_evaluator_->evaluate_expression(node->init_expr.get());
                    // 通常の数値戻り値の場合はエラー
                    throw std::runtime_error("Expected struct return but got numeric value");
                } catch (const ReturnException &ret) {
                    if (ret.is_struct) {
                        // 構造体戻り値を変数に代入
                        var = ret.struct_value;
                        var.is_assigned = true;
                        
                        // 変数を登録
                        current_scope().variables[node->name] = var;
                        
                        // 個別メンバー変数も作成
                        for (const auto& member : ret.struct_value.struct_members) {
                            std::string member_path = node->name + "." + member.first;
                            current_scope().variables[member_path] = member.second;
                            // 配列メンバーの場合、個別要素変数も作成
                            if (member.second.is_array) {
                                for (int i = 0; i < member.second.array_size; i++) {
                                    std::string element_name = member_path + "[" + std::to_string(i) + "]";
                                    Variable element_var;
                                    element_var.type = member.second.type >= TYPE_ARRAY_BASE ? 
                                                      static_cast<TypeInfo>(member.second.type - TYPE_ARRAY_BASE) : 
                                                      member.second.type;
                                    element_var.is_assigned = true;
                                    
                                    if (element_var.type == TYPE_STRING && i < static_cast<int>(member.second.array_strings.size())) {
                                        element_var.str_value = member.second.array_strings[i];
                                    } else if (element_var.type != TYPE_STRING && i < static_cast<int>(member.second.array_values.size())) {
                                        element_var.value = member.second.array_values[i];
                                    }
                                    
                                    current_scope().variables[element_name] = element_var;
                                }
                            }
                        }
                        
                        return; // 構造体関数呼び出し処理完了後は早期リターン
                    } else {
                        throw std::runtime_error("Function did not return expected struct type");
                    }
                }

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
                if (interpreter_->debug_mode) {
                    debug_print("VAR_DEBUG: stored array var %s with is_unsigned=%d before literal assignment\n",
                                node->name.c_str(), var.is_unsigned ? 1 : 0);
                }

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
                        } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE || ret.type == TYPE_QUAD) {
                            // float/double/quad配列
                            if (!ret.double_array_3d.empty() &&
                                !ret.double_array_3d[0].empty()) {
                                
                                // typedef配列名から多次元配列かどうかを判定
                                std::string actual_type = interpreter_->type_manager_->resolve_typedef(ret.array_type_name);
                                bool is_multidim = (actual_type.find("[][]") != std::string::npos || 
                                                   ret.array_type_name.find("[][]") != std::string::npos ||
                                                   ret.double_array_3d.size() > 1 || 
                                                   (ret.double_array_3d.size() == 1 && ret.double_array_3d[0].size() > 1));

                                if (is_multidim) {
                                    // 多次元float/double配列の場合 - 全要素を展開
                                    if (ret.type == TYPE_FLOAT) {
                                        var.multidim_array_float_values.clear();
                                        for (const auto &plane : ret.double_array_3d) {
                                            for (const auto &row : plane) {
                                                for (const auto &element : row) {
                                                    var.multidim_array_float_values.push_back(static_cast<float>(element));
                                                }
                                            }
                                        }
                                        var.array_size = var.multidim_array_float_values.size();
                                    } else if (ret.type == TYPE_DOUBLE) {
                                        var.multidim_array_double_values.clear();
                                        for (const auto &plane : ret.double_array_3d) {
                                            for (const auto &row : plane) {
                                                for (const auto &element : row) {
                                                    var.multidim_array_double_values.push_back(element);
                                                }
                                            }
                                        }
                                        var.array_size = var.multidim_array_double_values.size();
                                    } else { // TYPE_QUAD
                                        var.multidim_array_quad_values.clear();
                                        for (const auto &plane : ret.double_array_3d) {
                                            for (const auto &row : plane) {
                                                for (const auto &element : row) {
                                                    var.multidim_array_quad_values.push_back(static_cast<long double>(element));
                                                }
                                            }
                                        }
                                        var.array_size = var.multidim_array_quad_values.size();
                                    }
                                    var.is_multidimensional = true;
                                    var.array_values.clear();
                                    
                                    // 配列の次元情報を設定
                                    if (!ret.double_array_3d[0].empty()) {
                                        var.array_dimensions.clear();
                                        var.array_dimensions.push_back(ret.double_array_3d[0].size());     // 行数
                                        var.array_dimensions.push_back(ret.double_array_3d[0][0].size()); // 列数
                                    }
                                } else if (!ret.double_array_3d[0][0].empty()) {
                                    // 1次元float/double配列の場合
                                    if (ret.type == TYPE_FLOAT) {
                                        var.array_float_values.clear();
                                        for (const auto &element : ret.double_array_3d[0][0]) {
                                            var.array_float_values.push_back(static_cast<float>(element));
                                        }
                                        var.array_size = var.array_float_values.size();
                                    } else if (ret.type == TYPE_DOUBLE) {
                                        var.array_double_values.clear();
                                        for (const auto &element : ret.double_array_3d[0][0]) {
                                            var.array_double_values.push_back(element);
                                        }
                                        var.array_size = var.array_double_values.size();
                                    } else { // TYPE_QUAD
                                        var.array_quad_values.clear();
                                        for (const auto &element : ret.double_array_3d[0][0]) {
                                            var.array_quad_values.push_back(static_cast<long double>(element));
                                        }
                                        var.array_size = var.array_quad_values.size();
                                    }
                                }
                                var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + ret.type);
                            }
                        } else {
                            // 整数型配列
                            if (!ret.int_array_3d.empty() &&
                                !ret.int_array_3d[0].empty()) {
                                
                                // typedef配列名から多次元配列かどうかを判定
                                // typedefの場合、実際の型を解決して確認
                                std::string actual_type = interpreter_->type_manager_->resolve_typedef(ret.array_type_name);
                                bool is_multidim = (actual_type.find("[][]") != std::string::npos || 
                                                   ret.array_type_name.find("[][]") != std::string::npos ||
                                                   ret.int_array_3d.size() > 1 || 
                                                   (ret.int_array_3d.size() == 1 && ret.int_array_3d[0].size() > 1));

                                if (is_multidim) {
                                    // 多次元配列の場合 - 全要素を展開
                                    var.multidim_array_values.clear();
                                    for (const auto &plane : ret.int_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                var.multidim_array_values.push_back(element);
                                            }
                                        }
                                    }
                                    var.array_size = var.multidim_array_values.size();
                                    var.is_multidimensional = true;
                                    var.array_values.clear();
                                    
                                    // 配列の次元情報を設定
                                    if (!ret.int_array_3d[0].empty()) {
                                        var.array_dimensions.clear();
                                        var.array_dimensions.push_back(ret.int_array_3d[0].size());     // 行数
                                        var.array_dimensions.push_back(ret.int_array_3d[0][0].size()); // 列数
                                    }
                                } else if (!ret.int_array_3d[0][0].empty()) {
                                    // 1次元配列の場合
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
                            int64_t numeric_value = ret.value;
                            clamp_unsigned_initial(var, numeric_value,
                                                   "initialized with function return");
                            var.value = numeric_value;
                        }
                        var.is_assigned = true;
                    }
                }
            } else {
                if (var.type == TYPE_STRING && node->init_expr->node_type == ASTNodeType::AST_ARRAY_REF) {
                    // 文字列配列アクセス初期化
                    // 配列名を取得
                    std::string array_name;
                    const ASTNode* base_node = node->init_expr.get();
                    while (base_node && base_node->node_type == ASTNodeType::AST_ARRAY_REF && base_node->left) {
                        base_node = base_node->left.get();
                    }
                    if (base_node && base_node->node_type == ASTNodeType::AST_VARIABLE) {
                        array_name = base_node->name;
                    }
                    
                    Variable* array_var = find_variable(array_name);
                    if (array_var && array_var->is_array && array_var->array_type_info.base_type == TYPE_STRING) {
                        debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_ACCESS, array_name.c_str());
                        
                        // 多次元インデックスを収集
                        std::vector<int64_t> indices;
                        const ASTNode* current_node = node->init_expr.get();
                        while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                            int64_t index = interpreter_->expression_evaluator_->evaluate_expression(current_node->array_index.get());
                            indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                            current_node = current_node->left.get();
                        }
                        
                        // インデックス情報をデバッグ出力
                        std::string indices_str;
                        for (size_t i = 0; i < indices.size(); ++i) {
                            if (i > 0) indices_str += ", ";
                            indices_str += std::to_string(indices[i]);
                        }
                        debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_INDICES, indices_str.c_str());
                        
                        try {
                            std::string str_value = interpreter_->getMultidimensionalStringArrayElement(*array_var, indices);
                            debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_VALUE, str_value.c_str());
                            var.str_value = str_value;
                            var.value = 0; // プレースホルダー
                            var.is_assigned = true;
                        } catch (const std::exception& e) {
                            var.str_value = "";
                            var.value = 0;
                            var.is_assigned = true;
                        }
                    } else {
                        // 配列アクセスではない場合は通常の処理にフォールバック
                        int64_t value = interpreter_->expression_evaluator_->evaluate_expression(node->init_expr.get());
                        var.str_value = std::to_string(value);
                        var.value = value;
                        var.is_assigned = true;
                    }
                } else if (node->init_expr->node_type == ASTNodeType::AST_FUNC_CALL) {
                    // 関数呼び出しの場合、型推論対応評価を使用
                    try {
                        TypedValue typed_result = interpreter_->expression_evaluator_
                                ->evaluate_typed_expression(node->init_expr.get());
                        
                        if (typed_result.is_string()) {
                            var.str_value = typed_result.string_value;
                            var.value = 0;
                        } else if (typed_result.numeric_type == TYPE_FLOAT || 
                                   typed_result.numeric_type == TYPE_DOUBLE || 
                                   typed_result.numeric_type == TYPE_QUAD) {
                            // float/double/quad戻り値の場合
                            long double quad_val = typed_result.as_quad();
                            
                            if (typed_result.numeric_type == TYPE_FLOAT) {
                                float f = static_cast<float>(quad_val);
                                var.float_value = f;
                                var.double_value = static_cast<double>(f);
                                var.quad_value = static_cast<long double>(f);
                                var.value = static_cast<int64_t>(f);
                            } else if (typed_result.numeric_type == TYPE_DOUBLE) {
                                double d = static_cast<double>(quad_val);
                                var.float_value = static_cast<float>(d);
                                var.double_value = d;
                                var.quad_value = static_cast<long double>(d);
                                var.value = static_cast<int64_t>(d);
                            } else { // TYPE_QUAD
                                var.float_value = static_cast<float>(quad_val);
                                var.double_value = static_cast<double>(quad_val);
                                var.quad_value = quad_val;
                                var.value = static_cast<int64_t>(quad_val);
                            }
                            var.str_value = "";
                        } else {
                            int64_t numeric_value = typed_result.value;
                            clamp_unsigned_initial(var, numeric_value,
                                                   "initialized with expression");
                            var.value = numeric_value;
                            var.str_value = "";
                        }
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
                            int64_t numeric_value = ret.value;
                            clamp_unsigned_initial(var, numeric_value,
                                                   "initialized with function return");
                            var.value = numeric_value;
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
                    // 型推論対応の式評価を使用して文字列・数値を取得
                    TypedValue typed_result = interpreter_->expression_evaluator_
                            ->evaluate_typed_expression(node->init_expr.get());

                    if (typed_result.is_string()) {
                        var.type = TYPE_STRING;
                        var.str_value = typed_result.string_value;
                        setNumericFields(var, 0.0L);
                    } else if (typed_result.is_numeric()) {
                        var.str_value.clear();

                        TypeInfo inferred_type = var.type;
                        if (inferred_type == TYPE_UNKNOWN &&
                            typed_result.numeric_type != TYPE_UNKNOWN) {
                            inferred_type = typed_result.numeric_type;
                            var.type = inferred_type;
                        }

                        const long double quad_value = typed_result.as_quad();
                        auto assign_from_quad = [&](long double value) {
                            setNumericFields(var, value);
                        };

                        switch (inferred_type) {
                        case TYPE_FLOAT: {
                            float f = static_cast<float>(quad_value);
                            assign_from_quad(static_cast<long double>(f));
                            break;
                        }
                        case TYPE_DOUBLE: {
                            double d = static_cast<double>(quad_value);
                            assign_from_quad(static_cast<long double>(d));
                            break;
                        }
                        case TYPE_QUAD:
                            assign_from_quad(quad_value);
                            break;
                        default: {
                            int64_t numeric_value = typed_result.as_numeric();
                            clamp_unsigned_initial(var, numeric_value,
                                                   "initialized with expression");
                            assign_from_quad(static_cast<long double>(numeric_value));

                            if (var.type == TYPE_UNKNOWN) {
                                if (typed_result.numeric_type != TYPE_UNKNOWN) {
                                    var.type = typed_result.numeric_type;
                                } else {
                                    var.type = TYPE_INT;
                                }
                            }
                            break;
                        }
                        }
                    } else {
                        // 非数値かつ非文字列の場合は0初期化
                        setNumericFields(var, 0.0L);
                        var.str_value.clear();
                    }
                    var.is_assigned = true;
                }

                // 型範囲チェック
                if (var.type != TYPE_STRING) {
                    interpreter_->type_manager_->check_type_range(
                        var.type, var.value, node->name,
                        var.is_unsigned);
                }
            }
        }

        if (var.is_assigned && !var.is_array && !var.is_struct &&
            var.type != TYPE_STRING) {
            clamp_unsigned_initial(var, var.value,
                                   "initialized with negative value");
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

        // 未定義型のチェック（基本的な変数宣言の場合）
        if (!node->type_name.empty() && node->type_info == TYPE_UNKNOWN) {
            // type_nameが指定されているがtype_infoがUNKNOWNの場合、未定義型の可能性
            std::string resolved = interpreter_->type_manager_->resolve_typedef(node->type_name);
            bool is_union = interpreter_->type_manager_->is_union_type(node->type_name);
            bool is_struct = (interpreter_->find_struct_definition(node->type_name) != nullptr);
            bool is_enum = (interpreter_->get_enum_manager() && interpreter_->get_enum_manager()->enum_exists(node->type_name));
            
            // typedef、union、struct、enumのいずれでもない場合はエラー
            if (resolved == node->type_name && !is_union && !is_struct && !is_enum) {
                throw std::runtime_error("Undefined type: " + node->type_name);
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
            
            Variable *var = find_variable(var_name);
            if (!var) {
                throw std::runtime_error("Undefined variable: " + var_name);
            }

            if (var->is_const && var->is_assigned) {
                throw std::runtime_error("Cannot reassign const variable: " +
                                         var_name);
            }

            if (var->type == TYPE_INTERFACE || !var->interface_name.empty()) {
                auto assign_from_source = [&](const Variable &source,
                                               const std::string &source_name) {
                    assign_interface_view(var_name, *var, source, source_name);
                };

                auto create_temp_primitive = [&](TypeInfo value_type,
                                                 int64_t numeric_value,
                                                 const std::string &string_value) {
                    Variable temp;
                    temp.is_assigned = true;
                    temp.type = value_type;
                    if (value_type == TYPE_STRING) {
                        temp.str_value = string_value;
                    } else {
                        temp.value = numeric_value;
                    }
                    temp.struct_type_name = getPrimitiveTypeNameForImpl(value_type);
                    return temp;
                };

                try {
                    const ASTNode *rhs = node->right.get();
                    if (rhs->node_type == ASTNodeType::AST_VARIABLE ||
                        rhs->node_type == ASTNodeType::AST_IDENTIFIER) {
                        std::string source_var_name = rhs->name;
                        Variable *source_var = find_variable(source_var_name);
                        if (!source_var) {
                            throw std::runtime_error("Source variable not found: " +
                                                     source_var_name);
                        }
                        assign_from_source(*source_var, source_var_name);
                        return;
                    }

                    if (rhs->node_type == ASTNodeType::AST_STRING_LITERAL) {
                        Variable temp = create_temp_primitive(TYPE_STRING, 0,
                                                              rhs->str_value);
                        assign_from_source(temp, "");
                        return;
                    }

                    int64_t numeric_value = interpreter_->expression_evaluator_->evaluate_expression(rhs);
                    TypeInfo resolved_type = rhs->type_info != TYPE_UNKNOWN
                                                 ? rhs->type_info
                                                 : TYPE_INT;
                    Variable temp = create_temp_primitive(resolved_type,
                                                          numeric_value, "");
                    assign_from_source(temp, "");
                    return;
                } catch (const ReturnException &ret) {
                    if (ret.is_array) {
                        throw std::runtime_error(
                            "Cannot assign array return value to interface variable '" +
                            var_name + "'");
                    }

                    if (!ret.is_struct) {
                        if (ret.type == TYPE_STRING) {
                            Variable temp = create_temp_primitive(TYPE_STRING, 0,
                                                                  ret.str_value);
                            assign_from_source(temp, "");
                            return;
                        }

                        TypeInfo resolved_type = ret.type != TYPE_UNKNOWN
                                                     ? ret.type
                                                     : TYPE_INT;
                        Variable temp = create_temp_primitive(resolved_type,
                                                              ret.value,
                                                              ret.str_value);
                        assign_from_source(temp, "");
                        return;
                    }

                    assign_from_source(ret.struct_value, "");
                    return;
                }
            }
            
            // Union型変数への代入の特別処理
            if (var->type == TYPE_UNION) {
                if (debug_mode) {
                    debug_print("UNION_ASSIGN_DEBUG: Processing union assignment for variable '%s'\n", var_name.c_str());
                }
                assign_union_value(*var, var->type_name, node->right.get());
                return; // Union型代入処理完了後は早期リターン
            }
            
            // 文字列変数への代入で、右辺が多次元配列アクセスの場合の特別処理
            if (var->type == TYPE_STRING && node->right->node_type == ASTNodeType::AST_ARRAY_REF) {
                // 配列名を取得
                std::string array_name;
                const ASTNode* base_node = node->right.get();
                while (base_node && base_node->node_type == ASTNodeType::AST_ARRAY_REF && base_node->left) {
                    base_node = base_node->left.get();
                }
                if (base_node && base_node->node_type == ASTNodeType::AST_VARIABLE) {
                    array_name = base_node->name;
                }
                
                Variable* array_var = find_variable(array_name);
                if (array_var && array_var->is_array && array_var->array_type_info.base_type == TYPE_STRING) {
                    debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_ACCESS, array_name.c_str());
                    
                    // 多次元インデックスを収集
                    std::vector<int64_t> indices;
                    const ASTNode* current_node = node->right.get();
                    while (current_node && current_node->node_type == ASTNodeType::AST_ARRAY_REF) {
                        int64_t index = interpreter_->expression_evaluator_->evaluate_expression(current_node->array_index.get());
                        indices.insert(indices.begin(), index); // 先頭に挿入（逆順になるため）
                        current_node = current_node->left.get();
                    }
                    
                    // インデックス情報をデバッグ出力
                    std::string indices_str;
                    for (size_t i = 0; i < indices.size(); ++i) {
                        if (i > 0) indices_str += ", ";
                        indices_str += std::to_string(indices[i]);
                    }
                    debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_INDICES, indices_str.c_str());
                    
                    try {
                        std::string str_value = interpreter_->getMultidimensionalStringArrayElement(*array_var, indices);
                        debug_msg(DebugMsgId::MULTIDIM_STRING_ARRAY_VALUE, str_value.c_str());
                        var->str_value = str_value;
                        var->is_assigned = true;
                        return;
                    } catch (const std::exception& e) {
                        var->str_value = "";
                        var->is_assigned = true;
                        return;
                    }
                }
            }
            
            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->right.get());

            clamp_unsigned_initial(*var, value, "received assignment");

            // varは既に上で定義済み
            if (var->is_const && var->is_assigned) {
                throw std::runtime_error("Cannot reassign const variable: " +
                                         var_name);
            }

            // 型範囲チェック（代入前に実行）
            interpreter_->type_manager_->check_type_range(var->type, value,
                                                          var_name,
                                                          var->is_unsigned);

            var->value = value;
            var->is_assigned = true;

        } else if (node->left &&
                   node->left->node_type == ASTNodeType::AST_VARIABLE) {
            // 通常の変数代入
            std::string var_name = node->left->name;
            
            Variable *var = find_variable(var_name);
            if (!var) {
                throw std::runtime_error("Undefined variable: " + var_name);
            }

            if (var->is_const && var->is_assigned) {
                throw std::runtime_error("Cannot reassign const variable: " +
                                         var_name);
            }

            // Union型変数への代入の特別処理
            if (var->type == TYPE_UNION) {
                if (debug_mode) {
                    debug_print("UNION_ASSIGN_DEBUG: Processing union assignment for variable '%s' (left node)\n", var_name.c_str());
                }
                assign_union_value(*var, var->type_name, node->right.get());
                return; // Union型代入処理完了後は早期リターン
            }

            int64_t value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    node->right.get());

            clamp_unsigned_initial(*var, value, "received assignment");

            // 型範囲チェック（代入前に実行）
            interpreter_->type_manager_->check_type_range(var->type, value,
                                                          var_name,
                                                          var->is_unsigned);

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
                if (var->is_const && var->is_assigned) {
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
            // struct メンバー配列要素代入の処理: obj.member[index] = value または func().member[index] = value
            std::string member_name = node->left->name;

            if (!node->left->left) {
                throw std::runtime_error("Invalid struct member array access");
            }

            // 関数呼び出しの場合と通常の変数の場合を分岐
            if (node->left->left->node_type == ASTNodeType::AST_FUNC_CALL) {
                // 関数呼び出しの場合: func().member[index] = value
                
                try {
                    interpreter_->expression_evaluator_->evaluate_expression(node->left->left.get());
                    throw std::runtime_error("Function did not return a struct for member array assignment");
                } catch (const ReturnException& ret_ex) {
                    Variable base_struct = ret_ex.struct_value;
                    
                    // メンバー配列を取得
                    auto member_it = base_struct.struct_members.find(member_name);
                    if (member_it == base_struct.struct_members.end()) {
                        throw std::runtime_error("Struct member not found: " + member_name);
                    }
                    
                    Variable& member_var = member_it->second;
                    if (!member_var.is_array) {
                        throw std::runtime_error("Member is not an array: " + member_name);
                    }
                    
                    // インデックスを評価
                    std::vector<int64_t> indices;
                    if (node->left->array_indices.empty() && node->left->arguments.empty()) {
                        throw std::runtime_error("No indices found for array access");
                    }
                    
                    if (!node->left->array_indices.empty()) {
                        for (const auto& arg : node->left->array_indices) {
                            int64_t index = interpreter_->expression_evaluator_->evaluate_expression(arg.get());
                            indices.push_back(index);
                        }
                    } else {
                        for (const auto& arg : node->left->arguments) {
                            int64_t index = interpreter_->expression_evaluator_->evaluate_expression(arg.get());
                            indices.push_back(index);
                        }
                    }
                    
                    // 1次元配列の場合
                    if (indices.size() == 1) {
                        int64_t index = indices[0];
                        if (index < 0 || index >= static_cast<int>(member_var.array_values.size())) {
                            throw std::runtime_error("Array index out of bounds");
                        }
                        
                        // 右辺の値を評価（副作用のため実行）
                        (void)interpreter_->expression_evaluator_->evaluate_expression(node->right.get());
                        
                        // 値を代入（関数戻り値なので実際の代入はできないが、エラーを避けるため）
                        throw std::runtime_error("Cannot assign to function return value member array");
                    } else {
                        throw std::runtime_error("Multi-dimensional function return member array assignment not supported");
                    }
                }
                return;
            } else if (node->left->left->node_type != ASTNodeType::AST_VARIABLE) {
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
                                                          value, element_name,
                                                          element_var->is_unsigned);

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

void VariableManager::assign_union_value(Variable& var, const std::string& union_type_name, const ASTNode* value_node) {
    // union型変数への代入を実行
    if (var.type != TYPE_UNION) {
        throw std::runtime_error("Variable is not a union type");
    }
    
    // 値の型に応じて検証と代入を実行
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列値
        std::string str_value = value_node->str_value;
        if (interpreter_->get_type_manager()->is_value_allowed_for_union(union_type_name, str_value)) {
            var.str_value = str_value;
            var.current_type = TYPE_STRING;
            var.is_assigned = true;
            if (debug_mode) {
                debug_print("UNION_DEBUG: Assigned string '%s' to union variable\n", str_value.c_str());
            }
        } else {
            throw std::runtime_error("String value '" + str_value + "' is not allowed for union type " + union_type_name);
        }
    } else if (value_node->node_type == ASTNodeType::AST_NUMBER) {
        // 数値
        int64_t int_value = value_node->int_value;
        if (interpreter_->get_type_manager()->is_value_allowed_for_union(union_type_name, int_value)) {
            var.value = int_value;
            var.current_type = TYPE_INT;
            var.is_assigned = true;
            if (debug_mode) {
                debug_print("UNION_DEBUG: Assigned integer %lld to union variable\n", int_value);
            }
        } else {
            throw std::runtime_error("Integer value " + std::to_string(int_value) + " is not allowed for union type " + union_type_name);
        }
    } else if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数参照の場合、その変数の型がカスタム型unionで許可されているかチェック
        std::string var_name = value_node->name;
        Variable* source_var = find_variable(var_name);
        if (source_var) {
            if (debug_mode) {
                debug_print("UNION_DEBUG: Checking variable reference '%s' (type_name='%s', current_type=%d)\n", 
                           var_name.c_str(), source_var->type_name.c_str(), static_cast<int>(source_var->current_type));
            }
            
            // 1. カスタム型（typedef型）のチェック
            if (!source_var->type_name.empty()) {
                if (interpreter_->get_type_manager()->is_custom_type_allowed_for_union(union_type_name, source_var->type_name)) {
                    // カスタム型として許可されている場合、値をコピー
                    var.value = source_var->value;
                    var.str_value = source_var->str_value;
                    var.current_type = source_var->current_type;
                    // var.type_name = source_var->type_name; // Union型変数の型名は変更しない
                    
                    // 構造体の場合は構造体データも完全にコピー
                    if (source_var->is_struct) {
                        var.is_struct = true;
                        var.struct_type_name = source_var->struct_type_name;
                        var.struct_members = source_var->struct_members;
                        var.current_type = TYPE_STRUCT;
                    }
                    
                    var.is_assigned = true;
                    if (debug_mode) {
                        debug_print("UNION_DEBUG: Assigned custom type '%s' to union variable (current_type=%d, str_value='%s')\n", 
                                   source_var->type_name.c_str(), static_cast<int>(source_var->current_type), source_var->str_value.c_str());
                    }
                    return;
                } else {
                    // カスタム型が許可されていない場合はエラー
                    throw std::runtime_error("Type mismatch: Custom type '" + source_var->type_name + 
                                           "' is not allowed for union type " + union_type_name);
                }
            }
            
            // 2. 構造体型のチェック
            if (source_var->is_struct && !source_var->struct_type_name.empty() && 
                interpreter_->get_type_manager()->is_custom_type_allowed_for_union(union_type_name, source_var->struct_type_name)) {
                // 構造体型として許可されている場合、構造体全体をコピー
                var.value = source_var->value;
                var.str_value = source_var->str_value;
                var.current_type = TYPE_STRUCT;
                // var.type_name = source_var->struct_type_name; // Union型変数の型名は変更しない
                var.is_struct = true;
                var.struct_type_name = source_var->struct_type_name;
                var.struct_members = source_var->struct_members;
                var.is_assigned = true;
                if (debug_mode) {
                    debug_print("UNION_DEBUG: Assigned struct type '%s' to union variable\n", 
                               source_var->struct_type_name.c_str());
                }
                return;
            }
            
            // 3. 配列型のチェック
            if (source_var->is_array) {
                // 配列の型名を構築 (例: int[3], bool[2])
                std::string array_type_name;
                TypeInfo base_type = static_cast<TypeInfo>(source_var->type - TYPE_ARRAY_BASE);
                
                // 基本型を文字列に変換
                std::string base_type_str;
                switch (base_type) {
                    case TYPE_INT: base_type_str = "int"; break;
                    case TYPE_LONG: base_type_str = "long"; break;
                    case TYPE_SHORT: base_type_str = "short"; break;
                    case TYPE_TINY: base_type_str = "tiny"; break;
                    case TYPE_BOOL: base_type_str = "bool"; break;
                    case TYPE_STRING: base_type_str = "string"; break;
                    case TYPE_CHAR: base_type_str = "char"; break;
                    default: base_type_str = "unknown"; break;
                }
                
                if (source_var->array_dimensions.size() > 0) {
                    array_type_name = base_type_str;
                    for (size_t dim : source_var->array_dimensions) {
                        array_type_name += "[" + std::to_string(dim) + "]";
                    }
                } else if (source_var->array_size > 0) {
                    array_type_name = base_type_str + "[" + std::to_string(source_var->array_size) + "]";
                }
                
                if (!array_type_name.empty() && 
                    interpreter_->get_type_manager()->is_array_type_allowed_for_union(union_type_name, array_type_name)) {
                    // 配列型として許可されている場合、配列全体をコピー
                    var.value = source_var->value;
                    var.str_value = source_var->str_value;
                    var.current_type = source_var->type;
                    // var.type_name = array_type_name; // Union型変数の型名は変更しない
                    var.is_array = true;
                    var.array_size = source_var->array_size;
                    var.array_dimensions = source_var->array_dimensions;
                    var.array_values = source_var->array_values;
                    var.array_strings = source_var->array_strings;
                    var.is_multidimensional = source_var->is_multidimensional;
                    var.multidim_array_values = source_var->multidim_array_values;
                    var.is_assigned = true;
                    if (debug_mode) {
                        debug_print("UNION_DEBUG: Assigned array type '%s' to union variable\n", 
                                   array_type_name.c_str());
                    }
                    return;
                }
            }
            
            // If not a custom type, fall through to expression evaluation
        }
        
        // Fall through to expression evaluation for non-custom-type variables
        try {
            int64_t int_value = interpreter_->expression_evaluator_->evaluate_expression(value_node);
            if (interpreter_->get_type_manager()->is_value_allowed_for_union(union_type_name, int_value)) {
                var.value = int_value;
                var.current_type = TYPE_INT;
                var.is_assigned = true;
                if (debug_mode) {
                    debug_print("UNION_DEBUG: Assigned evaluated integer %lld to union variable\n", int_value);
                }
            } else {
                throw std::runtime_error("Value " + std::to_string(int_value) + " is not allowed for union type " + union_type_name);
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to assign variable reference to union: " + std::string(e.what()));
        }
    } else {
        // 式の評価
        try {
            // 数値として評価
            int64_t int_value = interpreter_->expression_evaluator_->evaluate_expression(value_node);
            if (interpreter_->get_type_manager()->is_value_allowed_for_union(union_type_name, int_value)) {
                var.value = int_value;
                var.current_type = TYPE_INT;
                var.is_assigned = true;
                if (debug_mode) {
                    debug_print("UNION_DEBUG: Assigned evaluated integer %lld to union variable\n", int_value);
                }
            } else {
                throw std::runtime_error("Value " + std::to_string(int_value) + " is not allowed for union type " + union_type_name);
            }
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to assign value to union variable: " + std::string(e.what()));
        }
    }
}

// Priority 3: 変数ポインターから名前を検索
std::string VariableManager::find_variable_name(const Variable* target_var) {
    if (!target_var) return "";
    
    // 実装を簡素化：変数名の逆引きは複雑なので、
    // フォールバック戦略として空文字列を返す
    // これにより、呼び出し元は従来の方法にフォールバックする
    return "";
}

void VariableManager::handle_ternary_initialization(Variable& var, const ASTNode* ternary_node) {
    debug_msg(DebugMsgId::TERNARY_VAR_INIT_START);
    auto clamp_unsigned_ternary = [&](int64_t &value, const char *context) {
        if (!var.is_unsigned || value >= 0) {
            return;
        }
        std::string var_name = find_variable_name(&var);
        if (var_name.empty()) {
            var_name = std::string("<ternary>");
        }
        DEBUG_WARN(VARIABLE,
                   "Unsigned variable %s %s negative value (%lld); clamping to 0",
                   var_name.c_str(), context, static_cast<long long>(value));
        value = 0;
    };
    
    // 三項演算子の条件を評価
    int64_t condition = interpreter_->evaluate(ternary_node->left.get());
    debug_msg(DebugMsgId::TERNARY_VAR_CONDITION, condition);
    
    // 条件に基づいて選択される分岐を決定
    const ASTNode* selected_branch = condition ? ternary_node->right.get() : ternary_node->third.get();
    debug_msg(DebugMsgId::TERNARY_VAR_BRANCH_TYPE, static_cast<int>(selected_branch->node_type));
    
    // 選択された分岐の型に基づいて初期化
    if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの初期化
        debug_msg(DebugMsgId::TERNARY_VAR_STRING_SET, selected_branch->str_value.c_str());
        var.str_value = selected_branch->str_value;
        var.type = TYPE_STRING;
        var.is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_NUMBER) {
        // 数値リテラルの初期化
        debug_msg(DebugMsgId::TERNARY_VAR_NUMERIC_SET, selected_branch->int_value);
        int64_t numeric_value = selected_branch->int_value;
        clamp_unsigned_ternary(numeric_value, "initialized with ternary literal");
        var.value = numeric_value;
        var.is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 配列リテラルの初期化
        std::string temp_var_name = "__temp_ternary_var__";
        interpreter_->current_scope().variables[temp_var_name] = var;
        interpreter_->assign_array_literal(temp_var_name, selected_branch);
        var = interpreter_->current_scope().variables[temp_var_name];
        interpreter_->current_scope().variables.erase(temp_var_name);
        var.is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        // 構造体リテラルの初期化
        std::string temp_var_name = "__temp_ternary_var__";
        interpreter_->current_scope().variables[temp_var_name] = var;
        interpreter_->assign_struct_literal(temp_var_name, selected_branch);
        var = interpreter_->current_scope().variables[temp_var_name];
        interpreter_->current_scope().variables.erase(temp_var_name);
        var.is_assigned = true;
    } else {
        // その他（関数呼び出しなど）の場合は通常の評価
        try {
            int64_t value = interpreter_->evaluate(selected_branch);
            clamp_unsigned_ternary(value, "initialized with ternary expression");
            var.value = value;
            var.is_assigned = true;
        } catch (const ReturnException& ret) {
            if (ret.type == TYPE_STRING) {
                var.str_value = ret.str_value;
                var.type = TYPE_STRING;
            } else {
                int64_t numeric_value = ret.value;
                clamp_unsigned_ternary(numeric_value, "initialized with ternary return");
                var.value = numeric_value;
            }
            var.is_assigned = true;
        }
    }
}
