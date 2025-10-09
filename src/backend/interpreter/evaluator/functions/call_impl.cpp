// expression_function_call_impl.cpp
// AST_FUNC_CALL case implementation extracted from expression_evaluator.cpp
// This file contains the complete function call evaluation logic including:
// - Function pointer calls (Form 2: ptr(args))
// - Function pointer chains (getOperation(3)(6, 7))
// - Method calls with receivers
// - Self context setup for methods
// - Parameter binding (references, arrays, function pointers, structs,
// interfaces)
// - Return value handling

#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../common/type_helpers.h"
#include "../../core/error_handler.h"
#include "../../core/interpreter.h"
#include "../../managers/types/manager.h"
#include "evaluator/access/receiver_resolution.h"
#include "evaluator/core/evaluator.h"
#include "evaluator/core/helpers.h"
#include <cstdlib>
#include <iomanip>
#include <sstream>

int64_t ExpressionEvaluator::evaluate_function_call_impl(const ASTNode *node) {
    if (interpreter_.is_debug_mode()) {
        std::cerr << "[DEBUG_IMPL] evaluate_function_call_impl called for: "
                  << node->name << std::endl;
    }

    // 関数を探す
    const ASTNode *func = nullptr;

    // チェーン呼び出しのチェック: func()() (関数ポインタのチェーン)
    // leftが設定されている場合、それは関数ポインタチェーンまたはメソッドチェーンの可能性
    // 関数ポインタチェーンは非常に稀なケースなので、デフォルトでメソッドチェーンとして処理
    // ただし、leftがメソッド呼び出しでない場合（node->left->left ==
    // nullptr）で、
    // かつ戻り値が関数ポインタの場合のみ、関数ポインタチェーンとして処理する
    //
    // 判定ロジック：
    // - node->leftが存在 && node->left->node_type == AST_FUNC_CALL
    // - node->left->left == nullptr (メソッド呼び出しではない)
    // - 実行してみて、is_function_pointer == true
    //
    // それ以外の場合は、後続のis_method_call処理に任せる
    //
    // 後続の処理でメソッドチェーンとして適切に処理される
    // （resolve_method_receiverとcreate_chain_receiver_from_expressionが処理する）

    // Form 2: ptr(args) - 関数ポインタの可能性をチェック
    if (!node->left) { // メソッド呼び出しでない場合
        std::string func_name = node->name;

        if (interpreter_.is_debug_mode()) {
            std::cerr << "[DEBUG_FUNCPTR] Checking function pointer for: "
                      << func_name << std::endl;
        }

        // 1. function_pointersマップから検索
        auto &func_ptrs = interpreter_.current_scope().function_pointers;
        auto it = func_ptrs.find(func_name);
        bool found_in_local = (it != func_ptrs.end());

        if (interpreter_.is_debug_mode()) {
            std::cerr << "[DEBUG_FUNCPTR] found_in_local = " << found_in_local
                      << std::endl;
        }

        // ローカルスコープで見つからない場合、グローバルスコープを検索
        if (!found_in_local) {
            auto &global_func_ptrs =
                interpreter_.get_global_scope().function_pointers;
            it = global_func_ptrs.find(func_name);

            if (interpreter_.is_debug_mode()) {
                bool found_in_global = (it != global_func_ptrs.end());
                std::cerr << "[DEBUG_FUNCPTR] found_in_global = "
                          << found_in_global << std::endl;
            }
        }

        // 2. 変数として定義された関数ポインタもチェック
        if (!found_in_local &&
            it == interpreter_.get_global_scope().function_pointers.end()) {
            Variable *var = interpreter_.find_variable(func_name);

            if (var && var->is_function_pointer) {
                // 変数として定義された関数ポインタをfunction_pointersから取得
                // var->valueにはFunctionPointerへのポインタが格納されている
                FunctionPointer *fp =
                    reinterpret_cast<FunctionPointer *>(var->value);
                if (fp) {
                    // 一時的にfunction_pointersマップに追加（または直接処理）
                    // ここでは直接処理する方が簡単
                    const ASTNode *func_node = fp->function_node;

                    if (interpreter_.is_debug_mode()) {
                        std::cerr << "[FUNC_PTR] Form 2 call (variable): "
                                  << func_name << " -> " << fp->function_name
                                  << std::endl;
                    }

                    // 引数を評価して関数を呼び出す処理に直接進む
                    std::vector<int64_t> arg_values;
                    std::vector<std::string> arg_strings;

                    for (const auto &arg : node->arguments) {
                        TypedValue typed_val =
                            interpreter_.evaluate_typed(arg.get());
                        arg_values.push_back(typed_val.value);
                        if (TypeHelpers::isString(typed_val)) {
                            arg_strings.push_back(typed_val.string_value);
                        }
                    }

                    // 関数を呼び出し
                    interpreter_.push_interpreter_scope();

                    // 仮引数と実引数をバインド
                    size_t param_idx = 0;
                    for (const auto &param : func_node->parameters) {
                        if (param_idx >= arg_values.size()) {
                            throw std::runtime_error(
                                "Too few arguments for function pointer call");
                        }

                        std::string param_name = param->name;
                        TypeInfo param_type = param->type_info;
                        bool is_unsigned = param->is_unsigned;

                        if (param_type == TYPE_STRING) {
                            interpreter_.assign_variable(
                                param_name, arg_strings[param_idx]);
                        } else {
                            interpreter_.assign_function_parameter(
                                param_name, arg_values[param_idx], param_type,
                                is_unsigned);
                        }

                        param_idx++;
                    }

                    // 関数本体を実行
                    int64_t result = 0;
                    try {
                        if (func_node->body) {
                            interpreter_.exec_statement(func_node->body.get());
                        }
                    } catch (const ReturnException &ret) {
                        interpreter_.pop_interpreter_scope();
                        // 戻り値を取得
                        if (ret.is_function_pointer ||
                            TypeHelpers::isString(ret.type) || ret.is_struct ||
                            ret.is_array) {
                            throw ret; // 複雑な型の場合はexceptionとして伝播
                        } else {
                            result = ret.value;
                        }
                        return result;
                    }

                    interpreter_.pop_interpreter_scope();
                    return result;
                }
            }
        }

        // 関数ポインタが見つかった場合（マップから）
        if (found_in_local ||
            it != interpreter_.get_global_scope().function_pointers.end()) {
            const FunctionPointer &func_ptr = it->second;
            const ASTNode *func_node = func_ptr.function_node;

            if (debug_mode) {
                std::cerr << "[FUNC_PTR] Form 2 call: " << func_name << " -> "
                          << func_ptr.function_name << std::endl;
            }

            // 引数を評価
            std::vector<int64_t> arg_values;
            std::vector<std::string> arg_strings;

            for (const auto &arg : node->arguments) {
                TypedValue typed_val = interpreter_.evaluate_typed(arg.get());
                arg_values.push_back(typed_val.value);
                if (TypeHelpers::isString(typed_val)) {
                    arg_strings.push_back(typed_val.string_value);
                }
            }

            // 関数を呼び出し
            interpreter_.push_interpreter_scope();

            // 仮引数と実引数をバインド
            size_t param_idx = 0;
            for (const auto &param : func_node->parameters) {
                if (param_idx >= arg_values.size()) {
                    throw std::runtime_error(
                        "Too few arguments for function pointer call");
                }

                std::string param_name = param->name;
                TypeInfo param_type = param->type_info;
                bool is_unsigned = param->is_unsigned;

                if (param_type == TYPE_STRING) {
                    interpreter_.assign_variable(param_name,
                                                 arg_strings[param_idx]);
                } else {
                    interpreter_.assign_function_parameter(
                        param_name, arg_values[param_idx], param_type,
                        is_unsigned);
                }

                param_idx++;
            }

            // 関数本体を実行
            int64_t result = 0;
            try {
                if (func_node->body) {
                    interpreter_.exec_statement(func_node->body.get());
                }
            } catch (const ReturnException &ret) {
                interpreter_.pop_interpreter_scope();
                // 戻り値を取得
                if (ret.is_function_pointer ||
                    TypeHelpers::isString(ret.type) || ret.is_struct ||
                    ret.is_array) {
                    throw ret; // 複雑な型の場合はexceptionとして伝播
                } else {
                    result = ret.value;
                }
                return result;
            }

            interpreter_.pop_interpreter_scope();
            return result;
        }
    }

    // 通常の関数呼び出し
    // 通常の関数呼び出し
    bool is_method_call =
        (node->left != nullptr); // レシーバーがある場合はメソッド呼び出し
    bool has_receiver = is_method_call;
    std::string receiver_name;
    MethodReceiverResolution receiver_resolution;
    bool impl_context_active = false; // implコンテキストが有効かどうか
    struct MethodCallContext {
        bool uses_temp_receiver = false;
        std::string temp_variable_name;
        std::shared_ptr<ReturnException> chain_value;
        Variable concrete_receiver;
    } method_context;

    auto capture_numeric_return = [&](const TypedValue &typed_value) {
        if (node) {
            last_captured_function_value_ = std::make_pair(node, typed_value);
        }
    };

    if (is_method_call) {
        debug_msg(DebugMsgId::METHOD_CALL_START, node->name.c_str());
        receiver_resolution =
            ReceiverResolutionHelpers::resolve_method_receiver(node->left.get(),
                                                               *this);

        if (receiver_resolution.kind ==
                MethodReceiverResolution::Kind::Direct &&
            receiver_resolution.variable_ptr) {
            receiver_name = receiver_resolution.canonical_name;
        } else if (receiver_resolution.kind ==
                       MethodReceiverResolution::Kind::Chain &&
                   receiver_resolution.chain_value) {
            method_context.chain_value = receiver_resolution.chain_value;

            const ReturnException &chain_ret = *receiver_resolution.chain_value;
            if (chain_ret.is_array) {
                throw chain_ret;
            }

            // 関数ポインタチェーン: getOperation(3)(6, 7)のようなケース
            if (chain_ret.is_function_pointer) {
                if (debug_mode) {
                    std::cerr << "[FUNC_PTR_CHAIN] Function pointer chain "
                                 "detected, value="
                              << chain_ret.value << std::endl;
                }

                // function_pointersマップから関数ポインタ情報を探す
                const FunctionPointer *found_ptr = nullptr;

                // 現在のスコープを検索
                for (const auto &pair :
                     interpreter_.current_scope().function_pointers) {
                    Variable *var = interpreter_.find_variable(pair.first);
                    if (var && var->value == chain_ret.value) {
                        found_ptr = &pair.second;
                        break;
                    }
                }

                // グローバルスコープも検索
                if (!found_ptr) {
                    for (const auto &pair :
                         interpreter_.get_global_scope().function_pointers) {
                        Variable *var = interpreter_.find_variable(pair.first);
                        if (var && var->value == chain_ret.value) {
                            found_ptr = &pair.second;
                            break;
                        }
                    }
                }

                if (!found_ptr) {
                    throw std::runtime_error(
                        "Function pointer chain: pointer not found in "
                        "function_pointers map");
                }

                const ASTNode *func_node = found_ptr->function_node;

                if (debug_mode) {
                    std::cerr << "[FUNC_PTR_CHAIN] Calling function: "
                              << found_ptr->function_name << std::endl;
                }

                // 引数を評価
                std::vector<int64_t> arg_values;
                std::vector<std::string> arg_strings;

                for (const auto &arg : node->arguments) {
                    TypedValue typed_val =
                        interpreter_.evaluate_typed(arg.get());
                    arg_values.push_back(typed_val.value);
                    if (TypeHelpers::isString(typed_val)) {
                        arg_strings.push_back(typed_val.string_value);
                    }
                }

                // 関数を呼び出し
                interpreter_.push_interpreter_scope();

                // 仮引数と実引数をバインド
                size_t param_idx = 0;
                for (const auto &param : func_node->parameters) {
                    if (param_idx >= arg_values.size()) {
                        throw std::runtime_error("Too few arguments for "
                                                 "function pointer chain call");
                    }

                    std::string param_name = param->name;
                    TypeInfo param_type = param->type_info;
                    bool is_unsigned = param->is_unsigned;

                    if (param_type == TYPE_STRING) {
                        interpreter_.assign_variable(param_name,
                                                     arg_strings[param_idx]);
                    } else {
                        interpreter_.assign_function_parameter(
                            param_name, arg_values[param_idx], param_type,
                            is_unsigned);
                    }

                    param_idx++;
                }

                // 関数本体を実行
                int64_t result = 0;
                try {
                    if (func_node->body) {
                        interpreter_.exec_statement(func_node->body.get());
                    }
                } catch (const ReturnException &ret) {
                    interpreter_.pop_interpreter_scope();
                    if (ret.is_function_pointer ||
                        TypeHelpers::isString(ret.type) || ret.is_struct ||
                        ret.is_array) {
                        throw ret; // 複雑な型の場合はexceptionとして伝播
                    } else {
                        result = ret.value;
                    }
                    return result;
                }

                interpreter_.pop_interpreter_scope();
                return result;
            }

            method_context.uses_temp_receiver = true;
            method_context.temp_variable_name =
                "__chain_receiver_" + std::to_string(rand() % 10000);

            Variable temp_receiver;
            temp_receiver.is_assigned = true;

            if (TypeHelpers::isStruct(chain_ret.type) ||
                TypeHelpers::isInterface(chain_ret.type) ||
                chain_ret.is_struct) {
                temp_receiver = chain_ret.struct_value;

                if (TypeHelpers::isInterface(temp_receiver.type)) {
                    bool has_struct_members =
                        temp_receiver.is_struct ||
                        !temp_receiver.struct_members.empty();
                    if (has_struct_members) {
                        temp_receiver.type = TYPE_STRUCT;
                        temp_receiver.is_struct = true;
                    } else {
                        TypeInfo resolved = TYPE_UNKNOWN;
                        if (!temp_receiver.struct_type_name.empty()) {
                            resolved = interpreter_.get_type_manager()
                                           ->string_to_type_info(
                                               temp_receiver.struct_type_name);
                        }
                        if (resolved == TYPE_UNKNOWN &&
                            temp_receiver.current_type != TYPE_UNKNOWN) {
                            resolved = temp_receiver.current_type;
                        }
                        if (resolved == TYPE_UNKNOWN) {
                            resolved = TYPE_INT;
                        }
                        // Interface型の場合、現在保持している値を保持する必要がある
                        // (temp_receiverはchain_ret.struct_valueからコピーされており、既に正しい値を持っている)
                        temp_receiver.type = resolved;
                        temp_receiver.is_struct = false;
                    }
                } else if (temp_receiver.type != TYPE_STRUCT &&
                           temp_receiver.is_struct) {
                    temp_receiver.type = TYPE_STRUCT;
                }

                if (TypeHelpers::isStruct(temp_receiver.type)) {
                    temp_receiver.is_struct = true;
                }
            } else if (TypeHelpers::isString(chain_ret.type)) {
                temp_receiver.type = TYPE_STRING;
                temp_receiver.str_value = chain_ret.str_value;
            } else {
                temp_receiver.type = chain_ret.type;
                temp_receiver.value = chain_ret.value;
            }

            method_context.concrete_receiver = temp_receiver;
            interpreter_.add_temp_variable(method_context.temp_variable_name,
                                           temp_receiver);
            receiver_name = method_context.temp_variable_name;
            receiver_resolution.kind = MethodReceiverResolution::Kind::Direct;
            receiver_resolution.variable_ptr =
                interpreter_.find_variable(receiver_name);
        } else {
            throw std::runtime_error("Invalid method receiver");
        }

        Variable *receiver_var = receiver_resolution.variable_ptr;
        if (!receiver_var) {
            receiver_var = interpreter_.find_variable(receiver_name);
        }
        if (!receiver_var) {
            throw std::runtime_error("Undefined receiver: " + receiver_name);
        }
        debug_msg(DebugMsgId::METHOD_CALL_RECEIVER_FOUND,
                  receiver_name.c_str());
        debug_print("RECEIVER_DEBUG: Looking for receiver '%s'\n",
                    receiver_name.c_str());

        std::string type_name;

        auto resolve_struct_like_type =
            [&](const Variable &var) -> std::string {
            if (!var.struct_type_name.empty()) {
                return var.struct_type_name;
            }
            if (!var.implementing_struct.empty()) {
                return var.implementing_struct;
            }
            if (var.type == TYPE_UNION && var.current_type != TYPE_UNKNOWN) {
                return std::string(::type_info_to_string(var.current_type));
            }
            return std::string();
        };

        // Check if receiver is a pointer type
        if (receiver_var->type == TYPE_POINTER) {
            // Dereference the pointer to get the actual struct
            int64_t ptr_value = receiver_var->value;
            if (ptr_value == 0) {
                throw std::runtime_error(
                    "Null pointer dereference in method call");
            }
            Variable *pointed_struct = reinterpret_cast<Variable *>(ptr_value);
            if (pointed_struct) {
                if (debug_mode) {
                    debug_print("POINTER_DEREF_BEFORE: type=%d, is_struct=%d, "
                                "struct_type_name='%s'\n",
                                static_cast<int>(pointed_struct->type),
                                pointed_struct->is_struct ? 1 : 0,
                                pointed_struct->struct_type_name.c_str());
                }

                type_name = resolve_struct_like_type(*pointed_struct);
                if (type_name.empty() && (pointed_struct->type == TYPE_STRUCT ||
                                          pointed_struct->is_struct)) {
                    type_name = pointed_struct->struct_type_name;
                }

                // Ensure pointed_struct is recognized as a struct (but don't
                // overwrite TYPE_INTERFACE) Interface型の場合は型情報を保持する
                if (pointed_struct->type != TYPE_INTERFACE &&
                    pointed_struct->interface_name.empty() &&
                    (!pointed_struct->struct_type_name.empty() ||
                     !pointed_struct->struct_members.empty())) {
                    pointed_struct->type = TYPE_STRUCT;
                    pointed_struct->is_struct = true;
                }

                // Update receiver_var to point to the dereferenced struct
                receiver_var = pointed_struct;
                receiver_resolution.variable_ptr = pointed_struct;
                debug_print("POINTER_METHOD: Dereferenced pointer, type='%s', "
                            "is_struct=%d\n",
                            type_name.c_str(),
                            pointed_struct->is_struct ? 1 : 0);
            }
        }

        if (type_name.empty() &&
            (receiver_var->type >= TYPE_ARRAY_BASE || receiver_var->is_array)) {
            type_name = resolve_struct_like_type(*receiver_var);
            if (type_name.empty()) {
                TypeInfo base_type = TYPE_UNKNOWN;
                if (receiver_var->type >= TYPE_ARRAY_BASE) {
                    base_type = static_cast<TypeInfo>(receiver_var->type -
                                                      TYPE_ARRAY_BASE);
                } else if (receiver_var->array_type_info.base_type !=
                           TYPE_UNKNOWN) {
                    base_type = receiver_var->array_type_info.base_type;
                }
                if (base_type == TYPE_UNKNOWN) {
                    base_type = TYPE_INT;
                }
                type_name =
                    std::string(::type_info_to_string(base_type)) + "[]";
            }
        } else if (type_name.empty() && (receiver_var->type == TYPE_STRUCT ||
                                         receiver_var->is_struct)) {
            type_name = resolve_struct_like_type(*receiver_var);
        } else if (type_name.empty() &&
                   (receiver_var->type == TYPE_INTERFACE ||
                    !receiver_var->interface_name.empty())) {
            type_name = resolve_struct_like_type(*receiver_var);
            if (type_name.empty()) {
                type_name = receiver_var->interface_name;
            }
            debug_msg(DebugMsgId::METHOD_CALL_INTERFACE, node->name.c_str(),
                      type_name.c_str());
        } else {
            type_name = resolve_struct_like_type(*receiver_var);
            if (type_name.empty()) {
                type_name =
                    std::string(::type_info_to_string(receiver_var->type));
            }
        }

        if (type_name.empty()) {
            type_name = std::string(::type_info_to_string(receiver_var->type));
        }

        std::string method_key = type_name + "::" + node->name;
        auto &global_scope = interpreter_.get_global_scope();
        auto it = global_scope.functions.find(method_key);
        if (it != global_scope.functions.end()) {
            func = it->second;
        } else {
            for (const auto &impl_def : interpreter_.get_impl_definitions()) {
                if (impl_def.struct_name == type_name) {
                    std::string method_full_name = impl_def.interface_name +
                                                   "_" + impl_def.struct_name +
                                                   "_" + node->name;
                    auto it2 = global_scope.functions.find(method_full_name);
                    if (it2 != global_scope.functions.end()) {
                        func = it2->second;
                        break;
                    }
                }
            }
        }
    } else {
        auto &global_scope = interpreter_.get_global_scope();
        auto it = global_scope.functions.find(node->name);
        if (it != global_scope.functions.end()) {
            func = it->second;
        }
    }

    if (!func) {
        // 組み込み関数のチェック
        if (node->name == "hex" && !is_method_call) {
            // hex(num) - 整数を16進数文字列に変換
            if (node->arguments.size() != 1) {
                throw std::runtime_error("hex() requires exactly 1 argument");
            }

            int64_t value =
                interpreter_.eval_expression(node->arguments[0].get());
            uint64_t unsigned_value = static_cast<uint64_t>(value);

            // ポインタメタデータのタグビット（最上位ビット）を除去
            if (unsigned_value & (1ULL << 63)) {
                unsigned_value &= ~(1ULL << 63);
            }

            // 16進数文字列を生成
            std::ostringstream oss;
            oss << "0x" << std::hex << unsigned_value;
            std::string hex_str = oss.str();

            // 文字列を返す（ReturnExceptionを使用）
            throw ReturnException(hex_str);
        }

        if (is_method_call) {
            std::string debug_type_name;
            if (is_method_call) {
                if (!receiver_name.empty()) {
                    Variable *debug_receiver =
                        interpreter_.find_variable(receiver_name);
                    if (!debug_receiver && receiver_resolution.variable_ptr) {
                        debug_receiver = receiver_resolution.variable_ptr;
                    }
                    if (debug_receiver) {
                        if (!debug_receiver->struct_type_name.empty()) {
                            debug_type_name = debug_receiver->struct_type_name;
                        } else {
                            debug_type_name = std::string(
                                ::type_info_to_string(debug_receiver->type));
                        }
                    }
                }
            }
            std::cerr << "[METHOD_LOOKUP_FAIL] receiver='" << receiver_name
                      << "' type='" << debug_type_name << "' method='"
                      << node->name << "'" << std::endl;
        }
        throw std::runtime_error("Undefined function: " + node->name);
    }

    if (is_method_call && !receiver_name.empty()) {
        std::string private_check_name = receiver_name;

        if (private_check_name != "self") {
            Variable *receiver_var =
                interpreter_.find_variable(private_check_name);
            if (!receiver_var && receiver_resolution.variable_ptr) {
                receiver_var = receiver_resolution.variable_ptr;
            }
            if (receiver_var) {
                std::string type_name;
                if (receiver_var->type == TYPE_STRUCT) {
                    type_name = receiver_var->struct_type_name;
                } else if (!receiver_var->interface_name.empty()) {
                    type_name = receiver_var->struct_type_name;
                } else {
                    type_name =
                        std::string(::type_info_to_string(receiver_var->type));
                }
                for (const auto &impl_def :
                     interpreter_.get_impl_definitions()) {
                    if (impl_def.struct_name == type_name) {
                        for (const auto &method : impl_def.methods) {
                            if (method->name == node->name &&
                                method->is_private_method) {
                                throw std::runtime_error(
                                    "Cannot access private method '" +
                                    node->name +
                                    "' from outside the impl block");
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    // 新しいスコープを作成
    auto cleanup_method_context = [&]() {
        if (method_context.uses_temp_receiver &&
            !method_context.temp_variable_name.empty()) {
            Variable *temp_var =
                interpreter_.find_variable(method_context.temp_variable_name);
            if (temp_var && method_context.chain_value) {
                if (temp_var->type == TYPE_STRUCT || temp_var->is_struct) {
                    method_context.chain_value->struct_value = *temp_var;
                    method_context.chain_value->struct_value.type = TYPE_STRUCT;
                    method_context.chain_value->struct_value.is_struct = true;
                    method_context.chain_value->is_struct = true;
                    method_context.chain_value->type = TYPE_STRUCT;
                } else if (temp_var->type == TYPE_STRING) {
                    method_context.chain_value->str_value = temp_var->str_value;
                    method_context.chain_value->type = TYPE_STRING;
                    method_context.chain_value->is_struct = false;
                    method_context.chain_value->is_array = false;
                } else {
                    method_context.chain_value->value = temp_var->value;
                    method_context.chain_value->type = temp_var->type;
                    method_context.chain_value->is_struct = false;
                    method_context.chain_value->is_array = false;
                }
            }
            interpreter_.remove_temp_variable(
                method_context.temp_variable_name);
            method_context.uses_temp_receiver = false;
        }
    };
    interpreter_.push_scope();
    bool method_scope_active = true;

    // メソッド呼び出しの場合、selfコンテキストを設定
    bool used_resolution_ptr = false; // Track if we used pointer dereference
    Variable *dereferenced_struct_ptr =
        nullptr; // Store the dereferenced struct pointer

    if (is_method_call) {
        Variable *receiver_var = nullptr;

        // Prioritize receiver_resolution.variable_ptr (e.g., after pointer
        // dereference)
        used_resolution_ptr = false;
        if (debug_mode) {
            debug_print(
                "SELF_SETUP_RESOLUTION: receiver_resolution.variable_ptr=%p, "
                "receiver_name='%s'\n",
                static_cast<void *>(receiver_resolution.variable_ptr),
                receiver_name.c_str());
        }
        if (receiver_resolution.variable_ptr) {
            receiver_var = receiver_resolution.variable_ptr;
            used_resolution_ptr = true;
            dereferenced_struct_ptr =
                receiver_resolution.variable_ptr; // Save pointer for writeback
            if (debug_mode) {
                debug_print("SELF_SETUP_USING_RESOLUTION: type=%d, "
                            "is_struct=%d, struct_type_name='%s'\n",
                            static_cast<int>(receiver_var->type),
                            receiver_var->is_struct ? 1 : 0,
                            receiver_var->struct_type_name.c_str());
            }
        } else if (!receiver_name.empty()) {
            receiver_var = interpreter_.find_variable(receiver_name);
        } else if (node->left &&
                   (node->left->node_type == ASTNodeType::AST_VARIABLE ||
                    node->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
            receiver_var = interpreter_.find_variable(node->left->name);
            if (receiver_name.empty()) {
                receiver_name = node->left->name;
            }
        }

        if (!receiver_var) {
            std::string error_name = receiver_name;
            if (error_name.empty() && node->left) {
                error_name = node->left->name;
            }
            throw std::runtime_error("Receiver variable not found: " +
                                     error_name);
        }

        // Only sync if receiver_var was not from pointer dereference
        // (i.e., not from receiver_resolution.variable_ptr)
        if (!used_resolution_ptr && !receiver_name.empty() &&
            (receiver_var->type == TYPE_STRUCT ||
             receiver_var->type == TYPE_INTERFACE || receiver_var->is_struct)) {
            interpreter_.sync_struct_members_from_direct_access(receiver_name);
            Variable *synced_var = interpreter_.find_variable(receiver_name);
            if (synced_var) {
                receiver_var = synced_var;
            }
        }

        auto &current_scope = interpreter_.get_current_scope();

        // Copy receiver to self
        current_scope.variables["self"] = *receiver_var;

        // Ensure self has correct type info after copy
        Variable &self_var = current_scope.variables["self"];
        if (debug_mode) {
            debug_print("SELF_SETUP_BEFORE: self.type=%d, self.is_struct=%d, "
                        "struct_type_name='%s', struct_members=%zu\n",
                        static_cast<int>(self_var.type),
                        self_var.is_struct ? 1 : 0,
                        self_var.struct_type_name.c_str(),
                        self_var.struct_members.size());
        }
        // Only mark as struct if it actually has struct members or is already
        // TYPE_STRUCT Don't mark primitive types as struct even if they have a
        // type_name
        if (TypeHelpers::isStruct(self_var.type) ||
            !self_var.struct_members.empty()) {
            self_var.type = TYPE_STRUCT;
            self_var.is_struct = true;
        }
        if (debug_mode) {
            debug_print("SELF_SETUP_AFTER: self.type=%d, self.is_struct=%d\n",
                        static_cast<int>(self_var.type),
                        self_var.is_struct ? 1 : 0);
        }

        if (!receiver_name.empty()) {
            Variable receiver_info;
            receiver_info.type = TYPE_STRING;
            receiver_info.str_value = receiver_name;
            receiver_info.is_assigned = true;
            current_scope.variables["__self_receiver__"] = receiver_info;
            debug_msg(DebugMsgId::METHOD_CALL_SELF_CONTEXT_SET,
                      receiver_name.c_str());
        }

        if (receiver_var->type == TYPE_STRUCT ||
            receiver_var->type == TYPE_INTERFACE || receiver_var->is_struct) {
            for (const auto &member_pair : receiver_var->struct_members) {
                const std::string &member_name = member_pair.first;
                std::string self_member_path = "self." + member_name;
                Variable member_value = member_pair.second;

                if (!receiver_name.empty()) {
                    if (Variable *direct_member_var =
                            interpreter_.find_variable(receiver_name + "." +
                                                       member_name)) {
                        member_value = *direct_member_var;
                    } else {
                        try {
                            if (Variable *struct_member =
                                    interpreter_.get_struct_member(
                                        receiver_name, member_name)) {
                                member_value = *struct_member;
                            }
                        } catch (...) {
                            // ignore fallback failures
                        }
                    }
                }

                if (member_pair.second.is_multidimensional) {
                    member_value.is_multidimensional = true;
                    member_value.array_dimensions =
                        member_pair.second.array_dimensions;
                    member_value.multidim_array_values =
                        member_pair.second.multidim_array_values;
                    debug_print(
                        "SELF_SETUP: Preserved multidimensional info for %s "
                        "(dimensions: %zu, values: %zu)\n",
                        self_member_path.c_str(),
                        member_pair.second.array_dimensions.size(),
                        member_pair.second.multidim_array_values.size());
                }

                if (member_value.is_array) {
                    const bool is_string_array =
                        TypeHelpers::isString(member_value.type);

                    int total_elements = member_value.array_size;
                    if (total_elements <= 0) {
                        if (member_value.is_multidimensional &&
                            !member_value.multidim_array_values.empty()) {
                            total_elements = static_cast<int>(
                                member_value.multidim_array_values.size());
                        } else if (!member_value.array_values.empty()) {
                            total_elements = static_cast<int>(
                                member_value.array_values.size());
                        } else if (!member_value.array_dimensions.empty()) {
                            total_elements = 1;
                            for (int dim_size : member_value.array_dimensions) {
                                if (dim_size == 0) {
                                    total_elements = 0;
                                    break;
                                }
                                total_elements *= dim_size;
                            }
                        }
                    }

                    if (total_elements < 0) {
                        total_elements = 0;
                    }

                    member_value.array_size = total_elements;

                    if (!is_string_array) {
                        if (member_value.is_multidimensional) {
                            if (member_value.array_values.size() <
                                member_value.multidim_array_values.size()) {
                                member_value.array_values =
                                    member_value.multidim_array_values;
                            } else if (member_value.array_values.empty() &&
                                       !member_value.multidim_array_values
                                            .empty()) {
                                member_value.array_values =
                                    member_value.multidim_array_values;
                            }
                        }
                        if (member_value.array_values.size() <
                            static_cast<size_t>(total_elements)) {
                            member_value.array_values.resize(total_elements, 0);
                        }
                        if (member_value.is_multidimensional &&
                            member_value.multidim_array_values.size() <
                                static_cast<size_t>(total_elements)) {
                            member_value.multidim_array_values.resize(
                                total_elements, 0);
                        }
                    } else {
                        if (member_value.array_strings.size() <
                            static_cast<size_t>(total_elements)) {
                            member_value.array_strings.resize(total_elements);
                        }
                    }

                    for (int idx = 0; idx < total_elements; ++idx) {
                        std::string element_path =
                            self_member_path + "[" + std::to_string(idx) + "]";
                        Variable element_var;
                        bool element_assigned = false;

                        if (!receiver_name.empty()) {
                            std::string receiver_element_path =
                                receiver_name + "." + member_name + "[" +
                                std::to_string(idx) + "]";
                            if (Variable *receiver_element =
                                    interpreter_.find_variable(
                                        receiver_element_path)) {
                                element_var = *receiver_element;
                                element_assigned = true;
                            }
                        }

                        if (!element_assigned) {
                            element_var.type = is_string_array
                                                   ? TYPE_STRING
                                                   : member_value.type;
                            element_var.is_assigned = true;
                            if (is_string_array) {
                                std::string value =
                                    (idx <
                                     static_cast<int>(
                                         member_value.array_strings.size()))
                                        ? member_value.array_strings[idx]
                                        : std::string();
                                element_var.str_value = value;
                            } else {
                                int64_t value = 0;
                                if (member_value.is_multidimensional &&
                                    idx < static_cast<int>(
                                              member_value.multidim_array_values
                                                  .size())) {
                                    value =
                                        member_value.multidim_array_values[idx];
                                } else if (idx < static_cast<int>(
                                                     member_value.array_values
                                                         .size())) {
                                    value = member_value.array_values[idx];
                                }
                                element_var.value = value;
                            }
                        }

                        current_scope.variables[element_path] = element_var;

                        if (is_string_array) {
                            if (idx >= static_cast<int>(
                                           member_value.array_strings.size())) {
                                member_value.array_strings.resize(idx + 1);
                            }
                            member_value.array_strings[idx] =
                                element_var.str_value;
                        } else {
                            if (idx >= static_cast<int>(
                                           member_value.array_values.size())) {
                                member_value.array_values.resize(idx + 1);
                            }
                            member_value.array_values[idx] = element_var.value;
                            if (member_value.is_multidimensional) {
                                if (idx >=
                                    static_cast<int>(
                                        member_value.multidim_array_values
                                            .size())) {
                                    member_value.multidim_array_values.resize(
                                        idx + 1);
                                }
                                member_value.multidim_array_values[idx] =
                                    element_var.value;
                            }
                        }
                    }
                }

                current_scope.variables[self_member_path] = member_value;
                debug_print("SELF_SETUP: Created %s\n",
                            self_member_path.c_str());

                // メンバーが構造体の場合、そのネストメンバーも再帰的に作成
                if (TypeHelpers::isStruct(member_value.type) ||
                    member_value.is_struct) {
                    std::string nested_base_name =
                        receiver_name + "." + member_name;

                    // ネストした構造体の個別変数を作成
                    for (const auto &nested_member_pair :
                         member_value.struct_members) {
                        const std::string &nested_member_name =
                            nested_member_pair.first;
                        std::string nested_self_path =
                            self_member_path + "." + nested_member_name;
                        std::string nested_receiver_path =
                            nested_base_name + "." + nested_member_name;

                        Variable nested_member_value =
                            nested_member_pair.second;

                        // receiver側の個別変数から値を取得
                        if (Variable *nested_direct_var =
                                interpreter_.find_variable(
                                    nested_receiver_path)) {
                            nested_member_value = *nested_direct_var;
                        }

                        current_scope.variables[nested_self_path] =
                            nested_member_value;
                        debug_print(
                            "SELF_SETUP: Created nested member %s = %lld\n",
                            nested_self_path.c_str(),
                            nested_member_value.value);
                    }
                }
            }
            debug_msg(DebugMsgId::METHOD_CALL_SELF_MEMBER_SETUP);
        }
    }

    // 現在の関数名を設定
    std::string prev_function_name = interpreter_.current_function_name;
    interpreter_.current_function_name = node->name;

    debug_msg(DebugMsgId::METHOD_CALL_EXECUTE, node->name.c_str());

    try {
        // パラメータの評価と設定
        if (func->parameters.size() != node->arguments.size()) {
            if (debug_mode) {
                std::cerr << "[FUNC_CALL] Argument count mismatch: function '"
                          << node->name << "' expected "
                          << func->parameters.size() << " args, got "
                          << node->arguments.size() << std::endl;
                std::cerr << "[FUNC_CALL] Parameters:" << std::endl;
                for (const auto &param : func->parameters) {
                    std::cerr << "  - " << param->name
                              << " type_info=" << param->type_info
                              << " is_array=" << param->is_array
                              << " is_reference=" << param->is_reference
                              << std::endl;
                }
                std::cerr << "[FUNC_CALL] Arguments:" << std::endl;
                for (const auto &arg : node->arguments) {
                    std::cerr
                        << "  - node_type=" << static_cast<int>(arg->node_type)
                        << " type_info=" << arg->type_info
                        << " is_array=" << arg->is_array << " name='"
                        << arg->name << "'" << std::endl;
                }
            }
            throw std::runtime_error("Argument count mismatch for function: " +
                                     node->name);
        }

        for (size_t i = 0; i < func->parameters.size(); i++) {
            const auto &param = func->parameters[i];
            const auto &arg = node->arguments[i];

            // 関数ポインタパラメータのサポート
            if (param->type_info == TYPE_POINTER &&
                arg->node_type == ASTNodeType::AST_UNARY_OP &&
                arg->op == "ADDRESS_OF" && arg->is_function_address) {
                // 引数が関数アドレス（&func形式）の場合
                // まず関数が実際に存在するかを確認
                std::string func_name = arg->function_address_name;
                const ASTNode *target_func =
                    interpreter_.find_function(func_name);

                // 関数が見つかった場合のみ関数ポインタとして処理
                if (target_func) {
                    // 関数ポインタとして登録
                    FunctionPointer func_ptr(target_func, func_name,
                                             target_func->type_info);
                    interpreter_.current_scope()
                        .function_pointers[param->name] = func_ptr;

                    // 変数としても登録（値は関数ノードの実際のメモリアドレス）
                    int64_t func_address =
                        reinterpret_cast<int64_t>(target_func);
                    interpreter_.assign_function_parameter(
                        param->name, func_address, TYPE_POINTER, false);

                    // 変数に関数ポインタフラグを設定
                    Variable *param_var =
                        interpreter_.find_variable(param->name);
                    if (param_var) {
                        param_var->is_function_pointer = true;
                        param_var->function_pointer_name = func_name;
                    }

                    if (debug_mode) {
                        std::cerr << "[FUNC_CALL] Registered function pointer "
                                     "argument: "
                                  << param->name << " = &" << func_name
                                  << std::endl;
                    }

                    continue; // 次のパラメータへ
                }
                // 関数が見つからない場合は通常のポインタパラメータとして処理を継続
            }

            // 参照パラメータのサポート
            if (param->is_reference) {
                // 参照パラメータは変数のみを受け取れる
                if (arg->node_type != ASTNodeType::AST_VARIABLE &&
                    arg->node_type != ASTNodeType::AST_IDENTIFIER) {
                    throw std::runtime_error(
                        "Reference parameter '" + param->name +
                        "' requires a variable, not an expression");
                }

                // 引数の変数を取得
                Variable *source_var = interpreter_.find_variable(arg->name);
                if (!source_var) {
                    throw std::runtime_error(
                        "Undefined variable for reference parameter: " +
                        arg->name);
                }

                // 参照変数を作成（参照先のポインタを保存）
                Variable ref_var;
                ref_var.is_reference = true;
                ref_var.is_assigned = true;
                ref_var.type = source_var->type;
                ref_var.value = reinterpret_cast<int64_t>(source_var);

                // 構造体型情報をコピー
                ref_var.struct_type_name = source_var->struct_type_name;
                ref_var.is_struct = source_var->is_struct;
                ref_var.type_name = source_var->type_name;
                ref_var.interface_name = source_var->interface_name;
                ref_var.implementing_struct = source_var->implementing_struct;

                // ポインタ型情報をコピー
                ref_var.is_pointer = source_var->is_pointer;
                ref_var.pointer_depth = source_var->pointer_depth;
                ref_var.pointer_base_type = source_var->pointer_base_type;
                ref_var.pointer_base_type_name =
                    source_var->pointer_base_type_name;

                // 参照の連鎖対応（source_varも参照なら実体を取得）
                if (source_var->is_reference) {
                    Variable *target_var =
                        reinterpret_cast<Variable *>(source_var->value);
                    ref_var.value = reinterpret_cast<int64_t>(target_var);
                    // 参照先の型情報も更新
                    ref_var.type = target_var->type;
                    ref_var.struct_type_name = target_var->struct_type_name;
                    ref_var.is_struct = target_var->is_struct;
                    ref_var.type_name = target_var->type_name;
                }

                // パラメータスコープに参照変数を登録
                interpreter_.current_scope().variables[param->name] = ref_var;
                continue; // 次のパラメータへ
            }

            // 配列パラメータのサポート
            if (param->is_array) {
                if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                    // 変数として渡された場合
                    Variable *source_var =
                        interpreter_.find_variable(arg->name);
                    if (!source_var || !source_var->is_array) {
                        throw std::runtime_error(
                            "Array argument expected for parameter: " +
                            param->name);
                    }

                    // 配列は参照として渡される（C/C++と同じ動作）
                    // 参照変数を作成
                    Variable array_ref;
                    array_ref.is_reference = true;
                    array_ref.is_array = true;
                    array_ref.is_assigned = true;
                    // 型情報は元の配列と同じにする（配列型を保持）
                    array_ref.type = source_var->type;

                    // 元の配列変数へのポインタを保存
                    array_ref.value = reinterpret_cast<int64_t>(source_var);

                    // 配列情報をコピー（参照として動作するために必要）
                    array_ref.is_multidimensional =
                        source_var->is_multidimensional;
                    array_ref.array_size = source_var->array_size;
                    array_ref.array_dimensions = source_var->array_dimensions;
                    array_ref.array_type_info = source_var->array_type_info;

                    // ポインタ配列情報もコピー
                    array_ref.is_pointer = source_var->is_pointer;
                    array_ref.pointer_depth = source_var->pointer_depth;
                    array_ref.pointer_base_type = source_var->pointer_base_type;
                    array_ref.pointer_base_type_name =
                        source_var->pointer_base_type_name;

                    // struct配列情報もコピー
                    array_ref.is_struct = source_var->is_struct;
                    array_ref.struct_type_name = source_var->struct_type_name;

                    // unsigned情報もコピー
                    array_ref.is_unsigned = source_var->is_unsigned;

                    // 実データベクトルもコピー（参照でも正しいデータにアクセスできるように）
                    // これにより、参照解決前でも型情報に基づいた正しいアクセスが可能
                    // 書き込み時は元の配列とこのコピー両方を更新
                    // 関数終了時にはこのコピーから元の配列にコピーバックする必要がある
                    if (source_var->is_multidimensional) {
                        array_ref.multidim_array_values =
                            source_var->multidim_array_values;
                        array_ref.multidim_array_float_values =
                            source_var->multidim_array_float_values;
                        array_ref.multidim_array_double_values =
                            source_var->multidim_array_double_values;
                        array_ref.multidim_array_quad_values =
                            source_var->multidim_array_quad_values;
                        array_ref.multidim_array_strings =
                            source_var->multidim_array_strings;
                    } else {
                        array_ref.array_values = source_var->array_values;
                        array_ref.array_float_values =
                            source_var->array_float_values;
                        array_ref.array_double_values =
                            source_var->array_double_values;
                        array_ref.array_quad_values =
                            source_var->array_quad_values;
                        array_ref.array_strings = source_var->array_strings;
                    }

                    // const修飾を設定
                    if (param->is_const) {
                        array_ref.is_const = true;
                    }

                    // パラメータとして登録
                    interpreter_.current_scope().variables[param->name] =
                        array_ref;
                } else if (arg->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
                    // 配列リテラルとして直接渡された場合
                    debug_msg(
                        DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                        ("Processing array literal argument for parameter: " +
                         param->name)
                            .c_str());

                    // 一時的な配列変数を作成
                    std::string temp_var_name =
                        "__temp_array_" + std::to_string(i);
                    Variable temp_var;
                    temp_var.is_array = true;
                    temp_var.type = param->type_info;
                    temp_var.is_assigned = false;

                    // 配列リテラルから値を取得
                    std::vector<int64_t> values;
                    std::vector<std::string> str_values;

                    for (const auto &element : arg->arguments) {
                        if (element->node_type ==
                            ASTNodeType::AST_STRING_LITERAL) {
                            str_values.push_back(element->str_value);
                        } else {
                            int64_t val = evaluate_expression(element.get());
                            values.push_back(val);
                        }
                    }

                    // 一時変数に値を設定
                    if (!str_values.empty()) {
                        temp_var.array_strings = str_values;
                        temp_var.array_size = str_values.size();
                        temp_var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                                              TYPE_STRING);
                    } else {
                        temp_var.array_values = values;
                        temp_var.array_size = values.size();
                        temp_var.type =
                            static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_INT);
                    }
                    temp_var.is_assigned = true;

                    // パラメータに設定
                    interpreter_.assign_array_parameter(param->name, temp_var,
                                                        param->type_info);

                    // const修飾を設定
                    if (param->is_const) {
                        Variable *param_var =
                            interpreter_.find_variable(param->name);
                        if (param_var) {
                            param_var->is_const = true;
                        }
                    }
                } else {
                    throw std::runtime_error("Only array variables can be "
                                             "passed as array parameters");
                }
            } else {
                // 通常の値パラメータの型チェック
                // 引数の型を事前にチェック
                if (arg->node_type == ASTNodeType::AST_STRING_LITERAL &&
                    param->type_info != TYPE_STRING) {
                    throw std::runtime_error(
                        "Type mismatch: cannot pass string literal to "
                        "non-string parameter '" +
                        param->name + "'");
                }

                // 文字列パラメータの場合
                if (param->type_info == TYPE_STRING) {
                    if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                        // 文字列リテラルを直接代入
                        Variable param_var;
                        param_var.type = TYPE_STRING;
                        param_var.str_value = arg->str_value;
                        param_var.is_assigned = true;
                        param_var.is_const =
                            param->is_const; // パラメータのconst修飾を保持
                        interpreter_.current_scope().variables[param->name] =
                            param_var;
                    } else if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                        // 文字列変数を代入
                        Variable *source_var =
                            interpreter_.find_variable(arg->name);
                        if (!source_var || source_var->type != TYPE_STRING) {
                            throw std::runtime_error(
                                "Type mismatch: expected string variable for "
                                "parameter '" +
                                param->name + "'");
                        }
                        Variable param_var;
                        param_var.type = TYPE_STRING;
                        param_var.str_value = source_var->str_value;
                        param_var.is_assigned = true;
                        param_var.is_const =
                            param->is_const; // パラメータのconst修飾を保持
                        interpreter_.current_scope().variables[param->name] =
                            param_var;
                    } else {
                        throw std::runtime_error(
                            "Type mismatch: cannot pass non-string expression "
                            "to string parameter '" +
                            param->name + "'");
                    }
                } else {
                    auto is_interface_compatible = [](const Variable *var) {
                        if (!var) {
                            return false;
                        }
                        if (var->is_struct || var->type == TYPE_INTERFACE) {
                            return true;
                        }
                        if (var->type >= TYPE_ARRAY_BASE) {
                            return true;
                        }
                        switch (var->type) {
                        case TYPE_INT:
                        case TYPE_LONG:
                        case TYPE_SHORT:
                        case TYPE_TINY:
                        case TYPE_BOOL:
                        case TYPE_STRING:
                        case TYPE_CHAR:
                            return true;
                        default:
                            return false;
                        }
                    };

                    auto assign_interface_argument =
                        [&](const Variable &source,
                            const std::string &source_name) {
                            Variable interface_placeholder(param->type_name,
                                                           true);
                            interpreter_.assign_interface_view(
                                param->name, interface_placeholder, source,
                                source_name);
                        };

                    bool param_is_interface = false;
                    if (param->type_info == TYPE_INTERFACE) {
                        param_is_interface = true;
                    } else if (!param->type_name.empty()) {
                        if (interpreter_.find_interface_definition(
                                param->type_name) != nullptr) {
                            param_is_interface = true;
                        }
                    }

                    if (param_is_interface) {
                        if (arg->node_type == ASTNodeType::AST_VARIABLE ||
                            arg->node_type == ASTNodeType::AST_IDENTIFIER) {
                            std::string source_name = arg->name;
                            Variable *source_var =
                                interpreter_.find_variable(source_name);
                            if (!source_var) {
                                throw std::runtime_error(
                                    "Source variable not found: " +
                                    source_name);
                            }
                            if (!is_interface_compatible(source_var)) {
                                throw std::runtime_error(
                                    "Cannot pass non-struct/non-primitive to "
                                    "interface parameter '" +
                                    param->name + "'");
                            }
                            assign_interface_argument(*source_var, source_name);
                        } else if (arg->node_type ==
                                   ASTNodeType::AST_STRING_LITERAL) {
                            Variable temp;
                            temp.type = TYPE_STRING;
                            temp.str_value = arg->str_value;
                            temp.is_assigned = true;
                            temp.struct_type_name = "string";
                            assign_interface_argument(temp, "");
                        } else {
                            auto build_temp_from_primitive =
                                [&](TypeInfo value_type, int64_t numeric_value,
                                    const std::string &string_value) {
                                    Variable temp;
                                    temp.type = value_type;
                                    temp.is_assigned = true;
                                    if (!arg->type_name.empty()) {
                                        temp.struct_type_name = arg->type_name;
                                    } else {
                                        temp.struct_type_name = std::string(
                                            ::type_info_to_string(value_type));
                                    }
                                    if (value_type == TYPE_STRING) {
                                        temp.str_value = string_value;
                                    } else {
                                        temp.value = numeric_value;
                                    }
                                    return temp;
                                };

                            try {
                                int64_t numeric_value =
                                    evaluate_expression(arg.get());
                                TypeInfo resolved_type =
                                    arg->type_info != TYPE_UNKNOWN
                                        ? arg->type_info
                                        : TYPE_INT;
                                if (resolved_type == TYPE_STRING) {
                                    Variable temp = build_temp_from_primitive(
                                        TYPE_STRING, 0, arg->str_value);
                                    assign_interface_argument(temp, "");
                                } else {
                                    Variable temp = build_temp_from_primitive(
                                        resolved_type, numeric_value, "");
                                    assign_interface_argument(temp, "");
                                }
                            } catch (const ReturnException &ret) {
                                if (ret.is_array) {
                                    throw std::runtime_error(
                                        "Cannot pass array return value to "
                                        "interface parameter '" +
                                        param->name + "'");
                                }
                                if (!ret.is_struct &&
                                    TypeHelpers::isString(ret.type)) {
                                    Variable temp = build_temp_from_primitive(
                                        TYPE_STRING, 0, ret.str_value);
                                    assign_interface_argument(temp, "");
                                } else if (!ret.is_struct) {
                                    Variable temp = build_temp_from_primitive(
                                        ret.type, ret.value, ret.str_value);
                                    assign_interface_argument(temp, "");
                                } else {
                                    assign_interface_argument(ret.struct_value,
                                                              "");
                                }
                            }
                        }
                        continue;
                    }

                    // struct型パラメータかチェック
                    if (param->type_info == TYPE_STRUCT) {
                        Variable *source_var = nullptr;
                        std::string source_var_name;

                        if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                            // struct変数を引数として渡す場合
                            source_var_name = arg->name;
                            source_var = interpreter_.find_variable(arg->name);
                        } else if (arg->node_type ==
                                   ASTNodeType::AST_ARRAY_REF) {
                            // 構造体配列要素を引数として渡す場合
                            // (struct_array[0])
                            std::string array_name = arg->left->name;
                            int64_t index =
                                evaluate_expression(arg->array_index.get());
                            source_var_name =
                                array_name + "[" + std::to_string(index) + "]";

                            // 配列要素の最新状態を同期
                            interpreter_.sync_struct_members_from_direct_access(
                                source_var_name);
                            // 同期後に再度取得
                            source_var =
                                interpreter_.find_variable(source_var_name);
                        }

                        if (source_var && source_var->is_struct) {

                            // typedef名を実際のstruct名に解決
                            std::string resolved_struct_type =
                                interpreter_.resolve_typedef(param->type_name);
                            std::string source_resolved_type =
                                interpreter_.resolve_typedef(
                                    source_var->struct_type_name);

                            // struct型の互換性チェック
                            // "struct Point"と"Point"は同じ型として扱う
                            std::string normalized_resolved =
                                resolved_struct_type;
                            std::string normalized_source =
                                source_resolved_type;

                            // "struct StructName"を"StructName"に正規化
                            if (normalized_resolved.substr(0, 7) == "struct " &&
                                normalized_resolved.length() > 7) {
                                normalized_resolved =
                                    normalized_resolved.substr(7);
                            }
                            if (normalized_source.substr(0, 7) == "struct " &&
                                normalized_source.length() > 7) {
                                normalized_source = normalized_source.substr(7);
                            }

                            if (normalized_resolved != normalized_source) {
                                throw std::runtime_error(
                                    "Type mismatch: cannot pass struct type '" +
                                    source_var->struct_type_name +
                                    "' to parameter '" + param->name +
                                    "' of type '" + param->type_name + "'");
                            }

                            // ソース構造体の最新状態を同期
                            Variable *sync_source_var = nullptr;
                            if (!source_var_name.empty()) {
                                interpreter_
                                    .sync_struct_members_from_direct_access(
                                        source_var_name);
                                sync_source_var =
                                    interpreter_.find_variable(source_var_name);
                            } else {
                                debug_print("WARNING: Empty source_var_name, "
                                            "skipping sync\n");
                            }

                            if (!sync_source_var) {
                                throw std::runtime_error(
                                    "Source struct variable not found: " +
                                    source_var_name);
                            }

                            // 文字列配列メンバの場合、追加で確実にarray_stringsを同期
                            for (auto &source_member_pair :
                                 sync_source_var->struct_members) {
                                if (source_member_pair.second.is_array &&
                                    source_member_pair.second.type ==
                                        TYPE_STRING) {
                                    // 個別要素変数から文字列配列を再構築
                                    std::string base_name =
                                        source_var_name.empty()
                                            ? "unknown"
                                            : source_var_name;
                                    std::string source_member_name =
                                        base_name + "." +
                                        source_member_pair.first;
                                    for (int i = 0;
                                         i <
                                         source_member_pair.second.array_size;
                                         i++) {
                                        std::string element_name =
                                            source_member_name + "[" +
                                            std::to_string(i) + "]";
                                        Variable *element_var =
                                            interpreter_.find_variable(
                                                element_name);
                                        if (element_var &&
                                            element_var->type == TYPE_STRING) {
                                            if (source_member_pair.second
                                                    .array_strings.size() <=
                                                static_cast<size_t>(i)) {
                                                source_member_pair.second
                                                    .array_strings.resize(i +
                                                                          1);
                                            }
                                            source_member_pair.second
                                                .array_strings[i] =
                                                element_var->str_value;
                                        }
                                    }
                                }
                            }

                            // struct変数をコピーしてパラメータに設定
                            Variable param_var = *sync_source_var;
                            param_var.is_const =
                                param->is_const; // パラメータのconst修飾を保持
                            param_var.is_struct =
                                true; // 明示的にstructフラグを設定
                            param_var.type = TYPE_STRUCT; // 型情報も設定
                            // 解決されたstruct型名を設定
                            param_var.struct_type_name = resolved_struct_type;

                            // struct_membersの配列要素も確実にコピー
                            for (auto &member_pair : param_var.struct_members) {
                                if (member_pair.second.is_array &&
                                    TypeHelpers::isString(
                                        member_pair.second.type)) {
                                    // 文字列配列の場合、array_stringsを確実にコピー
                                    const auto &source_member =
                                        sync_source_var->struct_members.find(
                                            member_pair.first);
                                    if (source_member !=
                                        sync_source_var->struct_members.end()) {
                                        debug_print(
                                            "DEBUG: Copying string array %s: "
                                            "size=%d\n",
                                            member_pair.first.c_str(),
                                            static_cast<int>(
                                                source_member->second
                                                    .array_strings.size()));
                                        member_pair.second.array_strings =
                                            source_member->second.array_strings;
                                        if (!source_member->second.array_strings
                                                 .empty()) {
                                            debug_print(
                                                "DEBUG: First element: '%s'\n",
                                                source_member->second
                                                    .array_strings[0]
                                                    .c_str());
                                        }
                                    }
                                }
                            }

                            interpreter_.current_scope()
                                .variables[param->name] = param_var;

                            // 個別メンバー変数も作成（値を正しく設定）
                            // 元の構造体定義から type_name 情報を取得
                            const StructDefinition *struct_def =
                                interpreter_.find_struct_definition(
                                    resolved_struct_type);
                            for (const auto &member_pair :
                                 sync_source_var->struct_members) {
                                // 配列要素のキー (例: "dimensions[0]")
                                // をスキップ
                                if (member_pair.first.find('[') !=
                                    std::string::npos) {
                                    continue;
                                }

                                std::string full_member_name =
                                    param->name + "." + member_pair.first;
                                Variable member_var = member_pair.second;
                                // 値を確実に設定
                                member_var.is_assigned = true;

                                // 元の構造体定義から type_name を取得して設定
                                if (struct_def) {
                                    for (const auto &member :
                                         struct_def->members) {
                                        if (member.name == member_pair.first) {
                                            member_var.type_name =
                                                member.type_alias;
                                            member_var.is_pointer =
                                                member.is_pointer;
                                            member_var.pointer_depth =
                                                member.pointer_depth;
                                            member_var.pointer_base_type_name =
                                                member.pointer_base_type_name;
                                            member_var.pointer_base_type =
                                                member.pointer_base_type;
                                            member_var.is_reference =
                                                member.is_reference;
                                            member_var.is_unsigned =
                                                member.is_unsigned;
                                            break;
                                        }
                                    }
                                }

                                interpreter_.current_scope()
                                    .variables[full_member_name] = member_var;

                                // 配列メンバの場合、個別要素変数も作成
                                if (member_var.is_array) {
                                    // ソース側の配列要素変数をコピー
                                    std::string source_member_name =
                                        source_var_name + "." +
                                        member_pair.first;
                                    for (int i = 0; i < member_var.array_size;
                                         i++) {
                                        std::string source_element_name =
                                            source_member_name + "[" +
                                            std::to_string(i) + "]";
                                        std::string param_element_name =
                                            full_member_name + "[" +
                                            std::to_string(i) + "]";

                                        Variable *source_element =
                                            interpreter_.find_variable(
                                                source_element_name);
                                        if (source_element) {
                                            Variable element_var =
                                                *source_element;
                                            element_var.is_assigned = true;
                                            interpreter_.current_scope()
                                                .variables[param_element_name] =
                                                element_var;
                                        } else {
                                            // 個別要素変数が存在しない場合、struct_membersの配列から作成
                                            Variable element_var;
                                            if (member_var.type ==
                                                    TYPE_STRING &&
                                                i < static_cast<int>(
                                                        sync_source_var
                                                            ->struct_members
                                                                [member_pair
                                                                     .first]
                                                            .array_strings
                                                            .size())) {
                                                element_var.type = TYPE_STRING;
                                                element_var.str_value =
                                                    sync_source_var
                                                        ->struct_members
                                                            [member_pair.first]
                                                        .array_strings[i];
                                            } else if (
                                                member_var.type !=
                                                    TYPE_STRING &&
                                                i < static_cast<int>(
                                                        sync_source_var
                                                            ->struct_members
                                                                [member_pair
                                                                     .first]
                                                            .array_values
                                                            .size())) {
                                                element_var.type =
                                                    member_var.type;
                                                element_var.value =
                                                    sync_source_var
                                                        ->struct_members
                                                            [member_pair.first]
                                                        .array_values[i];
                                            } else {
                                                // デフォルト値を設定
                                                element_var.type =
                                                    member_var.type;
                                                if (member_var.type ==
                                                    TYPE_STRING) {
                                                    element_var.str_value = "";
                                                } else {
                                                    element_var.value = 0;
                                                }
                                            }
                                            element_var.is_assigned = true;
                                            interpreter_.current_scope()
                                                .variables[param_element_name] =
                                                element_var;
                                        }
                                    }
                                }
                            }
                        } else {
                            throw std::runtime_error(
                                "Type mismatch: cannot pass non-struct "
                                "expression to struct parameter '" +
                                param->name + "'");
                        }
                    } else {
                        // 数値パラメータの場合
                        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                            throw std::runtime_error(
                                "Type mismatch: cannot pass string literal to "
                                "numeric parameter '" +
                                param->name + "'");
                        }

                        TypedValue arg_value =
                            evaluate_typed_expression(arg.get());
                        interpreter_.assign_function_parameter(
                            param->name, arg_value, param->type_info,
                            param->is_unsigned);

                        // const修飾を設定
                        if (param->is_const) {
                            Variable *param_var =
                                interpreter_.find_variable(param->name);
                            if (param_var) {
                                param_var->is_const = true;
                            }
                        }
                    }
                }
            }
        }

        // implメソッド呼び出しの場合、implコンテキストを設定
        if (is_method_call && !receiver_name.empty()) {
            Variable *receiver_var = nullptr;
            if (used_resolution_ptr && dereferenced_struct_ptr) {
                receiver_var = dereferenced_struct_ptr;
            } else {
                receiver_var = interpreter_.find_variable(receiver_name);
            }

            // Interface型のレシーバーの場合、implコンテキストを設定
            if (receiver_var && receiver_var->type == TYPE_INTERFACE) {
                std::string interface_name = receiver_var->interface_name;
                std::string struct_type = receiver_var->struct_type_name;

                if (!interface_name.empty() && !struct_type.empty()) {
                    interpreter_.enter_impl_context(interface_name,
                                                    struct_type);
                    impl_context_active = true;
                    if (debug_mode) {
                        debug_print(
                            "IMPL_CONTEXT: Entered %s::%s for method %s\n",
                            interface_name.c_str(), struct_type.c_str(),
                            node->name.c_str());
                    }
                }
            }
        }

        // 関数本体を実行
        try {
            if (func->body) {
                interpreter_.execute_statement(func->body.get());
            }

            // implコンテキストをクリア
            if (impl_context_active) {
                interpreter_.exit_impl_context();
                impl_context_active = false;
            }

            // void関数は0を返す

            // メソッド実行後、selfの変更をレシーバーに同期
            if (has_receiver && !receiver_name.empty()) {
                Variable *receiver_var = nullptr;

                // If we used pointer dereference, write back to the
                // dereferenced struct
                if (used_resolution_ptr && dereferenced_struct_ptr) {
                    receiver_var = dereferenced_struct_ptr;
                    if (debug_mode) {
                        debug_print(
                            "SELF_WRITEBACK_PTR: Using dereferenced struct at "
                            "%p\n",
                            static_cast<void *>(dereferenced_struct_ptr));
                    }
                } else {
                    receiver_var = interpreter_.find_variable(receiver_name);
                }

                if (receiver_var && (receiver_var->type == TYPE_STRUCT ||
                                     receiver_var->type == TYPE_INTERFACE)) {
                    // すべての self.* 変数を検索して書き戻し
                    auto &current_scope = interpreter_.get_current_scope();
                    for (const auto &var_pair : current_scope.variables) {
                        const std::string &var_name = var_pair.first;

                        // self. で始まる変数を検索
                        if (var_name.find("self.") == 0) {
                            // self.member または self.member.nested の形式
                            std::string member_path =
                                var_name.substr(5); // "self." を除去

                            const Variable &self_member_var = var_pair.second;

                            // If using dereferenced pointer, write directly to
                            // struct_members
                            if (used_resolution_ptr &&
                                dereferenced_struct_ptr) {
                                // Extract member name (first component of
                                // member_path)
                                std::string member_name = member_path;
                                size_t dot_pos = member_path.find('.');
                                if (dot_pos != std::string::npos) {
                                    member_name =
                                        member_path.substr(0, dot_pos);
                                }

                                // Write directly to struct_members
                                if (receiver_var->struct_members.find(
                                        member_name) !=
                                    receiver_var->struct_members.end()) {
                                    receiver_var->struct_members[member_name]
                                        .value = self_member_var.value;
                                    receiver_var->struct_members[member_name]
                                        .str_value = self_member_var.str_value;
                                    receiver_var->struct_members[member_name]
                                        .is_assigned =
                                        self_member_var.is_assigned;
                                    receiver_var->struct_members[member_name]
                                        .float_value =
                                        self_member_var.float_value;
                                    receiver_var->struct_members[member_name]
                                        .double_value =
                                        self_member_var.double_value;
                                    receiver_var->struct_members[member_name]
                                        .quad_value =
                                        self_member_var.quad_value;

                                    // Also sync to individual variable if it
                                    // exists
                                    interpreter_
                                        .sync_individual_member_from_struct(
                                            receiver_var, member_name);

                                    if (debug_mode) {
                                        debug_print(
                                            "SELF_WRITEBACK_PTR: %s -> "
                                            "struct_members[%s] (value=%lld)\n",
                                            var_name.c_str(),
                                            member_name.c_str(),
                                            self_member_var.value);
                                    }
                                }
                            } else {
                                // Normal writeback to named variables
                                std::string receiver_path =
                                    receiver_name + "." + member_path;

                                // receiver側の対応する変数に値を書き戻し
                                Variable *receiver_member_var =
                                    interpreter_.find_variable(receiver_path);
                                if (receiver_member_var) {
                                    receiver_member_var->value =
                                        self_member_var.value;
                                    receiver_member_var->str_value =
                                        self_member_var.str_value;
                                    receiver_member_var->is_assigned =
                                        self_member_var.is_assigned;
                                    receiver_member_var->float_value =
                                        self_member_var.float_value;
                                    receiver_member_var->double_value =
                                        self_member_var.double_value;
                                    receiver_member_var->quad_value =
                                        self_member_var.quad_value;

                                    debug_print("SELF_WRITEBACK: %s -> %s "
                                                "(value=%lld)\n",
                                                var_name.c_str(),
                                                receiver_path.c_str(),
                                                self_member_var.value);
                                }
                            }
                        }
                    }
                }
            }

            // 配列参照のコピーバック処理
            // 関数終了時に、参照変数のデータベクトルを元の配列にコピーバック
            for (auto &var_pair : interpreter_.current_scope().variables) {
                Variable &var = var_pair.second;
                if (var.is_reference && var.is_array) {
                    // 元の配列へのポインタを取得
                    Variable *original_array =
                        reinterpret_cast<Variable *>(var.value);
                    if (original_array) {
                        // データベクトルをコピーバック
                        if (var.is_multidimensional) {
                            original_array->multidim_array_values =
                                var.multidim_array_values;
                            original_array->multidim_array_float_values =
                                var.multidim_array_float_values;
                            original_array->multidim_array_double_values =
                                var.multidim_array_double_values;
                            original_array->multidim_array_quad_values =
                                var.multidim_array_quad_values;
                            original_array->multidim_array_strings =
                                var.multidim_array_strings;
                        } else {
                            original_array->array_values = var.array_values;
                            original_array->array_float_values =
                                var.array_float_values;
                            original_array->array_double_values =
                                var.array_double_values;
                            original_array->array_quad_values =
                                var.array_quad_values;
                            original_array->array_strings = var.array_strings;
                        }
                    }
                }
            }

            cleanup_method_context();
            interpreter_.pop_scope();
            method_scope_active = false;
            interpreter_.current_function_name = prev_function_name;
            return 0;
        } catch (const ReturnException &ret) {
            // implコンテキストをクリア
            if (impl_context_active) {
                interpreter_.exit_impl_context();
                impl_context_active = false;
            }

            // return文で戻り値がある場合

            // メソッド実行後、selfの変更をレシーバーに同期
            if (has_receiver && !receiver_name.empty()) {
                Variable *receiver_var = nullptr;

                // If we used pointer dereference, write back to the
                // dereferenced struct
                if (used_resolution_ptr && dereferenced_struct_ptr) {
                    receiver_var = dereferenced_struct_ptr;
                    if (debug_mode) {
                        debug_print(
                            "SELF_WRITEBACK_PTR: Using dereferenced struct at "
                            "%p\n",
                            static_cast<void *>(dereferenced_struct_ptr));
                    }
                } else {
                    receiver_var = interpreter_.find_variable(receiver_name);
                }

                if (receiver_var && (receiver_var->type == TYPE_STRUCT ||
                                     receiver_var->type == TYPE_INTERFACE)) {
                    // すべての self.* 変数を検索して書き戻し
                    auto &current_scope = interpreter_.get_current_scope();
                    for (const auto &var_pair : current_scope.variables) {
                        const std::string &var_name = var_pair.first;

                        // self. で始まる変数を検索
                        if (var_name.find("self.") == 0) {
                            // self.member または self.member.nested の形式
                            std::string member_path =
                                var_name.substr(5); // "self." を除去

                            const Variable &self_member_var = var_pair.second;

                            // If using dereferenced pointer, write directly to
                            // struct_members
                            if (used_resolution_ptr &&
                                dereferenced_struct_ptr) {
                                // Extract member name (first component of
                                // member_path)
                                std::string member_name = member_path;
                                size_t dot_pos = member_path.find('.');
                                if (dot_pos != std::string::npos) {
                                    member_name =
                                        member_path.substr(0, dot_pos);
                                }

                                // Write directly to struct_members
                                if (receiver_var->struct_members.find(
                                        member_name) !=
                                    receiver_var->struct_members.end()) {
                                    receiver_var->struct_members[member_name]
                                        .value = self_member_var.value;
                                    receiver_var->struct_members[member_name]
                                        .str_value = self_member_var.str_value;
                                    receiver_var->struct_members[member_name]
                                        .is_assigned =
                                        self_member_var.is_assigned;
                                    receiver_var->struct_members[member_name]
                                        .float_value =
                                        self_member_var.float_value;
                                    receiver_var->struct_members[member_name]
                                        .double_value =
                                        self_member_var.double_value;
                                    receiver_var->struct_members[member_name]
                                        .quad_value =
                                        self_member_var.quad_value;

                                    // Also sync to individual variable if it
                                    // exists
                                    interpreter_
                                        .sync_individual_member_from_struct(
                                            receiver_var, member_name);

                                    if (debug_mode) {
                                        debug_print(
                                            "SELF_WRITEBACK_PTR: %s -> "
                                            "struct_members[%s] (value=%lld)\n",
                                            var_name.c_str(),
                                            member_name.c_str(),
                                            self_member_var.value);
                                    }
                                }
                            } else {
                                // Normal writeback to named variables
                                std::string receiver_path =
                                    receiver_name + "." + member_path;

                                // receiver側の対応する変数に値を書き戻し
                                Variable *receiver_member_var =
                                    interpreter_.find_variable(receiver_path);
                                if (receiver_member_var) {
                                    receiver_member_var->value =
                                        self_member_var.value;
                                    receiver_member_var->str_value =
                                        self_member_var.str_value;
                                    receiver_member_var->is_assigned =
                                        self_member_var.is_assigned;
                                    receiver_member_var->float_value =
                                        self_member_var.float_value;
                                    receiver_member_var->double_value =
                                        self_member_var.double_value;
                                    receiver_member_var->quad_value =
                                        self_member_var.quad_value;

                                    debug_print("SELF_WRITEBACK: %s -> %s "
                                                "(value=%lld)\n",
                                                var_name.c_str(),
                                                receiver_path.c_str(),
                                                self_member_var.value);
                                }
                            }
                        }
                    }
                }
            }

            // 配列参照のコピーバック処理
            for (auto &var_pair : interpreter_.current_scope().variables) {
                Variable &var = var_pair.second;
                if (var.is_reference && var.is_array) {
                    Variable *original_array =
                        reinterpret_cast<Variable *>(var.value);
                    if (original_array) {
                        if (var.is_multidimensional) {
                            original_array->multidim_array_values =
                                var.multidim_array_values;
                            original_array->multidim_array_float_values =
                                var.multidim_array_float_values;
                            original_array->multidim_array_double_values =
                                var.multidim_array_double_values;
                            original_array->multidim_array_quad_values =
                                var.multidim_array_quad_values;
                            original_array->multidim_array_strings =
                                var.multidim_array_strings;
                        } else {
                            original_array->array_values = var.array_values;
                            original_array->array_float_values =
                                var.array_float_values;
                            original_array->array_double_values =
                                var.array_double_values;
                            original_array->array_quad_values =
                                var.array_quad_values;
                            original_array->array_strings = var.array_strings;
                        }
                    }
                }
            }

            cleanup_method_context();
            interpreter_.pop_scope();
            method_scope_active = false;
            interpreter_.current_function_name = prev_function_name;

            // 関数ポインタ戻り値の場合は例外を再度投げる
            if (ret.is_function_pointer) {
                throw ret;
            }

            if (ret.is_struct) {
                // struct戻り値の場合、構造体を一時的に処理して戻り値として使用
                debug_msg(DebugMsgId::INTERPRETER_GET_STRUCT_MEMBER,
                          "Processing struct return value");
                // 構造体戻り値は0を返す（実際の構造体はReturnExceptionで管理）
                throw ret; // 上位レベルでstruct処理が必要な場合は例外を伝播
            } else if (ret.is_array) {
                // 配列戻り値の場合は例外を再度投げる
                throw ret;
            }
            // 文字列戻り値の場合は例外を再度投げる
            if (TypeHelpers::isString(ret.type)) {
                throw ret;
            }
            // float/double/quad戻り値の場合は例外を再度投げる
            // (evaluate_expressionはint64_tしか返せないため、上位でTypedValueとして処理する必要がある)
            if (TypeHelpers::isFloating(ret.type) || ret.type == TYPE_QUAD) {
                throw ret;
            }
            // 参照戻り値の場合は例外を再度投げる
            if (ret.is_reference) {
                throw ret;
            }
            // 通常の戻り値の場合
            auto make_typed_from_return =
                [&](int64_t coerced_numeric) -> TypedValue {
                if (ret.type == TYPE_FLOAT) {
                    return TypedValue(ret.double_value,
                                      InferredType(TYPE_FLOAT, "float"));
                }
                if (ret.type == TYPE_DOUBLE) {
                    return TypedValue(ret.double_value,
                                      InferredType(TYPE_DOUBLE, "double"));
                }
                if (ret.type == TYPE_QUAD) {
                    return TypedValue(ret.quad_value,
                                      InferredType(TYPE_QUAD, "quad"));
                }
                TypeInfo resolved =
                    ret.type != TYPE_UNKNOWN ? ret.type : TYPE_INT;
                std::string resolved_name =
                    std::string(::type_info_to_string(resolved));
                if (resolved_name.empty()) {
                    resolved = TYPE_INT;
                    resolved_name =
                        std::string(::type_info_to_string(resolved));
                }
                return TypedValue(coerced_numeric,
                                  InferredType(resolved, resolved_name));
            };

            int64_t return_value = ret.value;
            if (func && func->is_unsigned && return_value < 0) {
                const char *call_kind = is_method_call ? "method" : "function";
                // DEBUG_WARN(FUNCTION, "Unsigned %s '%s' returned negative
                // value (%lld); clamping to 0", ...);
                if (debug_mode) {
                    std::cerr << "WARNING: Unsigned " << call_kind << " '"
                              << func->name << "' returned negative value ("
                              << return_value << "); clamping to 0"
                              << std::endl;
                }
                return_value = 0;
            }
            TypedValue typed_return = make_typed_from_return(return_value);
            capture_numeric_return(typed_return);
            return return_value;
        }
    } catch (const ReturnException &ret) {
        // implコンテキストをクリア
        if (impl_context_active) {
            interpreter_.exit_impl_context();
            impl_context_active = false;
        }

        // 再投げされたReturnExceptionを処理
        // 配列参照のコピーバック処理
        if (method_scope_active) {
            for (auto &var_pair : interpreter_.current_scope().variables) {
                Variable &var = var_pair.second;
                if (var.is_reference && var.is_array) {
                    Variable *original_array =
                        reinterpret_cast<Variable *>(var.value);
                    if (original_array) {
                        if (var.is_multidimensional) {
                            original_array->multidim_array_values =
                                var.multidim_array_values;
                            original_array->multidim_array_float_values =
                                var.multidim_array_float_values;
                            original_array->multidim_array_double_values =
                                var.multidim_array_double_values;
                            original_array->multidim_array_quad_values =
                                var.multidim_array_quad_values;
                            original_array->multidim_array_strings =
                                var.multidim_array_strings;
                        } else {
                            original_array->array_values = var.array_values;
                            original_array->array_float_values =
                                var.array_float_values;
                            original_array->array_double_values =
                                var.array_double_values;
                            original_array->array_quad_values =
                                var.array_quad_values;
                            original_array->array_strings = var.array_strings;
                        }
                    }
                }
            }
        }
        cleanup_method_context();
        if (method_scope_active) {
            interpreter_.pop_scope();
            method_scope_active = false;
        }
        interpreter_.current_function_name = prev_function_name;
        throw ret;
    } catch (...) {
        // implコンテキストをクリア
        if (impl_context_active) {
            interpreter_.exit_impl_context();
            impl_context_active = false;
        }

        // 配列参照のコピーバック処理
        if (method_scope_active) {
            for (auto &var_pair : interpreter_.current_scope().variables) {
                Variable &var = var_pair.second;
                if (var.is_reference && var.is_array) {
                    Variable *original_array =
                        reinterpret_cast<Variable *>(var.value);
                    if (original_array) {
                        if (var.is_multidimensional) {
                            original_array->multidim_array_values =
                                var.multidim_array_values;
                            original_array->multidim_array_float_values =
                                var.multidim_array_float_values;
                            original_array->multidim_array_double_values =
                                var.multidim_array_double_values;
                            original_array->multidim_array_quad_values =
                                var.multidim_array_quad_values;
                            original_array->multidim_array_strings =
                                var.multidim_array_strings;
                        } else {
                            original_array->array_values = var.array_values;
                            original_array->array_float_values =
                                var.array_float_values;
                            original_array->array_double_values =
                                var.array_double_values;
                            original_array->array_quad_values =
                                var.array_quad_values;
                            original_array->array_strings = var.array_strings;
                        }
                    }
                }
            }
        }
        cleanup_method_context();
        if (method_scope_active) {
            interpreter_.pop_scope();
            method_scope_active = false;
        }
        interpreter_.current_function_name = prev_function_name;
        throw;
    }
}
