#include "variable_declaration.h"
#include "../../../../common/debug.h"
#include "../../../../common/type_helpers.h"
#include "../statement_executor.h"
#include "core/error_handler.h"
#include "core/interpreter.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include <cstdio>
#include <iomanip>
#include <sstream>

namespace DeclarationHandlers {

void execute_variable_declaration(StatementExecutor *executor,
                                  Interpreter &interpreter,
                                  const ASTNode *node) {
    if (debug_mode) {
        debug_log_line("[DEBUG_EXEC] Executing variable declaration: " +
                       node->name);
        debug_log_line("  type_info: " +
                       std::to_string(static_cast<int>(node->type_info)));
        debug_log_line("  type_name: " + node->type_name);
        debug_log_line(std::string("  is_pointer: ") +
                       (node->is_pointer ? "true" : "false"));
        debug_log_line(
            "  pointer_base_type: " +
            std::to_string(static_cast<int>(node->pointer_base_type)));
        debug_log_line(std::string("  is_reference: ") +
                       (node->is_reference ? "true" : "false"));
    }

    // 参照型の場合の特別処理
    if (node->is_reference) {
        // 参照は必ず初期化が必要
        if (!node->init_expr && !node->right) {
            throw std::runtime_error("Reference variable '" + node->name +
                                     "' must be initialized");
        }

        // 初期化式を評価して参照先変数を取得
        ASTNode *init_node =
            node->init_expr ? node->init_expr.get() : node->right.get();

        // 参照先が変数でなければエラー
        if (init_node->node_type != ASTNodeType::AST_VARIABLE) {
            throw std::runtime_error("Reference variable '" + node->name +
                                     "' must be initialized with a variable");
        }

        std::string target_var_name = init_node->name;

        // 参照先変数が存在するかチェック
        Variable *target_var = interpreter.find_variable(target_var_name);
        if (!target_var) {
            throw std::runtime_error("Reference target variable '" +
                                     target_var_name + "' not found");
        }

        if (debug_mode) {
            debug_log_line("[DEBUG_EXEC] Creating reference " + node->name +
                           " -> " + target_var_name);
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

        if (debug_mode) {
            debug_log_line(
                "[DEBUG_EXEC] Creating reference variable: " + node->name +
                ", target_value: " + std::to_string(target_var->value));
        }

        interpreter.current_scope().variables[node->name] = ref_var;
        return;
    }

    // 関数ポインタの初期化を先にチェック
    ASTNode *init_node =
        node->init_expr ? node->init_expr.get() : node->right.get();
    if (debug_mode && init_node &&
        init_node->node_type == ASTNodeType::AST_UNARY_OP) {
        debug_log_line(
            "[FUNC_PTR_CHECK] Found UNARY_OP: op=" + init_node->op +
            ", is_function_address=" +
            (init_node->is_function_address ? "true" : "false") +
            ", function_address_name=" + init_node->function_address_name);
    }
    if (init_node && init_node->node_type == ASTNodeType::AST_UNARY_OP &&
        init_node->op == "ADDRESS_OF" && init_node->is_function_address) {

        std::string func_name = init_node->function_address_name;
        const ASTNode *func_node = interpreter.find_function(func_name);
        if (!func_node) {
            throw std::runtime_error("Undefined function: " + func_name);
        }

        // 先に変数を作成
        Variable var;
        var.type = func_node->type_info; // 関数の戻り値型
        var.is_const = node->is_const;
        var.is_function_pointer = true;
        var.function_pointer_name = func_name;
        var.is_assigned = true;
        interpreter.current_scope().variables[node->name] = var;

        // 関数ポインタを登録
        FunctionPointer func_ptr(func_node, func_name, func_node->type_info);
        interpreter.current_scope().function_pointers[node->name] = func_ptr;

        if (debug_mode) {
            debug_log_line(
                "[FUNC_PTR] Registered function pointer during declaration: " +
                node->name + " -> " + func_name);
        }

        return; // 関数ポインタの処理完了
    }

    // Debug output removed - use --debug option if needed

    Variable var;
    var.type = node->type_info;
    var.is_const = node->is_const;
    var.is_array = false;
    var.is_unsigned = node->is_unsigned;

    // ポインタ情報
    if (node->is_pointer) {
        var.type = TYPE_POINTER;
        var.is_pointer = true;
        var.pointer_depth = node->pointer_depth;
        var.pointer_base_type = node->pointer_base_type;
        var.pointer_base_type_name = node->pointer_base_type_name;
    }

    // ポインタのconst修飾子
    var.is_pointer_const = node->is_pointer_const_qualifier;
    var.is_pointee_const = node->is_pointee_const_qualifier;

    // typedef配列の場合の特別処理
    if (node->array_type_info.base_type != TYPE_UNKNOWN) {
        // ArrayTypeInfoが設定されている場合は配列として処理
        var.is_array = true;
        // ポインタ配列の場合（例:
        // double*[5]）、base_typeはTYPE_POINTERであるべき
        // しかし、パーサーのバグでTYPE_INTなどになっている場合は、is_pointerフラグから判定
        TypeInfo base_type = node->array_type_info.base_type;
        if (debug_mode) {
            debug_log_line("DEBUG: Array declaration for " + node->name);
            debug_log_line(std::string("  node->is_pointer: ") +
                           (node->is_pointer ? "true" : "false"));
            debug_log_line("  node->array_type_info.base_type: " +
                           std::to_string(static_cast<int>(base_type)));
            debug_log_line("  TYPE_POINTER: " +
                           std::to_string(static_cast<int>(TYPE_POINTER)));
        }

        if (node->is_pointer && base_type != TYPE_POINTER) {
            // ポインタ配列だがbase_typeが正しくない場合、TYPE_POINTERに修正
            if (debug_mode) {
                debug_log_line("  CORRECTING to TYPE_POINTER");
            }
            base_type = TYPE_POINTER;
        }
        // 配列の型は TYPE_ARRAY_BASE + 基本型
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE + base_type);
        if (debug_mode) {
            debug_log_line("  Final var.type: " +
                           std::to_string(static_cast<int>(var.type)));
        }

        // typedef名を保存（interfaceでの型マッチングに使用）
        if (!node->type_name.empty() || !node->original_type_name.empty()) {
            std::string declared_name = !node->original_type_name.empty()
                                            ? node->original_type_name
                                            : node->type_name;

            if (!declared_name.empty()) {
                std::string resolved_name =
                    interpreter.get_type_manager()->resolve_typedef(
                        declared_name);
                bool is_alias = (resolved_name != declared_name);

                if (is_alias) {
                    var.struct_type_name = declared_name;
                    var.type_name = declared_name;
                } else if (node->array_type_info.base_type == TYPE_STRUCT) {
                    std::string struct_type = resolved_name;
                    size_t bracket_pos = struct_type.find('[');
                    if (bracket_pos != std::string::npos) {
                        struct_type = struct_type.substr(0, bracket_pos);
                    }
                    var.struct_type_name = struct_type;
                }
            }
        }

        // struct配列の場合、is_structフラグも設定
        if (node->array_type_info.base_type == TYPE_STRUCT) {
            var.is_struct = true;
        }

        // デバッグ出力
        if (debug_mode) {
            debug_log_line("DEBUG: Setting array for typedef variable " +
                           node->name + " with base_type=" +
                           std::to_string(static_cast<int>(var.type)) +
                           " is_array=" + (var.is_array ? "true" : "false"));
        }

        // 配列サイズ情報をコピー
        for (const auto &dim : node->array_type_info.dimensions) {
            var.array_dimensions.push_back(dim.size);
            if (debug_mode) {
                debug_log_line("DEBUG: Adding dimension size=" +
                               std::to_string(dim.size));
            }
        }

        // 配列初期化
        if (!var.array_dimensions.empty()) {
            int total_size = 1;
            for (int dim : var.array_dimensions) {
                total_size *= dim;
            }

            // 文字列配列の場合は array_strings を初期化
            if (TypeHelpers::isString(var.type)) {
                var.array_strings.resize(total_size, "");
                if (debug_mode) {
                    debug_log_line(
                        "DEBUG: Initialized string array with size=" +
                        std::to_string(total_size));
                }
            } else {
                var.array_values.resize(total_size, 0);
                if (debug_mode) {
                    debug_log_line(
                        "DEBUG: Initialized numeric array with size=" +
                        std::to_string(total_size));
                }
            }
        }
    }

    // 型を確定する
    if (node->type_info == TYPE_UNKNOWN && !node->str_value.empty()) {
        // 単純な型エイリアス解決
        var.type = TYPE_INT;    // デフォルト
    } else if (!var.is_array) { // 配列でない場合のみ設定
        var.type = node->type_info;
    } else {
        // 配列の場合、TYPE_ARRAY_BASE + 基本型に設定
        // ただし、既にarray_type_info.base_typeから設定されている場合はそれを維持
        if (var.type < TYPE_ARRAY_BASE &&
            node->array_type_info.base_type != TYPE_UNKNOWN) {
            var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                             node->array_type_info.base_type);
            if (debug_mode) {
                debug_log_line("DEBUG: Array type set to TYPE_ARRAY_BASE + " +
                               std::to_string(static_cast<int>(
                                   node->array_type_info.base_type)) +
                               " = " +
                               std::to_string(static_cast<int>(var.type)));
            }
        }
    }

    // struct型の特別処理
    if (node->type_info == TYPE_STRUCT && !node->type_name.empty()) {
        // struct変数を作成
        if (debug_mode) {
            debug_log_line("[DEBUG_STMT] Creating struct variable: " +
                           node->name + " of type: " + node->type_name);
        }

        interpreter.create_struct_variable(node->name, node->type_name);

        return; // struct変数は専用処理で完了
    }

    // union型の特別処理
    if (!node->type_name.empty() &&
        interpreter.get_type_manager()->is_union_type(node->type_name)) {
        if (debug_mode) {
            debug_log_line("[DEBUG_STMT] Creating union variable: " +
                           node->name + " of type: " + node->type_name);
        }

        // union型変数を作成（初期値なし）
        var.type = TYPE_UNION;
        var.type_name = node->type_name; // union型名を保存
        interpreter.current_scope().variables[node->name] = var;

        // 初期化値がある場合は検証して代入
        if (init_node) {
            executor->execute_union_assignment(node->name, init_node);
        }
        return; // union変数は専用処理で完了
    }

    // 変数を現在のスコープに登録（配列リテラル代入前に必要）
    interpreter.current_scope().variables[node->name] = var;

    if (init_node) {
        // ポインタ型の初期化を最優先で処理 (type_infoまたはis_pointerで判定)
        if (node->type_info == TYPE_POINTER || node->is_pointer) {
            // const安全性チェック:
            // const変数のアドレスを非constポインタで初期化しようとしていないか確認
            if (init_node->node_type == ASTNodeType::AST_UNARY_OP &&
                init_node->op == "ADDRESS_OF" && init_node->left &&
                init_node->left->node_type == ASTNodeType::AST_VARIABLE) {
                Variable *target_var =
                    interpreter.find_variable(init_node->left->name);

                // ケース1: const変数のアドレスを非constポインタで初期化
                if (target_var && target_var->is_const &&
                    !node->is_pointee_const_qualifier) {
                    throw std::runtime_error(
                        "Cannot initialize non-const pointer '" + node->name +
                        "' with address of const variable '" +
                        init_node->left->name + "'. Use 'const " +
                        type_info_to_string(node->pointer_base_type) +
                        "*' instead of '" +
                        type_info_to_string(node->pointer_base_type) + "*'");
                }

                // ケース2: const pointer (const T*) のアドレスを非const double
                // pointer (T**) で初期化
                if (target_var && target_var->type == TYPE_POINTER &&
                    target_var->is_pointee_const && node->pointer_depth >= 2 &&
                    !node->is_pointee_const_qualifier) {
                    throw std::runtime_error(
                        "Cannot initialize non-const double pointer '" +
                        node->name +
                        "' with address of pointer to const (const T*) '" +
                        init_node->left->name +
                        "'. The pointee should be 'const T**', not 'T**'");
                }

                // ケース3: const pointer (T* const) のアドレスを取得する場合
                if (target_var && target_var->type == TYPE_POINTER &&
                    target_var->is_pointer_const && node->pointer_depth >= 2 &&
                    !node->is_pointee_const_qualifier) {
                    throw std::runtime_error(
                        "Cannot initialize non-const double pointer '" +
                        node->name +
                        "' with address of const pointer (T* const) '" +
                        init_node->left->name +
                        "'. Use 'const' qualifier appropriately");
                }
            }

            TypedValue typed_value = interpreter.evaluate_typed(init_node);
            if (debug_mode) {
                std::ostringstream oss;
                oss << "[STMT_EXEC] Pointer initialization: typed_value.value="
                    << typed_value.value << " (0x" << std::hex
                    << typed_value.value << std::dec << ")";
                debug_log_line(oss.str());
            }
            interpreter.current_scope().variables[node->name].value =
                typed_value.value;
            interpreter.current_scope().variables[node->name].type =
                TYPE_POINTER;
            interpreter.current_scope().variables[node->name].is_assigned =
                true;

            // constポインタフラグは既にLine 128-129で設定済み
            if (debug_mode) {
                std::ostringstream oss;
                oss << "[STMT_EXEC] Pointer initialization complete: variables["
                    << node->name << "].value="
                    << interpreter.current_scope().variables[node->name].value
                    << " (0x" << std::hex
                    << interpreter.current_scope().variables[node->name].value
                    << std::dec << "), is_pointer_const="
                    << node->is_pointer_const_qualifier << ", is_pointee_const="
                    << node->is_pointee_const_qualifier;
                debug_log_line(oss.str());
            }
            return; // ポインタ型の初期化はここで完了
        } else if (init_node->node_type == ASTNodeType::AST_TERNARY_OP) {
            // 三項演算子による初期化
            executor->execute_ternary_variable_initialization(node, init_node);
        } else if (var.is_array &&
                   init_node->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 配列リテラル初期化
            interpreter.assign_array_literal(node->name, init_node);
            // 代入後に変数を再取得して更新
            interpreter.current_scope().variables[node->name].is_assigned =
                true;
        } else if (var.is_array &&
                   init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 配列を返す関数呼び出し
            try {
                int64_t value = interpreter.evaluate(init_node);
                // void関数の場合
                interpreter.current_scope().variables[node->name].value = value;
                interpreter.current_scope().variables[node->name].is_assigned =
                    true;
            } catch (const ReturnException &ret) {
                if (debug_mode) {
                    debug_log_line(
                        "[DEBUG_STMT] ReturnException caught: is_array=" +
                        std::to_string(ret.is_array) +
                        ", is_struct=" + std::to_string(ret.is_struct) +
                        ", type=" + std::to_string(static_cast<int>(ret.type)));
                }
                if (ret.is_array) {
                    // 配列戻り値の場合
                    Variable &target_var =
                        interpreter.current_scope().variables[node->name];

                    if (TypeHelpers::isString(ret.type)) {
                        // 文字列配列
                        if (!ret.str_array_3d.empty()) {
                            // 多次元配列かどうかを判定（typedef配列名に[][]が含まれる場合）
                            bool is_multidim =
                                (ret.array_type_name.find("[][]") !=
                                 std::string::npos);
                            if (is_multidim) {
                                // 多次元配列の場合は全要素を展開
                                target_var.array_strings.clear();
                                for (const auto &plane : ret.str_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.array_strings.push_back(
                                                element);
                                        }
                                    }
                                }
                                target_var.array_size =
                                    target_var.array_strings.size();
                            } else if (!ret.str_array_3d[0].empty() &&
                                       !ret.str_array_3d[0][0].empty()) {
                                // 1次元配列の場合
                                target_var.array_strings =
                                    ret.str_array_3d[0][0];
                                target_var.array_size =
                                    target_var.array_strings.size();
                            }
                            target_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + TYPE_STRING);
                        }
                    } else if (ret.type == TYPE_FLOAT ||
                               ret.type == TYPE_DOUBLE ||
                               ret.type == TYPE_QUAD) {
                        // float/double/quad配列
                        if (!ret.double_array_3d.empty()) {
                            // 多次元配列かどうかを判定（typedef配列名に[][]が含まれる場合）
                            bool is_multidim =
                                (ret.array_type_name.find("[][]") !=
                                 std::string::npos);
                            if (is_multidim) {
                                // 多次元float/double配列の場合は全要素を展開してmultidim_array_*_valuesに設定
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.multidim_array_float_values
                                        .clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var
                                                    .multidim_array_float_values
                                                    .push_back(
                                                        static_cast<float>(
                                                            element));
                                            }
                                        }
                                    }
                                    target_var.array_size =
                                        target_var.multidim_array_float_values
                                            .size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.multidim_array_double_values
                                        .clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var
                                                    .multidim_array_double_values
                                                    .push_back(element);
                                            }
                                        }
                                    }
                                    target_var.array_size =
                                        target_var.multidim_array_double_values
                                            .size();
                                } else { // TYPE_QUAD
                                    target_var.multidim_array_quad_values
                                        .clear();
                                    for (const auto &plane :
                                         ret.double_array_3d) {
                                        for (const auto &row : plane) {
                                            for (const auto &element : row) {
                                                target_var
                                                    .multidim_array_quad_values
                                                    .push_back(static_cast<
                                                               long double>(
                                                        element));
                                            }
                                        }
                                    }
                                    target_var.array_size =
                                        target_var.multidim_array_quad_values
                                            .size();
                                }

                                // 配列の次元情報も設定（2D配列の場合）
                                target_var.is_multidimensional = true;
                                target_var.array_values
                                    .clear(); // 1次元配列はクリア

                                // 2次元配列の次元情報を設定
                                if (!ret.double_array_3d.empty() &&
                                    !ret.double_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(
                                        ret.double_array_3d[0].size()); // 行数
                                    target_var.array_dimensions.push_back(
                                        ret.double_array_3d[0][0]
                                            .size()); // 列数
                                }
                            } else if (!ret.double_array_3d[0].empty() &&
                                       !ret.double_array_3d[0][0].empty()) {
                                // 1次元float/double配列の場合
                                if (ret.type == TYPE_FLOAT) {
                                    target_var.array_float_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        target_var.array_float_values.push_back(
                                            static_cast<float>(element));
                                    }
                                    target_var.array_size =
                                        target_var.array_float_values.size();
                                } else if (ret.type == TYPE_DOUBLE) {
                                    target_var.array_double_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        target_var.array_double_values
                                            .push_back(element);
                                    }
                                    target_var.array_size =
                                        target_var.array_double_values.size();
                                } else { // TYPE_QUAD
                                    target_var.array_quad_values.clear();
                                    for (const auto &element :
                                         ret.double_array_3d[0][0]) {
                                        target_var.array_quad_values.push_back(
                                            static_cast<long double>(element));
                                    }
                                    target_var.array_size =
                                        target_var.array_quad_values.size();
                                }
                            }
                            target_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + ret.type);
                        }
                    } else if (ret.is_struct) {
                        // 構造体配列の場合
                        if (debug_mode) {
                            debug_log_line(
                                "[DEBUG_STMT] Struct array return caught, "
                                "calling assign_array_from_return for " +
                                node->name);
                        }
                        interpreter.assign_array_from_return(node->name, ret);
                    } else {
                        // 整数型配列
                        if (!ret.int_array_3d.empty()) {
                            // 多次元配列かどうかを判定（typedef配列名に[][]が含まれる場合）
                            bool is_multidim =
                                (ret.array_type_name.find("[][]") !=
                                 std::string::npos);
                            if (is_multidim) {
                                // 多次元配列の場合は全要素を展開してmultidim_array_valuesに設定
                                target_var.multidim_array_values.clear();
                                for (const auto &plane : ret.int_array_3d) {
                                    for (const auto &row : plane) {
                                        for (const auto &element : row) {
                                            target_var.multidim_array_values
                                                .push_back(element);
                                        }
                                    }
                                }
                                // 配列の次元情報も設定（2D配列の場合）
                                target_var.is_multidimensional = true;
                                target_var.array_size =
                                    target_var.multidim_array_values.size();
                                target_var.array_values
                                    .clear(); // 1次元配列はクリア

                                // 2次元配列の次元情報を設定
                                if (!ret.int_array_3d.empty() &&
                                    !ret.int_array_3d[0].empty()) {
                                    target_var.array_dimensions.clear();
                                    target_var.array_dimensions.push_back(
                                        ret.int_array_3d[0].size()); // 行数
                                    target_var.array_dimensions.push_back(
                                        ret.int_array_3d[0][0].size()); // 列数
                                }
                            } else if (!ret.int_array_3d[0].empty() &&
                                       !ret.int_array_3d[0][0].empty()) {
                                // 1次元配列の場合
                                target_var.array_values =
                                    ret.int_array_3d[0][0];
                                target_var.array_size =
                                    target_var.array_values.size();
                            }
                            target_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + ret.type);
                        }
                    }
                    target_var.is_assigned = true;
                } else {
                    // 非配列戻り値の場合
                    if (ret.is_struct) {
                        // 構造体戻り値の場合
                        // printf("STRUCT_VAR_DECL_DEBUG: Assigning struct
                        // return to variable %s\n", node->name.c_str());

                        // 変数を構造体型に設定
                        Variable &target_var =
                            interpreter.current_scope().variables[node->name];
                        target_var = ret.struct_value; // 構造体をコピー
                        target_var.is_assigned = true;

                        // 個別メンバー変数も作成
                        for (const auto &member :
                             ret.struct_value.struct_members) {
                            std::string member_path =
                                node->name + "." + member.first;
                            interpreter.current_scope().variables[member_path] =
                                member.second;
                        }
                    } else if (TypeHelpers::isString(ret.type)) {
                        interpreter.current_scope()
                            .variables[node->name]
                            .str_value = ret.str_value;
                    } else {
                        // 数値戻り値（float/double/quad対応）
                        if (ret.type == TYPE_FLOAT) {
                            InferredType float_type(TYPE_FLOAT, "float");
                            TypedValue typed_val(ret.double_value, float_type);
                            interpreter.assign_variable(node->name, typed_val,
                                                        ret.type, false);
                        } else if (ret.type == TYPE_DOUBLE) {
                            InferredType double_type(TYPE_DOUBLE, "double");
                            TypedValue typed_val(ret.double_value, double_type);
                            interpreter.assign_variable(node->name, typed_val,
                                                        ret.type, false);
                        } else if (ret.type == TYPE_QUAD) {
                            InferredType quad_type(TYPE_QUAD, "quad");
                            TypedValue typed_val(ret.quad_value, quad_type);
                            interpreter.assign_variable(node->name, typed_val,
                                                        ret.type, false);
                        } else {
                            interpreter.assign_variable(node->name, ret.value,
                                                        ret.type);
                        }
                    }
                    interpreter.current_scope()
                        .variables[node->name]
                        .is_assigned = true;
                }
            }
        } else {
            // 通常の初期化 - TypedValue を使用して float/double を保持
            if (init_node->node_type == ASTNodeType::AST_FUNC_CALL) {
                try {
                    TypedValue typed_value =
                        interpreter.evaluate_typed(init_node);
                    if (TypeHelpers::isString(var.type) &&
                        !typed_value.is_string()) {
                        // 文字列型なのに数値が返された場合
                        throw std::runtime_error(
                            "Type mismatch: expected string but got numeric "
                            "value");
                    } else {
                        interpreter.assign_variable(node->name, typed_value,
                                                    node->type_info, false);
                    }
                    interpreter.current_scope()
                        .variables[node->name]
                        .is_assigned = true;
                } catch (const ReturnException &ret) {
                    if (ret.is_struct) {
                        // 構造体戻り値の場合
                        printf("STRUCT_INIT_DEBUG: Assigning struct return to "
                               "variable %s\n",
                               node->name.c_str());

                        Variable &target_var =
                            interpreter.current_scope().variables[node->name];
                        target_var = ret.struct_value; // 構造体をコピー
                        target_var.is_assigned = true;

                        // 個別メンバー変数も作成
                        for (const auto &member :
                             ret.struct_value.struct_members) {
                            std::string member_path =
                                node->name + "." + member.first;
                            interpreter.current_scope().variables[member_path] =
                                member.second;
                        }
                    } else if (TypeHelpers::isString(ret.type)) {
                        interpreter.current_scope()
                            .variables[node->name]
                            .str_value = ret.str_value;
                        interpreter.current_scope().variables[node->name].type =
                            TYPE_STRING;
                    } else {
                        // 数値戻り値（float/double/quad対応）
                        if (ret.type == TYPE_FLOAT) {
                            InferredType float_type(TYPE_FLOAT, "float");
                            TypedValue typed_val(ret.double_value, float_type);
                            interpreter.assign_variable(node->name, typed_val,
                                                        ret.type, false);
                        } else if (ret.type == TYPE_DOUBLE) {
                            InferredType double_type(TYPE_DOUBLE, "double");
                            TypedValue typed_val(ret.double_value, double_type);
                            interpreter.assign_variable(node->name, typed_val,
                                                        ret.type, false);
                        } else if (ret.type == TYPE_QUAD) {
                            InferredType quad_type(TYPE_QUAD, "quad");
                            TypedValue typed_val(ret.quad_value, quad_type);
                            interpreter.assign_variable(node->name, typed_val,
                                                        ret.type, false);
                        } else {
                            interpreter.assign_variable(node->name, ret.value,
                                                        ret.type);
                        }
                    }
                    interpreter.current_scope()
                        .variables[node->name]
                        .is_assigned = true;
                }
            } else {
                // float/double リテラルを含む全ての初期化式で TypedValue を使用
                TypedValue typed_value = interpreter.evaluate_typed(init_node);

                if (TypeHelpers::isString(var.type)) {
                    interpreter.current_scope()
                        .variables[node->name]
                        .str_value = init_node->str_value;
                } else {
                    // const安全性チェック:
                    // const変数のアドレスを非constポインタで初期化しようとしていないか確認
                    // (ポインタ型だがTYPE_POINTERではない場合のフォールバック)
                    if (node->is_pointer && init_node &&
                        init_node->node_type == ASTNodeType::AST_UNARY_OP &&
                        init_node->op == "ADDRESS_OF" && init_node->left &&
                        init_node->left->node_type ==
                            ASTNodeType::AST_VARIABLE) {
                        Variable *target_var =
                            interpreter.find_variable(init_node->left->name);

                        // ケース1: const変数のアドレスを非constポインタで初期化
                        if (target_var && target_var->is_const &&
                            !node->is_pointee_const_qualifier) {
                            throw std::runtime_error(
                                "Cannot initialize non-const pointer '" +
                                node->name +
                                "' with address of const variable '" +
                                init_node->left->name + "'. Use 'const " +
                                type_info_to_string(node->pointer_base_type) +
                                "*' instead of '" +
                                type_info_to_string(node->pointer_base_type) +
                                "*'");
                        }

                        // ケース2: const pointer (const T*) のアドレスを非const
                        // double pointer (T**) で初期化
                        if (target_var && target_var->type == TYPE_POINTER &&
                            target_var->is_pointee_const &&
                            node->pointer_depth >= 2 &&
                            !node->is_pointee_const_qualifier) {
                            throw std::runtime_error(
                                "Cannot initialize non-const double pointer '" +
                                node->name +
                                "' with address of pointer to const (const T*) "
                                "'" +
                                init_node->left->name +
                                "'. The pointee should be 'const T**', not "
                                "'T**'");
                        }

                        // ケース3: const pointer (T* const)
                        // のアドレスを取得する場合
                        if (target_var && target_var->type == TYPE_POINTER &&
                            target_var->is_pointer_const &&
                            node->pointer_depth >= 2 &&
                            !node->is_pointee_const_qualifier) {
                            throw std::runtime_error(
                                "Cannot initialize non-const double pointer '" +
                                node->name +
                                "' with address of const pointer (T* const) '" +
                                init_node->left->name +
                                "'. Use 'const' qualifier appropriately");
                        }
                    }

                    interpreter.assign_variable(node->name, typed_value,
                                                node->type_info, false);
                }
                interpreter.current_scope().variables[node->name].is_assigned =
                    true;
            }
        }
    }
}

void execute_multiple_var_decl(StatementExecutor *executor,
                               Interpreter &interpreter, const ASTNode *node) {
    // 複数変数宣言の処理
    for (const auto &child : node->children) {
        if (child->node_type == ASTNodeType::AST_VAR_DECL) {
            execute_variable_declaration(executor, interpreter, child.get());
        }
    }
}

} // namespace DeclarationHandlers
