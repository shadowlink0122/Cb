#include "managers/variables/manager.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../services/debug_service.h"
#include "../../core/interpreter.h"
#include "core/type_inference.h"
#include "evaluator/expression_evaluator.h"
#include "managers/arrays/manager.h"
#include "managers/common/operations.h"
#include "managers/types/enums.h"
#include "managers/types/manager.h"
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

void VariableManager::clamp_unsigned_value(Variable &target, int64_t &value,
                                            const char *context,
                                            const ASTNode *node) {
    if (!target.is_unsigned || value >= 0) {
        return;
    }
    const char *var_name = node ? node->name.c_str() : "<anonymous>";
    DEBUG_WARN(VARIABLE,
               "Unsigned variable %s %s negative value (%lld); clamping to 0",
               var_name, context, static_cast<long long>(value));
    value = 0;
}

bool VariableManager::handle_typedef_resolution(const ASTNode *node,
                                                 Variable &var) {
    // typedef解決処理（ArrayTypeInfoが設定されていない場合）
    // type_infoが基本型でも、type_nameがtypedef名の場合は処理する
    if (!node->type_name.empty() &&
        interpreter_->type_manager_->resolve_typedef(node->type_name) !=
            node->type_name) {
        if (debug_mode) {
            debug_print("TYPEDEF_DEBUG: Entering typedef resolution branch\n");
        }
        std::string resolved_type =
            interpreter_->type_manager_->resolve_typedef(node->type_name);

        if (debug_mode) {
            debug_print("TYPEDEF_DEBUG: Resolving typedef '%s' -> '%s' "
                        "(type_info=%d)\n",
                        node->type_name.c_str(), resolved_type.c_str(),
                        static_cast<int>(node->type_info));
        }

        // union typedefの場合（早期return）
        if (handle_union_typedef_declaration(node, var)) {
            return true; // 処理完了（早期returnが既に実行済み）
        }

        // 配列typedefの場合
        if (resolved_type.find("[") != std::string::npos) {
            std::string base = resolved_type.substr(0, resolved_type.find("["));
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
                debug_print("TYPEDEF_DEBUG: Processing typedef array: %s "
                            "(array_part: %s)\n",
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
                    if (const_var && const_var->is_const &&
                        const_var->type == TYPE_INT) {
                        dimension_size = static_cast<int>(const_var->value);
                    } else {
                        throw std::runtime_error(
                            "Array size must be a constant integer: " +
                            size_str);
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
                    debug_print("TYPEDEF_DEBUG: Multidim array created: "
                                "dimensions=%zu, total_size=%d\n",
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
            const StructDefinition *struct_def =
                interpreter_->find_struct_definition(resolved_type);
            if (struct_def) {
                if (debug_mode) {
                    debug_print("TYPEDEF_DEBUG: Resolving struct typedef "
                                "'%s' -> '%s'\n",
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
                        debug_print("TYPEDEF_DEBUG: Added struct member: "
                                    "%s (type: %d)\n",
                                    member.name.c_str(), (int)member.type);
                    }
                }
            } else {
                // プリミティブtypedefの場合
                var.type =
                    interpreter_->type_manager_->string_to_type_info(resolved_type);

                // プリミティブtypedefでもimpl解決のためにstruct_type_nameを設定
                var.struct_type_name = node->type_name;

                if (debug_mode) {
                    debug_print("TYPEDEF_DEBUG: Set primitive typedef '%s' "
                                "with struct_type_name='%s'\n",
                                node->type_name.c_str(), node->type_name.c_str());
                }
            }
        }

        // カスタム型の保存（union以外）
        if (var.type != TYPE_UNION) {
            var.type_name = node->type_name;
            var.current_type = var.type;
        }

        return true; // typedef解決完了
    }
    return false; // typedefではない
}

void VariableManager::assign_union_value(Variable &var,
                                         const std::string &union_type_name,
                                         const ASTNode *value_node) {
    // union型変数への代入を実行
    if (var.type != TYPE_UNION) {
        throw std::runtime_error("Variable is not a union type");
    }

    // 値の型に応じて検証と代入を実行
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列値
        std::string str_value = value_node->str_value;
        if (interpreter_->get_type_manager()->is_value_allowed_for_union(
                union_type_name, str_value)) {
            var.str_value = str_value;
            var.current_type = TYPE_STRING;
            var.is_assigned = true;
            if (debug_mode) {
                debug_print(
                    "UNION_DEBUG: Assigned string '%s' to union variable\n",
                    str_value.c_str());
            }
        } else {
            throw std::runtime_error("String value '" + str_value +
                                     "' is not allowed for union type " +
                                     union_type_name);
        }
    } else if (value_node->node_type == ASTNodeType::AST_NUMBER) {
        // 数値
        int64_t int_value = value_node->int_value;
        if (interpreter_->get_type_manager()->is_value_allowed_for_union(
                union_type_name, int_value)) {
            var.value = int_value;
            var.current_type = TYPE_INT;
            var.is_assigned = true;
            if (debug_mode) {
                debug_print(
                    "UNION_DEBUG: Assigned integer %lld to union variable\n",
                    int_value);
            }
        } else {
            throw std::runtime_error(
                "Integer value " + std::to_string(int_value) +
                " is not allowed for union type " + union_type_name);
        }
    } else if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数参照の場合、その変数の型がカスタム型unionで許可されているかチェック
        std::string var_name = value_node->name;
        Variable *source_var = find_variable(var_name);
        if (source_var) {
            if (debug_mode) {
                debug_print("UNION_DEBUG: Checking variable reference '%s' "
                            "(type_name='%s', current_type=%d)\n",
                            var_name.c_str(), source_var->type_name.c_str(),
                            static_cast<int>(source_var->current_type));
            }

            // 1. カスタム型（typedef型）のチェック
            if (!source_var->type_name.empty()) {
                if (interpreter_->get_type_manager()
                        ->is_custom_type_allowed_for_union(
                            union_type_name, source_var->type_name)) {
                    // カスタム型として許可されている場合、値をコピー
                    var.value = source_var->value;
                    var.str_value = source_var->str_value;
                    var.current_type = source_var->current_type;
                    // var.type_name = source_var->type_name; //
                    // Union型変数の型名は変更しない

                    // 構造体の場合は構造体データも完全にコピー
                    if (source_var->is_struct) {
                        var.is_struct = true;
                        var.struct_type_name = source_var->struct_type_name;
                        var.struct_members = source_var->struct_members;
                        var.current_type = TYPE_STRUCT;
                    }

                    var.is_assigned = true;
                    if (debug_mode) {
                        debug_print(
                            "UNION_DEBUG: Assigned custom type '%s' to union "
                            "variable (current_type=%d, str_value='%s')\n",
                            source_var->type_name.c_str(),
                            static_cast<int>(source_var->current_type),
                            source_var->str_value.c_str());
                    }
                    return;
                } else {
                    // カスタム型が許可されていない場合はエラー
                    throw std::runtime_error(
                        "Type mismatch: Custom type '" + source_var->type_name +
                        "' is not allowed for union type " + union_type_name);
                }
            }

            // 2. 構造体型のチェック
            if (source_var->is_struct &&
                !source_var->struct_type_name.empty() &&
                interpreter_->get_type_manager()
                    ->is_custom_type_allowed_for_union(
                        union_type_name, source_var->struct_type_name)) {
                // 構造体型として許可されている場合、構造体全体をコピー
                var.value = source_var->value;
                var.str_value = source_var->str_value;
                var.current_type = TYPE_STRUCT;
                // var.type_name = source_var->struct_type_name; //
                // Union型変数の型名は変更しない
                var.is_struct = true;
                var.struct_type_name = source_var->struct_type_name;
                var.struct_members = source_var->struct_members;
                var.is_assigned = true;
                if (debug_mode) {
                    debug_print("UNION_DEBUG: Assigned struct type '%s' to "
                                "union variable\n",
                                source_var->struct_type_name.c_str());
                }
                return;
            }

            // 3. 配列型のチェック
            if (source_var->is_array) {
                // 配列の型名を構築 (例: int[3], bool[2])
                std::string array_type_name;
                TypeInfo base_type =
                    static_cast<TypeInfo>(source_var->type - TYPE_ARRAY_BASE);

                // 基本型を文字列に変換
                std::string base_type_str;
                switch (base_type) {
                case TYPE_INT:
                    base_type_str = "int";
                    break;
                case TYPE_LONG:
                    base_type_str = "long";
                    break;
                case TYPE_SHORT:
                    base_type_str = "short";
                    break;
                case TYPE_TINY:
                    base_type_str = "tiny";
                    break;
                case TYPE_BOOL:
                    base_type_str = "bool";
                    break;
                case TYPE_STRING:
                    base_type_str = "string";
                    break;
                case TYPE_CHAR:
                    base_type_str = "char";
                    break;
                default:
                    base_type_str = "unknown";
                    break;
                }

                if (source_var->array_dimensions.size() > 0) {
                    array_type_name = base_type_str;
                    for (size_t dim : source_var->array_dimensions) {
                        array_type_name += "[" + std::to_string(dim) + "]";
                    }
                } else if (source_var->array_size > 0) {
                    array_type_name = base_type_str + "[" +
                                      std::to_string(source_var->array_size) +
                                      "]";
                }

                if (!array_type_name.empty() &&
                    interpreter_->get_type_manager()
                        ->is_array_type_allowed_for_union(union_type_name,
                                                          array_type_name)) {
                    // 配列型として許可されている場合、配列全体をコピー
                    var.value = source_var->value;
                    var.str_value = source_var->str_value;
                    var.current_type = source_var->type;
                    // var.type_name = array_type_name; //
                    // Union型変数の型名は変更しない
                    var.is_array = true;
                    var.array_size = source_var->array_size;
                    var.array_dimensions = source_var->array_dimensions;
                    var.array_values = source_var->array_values;
                    var.array_strings = source_var->array_strings;
                    var.is_multidimensional = source_var->is_multidimensional;
                    var.multidim_array_values =
                        source_var->multidim_array_values;
                    var.is_assigned = true;
                    if (debug_mode) {
                        debug_print("UNION_DEBUG: Assigned array type '%s' to "
                                    "union variable\n",
                                    array_type_name.c_str());
                    }
                    return;
                }
            }

            // If not a custom type, fall through to expression evaluation
        }

        // Fall through to expression evaluation for non-custom-type variables
        try {
            int64_t int_value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    value_node);
            if (interpreter_->get_type_manager()->is_value_allowed_for_union(
                    union_type_name, int_value)) {
                var.value = int_value;
                var.current_type = TYPE_INT;
                var.is_assigned = true;
                if (debug_mode) {
                    debug_print("UNION_DEBUG: Assigned evaluated integer %lld "
                                "to union variable\n",
                                int_value);
                }
            } else {
                throw std::runtime_error("Value " + std::to_string(int_value) +
                                         " is not allowed for union type " +
                                         union_type_name);
            }
        } catch (const std::exception &e) {
            throw std::runtime_error(
                "Failed to assign variable reference to union: " +
                std::string(e.what()));
        }
    } else {
        // 式の評価
        try {
            // 数値として評価
            int64_t int_value =
                interpreter_->expression_evaluator_->evaluate_expression(
                    value_node);
            if (interpreter_->get_type_manager()->is_value_allowed_for_union(
                    union_type_name, int_value)) {
                var.value = int_value;
                var.current_type = TYPE_INT;
                var.is_assigned = true;
                if (debug_mode) {
                    debug_print("UNION_DEBUG: Assigned evaluated integer %lld "
                                "to union variable\n",
                                int_value);
                }
            } else {
                throw std::runtime_error("Value " + std::to_string(int_value) +
                                         " is not allowed for union type " +
                                         union_type_name);
            }
        } catch (const std::exception &e) {
            throw std::runtime_error(
                "Failed to assign value to union variable: " +
                std::string(e.what()));
        }
    }
}

// Priority 3: 変数ポインターから名前を検索
std::string VariableManager::find_variable_name(const Variable *target_var) {
    if (!target_var)
        return "";

    // 実装を簡素化：変数名の逆引きは複雑なので、
    // フォールバック戦略として空文字列を返す
    // これにより、呼び出し元は従来の方法にフォールバックする
    return "";
}

void VariableManager::handle_ternary_initialization(
    Variable &var, const ASTNode *ternary_node) {
    debug_msg(DebugMsgId::TERNARY_VAR_INIT_START);
    auto clamp_unsigned_ternary = [&](int64_t &value, const char *context) {
        if (!var.is_unsigned || value >= 0) {
            return;
        }
        std::string var_name = find_variable_name(&var);
        if (var_name.empty()) {
            var_name = std::string("<ternary>");
        }
        DEBUG_WARN(
            VARIABLE,
            "Unsigned variable %s %s negative value (%lld); clamping to 0",
            var_name.c_str(), context, static_cast<long long>(value));
        value = 0;
    };

    // 三項演算子の条件を評価
    int64_t condition = interpreter_->evaluate(ternary_node->left.get());
    debug_msg(DebugMsgId::TERNARY_VAR_CONDITION, condition);

    // 条件に基づいて選択される分岐を決定
    const ASTNode *selected_branch =
        condition ? ternary_node->right.get() : ternary_node->third.get();
    debug_msg(DebugMsgId::TERNARY_VAR_BRANCH_TYPE,
              static_cast<int>(selected_branch->node_type));

    // 選択された分岐の型に基づいて初期化
    if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの初期化
        debug_msg(DebugMsgId::TERNARY_VAR_STRING_SET,
                  selected_branch->str_value.c_str());
        var.str_value = selected_branch->str_value;
        var.type = TYPE_STRING;
        var.is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_NUMBER) {
        // 数値リテラルの初期化
        debug_msg(DebugMsgId::TERNARY_VAR_NUMERIC_SET,
                  selected_branch->int_value);
        int64_t numeric_value = selected_branch->int_value;
        clamp_unsigned_ternary(numeric_value,
                               "initialized with ternary literal");
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
            clamp_unsigned_ternary(value,
                                   "initialized with ternary expression");
            var.value = value;
            var.is_assigned = true;
        } catch (const ReturnException &ret) {
            if (ret.type == TYPE_STRING) {
                var.str_value = ret.str_value;
                var.type = TYPE_STRING;
            } else {
                int64_t numeric_value = ret.value;
                clamp_unsigned_ternary(numeric_value,
                                       "initialized with ternary return");
                var.value = numeric_value;
            }
            var.is_assigned = true;
        }
    }
}

// ============================================================================
// Helper methods for process_var_decl_or_assign
// ============================================================================

bool VariableManager::handle_function_pointer(const ASTNode *node) {
    if (node->node_type == ASTNodeType::AST_VAR_DECL &&
        node->type_info == TYPE_POINTER) {
        if (interpreter_->debug_mode) {
            std::cerr << "[VAR_MANAGER] Checking if pointer is function pointer"
                      << std::endl;
        }
        ASTNode *init_node = node->init_expr
                                 ? node->init_expr.get()
                                 : (node->right ? node->right.get() : nullptr);
        if (interpreter_->debug_mode && init_node) {
            std::cerr << "[VAR_MANAGER] Init node exists: type="
                      << static_cast<int>(init_node->node_type)
                      << ", op=" << init_node->op << ", is_function_address="
                      << init_node->is_function_address << std::endl;
        }
        if (init_node && init_node->node_type == ASTNodeType::AST_UNARY_OP &&
            init_node->op == "ADDRESS_OF" && init_node->is_function_address) {

            std::string func_name = init_node->function_address_name;
            const ASTNode *func_node = interpreter_->find_function(func_name);

            // 関数が見つかった場合のみ関数ポインタとして処理
            if (func_node) {
                // 関数ポインタ変数を作成
                Variable var;
                var.is_function_pointer = true;
                var.function_pointer_name = func_name;
                var.type = TYPE_POINTER; // ポインタ型として扱う
                var.is_assigned = true;
                var.is_const = node->is_const;
                // 関数ノードの実際のメモリアドレスを値として格納
                var.value = reinterpret_cast<int64_t>(func_node);

                // 変数を登録
                current_scope().variables[node->name] = var;

                // FunctionPointerを登録
                FunctionPointer func_ptr(func_node, func_name,
                                         func_node->type_info);
                interpreter_->current_scope().function_pointers[node->name] =
                    func_ptr;

                if (interpreter_->debug_mode) {
                    std::cerr
                        << "[VAR_MANAGER] Registered function pointer (early): "
                        << node->name << " -> " << func_name << std::endl;
                }

                return true; // 処理完了
            }
            // 関数が見つからない場合は通常の変数アドレスとして処理を継続
            if (interpreter_->debug_mode) {
                std::cerr << "[VAR_MANAGER] Not a function, treating as "
                             "variable address: "
                          << func_name << std::endl;
            }
        }
    }
    return false; // 関数ポインタではない
}

bool VariableManager::handle_reference_variable(const ASTNode *node) {
    if (node->is_reference && node->node_type == ASTNodeType::AST_VAR_DECL) {
        if (interpreter_->is_debug_mode()) {
            std::cerr << "[VAR_MANAGER] Processing reference variable: "
                      << node->name << std::endl;
        }

        // 参照は必ず初期化が必要
        if (!node->init_expr && !node->right) {
            throw std::runtime_error("Reference variable '" + node->name +
                                     "' must be initialized");
        }

        // 初期化式を評価して参照先変数を取得
        ASTNode *init_node =
            node->init_expr ? node->init_expr.get() : node->right.get();

        // 関数呼び出しの場合、ReturnExceptionから参照を取得
        if (init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
            try {
                interpreter_->expression_evaluator_->evaluate_expression(
                    init_node);
                throw std::runtime_error(
                    "Function did not return via exception");
            } catch (const ReturnException &ret) {
                if (!ret.is_reference || !ret.reference_target) {
                    throw std::runtime_error("Function '" + init_node->name +
                                             "' does not return a reference");
                }

                Variable *target_var = ret.reference_target;

                if (interpreter_->is_debug_mode()) {
                    std::cerr
                        << "[VAR_MANAGER] Creating reference " << node->name
                        << " from function return (value: " << target_var->value
                        << ")" << std::endl;
                }

                // 参照変数を作成
                Variable ref_var;
                ref_var.is_reference = true;
                ref_var.type = target_var->type;
                ref_var.is_const = node->is_const;
                ref_var.is_array = target_var->is_array;
                ref_var.is_unsigned = target_var->is_unsigned;
                ref_var.is_struct = target_var->is_struct;
                ref_var.struct_type_name = target_var->struct_type_name;

                // 参照先変数のポインタを値として保存
                ref_var.value = reinterpret_cast<int64_t>(target_var);
                ref_var.is_assigned = true;

                current_scope().variables[node->name] = ref_var;
                return true;
            }
        }

        // 参照先が変数でなければエラー
        if (init_node->node_type != ASTNodeType::AST_VARIABLE) {
            throw std::runtime_error("Reference variable '" + node->name +
                                     "' must be initialized with a variable");
        }

        std::string target_var_name = init_node->name;

        // 参照先変数が存在するかチェック
        Variable *target_var = find_variable(target_var_name);
        if (!target_var) {
            throw std::runtime_error("Reference target variable '" +
                                     target_var_name + "' not found");
        }

        // 参照先変数が参照型の場合、さらにデリファレンス
        if (target_var->is_reference) {
            target_var = reinterpret_cast<Variable *>(target_var->value);
            if (!target_var) {
                throw std::runtime_error(
                    "Invalid reference chain for variable: " + target_var_name);
            }
        }

        if (interpreter_->is_debug_mode()) {
            std::cerr << "[VAR_MANAGER] Creating reference " << node->name
                      << " -> " << target_var_name
                      << " (value: " << target_var->value << ")" << std::endl;
        }

        // 参照変数を作成
        Variable ref_var;
        ref_var.is_reference = true;
        ref_var.type = target_var->type;
        ref_var.is_const = node->is_const;
        ref_var.is_array = target_var->is_array;
        ref_var.is_unsigned = target_var->is_unsigned;
        ref_var.is_struct = target_var->is_struct;
        ref_var.struct_type_name = target_var->struct_type_name;

        // 参照先変数のポインタを値として保存
        ref_var.value = reinterpret_cast<int64_t>(target_var);
        ref_var.is_assigned = true;

        current_scope().variables[node->name] = ref_var;
        return true;
    }
    return false; // 参照変数ではない
}

bool VariableManager::handle_array_type_info_declaration(const ASTNode *node,
                                                          Variable &var) {
    // 新しいArrayTypeInfoが設定されている場合の処理
    if (node->array_type_info.base_type != TYPE_UNKNOWN) {
        debug_print("VAR_DEBUG: Taking ArrayTypeInfo branch (base_type=%d)\n",
                    static_cast<int>(node->array_type_info.base_type));

        // ArrayTypeInfoが設定されている場合は配列として処理
        var.is_array = true;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                         node->array_type_info.base_type);
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
                    if (const_var && const_var->is_const &&
                        const_var->type == TYPE_INT) {
                            resolved_size = static_cast<int>(const_var->value);
                    } else {
                        throw std::runtime_error(
                            "Array size must be a constant integer: " +
                            dim.size_expr);
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
            var.array_size = total_size; // 総サイズを設定

            if (debug_mode) {
                debug_print("VAR_DEBUG: ArrayTypeInfo - dimensions=%zu, "
                            "total_size=%d\n",
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
        return true; // ArrayTypeInfoで処理完了
    }
    return false; // ArrayTypeInfoが設定されていない
}

bool VariableManager::handle_union_typedef_declaration(const ASTNode *node,
                                                        Variable &var) {
    // union typedefの場合
    if (interpreter_->type_manager_->is_union_type(node->type_name)) {
        if (debug_mode) {
            debug_print("TYPEDEF_DEBUG: Processing union typedef: %s\n",
                        node->type_name.c_str());
        }
        var.type = TYPE_UNION;
        var.type_name = node->type_name; // union型名を保存
        var.current_type = TYPE_UNKNOWN; // まだ値が設定されていない

        // 初期化値がある場合は検証して代入
        if (node->right || node->init_expr) {
            ASTNode *init_node =
                node->init_expr ? node->init_expr.get() : node->right.get();
            assign_union_value(var, node->type_name, init_node);
        }

        // union型変数の処理完了後、変数を登録して終了
        interpreter_->current_scope().variables[node->name] = var;
        return true; // 処理完了（早期return）
    }
    return false; // union typedefではない
}

void VariableManager::handle_struct_member_initialization(const ASTNode *node,
                                                           Variable &var) {
    bool debug_mode = interpreter_->debug_mode;

    // struct型の場合のメンバー初期化処理
    if (node->type_info == TYPE_STRUCT ||
        (node->type_info == TYPE_UNKNOWN &&
         (interpreter_->find_struct_definition(node->type_name) != nullptr ||
          interpreter_->find_struct_definition(
              interpreter_->type_manager_->resolve_typedef(node->type_name)) !=
              nullptr))) {
        if (debug_mode) {
            debug_print("VAR_DEBUG: Taking STRUCT branch (type_info=%d, "
                        "TYPE_STRUCT=%d)\n",
                        (int)node->type_info, (int)TYPE_STRUCT);
        }
        debug_msg(DebugMsgId::VAR_MANAGER_STRUCT_CREATE, node->name.c_str(),
                  node->type_name.c_str());
        var.type = TYPE_STRUCT;
        var.is_struct = true;

        // 構造体のtype_nameを設定
        var.type_name = node->type_name;

        // typedef名を実際のstruct名に解決
        std::string resolved_struct_type =
            interpreter_->type_manager_->resolve_typedef(node->type_name);
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
                if (size_node &&
                    size_node->node_type == ASTNodeType::AST_NUMBER) {
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
                base_struct_type = resolved_struct_type.substr(0, bracket_pos);

                size_t close_bracket_pos =
                    resolved_struct_type.find("]", bracket_pos);
                if (close_bracket_pos != std::string::npos) {
                    std::string size_str = resolved_struct_type.substr(
                        bracket_pos + 1, close_bracket_pos - bracket_pos - 1);
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
        }

        // struct定義を取得してメンバ変数を初期化
        const StructDefinition *struct_def =
            interpreter_->find_struct_definition(
                interpreter_->type_manager_->resolve_typedef(base_struct_type));
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
                        member_var.type_name =
                            member.type_alias; // Union型名などを保持
                        member_var.is_pointer = member.is_pointer;
                        member_var.pointer_depth = member.pointer_depth;
                        member_var.pointer_base_type_name =
                            member.pointer_base_type_name;
                        member_var.pointer_base_type = member.pointer_base_type;
                        member_var.is_reference = member.is_reference;
                        member_var.is_unsigned = member.is_unsigned;
                        member_var.is_private_member = member.is_private;

                        // 構造体配列自体がconstの場合、すべてのメンバーをconstにする
                        // また、メンバー定義でconstが指定されている場合もconstにする
                        member_var.is_const = node->is_const || member.is_const;

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
                    member_var.type_name =
                        member.type_alias; // Union型名などを保持
                    member_var.is_pointer = member.is_pointer;
                    member_var.pointer_depth = member.pointer_depth;
                    member_var.pointer_base_type_name =
                        member.pointer_base_type_name;
                    member_var.pointer_base_type = member.pointer_base_type;
                    member_var.is_reference = member.is_reference;
                    member_var.is_unsigned = member.is_unsigned;
                    member_var.is_private_member = member.is_private;

                    // 構造体変数自体がconstの場合、すべてのメンバーをconstにする
                    // また、メンバー定義でconstが指定されている場合もconstにする
                    member_var.is_const = node->is_const || member.is_const;

                    // 配列メンバーの場合
                    if (member.array_info.is_array()) {
                        member_var.is_array = true;

                        // 多次元配列の総サイズを計算（定数解決を含む）
                        int total_size = 1;
                        for (const auto &dim : member.array_info.dimensions) {
                            int resolved_size = dim.size;

                            // 動的サイズの場合は定数識別子を解決
                            if (resolved_size == -1 && dim.is_dynamic &&
                                !dim.size_expr.empty()) {
                                Variable *const_var =
                                    interpreter_->find_variable(dim.size_expr);
                                if (const_var && const_var->is_assigned) {
                                    // const変数または初期化済み変数を許可
                                    resolved_size =
                                        static_cast<int>(const_var->value);
                                    if (interpreter_->debug_mode) {
                                        debug_print(
                                            "Resolved constant %s to %d "
                                            "for struct member %s\n",
                                            dim.size_expr.c_str(),
                                            resolved_size, member.name.c_str());
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
                                    "Invalid array size for struct member " +
                                    member.name);
                            }

                            total_size *= resolved_size;
                        }
                        member_var.array_size = total_size;

                        // array_dimensionsを設定（定数解決済み）
                        member_var.array_dimensions.clear();
                        for (const auto &dim : member.array_info.dimensions) {
                            int resolved_size = dim.size;

                            // 動的サイズの場合は定数識別子を解決
                            if (resolved_size == -1 && dim.is_dynamic &&
                                !dim.size_expr.empty()) {
                                Variable *const_var =
                                    interpreter_->find_variable(dim.size_expr);
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

                            member_var.array_dimensions.push_back(resolved_size);
                        }

                        // 多次元配列のフラグを設定
                        if (member_var.array_dimensions.size() > 1) {
                            member_var.is_multidimensional = true;

                            // array_type_info.dimensionsを設定
                            member_var.array_type_info.dimensions.clear();
                            for (const auto &dim_size :
                                 member_var.array_dimensions) {
                                ArrayDimension dimension(
                                    dim_size,
                                    false); // 構造体メンバーは静的サイズ
                                member_var.array_type_info.dimensions.push_back(
                                    dimension);
                            }
                            member_var.array_type_info.base_type = member.type;

                            debug_msg(DebugMsgId::VAR_MANAGER_MULTIDIM_FLAG,
                                      member.name.c_str(),
                                      member_var.array_dimensions.size());
                            if (interpreter_->debug_mode) {
                                debug_print(
                                    "Set multidimensional flag for struct "
                                    "member: %s (dimensions: %zu)\n",
                                    member.name.c_str(),
                                    member_var.array_dimensions.size());
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
                            element_var.is_assigned = false;
                            element_var.is_const =
                                node->is_const || member.is_const;

                            // 配列の要素型を取得
                            TypeInfo element_type_info =
                                member.array_info.base_type;
                            std::string element_type_alias = member.type_alias;

                            // type_aliasから配列のサイズ情報を削除（例:
                            // "Point[3]" -> "Point"）
                            size_t bracket_pos = element_type_alias.find('[');
                            if (bracket_pos != std::string::npos) {
                                element_type_alias =
                                    element_type_alias.substr(0, bracket_pos);
                            }

                            if (interpreter_->debug_mode) {
                                debug_print(
                                    "Processing array element %d: "
                                    "element_type=%d, TYPE_STRUCT=%d, "
                                    "type_alias='%s'\n",
                                    i, (int)element_type_info, (int)TYPE_STRUCT,
                                    element_type_alias.c_str());
                            }

                            // 構造体型の配列要素の場合
                            if (element_type_info == TYPE_STRUCT &&
                                !element_type_alias.empty()) {
                                element_var.type =
                                    TYPE_STRUCT; // 要素の型を正しく設定
                                element_var.is_struct = true;
                                element_var.struct_type_name =
                                    element_type_alias;

                                if (interpreter_->debug_mode) {
                                    debug_print("Creating struct array "
                                                "element: %s of type %s\n",
                                                element_name.c_str(),
                                                element_type_alias.c_str());
                                }

                                // 構造体定義を取得してメンバーを初期化
                                std::string resolved_type =
                                    interpreter_->type_manager_->resolve_typedef(
                                        element_type_alias);
                                const StructDefinition *element_struct_def =
                                    interpreter_->find_struct_definition(
                                        resolved_type);

                                if (element_struct_def) {
                                    for (const auto &element_member :
                                         element_struct_def->members) {
                                        Variable element_member_var;
                                        element_member_var.type =
                                            element_member.type;
                                        element_member_var.is_unsigned =
                                            element_member.is_unsigned;
                                        element_member_var.is_private_member =
                                            element_member.is_private;
                                        element_member_var.is_assigned = false;
                                        element_member_var.is_const =
                                            element_var.is_const ||
                                            element_member.is_const;

                                        if (element_member_var.type ==
                                            TYPE_STRING) {
                                            element_member_var.str_value = "";
                                        } else {
                                            element_member_var.value = 0;
                                        }

                                        element_var
                                            .struct_members[element_member.name] =
                                            element_member_var;

                                        // メンバーの直接アクセス用変数も作成
                                        std::string member_path =
                                            element_name + "." +
                                            element_member.name;
                                        this->current_scope()
                                            .variables[member_path] =
                                            element_member_var;
                                    }

                                    if (interpreter_->debug_mode) {
                                        debug_print(
                                            "Initialized struct array element "
                                            "with %zu members\n",
                                            element_var.struct_members.size());
                                    }
                                }
                            } else {
                                // プリミティブ型の配列要素
                                element_var.value = 0;
                                element_var.str_value = "";
                            }

                            this->current_scope().variables[element_name] =
                                element_var;

                            // 親構造体のstruct_membersにも配列要素を追加
                            // element_nameは "structName.arrayName[i]" の形式なので、
                            // キーは "arrayName[i]" にする
                            std::string element_key =
                                member.name + "[" + std::to_string(i) + "]";
                            var.struct_members[element_key] = element_var;

                            if (interpreter_->debug_mode) {
                                debug_print(
                                    "Created struct member array element: %s "
                                    "(key: %s), is_struct=%s, members=%zu\n",
                                    element_name.c_str(), element_key.c_str(),
                                    element_var.is_struct ? "true" : "false",
                                    element_var.struct_members.size());
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
                                member_var.multidim_array_strings.resize(
                                    total_size, "");
                            } else {
                                member_var.multidim_array_values.resize(
                                    total_size, 0);
                            }
                        }

                        var.struct_members[member.name] = member_var;

                        if (interpreter_->debug_mode) {
                            debug_print(
                                "Added to struct_members[%s]: "
                                "is_multidimensional=%s, "
                                "array_dimensions.size()=%zu\n",
                                member.name.c_str(),
                                member_var.is_multidimensional ? "true"
                                                               : "false",
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

                        // 構造体メンバの場合、is_structフラグと型名を設定
                        if (member_var.type == TYPE_STRUCT &&
                            !member.type_alias.empty()) {
                            member_var.is_struct = true;
                            member_var.struct_type_name = member.type_alias;
                        }

                        var.struct_members[member.name] = member_var;
                    }

                    // 通常のstruct変数でもメンバー直接アクセス変数を作成
                    std::string member_path = node->name + "." + member.name;
                    Variable member_direct_var = member_var;
                    current_scope().variables[member_path] = member_direct_var;

                    // 構造体メンバの場合、再帰的にサブメンバーの個別変数を作成
                    // 注意: var.struct_members[member.name]への参照を使用して再帰呼び出し
                    if (member.type == TYPE_STRUCT &&
                        !member.type_alias.empty()) {
                        if (interpreter_->debug_mode) {
                            debug_print(
                                "Recursively creating nested struct members "
                                "for: %s (type: %s)\n",
                                member_path.c_str(),
                                member.type_alias.c_str());
                        }
                        // struct_membersに既に追加されたメンバーを参照
                        auto &struct_member_ref = var.struct_members[member.name];
                        interpreter_
                            ->create_struct_member_variables_recursively(
                                member_path, member.type_alias,
                                struct_member_ref);
                    }

                    if (interpreter_->debug_mode) {
                        debug_print(
                            "Added member: %s (type: %d, is_array: %s)\n",
                            member.name.c_str(), (int)member.type,
                            member.array_info.is_array() ? "true" : "false");
                    }
                }
            }
        }
    }
}

bool VariableManager::handle_interface_initialization(const ASTNode *node,
                                                       Variable &var) {
    // Interface型変数（ポインタを除く）の初期化処理
    if (!var.interface_name.empty() && var.type != TYPE_POINTER &&
        node->init_expr) {
        auto assign_from_source = [&](const Variable &source,
                                      const std::string &source_name) {
            assign_interface_view(node->name, var, source, source_name);
        };

        if (node->init_expr->node_type == ASTNodeType::AST_VARIABLE ||
            node->init_expr->node_type == ASTNodeType::AST_IDENTIFIER) {
            std::string source_var_name = node->init_expr->name;
            Variable *source_var = find_variable(source_var_name);
            if (!source_var) {
                throw std::runtime_error("Source variable not found: " +
                                         source_var_name);
            }
            if (!source_var->is_struct && !isPrimitiveType(source_var) &&
                source_var->type < TYPE_ARRAY_BASE &&
                source_var->type != TYPE_INTERFACE) {
                throw std::runtime_error(
                    "Cannot assign non-struct/non-primitive to interface "
                    "variable");
            }

            debug_msg(DebugMsgId::INTERFACE_VARIABLE_ASSIGN,
                      var.interface_name.c_str(), source_var_name.c_str());
            assign_from_source(*source_var, source_var_name);
            return true; // 処理完了（早期return）
        }

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
            if (node->init_expr->node_type ==
                ASTNodeType::AST_STRING_LITERAL) {
                Variable temp = create_temp_primitive(
                    TYPE_STRING, 0, node->init_expr->str_value);
                assign_from_source(temp, "");
                return true; // 処理完了（早期return）
            }

            int64_t numeric_value =
                interpreter_->evaluate(node->init_expr.get());
            TypeInfo resolved_type = node->init_expr->type_info != TYPE_UNKNOWN
                                         ? node->init_expr->type_info
                                         : TYPE_INT;
            Variable temp =
                create_temp_primitive(resolved_type, numeric_value, "");
            assign_from_source(temp, "");
            return true; // 処理完了（早期return）
        } catch (const ReturnException &ret) {
            if (ret.is_array) {
                throw std::runtime_error(
                    "Cannot assign array return value to interface variable '" +
                    node->name + "'");
            }

            if (!ret.is_struct) {
                if (ret.type == TYPE_STRING) {
                    Variable temp =
                        create_temp_primitive(TYPE_STRING, 0, ret.str_value);
                    assign_from_source(temp, "");
                    return true; // 処理完了（早期return）
                }

                Variable temp =
                    create_temp_primitive(ret.type, ret.value, ret.str_value);
                assign_from_source(temp, "");
                return true; // 処理完了（早期return）
            }

            // 構造体戻り値の場合はインターフェースとして処理
            assign_from_source(ret.struct_value, "");
            return true; // 処理完了（早期return）
        }
    }

    return false; // Interface初期化ではない
}

bool VariableManager::handle_array_literal_initialization(const ASTNode *node,
                                                           Variable &var) {
    // 配列リテラル初期化の処理
    if (var.is_array && node->init_expr &&
        node->init_expr->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // まず変数を登録
        current_scope().variables[node->name] = var;
        if (interpreter_->debug_mode) {
            debug_print("VAR_DEBUG: stored array var %s with "
                        "is_unsigned=%d before literal assignment\n",
                        node->name.c_str(), var.is_unsigned ? 1 : 0);
        }

        // 配列リテラル代入を実行
        interpreter_->assign_array_literal(node->name, node->init_expr.get());

        // 代入後に変数を再取得して更新
        current_scope().variables[node->name].is_assigned = true;

        return true; // 配列リテラル処理完了（早期return）
    }

    return false; // 配列リテラル初期化ではない
}
