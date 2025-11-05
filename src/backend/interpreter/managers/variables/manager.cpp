#include "managers/variables/manager.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../common/generic_type_parser.h" // v0.11.0 Phase 1a
#include "../../../../common/type_helpers.h"
#include "../../core/interpreter.h"
#include "../../executors/assignments/const_check_helpers.h"
#include "../../services/debug_service.h"
#include "core/type_inference.h"
#include "evaluator/core/evaluator.h"
#include "managers/arrays/manager.h"
#include "managers/common/operations.h"
#include "managers/types/enums.h"
#include "managers/types/manager.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <numeric>
#include <utility>

namespace {

std::string getPrimitiveTypeNameForImpl(TypeInfo type) {
    return std::string(type_info_to_string(type));
}

void setNumericFields(Variable &var, long double quad_value) {
    var.quad_value = quad_value;
    var.double_value = static_cast<double>(quad_value);
    var.float_value = static_cast<float>(quad_value);
    var.value = static_cast<int64_t>(quad_value);
}

std::string trim(const std::string &str) {
    auto begin = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
                   return std::isspace(ch);
               }).base();
    if (begin >= end) {
        return "";
    }
    return std::string(begin, end);
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
        std::cerr << "DEBUG: Searching for temp variable: " << name
                  << std::endl;
        std::cerr << "DEBUG: Scope stack size: "
                  << interpreter_->scope_stack.size() << std::endl;
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
                std::cerr << "DEBUG: Found temp variable in local scope"
                          << std::endl;
            }

            // v0.10.0: 参照変数の場合は参照元の変数を辿る
            Variable *result = &var_it->second;
            if ((result->is_reference || result->is_rvalue_reference) &&
                !result->reference_target.empty()) {
                // 参照元の変数を再帰的に検索
                return find_variable(result->reference_target);
            }

            return result;
        }
    }

    // グローバルスコープから検索
    // std::cerr << "DEBUG: Searching in global scope" << std::endl;
    auto global_var_it = interpreter_->global_scope.variables.find(name);
    if (global_var_it != interpreter_->global_scope.variables.end()) {
        // v0.10.0: グローバルスコープでも参照を辿る
        Variable *result = &global_var_it->second;
        if ((result->is_reference || result->is_rvalue_reference) &&
            !result->reference_target.empty()) {
            return find_variable(result->reference_target);
        }
        // std::cerr << "DEBUG: Found " << name << " in global scope" <<
        // std::endl;
        return &global_var_it->second;
    }

    // static変数から検索
    Variable *static_var = interpreter_->find_static_variable(name);
    if (static_var) {
        return static_var;
    }

    // impl static変数から検索
    Variable *impl_static_var = interpreter_->find_impl_static_variable(name);
    if (impl_static_var) {
        return impl_static_var;
    }

    // std::cerr << "DEBUG: Variable " << name << " not found anywhere" <<
    // std::endl;

    return nullptr;
}

int VariableManager::resolve_array_size_expression(const std::string &size_expr,
                                                   const ASTNode *node) {
    std::string trimmed = trim(size_expr);
    if (trimmed.empty()) {
        error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                  node ? node->name.c_str() : "<anonymous>");
        throw std::runtime_error("Dynamic arrays are not supported yet");
    }

    try {
        size_t processed = 0;
        int value = std::stoi(trimmed, &processed, 0);
        if (processed == trimmed.size()) {
            if (value < 0) {
                throw std::runtime_error("Array size cannot be negative: " +
                                         trimmed);
            }
            return value;
        }
    } catch (const std::invalid_argument &) {
        // fall through to identifier resolution
    } catch (const std::out_of_range &) {
        throw std::runtime_error("Array size out of range: " + trimmed);
    }

    if (Variable *const_var = find_variable(trimmed)) {
        if (const_var->is_const && const_var->is_assigned) {
            switch (const_var->type) {
            case TYPE_TINY:
            case TYPE_SHORT:
            case TYPE_INT:
            case TYPE_LONG:
            case TYPE_CHAR:
            case TYPE_BOOL:
                if (const_var->value < 0) {
                    throw std::runtime_error("Array size cannot be negative: " +
                                             trimmed);
                }
                return static_cast<int>(const_var->value);
            default:
                break;
            }
        }
    }

    // enum参照 (EnumName::Member または EnumName.Member)
    EnumManager *enum_manager = interpreter_->get_enum_manager();
    if (enum_manager) {
        auto resolve_enum = [&](const std::string &separator,
                                int &out_value) -> bool {
            size_t pos = trimmed.find(separator);
            if (pos == std::string::npos) {
                return false;
            }
            std::string enum_name = trim(trimmed.substr(0, pos));
            std::string member_name =
                trim(trimmed.substr(pos + separator.size()));
            if (enum_name.empty() || member_name.empty()) {
                return false;
            }
            int64_t enum_value = 0;
            if (!enum_manager->get_enum_value(enum_name, member_name,
                                              enum_value)) {
                return false;
            }
            if (enum_value < 0) {
                throw std::runtime_error("Array size cannot be negative: " +
                                         trimmed);
            }
            out_value = static_cast<int>(enum_value);
            return true;
        };

        int enum_result = 0;
        if (resolve_enum("::", enum_result)) {
            return enum_result;
        }
        if (resolve_enum(".", enum_result)) {
            return enum_result;
        }
    }

    throw std::runtime_error("Array size must be a constant integer: " +
                             trimmed);
}

std::vector<ArrayDimension>
VariableManager::parse_array_dimensions(const std::string &array_part,
                                        const ASTNode *node) {
    std::vector<ArrayDimension> dimensions;
    std::string remaining = trim(array_part);

    while (!remaining.empty()) {
        if (remaining[0] != '[') {
            break;
        }

        size_t close_bracket = remaining.find(']');
        if (close_bracket == std::string::npos) {
            throw std::runtime_error("Invalid array syntax: missing ']' in " +
                                     remaining);
        }

        std::string size_str = remaining.substr(1, close_bracket - 1);
        std::string trimmed_size = trim(size_str);
        int dimension_size = resolve_array_size_expression(trimmed_size, node);

        dimensions.emplace_back(dimension_size, false, trimmed_size);

        if (close_bracket + 1 >= remaining.size()) {
            remaining.clear();
        } else {
            remaining = trim(remaining.substr(close_bracket + 1));
        }
    }

    return dimensions;
}

void VariableManager::initialize_array_from_dimensions(
    Variable &var, TypeInfo base_type,
    const std::vector<ArrayDimension> &dimensions) {
    var.array_type_info.base_type = base_type;
    var.array_type_info.dimensions = dimensions;
    var.array_dimensions.clear();
    var.is_array = true;

    if (dimensions.empty()) {
        var.array_size = 0;
        var.is_multidimensional = false;
        var.array_values.clear();
        var.array_float_values.clear();
        var.array_double_values.clear();
        var.array_quad_values.clear();
        var.array_strings.clear();
        var.multidim_array_values.clear();
        var.multidim_array_float_values.clear();
        var.multidim_array_double_values.clear();
        var.multidim_array_quad_values.clear();
        var.multidim_array_strings.clear();
        return;
    }

    int total_size = 1;
    for (const auto &dim : dimensions) {
        var.array_dimensions.push_back(dim.size);
        if (dim.size < 0) {
            error_msg(DebugMsgId::DYNAMIC_ARRAY_NOT_SUPPORTED,
                      var.type_name.c_str());
            throw std::runtime_error("Dynamic arrays are not supported yet");
        }
        total_size *= dim.size;
    }

    var.array_size = total_size;
    var.is_multidimensional = dimensions.size() > 1;

    auto clear_numeric_vectors = [&]() {
        var.array_values.clear();
        var.array_float_values.clear();
        var.array_double_values.clear();
        var.array_quad_values.clear();
        var.multidim_array_values.clear();
        var.multidim_array_float_values.clear();
        var.multidim_array_double_values.clear();
        var.multidim_array_quad_values.clear();
    };

    if (var.is_multidimensional) {
        if (base_type == TYPE_STRING) {
            var.multidim_array_strings.assign(total_size, "");
            clear_numeric_vectors();
        } else if (base_type == TYPE_FLOAT) {
            clear_numeric_vectors();
            var.multidim_array_float_values.assign(total_size, 0.0f);
        } else if (base_type == TYPE_DOUBLE) {
            clear_numeric_vectors();
            var.multidim_array_double_values.assign(total_size, 0.0);
        } else if (base_type == TYPE_QUAD) {
            clear_numeric_vectors();
            var.multidim_array_quad_values.assign(total_size, 0.0L);
        } else {
            clear_numeric_vectors();
            var.multidim_array_values.assign(total_size, 0);
        }
        var.array_strings.clear();
    } else {
        if (base_type == TYPE_STRING) {
            var.array_strings.assign(total_size, "");
            clear_numeric_vectors();
        } else if (base_type == TYPE_FLOAT) {
            clear_numeric_vectors();
            var.array_float_values.assign(total_size, 0.0f);
        } else if (base_type == TYPE_DOUBLE) {
            clear_numeric_vectors();
            var.array_double_values.assign(total_size, 0.0);
        } else if (base_type == TYPE_QUAD) {
            clear_numeric_vectors();
            var.array_quad_values.assign(total_size, 0.0L);
        } else {
            clear_numeric_vectors();
            var.array_values.assign(total_size, 0);
        }
        var.multidim_array_strings.clear();
    }
}

bool VariableManager::is_global_variable(const std::string &name) {
    // グローバルスコープに存在するかチェック
    auto global_var_it = interpreter_->global_scope.variables.find(name);
    return (global_var_it != interpreter_->global_scope.variables.end());
}

void VariableManager::assign_interface_view(
    const std::string &dest_name, Variable interface_var,
    const Variable &source_var, const std::string &source_var_name) {
    std::string source_type_name = resolve_interface_source_type(source_var);

    debug_print("ASSIGN_IFACE: About to call interface_impl_exists\n");

    if (!interface_impl_exists(interface_var.interface_name,
                               source_type_name)) {
        throw std::runtime_error("No impl found for interface '" +
                                 interface_var.interface_name +
                                 "' with type '" + source_type_name + "'");
    }

    debug_print(
        "ASSIGN_IFACE: interface_impl_exists returned true, continuing\n");

    if (!source_var_name.empty()) {
        debug_print("ASSIGN_IFACE: About to call "
                    "sync_struct_members_from_direct_access\n");
        interpreter_->sync_struct_members_from_direct_access(source_var_name);
        debug_print(
            "ASSIGN_IFACE: sync_struct_members_from_direct_access returned\n");
    }

    debug_print("ASSIGN_IFACE: Proceeding with variable assignment\n");

    Variable assigned_var = std::move(interface_var);
    assigned_var.struct_type_name = source_type_name;
    assigned_var.is_assigned = true;

    if (source_var.is_struct || (!source_var.struct_members.empty() &&
                                 TypeHelpers::isInterface(source_var.type))) {
        assigned_var.is_struct = true;
        assigned_var.struct_members.clear();
        for (const auto &member_pair : source_var.struct_members) {
            const std::string &member_name = member_pair.first;
            const Variable &source_member = member_pair.second;

            Variable dest_member = source_member;
            if (source_member.is_multidimensional) {
                dest_member.is_multidimensional = true;
                dest_member.array_dimensions = source_member.array_dimensions;
                dest_member.multidim_array_values =
                    source_member.multidim_array_values;
                dest_member.multidim_array_strings =
                    source_member.multidim_array_strings;
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
            TypeInfo base_type =
                static_cast<TypeInfo>(source_var.type - TYPE_ARRAY_BASE);
            assigned_var.struct_type_name =
                getPrimitiveTypeNameForImpl(base_type) + "[]";
        }
    } else {
        assigned_var.is_struct = false;
        assigned_var.type = source_var.type;
        assigned_var.value = source_var.value;
        assigned_var.str_value = source_var.str_value;

        if (!source_var.struct_type_name.empty()) {
            assigned_var.struct_type_name = source_var.struct_type_name;
        } else {
            assigned_var.struct_type_name =
                getPrimitiveTypeNameForImpl(source_var.type);
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
            std::string source_member_name =
                source_var_name + "." + member_name;
            if (Variable *source_member_var =
                    find_variable(source_member_name)) {
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
                std::string dest_element_name =
                    dest_member_name + "[" + std::to_string(i) + "]";
                Variable element_var;
                element_var.is_assigned = true;

                bool copied = false;
                if (!source_var_name.empty()) {
                    std::string source_element_name = source_var_name + "." +
                                                      member_name + "[" +
                                                      std::to_string(i) + "]";
                    if (Variable *source_element_var =
                            find_variable(source_element_name)) {
                        element_var = *source_element_var;
                        copied = true;
                    }
                }

                if (!copied) {
                    if (TypeHelpers::isString(member_var.type)) {
                        element_var.type = TYPE_STRING;
                        if (i <
                            static_cast<int>(member_var.array_strings.size())) {
                            element_var.str_value = member_var.array_strings[i];
                        } else if (i < static_cast<int>(
                                           member_var.multidim_array_strings
                                               .size())) {
                            element_var.str_value =
                                member_var.multidim_array_strings[i];
                        } else {
                            element_var.str_value = "";
                        }
                    } else {
                        element_var.type = member_var.type;
                        int64_t value = 0;
                        if (member_var.is_multidimensional &&
                            i < static_cast<int>(
                                    member_var.multidim_array_values.size())) {
                            value = member_var.multidim_array_values[i];
                        } else if (i < static_cast<int>(
                                           member_var.array_values.size())) {
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

bool VariableManager::interface_impl_exists(
    const std::string &interface_name,
    const std::string &struct_type_name) const {
    if (interpreter_->debug_mode) {
        debug_print("IMPL_SEARCH_BEFORE: About to call get_impl_definitions(), "
                    "interpreter=%p\n",
                    (void *)interpreter_);
    }

    const auto &impls = interpreter_->get_impl_definitions();

    if (interpreter_->debug_mode) {
        debug_print("IMPL_SEARCH: Looking for interface='%s', struct_type='%s' "
                    "(total impls=%zu, addr=%p, interpreter=%p)\n",
                    interface_name.c_str(), struct_type_name.c_str(),
                    impls.size(), (void *)&impls, (void *)interpreter_);
        debug_print("IMPL_SEARCH: About to iterate over %zu impls\n",
                    impls.size());
    }

    size_t idx = 0;
    for (const auto &impl_def : impls) {
        if (interpreter_->debug_mode) {
            debug_print(
                "IMPL_SEARCH: Iteration %zu, about to access impl_def fields\n",
                idx);
        }

        try {
            std::string iface = impl_def.interface_name;
            std::string sname = impl_def.struct_name;

            if (interpreter_->debug_mode) {
                debug_print("IMPL_SEARCH: [%zu] interface='%s', struct='%s'\n",
                            idx, iface.c_str(), sname.c_str());
            }

            if (iface == interface_name && sname == struct_type_name) {
                if (interpreter_->debug_mode) {
                    debug_print("IMPL_SEARCH: MATCH FOUND at index %zu!\n",
                                idx);
                }
                return true;
            }
        } catch (...) {
            debug_print("IMPL_SEARCH: EXCEPTION at index %zu!\n", idx);
            throw;
        }
        idx++;
    }

    if (interpreter_->debug_mode) {
        debug_print("IMPL_SEARCH: NO MATCH FOUND\n");
    }
    return false;
}

std::string VariableManager::resolve_interface_source_type(
    const Variable &source_var) const {
    if (!source_var.struct_type_name.empty()) {
        return source_var.struct_type_name;
    }

    if (!source_var.type_name.empty() && source_var.type != TYPE_UNION) {
        const std::string &alias_name = source_var.type_name;
        std::string resolved_type =
            interpreter_->type_manager_->resolve_typedef(alias_name);

        bool is_alias = resolved_type != alias_name;
        bool is_array_alias = alias_name.find('[') != std::string::npos;

        if (is_alias || is_array_alias) {
            return alias_name;
        }
    }

    if (TypeHelpers::isInterface(source_var.type) &&
        !source_var.implementing_struct.empty()) {
        return source_var.implementing_struct;
    }

    if (source_var.is_struct) {
        return source_var.struct_type_name;
    }

    if (source_var.type >= TYPE_ARRAY_BASE || source_var.is_array) {
        TypeInfo base_type = TYPE_UNKNOWN;
        if (source_var.type >= TYPE_ARRAY_BASE) {
            base_type =
                static_cast<TypeInfo>(source_var.type - TYPE_ARRAY_BASE);
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

    // 参照型変数の場合は、register時に宣言せず、execute時に処理する
    if (node->is_reference) {
        if (interpreter_->is_debug_mode()) {
            std::cerr
                << "[VAR_MANAGER] Skipping reference variable registration: "
                << node->name << " (will be created during execution)"
                << std::endl;
        }
        return;
    }

    // グローバル変数の重複宣言チェック
    if (interpreter_->global_scope.variables.find(node->name) !=
        interpreter_->global_scope.variables.end()) {
        error_msg(DebugMsgId::VAR_REDECLARE_ERROR, node->name.c_str());
        throw std::runtime_error("Variable redeclaration error");
    }

    Variable var;

    auto assign_custom_type_metadata = [&](Variable &target) {
        std::string declared = !node->original_type_name.empty()
                                   ? node->original_type_name
                                   : node->type_name;
        if (declared.empty()) {
            return;
        }

        std::string resolved =
            interpreter_->type_manager_->resolve_typedef(declared);
        std::string resolved_base = resolved;
        size_t bracket_pos = resolved_base.find('[');
        if (bracket_pos != std::string::npos) {
            resolved_base = resolved_base.substr(0, bracket_pos);
        }

        bool is_alias = (resolved != declared);
        bool is_struct_type =
            node->type_info == TYPE_STRUCT ||
            (!resolved_base.empty() &&
             interpreter_->find_struct_definition(resolved_base) != nullptr);
        bool is_union_alias =
            interpreter_->get_type_manager()->is_union_type(declared);

        if (!is_alias && !is_struct_type && !is_union_alias) {
            return;
        }

        std::string stored_name = is_alias ? declared : resolved_base;

        target.struct_type_name = stored_name;
        if (is_alias) {
            target.type_name = declared;
        } else if (is_struct_type) {
            target.type_name = stored_name;
        }
    };

    // typedef解決
    if (node->type_info == TYPE_UNKNOWN && !node->type_name.empty()) {
        std::string resolved_type =
            interpreter_->type_manager_->resolve_typedef(node->type_name);

        // 配列typedefの場合
        if (resolved_type.find("[") != std::string::npos) {
            std::string base =
                trim(resolved_type.substr(0, resolved_type.find("[")));
            std::string array_part =
                resolved_type.substr(resolved_type.find("["));

            TypeInfo base_type =
                interpreter_->type_manager_->string_to_type_info(base);
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);

            auto dimensions = parse_array_dimensions(array_part, node);
            initialize_array_from_dimensions(var, base_type, dimensions);

            var.current_type = var.type;

        } else {
            var.type = interpreter_->type_manager_->string_to_type_info(
                node->type_name);
            var.current_type = var.type;
        }
    } else if (!node->type_name.empty() &&
               node->type_name.find("[") != std::string::npos) {
        // 直接配列宣言の場合（例：int[5] global_array）
        std::string base =
            trim(node->type_name.substr(0, node->type_name.find("[")));
        std::string array_part =
            node->type_name.substr(node->type_name.find("["));

        TypeInfo base_type =
            interpreter_->type_manager_->string_to_type_info(base);
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);

        auto dimensions = parse_array_dimensions(array_part, node);
        initialize_array_from_dimensions(var, base_type, dimensions);

        var.current_type = var.type;

    } else {
        var.type = node->type_info;
        var.current_type = var.type;
    }

    if (node->is_pointer) {
        var.is_pointer = true;
        var.pointer_depth = node->pointer_depth;
        var.pointer_base_type_name = node->pointer_base_type_name;
        var.pointer_base_type = node->pointer_base_type;
        var.is_pointer_const = node->is_pointer_const_qualifier;
        if (var.type != TYPE_POINTER) {
            var.type = TYPE_POINTER;
        }
        if (var.type_name.empty()) {
            var.type_name = node->type_name;
        }
    }

    var.is_reference = node->is_reference;
    var.is_unsigned = node->is_unsigned;

    // ポインタ型の場合、is_constは変数自体のconstではなく、指し先のconstを意味するため無視
    // ポインタの場合はis_pointer_constとis_pointee_constで制御
    if (!node->is_pointer) {
        var.is_const = node->is_const;
    }
    var.is_assigned = false;

    if (var.current_type == TYPE_UNKNOWN) {
        var.current_type = var.type;
    }

    assign_custom_type_metadata(var);

    interpreter_->global_scope.variables[node->name] = var;
}

void VariableManager::declare_local_variable(const ASTNode *node) {
    // ローカル変数の宣言
    Variable var;
    var.is_array = false;
    var.array_size = 0;

    auto assign_custom_type_metadata = [&](Variable &target) {
        std::string declared = !node->original_type_name.empty()
                                   ? node->original_type_name
                                   : node->type_name;
        if (declared.empty()) {
            return;
        }

        std::string resolved =
            interpreter_->type_manager_->resolve_typedef(declared);
        std::string resolved_base = resolved;
        size_t bracket_pos = resolved_base.find('[');
        if (bracket_pos != std::string::npos) {
            resolved_base = resolved_base.substr(0, bracket_pos);
        }

        bool is_alias = (resolved != declared);
        bool is_struct_type =
            node->type_info == TYPE_STRUCT ||
            (!resolved_base.empty() &&
             interpreter_->find_struct_definition(resolved_base) != nullptr);
        bool is_union_alias =
            interpreter_->get_type_manager()->is_union_type(declared);

        if (!is_alias && !is_struct_type && !is_union_alias) {
            return;
        }

        std::string stored_name = is_alias ? declared : resolved_base;

        target.struct_type_name = stored_name;
        if (is_alias) {
            target.type_name = declared;
        } else if (is_struct_type) {
            target.type_name = stored_name;
        }
    };

    // ポインタ情報は最初に設定（型解決より前に処理）
    if (node->is_pointer) {
        var.is_pointer = true;
        var.pointer_depth = node->pointer_depth;
        var.pointer_base_type_name = node->pointer_base_type_name;
        var.pointer_base_type = node->pointer_base_type;
        var.is_pointer_const = node->is_pointer_const_qualifier;
        var.type = TYPE_POINTER;
        if (var.type_name.empty()) {
            var.type_name = node->type_name;
        }
    }

    // typedef解決
    if (node->type_info == TYPE_UNKNOWN && !node->type_name.empty()) {
        std::string resolved_type =
            interpreter_->type_manager_->resolve_typedef(node->type_name);

        debug_msg(DebugMsgId::VAR_MANAGER_TYPE_RESOLVED, node->name.c_str(),
                  node->type_name.c_str(), resolved_type.c_str());

        // 配列typedefの場合
        if (resolved_type.find("[") != std::string::npos) {
            std::string base =
                trim(resolved_type.substr(0, resolved_type.find("[")));
            std::string array_part =
                resolved_type.substr(resolved_type.find("["));

            TypeInfo base_type =
                interpreter_->type_manager_->string_to_type_info(base);
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);

            auto dimensions = parse_array_dimensions(array_part, node);
            initialize_array_from_dimensions(var, base_type, dimensions);

            var.current_type = var.type;
        } else {
            var.type =
                interpreter_->type_manager_->string_to_type_info(resolved_type);
            // カスタム型名を保持（例：MyString -> string に解決されても
            // MyString を記録）
            if (resolved_type != node->type_name) {
                var.type_name = node->type_name;
                // current_typeも設定（union代入で必要）
                var.current_type = var.type;
            }
        }
    } else if (!node->type_name.empty() &&
               node->type_name.find("[") != std::string::npos) {
        // 直接配列宣言の場合（例：int[5] local_array）
        std::string base =
            trim(node->type_name.substr(0, node->type_name.find("[")));
        std::string array_part =
            node->type_name.substr(node->type_name.find("["));

        TypeInfo base_type =
            interpreter_->type_manager_->string_to_type_info(base);
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);

        auto dimensions = parse_array_dimensions(array_part, node);
        initialize_array_from_dimensions(var, base_type, dimensions);
        var.current_type = var.type;
    } else if (!node->is_pointer) {
        // ポインタでない場合のみ型を設定
        var.type = node->type_info != TYPE_VOID ? node->type_info : TYPE_INT;
        var.current_type = var.type;
    }

    var.is_unsigned = node->is_unsigned;
    // ポインタ型の場合、is_constは変数自体のconstではなく、指し先のconstを意味するため無視
    // ポインタの場合はis_pointer_constとis_pointee_constで制御
    if (!node->is_pointer) {
        var.is_const = node->is_const;
    }
    var.is_assigned = false;

    // 初期値が指定されている場合は評価して設定
    if (node->children.size() > 0 && node->children[0]) {
        int64_t value = interpreter_->evaluate(node->children[0].get());
        if (var.is_unsigned && value < 0) {
            DEBUG_WARN(VARIABLE,
                       "Unsigned variable %s initialized with negative literal "
                       "(%lld); clamping to 0",
                       node->name.c_str(), static_cast<long long>(value));
            value = 0;
        }
        var.value = value;
        var.is_assigned = true;

        // 型範囲チェック（ポインタ型の場合はスキップ）
        if (!var.is_pointer) {
            interpreter_->type_manager_->check_type_range(
                var.type, value, node->name, var.is_unsigned);
        }
    }

    if (var.current_type == TYPE_UNKNOWN) {
        var.current_type = var.type;
    }

    assign_custom_type_metadata(var);

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

    // Union型変数への代入の特別処理
    Variable *var = interpreter_->find_variable(name);
    if (interpreter_->is_debug_mode()) {
        std::cerr << "[DEBUG_ASSIGN_VAR] assign_variable: name=" << name
                  << ", var=" << (var ? "found" : "null") << ", type="
                  << (var ? std::to_string(static_cast<int>(var->type)) : "N/A")
                  << ", TYPE_UNION=" << static_cast<int>(TYPE_UNION)
                  << std::endl;
    }
    if (var && var->type == TYPE_UNION) {
        if (interpreter_->is_debug_mode()) {
            std::cerr << "[UNION_ASSIGN_DEBUG] assign_variable called for "
                         "union variable: "
                      << name << std::endl;
        }
        // TypedValueから適切なASTノードを構築して assign_union_value を呼び出す
        // 数値の場合
        if (typed_value.is_numeric()) {
            auto temp_node = std::make_unique<ASTNode>(ASTNodeType::AST_NUMBER);
            temp_node->int_value = typed_value.as_numeric();
            assign_union_value(*var, var->type_name, temp_node.get());
            if (interpreter_->is_debug_mode()) {
                std::cerr
                    << "[UNION_ASSIGN_DEBUG] After assign_union_value: value="
                    << var->value
                    << ", current_type=" << static_cast<int>(var->current_type)
                    << std::endl;
            }
            return;
        }
        // 文字列の場合
        if (typed_value.is_string()) {
            auto temp_node =
                std::make_unique<ASTNode>(ASTNodeType::AST_STRING_LITERAL);
            temp_node->str_value = typed_value.string_value;
            assign_union_value(*var, var->type_name, temp_node.get());
            return;
        }
        // その他の場合は通常処理に任せる
    }

    // 参照変数への代入の場合、参照先変数に代入
    if (var && var->is_reference) {
        Variable *target_var = reinterpret_cast<Variable *>(var->value);
        if (!target_var) {
            throw std::runtime_error("Invalid reference variable: " + name);
        }

        if (interpreter_->is_debug_mode()) {
            std::cerr << "[VAR_MANAGER] Reference assignment: " << name
                      << " -> target variable (value before: "
                      << target_var->value << ")" << std::endl;
        }

        // 参照先変数に代入（再帰呼び出し、ただし参照先の名前は不明なので直接代入）
        if (typed_value.is_numeric()) {
            int64_t numeric_value = typed_value.as_numeric();
            target_var->value = numeric_value;
            target_var->is_assigned = true;
            if (target_var->type == TYPE_FLOAT ||
                target_var->type == TYPE_DOUBLE ||
                target_var->type == TYPE_QUAD) {
                double double_val = typed_value.as_double();
                if (target_var->type == TYPE_FLOAT) {
                    target_var->float_value = static_cast<float>(double_val);
                } else if (target_var->type == TYPE_DOUBLE) {
                    target_var->double_value = double_val;
                } else {
                    target_var->quad_value =
                        static_cast<long double>(double_val);
                }
            }
        } else if (typed_value.is_string()) {
            target_var->str_value = typed_value.string_value;
            // value フィールドに文字列のコピーのポインタを保存（generic
            // 型で使用される）
            target_var->value = reinterpret_cast<int64_t>(
                strdup(target_var->str_value.c_str()));
            target_var->is_assigned = true;
        } else if (typed_value.is_struct()) {
            if (typed_value.struct_data) {
                bool was_const = target_var->is_const;
                bool was_unsigned = target_var->is_unsigned;
                *target_var = *typed_value.struct_data;
                target_var->is_const = was_const;
                target_var->is_unsigned = was_unsigned;
                target_var->is_assigned = true;
            }
        }
        return;
    }

    // デフォルトメンバーへの暗黙的代入
    // 構造体変数に基本型の値を代入する場合、defaultメンバーがあれば
    // そのメンバーへの代入として処理
    if (var && var->is_struct && !typed_value.is_struct()) {
        if (interpreter_->is_debug_mode()) {
            std::cerr << "[DEFAULT_MEMBER_CHECK] Variable " << name
                      << " is struct, checking for default member" << std::endl;
        }
        auto it = interpreter_->struct_definitions_.find(var->struct_type_name);
        if (it != interpreter_->struct_definitions_.end()) {
            const StructDefinition &struct_def = it->second;
            if (interpreter_->is_debug_mode()) {
                std::cerr << "[DEFAULT_MEMBER_CHECK] Found struct definition: "
                          << var->struct_type_name << ", has_default_member="
                          << struct_def.has_default_member << std::endl;
            }

            if (struct_def.has_default_member) {
                // defaultメンバーの型を取得
                const StructMember *default_member =
                    struct_def.find_member(struct_def.default_member_name);

                if (default_member) {
                    // 型が一致するか確認
                    TypeInfo rhs_type = typed_value.numeric_type;
                    if (rhs_type == TYPE_UNKNOWN) {
                        if (typed_value.is_string()) {
                            rhs_type = TYPE_STRING;
                        } else if (typed_value.type.type_info != TYPE_UNKNOWN) {
                            rhs_type = typed_value.type.type_info;
                        }
                    }

                    if (interpreter_->is_debug_mode()) {
                        std::cerr << "[DEFAULT_MEMBER] Type check: "
                                     "default_member->type="
                                  << static_cast<int>(default_member->type)
                                  << ", rhs_type=" << static_cast<int>(rhs_type)
                                  << ", is_string=" << typed_value.is_string()
                                  << ", string_value='"
                                  << typed_value.string_value << "'"
                                  << std::endl;
                    }

                    // 型が一致すればdefaultメンバーへの代入として処理
                    // bool型は数値として扱われるため、intからの変換を許可
                    bool type_compatible =
                        (default_member->type == rhs_type) ||
                        (TypeHelpers::isNumeric(default_member->type) &&
                         TypeHelpers::isNumeric(rhs_type)) ||
                        (default_member->type == TYPE_BOOL &&
                         TypeHelpers::isNumeric(rhs_type));

                    if (type_compatible) {
                        if (interpreter_->is_debug_mode()) {
                            std::cerr
                                << "[DEFAULT_MEMBER] Implicit assignment to "
                                << "default member: " << name << "."
                                << struct_def.default_member_name << std::endl;
                        }

                        interpreter_->assign_struct_member(
                            name, struct_def.default_member_name, typed_value);
                        return;
                    } else {
                        if (interpreter_->is_debug_mode()) {
                            std::cerr << "[DEFAULT_MEMBER] Type mismatch, not "
                                         "assigning"
                                      << std::endl;
                        }
                    }
                }
            }
        }
    }

    if (interpreter_->is_debug_mode() && name == "ptr") {
        std::cerr << "[VAR_MANAGER] assign_variable called for ptr:"
                  << std::endl;
        std::cerr << "  type_hint=" << static_cast<int>(type_hint)
                  << " (TYPE_POINTER=" << static_cast<int>(TYPE_POINTER) << ")"
                  << std::endl;
        std::cerr << "  typed_value.value=" << typed_value.value << " (0x"
                  << std::hex << typed_value.value << std::dec << ")"
                  << std::endl;
        std::cerr << "  typed_value.numeric_type="
                  << static_cast<int>(typed_value.numeric_type) << std::endl;
    }

    auto apply_assignment = [&](Variable &target, bool allow_type_override) {
        // constポインタ自体への再代入チェック (T* const ptr の場合、ptr = ...
        // は不可)
        if (target.is_assigned) {
            AssignmentHelpers::check_const_pointer_reassignment(&target);
        }

        auto clamp_unsigned = [&](int64_t &numeric_value) {
            if (!target.is_unsigned || numeric_value >= 0) {
                return;
            }
            DEBUG_WARN(VARIABLE,
                       "Unsigned variable %s received negative assignment "
                       "(%lld); clamping to 0",
                       name.c_str(), static_cast<long long>(numeric_value));
            numeric_value = 0;
        };

        // 関数ポインタの代入処理
        if (typed_value.is_function_pointer) {
            target.value = typed_value.value;
            target.is_function_pointer = true;
            target.is_assigned = true;

            // function_pointersマップに登録
            FunctionPointer func_ptr(
                typed_value.function_pointer_node,
                typed_value.function_pointer_name,
                typed_value.function_pointer_node->type_info);
            interpreter_->current_scope().function_pointers[name] = func_ptr;

            if (interpreter_->is_debug_mode()) {
                std::cerr << "[VAR_MANAGER] Assigned function pointer: " << name
                          << " -> " << typed_value.function_pointer_name
                          << std::endl;
            }
            return;
        }

        if (typed_value.is_struct()) {
            if (interpreter_->debug_mode) {
                std::cerr << "[ASSIGN_VAR_DEBUG] Assigning struct to: " << name
                          << ", struct_data="
                          << (typed_value.struct_data ? "exists" : "null")
                          << std::endl;
            }
            if (typed_value.struct_data) {
                bool was_const = target.is_const;
                bool was_unsigned = target.is_unsigned;
                if (interpreter_->debug_mode) {
                    std::cerr
                        << "[ASSIGN_VAR_DEBUG] Before assignment: target.type="
                        << static_cast<int>(target.type) << std::endl;
                    std::cerr << "[ASSIGN_VAR_DEBUG] struct_data.type="
                              << static_cast<int>(typed_value.struct_data->type)
                              << std::endl;
                }
                target = *typed_value.struct_data;
                target.is_const = was_const;
                target.is_unsigned = was_unsigned;
                target.is_assigned = true;
                if (interpreter_->debug_mode) {
                    std::cerr
                        << "[ASSIGN_VAR_DEBUG] After assignment: target.type="
                        << static_cast<int>(target.type) << std::endl;
                }
                // 構造体戻り値や代入で生成された最新のメンバー状態を
                // ダイレクトアクセス変数にも反映させる
                interpreter_->sync_direct_access_from_struct_value(name,
                                                                   target);
            }
            return;
        }

        if (typed_value.is_string()) {
            // デバッグ出力: 文字列代入時の type_hint を確認
            if (interpreter_->is_debug_mode()) {
                std::cerr << "[VAR_MANAGER] String assignment: type_hint="
                          << static_cast<int>(type_hint)
                          << " (TYPE_POINTER=" << static_cast<int>(TYPE_POINTER)
                          << "), str=\"" << typed_value.string_value << "\""
                          << std::endl;
            }

            // type_hintがTYPE_POINTERの場合、char*パラメータとして扱う
            // (string -> char* の暗黙的な変換)
            if (type_hint == TYPE_POINTER) {
                // char*ポインタパラメータ: 単純にTYPE_STRING変数として扱う
                // is_pointerをfalseに設定することで、array.cppの
                // ポインタ経由アクセス処理を回避し、直接文字列アクセスが可能
                target.type = TYPE_STRING;
                target.str_value = typed_value.string_value;
                target.value =
                    reinterpret_cast<int64_t>(strdup(target.str_value.c_str()));
                target.is_assigned = true;

                // 明示的にis_pointerをfalseに設定（ポインタ経由アクセスを回避）
                target.is_pointer = false;
                target.pointer_depth = 0;

                target.float_value = 0.0f;
                target.double_value = 0.0;
                target.quad_value = 0.0L;
                target.big_value = 0;

                if (interpreter_->is_debug_mode()) {
                    std::cerr
                        << "[VAR_MANAGER] String to char* parameter: "
                        << "converted to TYPE_STRING for array access, str=\""
                        << typed_value.string_value << "\"" << std::endl;
                }
                return;
            }

            if ((allow_type_override || target.type == TYPE_UNKNOWN ||
                 TypeHelpers::isString(target.type)) &&
                target.type != TYPE_UNION) {
                target.type = TYPE_STRING;
            }
            target.str_value = typed_value.string_value;
            // value フィールドに文字列のコピーのポインタを保存（generic
            // 型で使用される）
            target.value =
                reinterpret_cast<int64_t>(strdup(target.str_value.c_str()));
            target.float_value = 0.0f;
            target.double_value = 0.0;
            target.quad_value = 0.0L;
            target.big_value = 0;
            target.is_assigned = true;
            if (target.type == TYPE_UNION) {
                target.current_type = TYPE_STRING;
            }
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
            resolved_type =
                (!allow_type_override && target.type != TYPE_UNKNOWN)
                    ? target.type
                    : TYPE_INT;
        }

        if (interpreter_->is_debug_mode() &&
            (type_hint == TYPE_POINTER || TypeHelpers::isPointer(target.type) ||
             typed_value.numeric_type == TYPE_POINTER)) {
            std::cerr
                << "[VAR_MANAGER] Pointer assignment detected for variable:"
                << std::endl;
            std::cerr << "  type_hint=" << static_cast<int>(type_hint)
                      << std::endl;
            std::cerr << "  target.type=" << static_cast<int>(target.type)
                      << std::endl;
            std::cerr << "  resolved_type=" << static_cast<int>(resolved_type)
                      << std::endl;
            std::cerr << "  typed_value.numeric_type="
                      << static_cast<int>(typed_value.numeric_type)
                      << std::endl;
            std::cerr << "  TYPE_POINTER=" << static_cast<int>(TYPE_POINTER)
                      << std::endl;
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
                debug_print(
                    "ASSIGN_DEBUG: name=%s target_type=%d resolved_type=%d "
                    "numeric_value=%lld allow_override=%d\n",
                    name.c_str(), static_cast<int>(target.type),
                    static_cast<int>(resolved_type),
                    static_cast<long long>(numeric_value),
                    allow_type_override ? 1 : 0);
            }
            TypeInfo range_check_type = resolved_type;
            if (target.type != TYPE_UNKNOWN && target.type != TYPE_UNION &&
                target.type != TYPE_INTERFACE && target.type != TYPE_STRUCT &&
                target.type < TYPE_ARRAY_BASE) {
                range_check_type = target.type;
            }

            // ポインタ型は精度損失を避けるため、long
            // double経由のキャストをスキップ
            // typed_value.numeric_typeもチェック（評価時に設定される型情報）
            // target.is_pointerもチェック（unsigned int* のような型の場合）
            if (resolved_type == TYPE_POINTER ||
                typed_value.numeric_type == TYPE_POINTER ||
                TypeHelpers::isPointer(target.type) || target.is_pointer) {
                target.value = numeric_value;
                target.float_value = 0.0f;
                target.double_value = 0.0;
                target.quad_value = 0.0L;
                if (interpreter_->is_debug_mode()) {
                    std::cerr << "[VAR_MANAGER] Assigned pointer value to "
                              << name << ": " << numeric_value << " (0x"
                              << std::hex << numeric_value << std::dec << ")"
                              << std::endl;
                }
            } else {
                interpreter_->type_manager_->check_type_range(
                    range_check_type, numeric_value, name, target.is_unsigned);
                setNumericFields(target,
                                 static_cast<long double>(numeric_value));
            }
        }

        target.is_assigned = true;
    };

    Variable *existing_var = find_variable(name);
    if (!existing_var) {
        Variable new_var;
        apply_assignment(new_var, true);
        new_var.is_const = is_const;
        current_scope().variables[name] = new_var;
        return;
    }

    if (existing_var->is_const && existing_var->is_assigned) {
        std::fprintf(stderr, "Cannot reassign const variable: %s\n",
                     name.c_str());
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, name.c_str());
        std::exit(1);
    }

    apply_assignment(*existing_var, false);
    if (is_const) {
        existing_var->is_const = true;
    }
}

void VariableManager::assign_function_parameter(const std::string &name,
                                                int64_t value, TypeInfo type,
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
    if (interpreter_->is_debug_mode()) {
        std::cerr << "[VAR_MANAGER] assign_function_parameter: name=" << name
                  << ", type=" << static_cast<int>(type)
                  << " (TYPE_POINTER=" << static_cast<int>(TYPE_POINTER) << ")"
                  << ", is_string=" << value.is_string() << ", str=\""
                  << (value.is_string() ? value.string_value : "N/A") << "\""
                  << std::endl;
    }

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

    // 関数ポインタ型のパラメータの場合、function_pointersマップにも登録
    if (type == TYPE_POINTER) {
        // 全てのスコープのfunction_pointersから関数ポインタ値に一致するものを探す
        bool found = false;

        // まず現在のスコープを検索
        for (const auto &pair : scope.function_pointers) {
            Variable *source_var = interpreter_->find_variable(pair.first);
            if (source_var && source_var->value == value.value) {
                scope.function_pointers[name] = pair.second;
                if (interpreter_->debug_mode) {
                    std::cerr << "[VAR_MANAGER] Registered function pointer "
                                 "parameter (local): "
                              << name << " -> " << pair.second.function_name
                              << std::endl;
                }
                found = true;
                break;
            }
        }

        // グローバルスコープも検索
        if (!found) {
            auto &global_func_ptrs =
                interpreter_->get_global_scope().function_pointers;
            for (const auto &pair : global_func_ptrs) {
                Variable *source_var = interpreter_->find_variable(pair.first);
                if (source_var && source_var->value == value.value) {
                    scope.function_pointers[name] = pair.second;
                    if (interpreter_->debug_mode) {
                        std::cerr << "[VAR_MANAGER] Registered function "
                                     "pointer parameter (global): "
                                  << name << " -> " << pair.second.function_name
                                  << std::endl;
                    }
                    found = true;
                    break;
                }
            }
        }

        // それでも見つからない場合、親スコープから呼び出し元の変数を探す
        if (!found && interpreter_->scope_stack.size() >= 2) {
            // 1つ前のスコープ（呼び出し元）を見る
            auto &parent_scope =
                interpreter_->scope_stack[interpreter_->scope_stack.size() - 2];
            for (const auto &pair : parent_scope.function_pointers) {
                Variable *source_var = nullptr;
                // 親スコープの変数を探す
                auto var_it = parent_scope.variables.find(pair.first);
                if (var_it != parent_scope.variables.end()) {
                    source_var = &(var_it->second);
                }
                if (source_var && source_var->value == value.value) {
                    scope.function_pointers[name] = pair.second;
                    if (interpreter_->debug_mode) {
                        std::cerr << "[VAR_MANAGER] Registered function "
                                     "pointer parameter (parent): "
                                  << name << " -> " << pair.second.function_name
                                  << std::endl;
                    }
                    found = true;
                    break;
                }
            }
        }
    }

    assign_variable(name, value, type, false);

    // assign_variable後の追加設定
    if (auto updated_iter = scope.variables.find(name);
        updated_iter != scope.variables.end()) {
        updated_iter->second.is_unsigned = is_unsigned;

        // char*パラメータにstring値が渡された場合、is_pointerをfalseに確実に設定
        // (assign_variable内での設定を確実にするための二重チェック)
        if (type == TYPE_POINTER && value.is_string()) {
            updated_iter->second.is_pointer = false;
            updated_iter->second.pointer_depth = 0;
            if (interpreter_->is_debug_mode()) {
                std::cerr
                    << "[VAR_MANAGER] Set pointer metadata for parameter '"
                    << name << "': pointer_base_type_name='"
                    << updated_iter->second.pointer_base_type_name
                    << "', type_name='" << updated_iter->second.type_name << "'"
                    << std::endl;
            }
        }
    } else if (Variable *updated_var = find_variable(name)) {
        updated_var->is_unsigned = is_unsigned;

        // char*パラメータにstring値が渡された場合、is_pointerをfalseに確実に設定
        if (type == TYPE_POINTER && value.is_string()) {
            updated_var->is_pointer = false;
            updated_var->pointer_depth = 0;
        }
    }
}

// v0.11.0 Phase 1a: 型名を受け取るオーバーロード（ジェネリックポインタ対応）
void VariableManager::assign_function_parameter(const std::string &name,
                                                const TypedValue &value,
                                                TypeInfo type,
                                                const std::string &type_name,
                                                bool is_unsigned) {
    // ジェネリックポインタ型の解析
    if (!type_name.empty() && GenericTypeParser::is_pointer_type(type_name)) {
        auto parsed = GenericTypeParser::parse_generic_type(type_name);

        if (interpreter_->is_debug_mode()) {
            std::cerr << "[VAR_MANAGER] Parsing parameter type: " << type_name
                      << std::endl;
            std::cerr << "  base_name=" << parsed.base_name << std::endl;
            std::cerr << "  is_pointer=" << parsed.is_pointer << std::endl;
            std::cerr << "  pointer_depth=" << parsed.pointer_depth
                      << std::endl;
            std::cerr << "  type_params.size()=" << parsed.type_params.size()
                      << std::endl;
        }

        if (parsed.is_pointer) {
            // 通常の代入を実行
            assign_function_parameter(name, value, type, is_unsigned);

            // 変数を取得してポインタメタデータを設定
            Variable *var = find_variable(name);
            if (var) {
                // char*パラメータにstring値が渡された場合の特別処理
                // is_pointerをfalseに設定し、通常の文字列として扱う
                if (parsed.base_name == "char" && value.is_string()) {
                    var->is_pointer = false;
                    var->pointer_depth = 0;
                    var->pointer_base_type_name = "char";
                    var->type_name = type_name;

                    if (interpreter_->is_debug_mode()) {
                        std::cerr << "[VAR_MANAGER] char* parameter with "
                                     "string value: "
                                  << "treating as TYPE_STRING, is_pointer=false"
                                  << std::endl;
                    }
                } else {
                    // 通常のポインタパラメータ（ジェネリック構造体など）
                    var->is_pointer = true;
                    var->pointer_depth = parsed.pointer_depth;
                    var->pointer_base_type =
                        TYPE_STRUCT; // ジェネリック構造体と仮定

                    // ベース型名を構築（型パラメータ含む）
                    if (!parsed.type_params.empty()) {
                        std::string full_base_type = parsed.base_name + "<";
                        for (size_t i = 0; i < parsed.type_params.size(); ++i) {
                            if (i > 0)
                                full_base_type += ", ";
                            full_base_type += parsed.type_params[i];
                        }
                        full_base_type += ">";
                        var->pointer_base_type_name = full_base_type;
                    } else {
                        var->pointer_base_type_name = parsed.base_name;
                    }

                    var->type_name = type_name;

                    if (interpreter_->is_debug_mode()) {
                        std::cerr << "[VAR_MANAGER] Set pointer metadata for "
                                     "parameter '"
                                  << name << "': pointer_base_type_name='"
                                  << var->pointer_base_type_name
                                  << "', type_name='" << var->type_name << "'"
                                  << std::endl;
                    }
                }
            }
            return;
        }
    }

    // 通常の処理
    assign_function_parameter(name, value, type, is_unsigned);
}

void VariableManager::assign_array_parameter(const std::string &name,
                                             const Variable &source_array,
                                             TypeInfo type) {
    // 配列は参照として渡される（C/C++の動作と同じ）
    // しかし、配列リテラルからの一時変数の場合はデータをコピーする必要がある

    // source_arrayが一時変数（is_referenceがfalseでデータが直接格納されている）の場合、
    // データをコピーして保存する
    bool is_temp_literal = !source_array.is_reference &&
                           (!source_array.array_values.empty() ||
                            !source_array.array_strings.empty() ||
                            !source_array.array_double_values.empty());

    Variable array_ref;

    if (is_temp_literal) {
        // 配列リテラルの場合: データを完全にコピー
        array_ref = source_array; // すべてのメンバをコピー
        array_ref.is_reference = false;
        array_ref.is_assigned = true;
    } else {
        // 配列変数の場合: 参照として渡す
        array_ref.is_reference = true;
        array_ref.is_assigned = true;
        array_ref.type = source_array.type;

        // 元の配列変数へのポインタを保存
        array_ref.value =
            reinterpret_cast<int64_t>(const_cast<Variable *>(&source_array));

        // 配列情報をコピー（参照として動作するために必要）
        array_ref.is_array = source_array.is_array;
        array_ref.is_multidimensional = source_array.is_multidimensional;
        array_ref.array_size = source_array.array_size;
        array_ref.array_dimensions = source_array.array_dimensions;
        array_ref.array_type_info = source_array.array_type_info;

        // ポインタ配列情報もコピー
        array_ref.is_pointer = source_array.is_pointer;
        array_ref.pointer_depth = source_array.pointer_depth;
        array_ref.pointer_base_type = source_array.pointer_base_type;
        array_ref.pointer_base_type_name = source_array.pointer_base_type_name;

        // struct配列情報もコピー
        array_ref.is_struct = source_array.is_struct;
        array_ref.struct_type_name = source_array.struct_type_name;

        // unsigned情報もコピー
        array_ref.is_unsigned = source_array.is_unsigned;
    }

    current_scope().variables[name] = array_ref;
}

void VariableManager::process_var_decl_or_assign(const ASTNode *node) {
    // debug_msg(DebugMsgId::VAR_MANAGER_PROCESS, (int)node->node_type,
    // node->name.c_str());
    if (interpreter_->debug_mode) {
        debug_print("VAR_DEBUG: process_var_decl_or_assign called for %s, "
                    "node_type=%d\n",
                    node->name.c_str(), static_cast<int>(node->node_type));
        debug_print("VAR_DEBUG: type_info=%d, type_name='%s'\n",
                    static_cast<int>(node->type_info), node->type_name.c_str());
        debug_print("VAR_DEBUG: node->is_unsigned=%d\n",
                    node->is_unsigned ? 1 : 0);
        debug_print("VAR_DEBUG: node->is_reference=%d\n",
                    node->is_reference ? 1 : 0);

        std::string resolved =
            interpreter_->type_manager_->resolve_typedef(node->type_name);
        debug_print("VAR_DEBUG: resolve_typedef('%s') = '%s'\n",
                    node->type_name.c_str(), resolved.c_str());
        debug_print(
            "VAR_DEBUG: condition check: !empty=%d, resolved!=original=%d\n",
            !node->type_name.empty(), resolved != node->type_name);
    }

    // 関数ポインタの早期チェック（evaluate前に処理）
    if (handle_function_pointer(node)) {
        return; // 処理完了、以降の処理をスキップ
    }

    // 参照型変数の特別処理
    if (handle_reference_variable(node)) {
        return; // 処理完了、以降の処理をスキップ
    }

    // ノードタイプに応じて適切なハンドラに委譲
    if (node->node_type == ASTNodeType::AST_VAR_DECL) {
        process_variable_declaration(node);
    } else if (node->node_type == ASTNodeType::AST_ASSIGN) {
        process_variable_assignment(node);
    } else {
        throw std::runtime_error(
            "Unexpected node type in process_var_decl_or_assign: " +
            std::to_string(static_cast<int>(node->node_type)));
    }
}

// ============================================================================
// Variable Declaration Processing (AST_VAR_DECL)
// ============================================================================
