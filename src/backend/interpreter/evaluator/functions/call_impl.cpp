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
#include "generic_instantiation.h"
#include <cstdlib>
#include <cstring> // for strdup
#include <cstring> // for std::memcpy
#include <iomanip>
#include <sstream>

int64_t ExpressionEvaluator::evaluate_function_call_impl(const ASTNode *node) {
    if (interpreter_.is_debug_mode()) {
        std::cerr << "[DEBUG_IMPL] evaluate_function_call_impl called for: "
                  << node->name << std::endl;
    }

    // ラムダの即座実行をチェック: int func(int x){return x;}(10) 形式
    if (node->is_lambda_call && node->left) {
        const ASTNode *lambda_node = node->left.get();

        if (lambda_node->node_type == ASTNodeType::AST_LAMBDA_EXPR) {
            if (interpreter_.is_debug_mode()) {
                std::cerr << "[LAMBDA_CALL] Direct lambda invocation with "
                          << node->arguments.size() << " arguments"
                          << std::endl;
            }

            // ラムダを一時的に関数ポインタとして登録
            std::string temp_lambda_name = lambda_node->internal_name;

            // ラムダを関数として登録（一時的）
            FunctionPointer lambda_fp;
            lambda_fp.function_name = temp_lambda_name;
            lambda_fp.function_node = lambda_node;

            interpreter_.current_scope().function_pointers[temp_lambda_name] =
                lambda_fp;

            // 新しいスコープを作成してラムダを実行
            interpreter_.push_scope();

            // パラメータをバインド
            if (node->arguments.size() != lambda_node->parameters.size()) {
                std::cerr
                    << "Error: Lambda call argument count mismatch: expected "
                    << lambda_node->parameters.size() << ", got "
                    << node->arguments.size() << std::endl;
                std::exit(1);
            }

            for (size_t i = 0; i < lambda_node->parameters.size(); ++i) {
                const ASTNode *param = lambda_node->parameters[i].get();
                int64_t arg_value =
                    evaluate_expression(node->arguments[i].get());

                Variable var;
                var.type = param->type_info;
                var.value = arg_value;
                var.is_const = param->is_const;

                interpreter_.current_scope().variables[param->name] = var;
            }

            // ラムダ本体を実行
            int64_t result = 0;
            if (lambda_node->lambda_body) {
                try {
                    // lambda_bodyはAST_STMT_LISTなので、その中の文を順次実行
                    for (const auto &stmt :
                         lambda_node->lambda_body->statements) {
                        interpreter_.execute_statement(stmt.get());
                    }
                } catch (const ReturnException &e) {
                    result = e.value;
                }
            }

            // スコープをクリーンアップ
            interpreter_.pop_scope();

            // 一時的な関数ポインタを削除
            interpreter_.current_scope().function_pointers.erase(
                temp_lambda_name);

            return result;
        }

        // node->leftが別の関数呼び出しの場合（チェーン呼び出し）
        if (lambda_node->node_type == ASTNodeType::AST_FUNC_CALL) {
            // 前の呼び出しを評価して、その結果（関数ポインタ）を使って呼び出す
            int64_t lambda_ptr = evaluate_expression(lambda_node);

            // lambda_ptrが関数ポインタか確認
            FunctionPointer *fp =
                reinterpret_cast<FunctionPointer *>(lambda_ptr);
            if (fp && fp->function_node) {
                // 関数ポインタを通常の関数呼び出しとして実行
                // TODO: この部分は既存の関数ポインタ呼び出しロジックと統合
            }
        }
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
                        std::string param_type_name =
                            param->type_name; // v0.11.0

                        if (param_type == TYPE_STRING) {
                            interpreter_.assign_variable(
                                param_name, arg_strings[param_idx]);
                        } else {
                            // v0.11.0: 型名も渡す
                            TypedValue typed_val(
                                arg_values[param_idx],
                                InferredType(param_type,
                                             type_info_to_string(param_type)));
                            interpreter_.assign_function_parameter(
                                param_name, typed_val, param_type,
                                param_type_name, is_unsigned);
                        }

                        param_idx++;
                    }

                    // 関数本体を実行
                    int64_t result = 0;
                    try {
                        // ラムダの場合はlambda_bodyを、通常の関数の場合はbodyを使用
                        const ASTNode *body_to_execute =
                            func_node->lambda_body
                                ? func_node->lambda_body.get()
                                : func_node->body.get();
                        if (body_to_execute) {
                            interpreter_.exec_statement(body_to_execute);
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
                std::string param_type_name =
                    param->type_name; // v0.11.0: 型名を取得

                if (param_type == TYPE_STRING) {
                    interpreter_.assign_variable(param_name,
                                                 arg_strings[param_idx]);
                } else {
                    // v0.11.0: 型名も渡す（ジェネリックポインタ対応）
                    TypedValue typed_val(
                        arg_values[param_idx],
                        InferredType(param_type,
                                     type_info_to_string(param_type)));
                    interpreter_.assign_function_parameter(
                        param_name, typed_val, param_type, param_type_name,
                        is_unsigned);
                }

                param_idx++;
            }

            // 関数本体を実行
            int64_t result = 0;
            try {
                // ラムダの場合はlambda_bodyを、通常の関数の場合はbodyを使用
                const ASTNode *body_to_execute =
                    func_node->lambda_body ? func_node->lambda_body.get()
                                           : func_node->body.get();
                if (body_to_execute) {
                    interpreter_.exec_statement(body_to_execute);
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
    // 修飾呼び出しのチェック: module.function()
    // node->leftがAST_VARIABLEで、変数として存在せず、モジュールとして存在する場合
    bool is_qualified_call = false;
    std::string qualified_module_name;
    if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
        std::string potential_module = node->left->name;
        // 変数として存在するかチェック
        bool is_variable =
            (interpreter_.find_variable(potential_module) != nullptr);
        // モジュールとして存在するかチェック
        bool is_module = interpreter_.is_module_imported(potential_module);

        if (!is_variable && is_module) {
            is_qualified_call = true;
            qualified_module_name = potential_module;

            if (interpreter_.is_debug_mode()) {
                std::cerr << "[QUALIFIED_CALL] Module: "
                          << qualified_module_name
                          << ", Function: " << node->name << std::endl;
            }
        }
    }

    bool is_method_call =
        (node->left != nullptr &&
         !is_qualified_call); // レシーバーがある場合はメソッド呼び出し
    bool has_receiver = is_method_call;
    std::string receiver_name;
    std::string
        type_name; // メソッド呼び出しの構造体型名（ジェネリックキャッシュに使用）
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
                    std::string param_type_name = param->type_name;

                    if (param_type == TYPE_STRING) {
                        interpreter_.assign_variable(param_name,
                                                     arg_strings[param_idx]);
                    } else {
                        TypedValue typed_val(
                            arg_values[param_idx],
                            InferredType(param_type,
                                         type_info_to_string(param_type)));
                        interpreter_.assign_function_parameter(
                            param_name, typed_val, param_type, param_type_name,
                            is_unsigned);
                    }

                    param_idx++;
                }

                // 関数本体を実行
                int64_t result = 0;
                try {
                    // ラムダの場合はlambda_bodyを、通常の関数の場合はbodyを使用
                    const ASTNode *body_to_execute =
                        func_node->lambda_body ? func_node->lambda_body.get()
                                               : func_node->body.get();
                    if (body_to_execute) {
                        interpreter_.exec_statement(body_to_execute);
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

        if (interpreter_.is_debug_mode()) {
            std::cerr << "[METHOD_SEARCH] Searching for: " << method_key
                      << " ... "
                      << (it != global_scope.functions.end() ? "FOUND"
                                                             : "NOT FOUND")
                      << std::endl;
            std::cerr << "[METHOD_SEARCH] Global functions count: "
                      << global_scope.functions.size() << std::endl;

            // Vectorに関連する関数をリスト
            for (const auto &[key, val] : global_scope.functions) {
                if (key.find("Vector") != std::string::npos &&
                    key.find("init") != std::string::npos) {
                    std::cerr << "[METHOD_SEARCH]   - " << key << std::endl;
                }
            }
        }

        if (it != global_scope.functions.end()) {
            func = it->second;
        } else {
            // v0.12.0: ジェネリックimplのインスタンス化を試みる
            // まず、マングル名を元の形式に変換: Vector_int -> Vector<int>
            std::string unmangled_type_name = type_name;
            size_t first_underscore = type_name.find('_');
            if (first_underscore != std::string::npos) {
                std::string base_name = type_name.substr(0, first_underscore);
                std::string params_part =
                    type_name.substr(first_underscore + 1);

                // パラメータを'_'で分割
                std::vector<std::string> params;
                size_t pos = 0;
                while (pos < params_part.length()) {
                    size_t next_underscore = params_part.find('_', pos);
                    if (next_underscore == std::string::npos) {
                        params.push_back(params_part.substr(pos));
                        break;
                    }
                    params.push_back(
                        params_part.substr(pos, next_underscore - pos));
                    pos = next_underscore + 1;
                }

                if (!params.empty()) {
                    unmangled_type_name = base_name + "<";
                    for (size_t i = 0; i < params.size(); ++i) {
                        if (i > 0)
                            unmangled_type_name += ", ";
                        unmangled_type_name += params[i];
                    }
                    unmangled_type_name += ">";
                }
            }

            // v0.12.0: ジェネリックimplのインスタンス化を試みる
            // find_impl_for_structは自動的にジェネリックimplをインスタンス化する
            if (interpreter_.is_debug_mode()) {
                debug_print("[CALL_IMPL] Before find_impl_for_struct: "
                            "unmangled_type_name='%s'\n",
                            unmangled_type_name.c_str());
            }
            const ImplDefinition *impl =
                interpreter_.find_impl_for_struct(unmangled_type_name, "");

            if (interpreter_.is_debug_mode()) {
                debug_print("[CALL_IMPL] After find_impl_for_struct: impl=%p\n",
                            (void *)impl);
            }

            if (impl) {
                // インスタンス化されたimplのメソッドを再検索
                method_key = type_name + "::" + node->name;
                if (interpreter_.is_debug_mode()) {
                    debug_print(
                        "[CALL_IMPL] Retrying method search: method_key='%s'\n",
                        method_key.c_str());
                }
                it = global_scope.functions.find(method_key);
                if (it != global_scope.functions.end()) {
                    func = it->second;
                    if (interpreter_.is_debug_mode()) {
                        debug_print(
                            "[CALL_IMPL] Retry succeeded! Found func=%p\n",
                            (void *)func);
                    }
                } else {
                    if (interpreter_.is_debug_mode()) {
                        debug_print("[CALL_IMPL] Retry failed: method still "
                                    "not found\n");
                    }
                }
            }

            // それでも見つからない場合、従来の検索を試みる
            if (!func) {
                // マングル名から元の型名への変換を試みる
                // 例: Vector_int_SystemAllocator -> Vector<int,
                // SystemAllocator>
                auto unmangle_type_name =
                    [](const std::string &mangled) -> std::string {
                    // '_'を検索して型パラメータの開始位置を特定
                    size_t first_underscore = mangled.find('_');
                    if (first_underscore == std::string::npos) {
                        return mangled; // マングルされていない
                    }

                    // 基本型名を取得（最初の'_'まで）
                    std::string base_name = mangled.substr(0, first_underscore);

                    // 残りの部分を型パラメータとして処理
                    std::string params_part =
                        mangled.substr(first_underscore + 1);

                    // '_'で分割して型パラメータを抽出
                    std::vector<std::string> params;
                    size_t pos = 0;
                    while (pos < params_part.length()) {
                        size_t next_underscore = params_part.find('_', pos);
                        if (next_underscore == std::string::npos) {
                            params.push_back(params_part.substr(pos));
                            break;
                        }
                        params.push_back(
                            params_part.substr(pos, next_underscore - pos));
                        pos = next_underscore + 1;
                    }

                    if (params.empty()) {
                        return mangled; // パラメータなし
                    }

                    // 元の形式に再構築: Base<Param1, Param2, ...>
                    std::string result = base_name + "<";
                    for (size_t i = 0; i < params.size(); ++i) {
                        if (i > 0)
                            result += ", ";
                        result += params[i];
                    }
                    result += ">";

                    return result;
                };

                std::string unmangled_type_name2 =
                    unmangle_type_name(type_name);

                for (const auto &impl_def :
                     interpreter_.get_impl_definitions()) {
                    // 元の型名とマングル名の両方でチェック
                    if (impl_def.struct_name == type_name ||
                        impl_def.struct_name == unmangled_type_name2) {
                        std::string method_full_name =
                            impl_def.interface_name + "_" +
                            impl_def.struct_name + "_" + node->name;
                        auto it2 =
                            global_scope.functions.find(method_full_name);
                        if (it2 != global_scope.functions.end()) {
                            func = it2->second;
                            break;
                        }
                    }
                }
            }
        }
    } else {
        auto &global_scope = interpreter_.get_global_scope();

        // 修飾呼び出しの場合: module.function()
        if (is_qualified_call) {
            // モジュール名をプレフィックスとして関数を検索
            std::string qualified_name =
                qualified_module_name + "." + node->name;
            auto it = global_scope.functions.find(qualified_name);
            if (it != global_scope.functions.end()) {
                func = it->second;

                if (interpreter_.is_debug_mode()) {
                    std::cerr
                        << "[QUALIFIED_CALL] Found function: " << qualified_name
                        << std::endl;
                }
            }
        } else {
            // 通常の関数呼び出し
            // v0.11.1: find_function()を使用（コンストラクタもチェックする）
            func = interpreter_.find_function(node->name);
        }
    }

    // v0.11.0: ジェネリック関数のインスタンス化（キャッシュ付き）
    std::unique_ptr<ASTNode> instantiated_func;
    const ASTNode *cached_func = nullptr;

    // デバッグ: 条件を表示
    if (interpreter_.is_debug_mode()) {
        std::cerr << "[GENERIC_DEBUG] func=" << (func ? "yes" : "no")
                  << " func->is_generic="
                  << (func && func->is_generic ? "yes" : "no")
                  << " node->is_generic=" << (node->is_generic ? "yes" : "no")
                  << " type_arguments.size()=" << node->type_arguments.size()
                  << std::endl;
        if (func) {
            std::cerr << "[GENERIC_DEBUG] Original func has "
                      << func->statements.size() << " statements" << std::endl;
        }
    }

    if (func && func->is_generic && node->is_generic &&
        !node->type_arguments.empty()) {
        // キャッシュキーを生成
        // FIX:
        // メソッド呼び出しの場合、type_name（正規化された構造体名）を含める
        // これにより Queue_int::push と Queue_long::push
        // が別々にキャッシュされる また、Vector<long>::push と
        // Queue<long>::push も別々にキャッシュされる
        std::string function_name = node->name;
        if (is_method_call) {
            if (!type_name.empty()) {
                // メソッド呼び出しの場合：type_name::method_name形式
                function_name = type_name + "::" + node->name;
            } else if (func->name.find("::") != std::string::npos) {
                // type_nameが空でもfunc->nameに::が含まれる場合は使用
                function_name = func->name;
            } else {
                // フォールバック：関数定義時の名前を使用
                // この場合でも一意性を保つために警告を出す
                if (interpreter_.is_debug_mode()) {
                    std::cerr << "[GENERIC_CACHE_KEY_WARNING] type_name is "
                                 "empty for method call: "
                              << node->name << std::endl;
                }
                // 少なくとも関数ポインタアドレスを含めて一意性を確保
                function_name =
                    std::to_string(reinterpret_cast<uintptr_t>(func)) + "_" +
                    node->name;
            }
        }
        std::string cache_key = GenericInstantiation::generate_cache_key(
            function_name, node->type_arguments);

        if (interpreter_.is_debug_mode()) {
            std::cerr << "[GENERIC_CACHE_KEY] is_method_call=" << is_method_call
                      << ", type_name='" << type_name << "', node->name='"
                      << node->name << "', function_name='" << function_name
                      << "', cache_key='" << cache_key << "'" << std::endl;
        }

        // キャッシュをチェック
        // FIX v0.11.0: キャッシュを無効化
        // 理由:
        // キャッシュからクローンしても、複数回呼び出しでローカル変数スコープが壊れる
        // TODO: 根本原因を調査して、キャッシュを再有効化する
        cached_func =
            nullptr; // GenericInstantiation::get_cached_instance(cache_key);

        if (cached_func) {
            // キャッシュヒット（現在無効化）
            instantiated_func =
                GenericInstantiation::clone_ast_node(cached_func);
            func = instantiated_func.get();
            if (interpreter_.is_debug_mode()) {
                std::cerr << "[GENERIC_CACHE] Cache hit for " << cache_key
                          << " (cloned)" << std::endl;
            }
        } else {
            // キャッシュミス：新しくインスタンス化
            try {
                instantiated_func =
                    GenericInstantiation::instantiate_generic_function(
                        func, node->type_arguments);
                func = instantiated_func.get();

                // FIX v0.11.0: キャッシュへの保存を無効化
                // 理由: キャッシュからの取得を無効化しているため、保存も不要
                // TODO: 根本原因を調査して、キャッシュを再有効化する
                // GenericInstantiation::cache_instance(
                //     cache_key, std::move(instantiated_func));
                // funcポインタはinstantiated_funcから取得しているので、
                // instantiated_funcをmoveしない限り有効

                if (interpreter_.is_debug_mode()) {
                    std::cerr
                        << "[GENERIC_INST] Instantiated generic function: "
                        << func->name << " with type arguments: ";
                    for (const auto &type_arg : node->type_arguments) {
                        std::cerr << type_arg << " ";
                    }
                    std::cerr << std::endl;
                    std::cerr << "[GENERIC_INST] Cached as " << cache_key
                              << std::endl;
                    std::cerr << "[GENERIC_INST] Instantiated func has "
                              << func->statements.size() << " statements, "
                              << func->parameters.size() << " parameters"
                              << std::endl;
                }
            } catch (const std::exception &e) {
                throw std::runtime_error(
                    "Failed to instantiate generic function " + node->name +
                    ": " + e.what());
            }
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

        // memcpy(dest, src, size) - メモリコピー組み込み関数
        if (node->name == "memcpy" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error("memcpy() requires exactly 3 "
                                         "arguments: memcpy(dest, src, size)");
            }

            // 引数を評価
            int64_t dest_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t src_value =
                interpreter_.eval_expression(node->arguments[1].get());
            int64_t size =
                interpreter_.eval_expression(node->arguments[2].get());

            // デバッグ出力（コメントアウト）
            // std::cerr << "[memcpy ENTRY] dest=" << std::hex << dest_value
            //           << ", src=" << src_value << ", size=" << std::dec <<
            //           size << std::endl;

            // null チェック
            if (dest_value == 0) {
                std::cerr << "[memcpy] Error: destination pointer is null"
                          << std::endl;
                return 0;
            }
            if (src_value == 0) {
                std::cerr << "[memcpy] Error: source pointer is null"
                          << std::endl;
                return 0;
            }

            // サイズチェック
            if (size <= 0) {
                return dest_value;
            }

            // Variable*として有効かどうかをチェック
            Variable *dest_var = reinterpret_cast<Variable *>(dest_value);
            Variable *src_var = reinterpret_cast<Variable *>(src_value);

            bool dest_is_var = false;
            bool src_is_var = false;
            void *actual_dest = reinterpret_cast<void *>(dest_value);
            void *actual_src = reinterpret_cast<void *>(src_value);

            // destがVariable*かチェック（安全に）
            try {
                // TypeInfoが有効範囲か簡易チェック（is_assignedは不要、未初期化変数もサポート）
                // FIX: TYPE_TINYとTYPE_SHORTも含める
                // FIX v0.11.0:
                // 型の範囲チェックを厳格化（生のメモリアドレスを誤認識しないため）
                // FIX v0.11.0: TYPE_POINTER, TYPE_STRUCTも含める
                if ((dest_var->type >= TYPE_TINY &&
                     dest_var->type <= TYPE_BIG) ||
                    dest_var->type == TYPE_POINTER ||
                    dest_var->type == TYPE_STRUCT) {
                    dest_is_var = true;

                    // std::cerr << "[memcpy DEBUG dest] Variable*=" << dest_var
                    //           << ", type=" <<
                    //           static_cast<int>(dest_var->type)
                    //           << ", is_struct=" << dest_var->is_struct
                    //           << ", is_array=" << dest_var->is_array
                    //           << ", value=" << dest_var->value << std::endl;

                    // プリミティブ型なら&(var->value)を使う
                    if (!dest_var->is_struct && !dest_var->is_array &&
                        dest_var->type != TYPE_POINTER &&
                        dest_var->type != TYPE_STRUCT) {
                        actual_dest = &(dest_var->value);
                    } else if (dest_var->is_array &&
                               !dest_var->array_values.empty()) {
                        actual_dest = dest_var->array_values.data();
                    } else if (dest_var->is_struct ||
                               dest_var->type == TYPE_POINTER ||
                               dest_var->type == TYPE_STRUCT) {
                        // 構造体ポインタの場合、valueに実際のアドレスが格納されている
                        actual_dest = reinterpret_cast<void *>(dest_var->value);
                        // std::cerr << "[memcpy DEBUG dest] Struct/Pointer:
                        // actual_dest=" << actual_dest << std::endl;
                    }
                }
            } catch (...) {
                dest_is_var = false;
            }

            // srcがVariable*かチェック（安全に）
            try {
                // FIX: TYPE_TINYとTYPE_SHORTも含める
                // FIX v0.11.0:
                // 型の範囲チェックを厳格化（生のメモリアドレスを誤認識しないため）
                // FIX v0.11.0: TYPE_POINTER, TYPE_STRUCTも含める
                if ((src_var->type >= TYPE_TINY && src_var->type <= TYPE_BIG) ||
                    src_var->type == TYPE_POINTER ||
                    src_var->type == TYPE_STRUCT) {
                    src_is_var = true;

                    // std::cerr << "[memcpy DEBUG src] Variable*=" << src_var
                    //           << ", type=" << static_cast<int>(src_var->type)
                    //           << ", is_struct=" << src_var->is_struct
                    //           << ", is_array=" << src_var->is_array
                    //           << ", value=" << src_var->value << std::endl;

                    // プリミティブ型なら&(var->value)を使う
                    if (!src_var->is_struct && !src_var->is_array &&
                        src_var->type != TYPE_POINTER &&
                        src_var->type != TYPE_STRUCT) {
                        actual_src = &(src_var->value);
                    } else if (src_var->is_array &&
                               !src_var->array_values.empty()) {
                        actual_src = src_var->array_values.data();
                    } else if (src_var->is_struct ||
                               src_var->type == TYPE_POINTER ||
                               src_var->type == TYPE_STRUCT) {
                        // 構造体ポインタの場合、valueに実際のアドレスが格納されている
                        actual_src = reinterpret_cast<void *>(src_var->value);
                        // std::cerr << "[memcpy DEBUG src] Struct/Pointer:
                        // actual_src=" << actual_src << std::endl;
                    }
                }
            } catch (...) {
                src_is_var = false;
            }

            // 構造体コピーの特別処理
            if (dest_is_var && src_is_var && dest_var->is_struct &&
                src_var->is_struct) {
                // v0.13.1: Variable構造体のstruct_membersをコピー（参照も考慮）
                auto &src_members = src_var->get_struct_members();
                auto &dest_members = dest_var->get_struct_members();
                for (const auto &member_pair : src_members) {
                    dest_members[member_pair.first] = member_pair.second;
                }

                if (interpreter_.is_debug_mode()) {
                    std::cerr << "[memcpy] Copied struct members from "
                              << src_var << " to " << dest_var << std::endl;
                }
            } else {
                // 通常のmemcpy: actual_destとactual_srcを使用
                std::memcpy(actual_dest, actual_src, static_cast<size_t>(size));

                if (interpreter_.is_debug_mode()) {
                    std::cerr << "[memcpy] Copied " << size << " bytes from "
                              << actual_src << " to " << actual_dest
                              << " (dest_is_var=" << dest_is_var
                              << ", src_is_var=" << src_is_var << ")"
                              << std::endl;

                    // 書き込み確認: actual_destから読み戻してみる
                    if (size == 8 && !dest_is_var && src_is_var) {
                        int64_t written_value =
                            *reinterpret_cast<int64_t *>(actual_dest);
                        int64_t source_value =
                            *reinterpret_cast<int64_t *>(actual_src);
                        std::cerr
                            << "[memcpy] Verification: wrote " << source_value
                            << ", read back " << written_value
                            << " (match=" << (written_value == source_value)
                            << ")" << std::endl;
                    }
                }
            }

            // destポインタを返す
            return dest_value;
        }

        // sizeof_type("T") - 型コンテキストからTの実際の型のサイズを返す
        // 使用例: int size = sizeof_type("T"); //
        // Queue<Vector<int>>の場合、"T"=Vector<int>で24を返す
        if (node->name == "sizeof_type" && !is_method_call) {
            if (node->arguments.size() != 1) {
                throw std::runtime_error(
                    "sizeof_type() requires 1 argument: sizeof_type(\"T\")");
            }

            // 型コンテキストから型パラメータを解決
            const TypeContext *type_ctx =
                interpreter_.get_current_type_context();
            std::string type_name;

            if (interpreter_.is_debug_mode()) {
                std::cerr << "[sizeof_type] type_ctx="
                          << (type_ctx ? "YES" : "NO");
                if (type_ctx) {
                    std::cerr
                        << ", has_T="
                        << (type_ctx->has_mapping_for("T") ? "YES" : "NO");
                }
                std::cerr << "\n";
            }

            if (type_ctx && type_ctx->has_mapping_for("T")) {
                type_name = type_ctx->resolve_type("T");
            } else {
                // フォールバック: デフォルトサイズ（プリミティブ型と仮定）
                if (interpreter_.is_debug_mode()) {
                    std::cerr << "[sizeof_type] No type context, returning "
                                 "default 8 bytes\n";
                }
                return 8;
            }

            // プリミティブ型のサイズ
            if (type_name == "int" || type_name == "long" ||
                type_name == "bool") {
                return 8; // 8バイトアライメント
            }
            if (type_name == "void*" ||
                type_name.find("*") != std::string::npos) {
                return 8; // ポインタは8バイト
            }

            // 構造体のサイズを計算
            auto struct_def = interpreter_.find_struct_definition(type_name);
            if (struct_def != nullptr) {
                size_t total_size = 0;
                for (const auto &member : struct_def->members) {
                    if (member.is_pointer) {
                        total_size += sizeof(void *); // 8 bytes
                    } else if (member.type == TYPE_LONG) {
                        total_size += sizeof(long); // 8 bytes
                    } else if (member.type == TYPE_INT) {
                        total_size += sizeof(long); // 8 bytes (アライメント)
                    } else {
                        total_size += sizeof(long); // 8 bytes デフォルト
                    }
                }

                if (interpreter_.is_debug_mode()) {
                    std::cerr << "[sizeof_type] T=" << type_name << " => "
                              << total_size << " bytes ("
                              << struct_def->members.size() << " members)\n";
                }

                return static_cast<int64_t>(total_size);
            }

            // 未知の型の場合はデフォルト8バイト
            if (interpreter_.is_debug_mode()) {
                std::cerr << "[sizeof_type] T=" << type_name
                          << " => 8 bytes (default)\n";
            }
            return 8;
        }

        // array_get(ptr, index) - 汎用配列要素取得（型推論版）
        // ジェネリクス対応: 型パラメータTから適切なarray_get_Tを呼び出す
        if (node->name == "array_get" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error(
                    "array_get() requires 2 arguments: array_get(ptr, index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (interpreter_.is_debug_mode()) {
                std::cerr << "[array_get] Called with ptr=0x" << std::hex
                          << ptr_value << std::dec << ", index=" << index
                          << "\n";
            }

            if (ptr_value == 0 || index < 0)
                return 0;

            // v0.13.1: 型コンテキストからTの実際の型を取得
            const TypeContext *type_ctx =
                interpreter_.get_current_type_context();
            if (type_ctx && type_ctx->has_mapping_for("T")) {
                std::string actual_type = type_ctx->resolve_type("T");

                if (interpreter_.is_debug_mode()) {
                    std::cerr << "[array_get] Resolved T to: " << actual_type
                              << "\n";
                }

                // 構造体型の場合、構造体定義を基にメモリから再構築
                // ネストしたジェネリック構造体（Vector<Queue<T>>など）のサポート
                auto struct_def =
                    interpreter_.find_struct_definition(actual_type);

                if (interpreter_.is_debug_mode()) {
                    std::cerr << "[array_get] struct_def for " << actual_type
                              << ": " << (struct_def ? "found" : "NOT FOUND")
                              << "\n";
                }
                if (struct_def != nullptr) {
                    // 構造体の実際のメモリサイズを取得
                    // 各メンバーの実際のサイズの合計を計算
                    size_t total_size = 0;
                    for (const auto &member : struct_def->members) {
                        if (member.is_pointer) {
                            total_size += sizeof(void *); // 8 bytes
                        } else if (member.type == TYPE_LONG) {
                            total_size += sizeof(long); // 8 bytes
                        } else if (member.type == TYPE_INT) {
                            total_size += sizeof(int); // 4 bytes
                        } else if (member.type == TYPE_FLOAT) {
                            total_size += sizeof(float); // 4 bytes
                        } else if (member.type == TYPE_DOUBLE) {
                            total_size += sizeof(double); // 8 bytes
                        } else if (member.type == TYPE_CHAR) {
                            total_size += sizeof(char); // 1 byte
                        } else {
                            total_size += sizeof(long); // 8 bytes デフォルト
                        }
                    }

                    // 配列内の要素へのポインタを計算
                    char *arr = reinterpret_cast<char *>(ptr_value);
                    char *element_ptr = arr + (index * total_size);

                    // メモリから構造体を再構築
                    Variable result;
                    result.is_struct = true;
                    result.struct_type_name = actual_type;
                    result.type_name = actual_type;
                    result.is_assigned = true;

                    // メンバーを再構築（8バイトアライメントで読み取り）
                    size_t offset = 0;
                    if (interpreter_.is_debug_mode()) {
                        std::cerr << "[array_get] Reconstructing struct "
                                  << actual_type << " from memory at "
                                  << (void *)element_ptr << ", "
                                  << struct_def->members.size() << " members\n";
                    }

                    for (const auto &member_def : struct_def->members) {
                        Variable member_var;
                        member_var.type = member_def.type;
                        member_var.is_pointer = member_def.is_pointer;

                        // 型に応じた実際のサイズで読み取り
                        if (member_def.is_pointer) {
                            member_var.value = *reinterpret_cast<int64_t *>(
                                element_ptr + offset);
                            offset += sizeof(void *);
                        } else if (member_def.type == TYPE_LONG) {
                            member_var.value = *reinterpret_cast<int64_t *>(
                                element_ptr + offset);
                            offset += sizeof(long);
                        } else if (member_def.type == TYPE_INT) {
                            member_var.value = *reinterpret_cast<int32_t *>(
                                element_ptr + offset);
                            offset += sizeof(int);
                        } else if (member_def.type == TYPE_FLOAT) {
                            float f_val = *reinterpret_cast<float *>(
                                element_ptr + offset);
                            member_var.float_value = f_val;
                            member_var.value = static_cast<int64_t>(f_val);
                            offset += sizeof(float);
                        } else if (member_def.type == TYPE_DOUBLE) {
                            double d_val = *reinterpret_cast<double *>(
                                element_ptr + offset);
                            member_var.double_value = d_val;
                            member_var.value = static_cast<int64_t>(d_val);
                            offset += sizeof(double);
                        } else if (member_def.type == TYPE_CHAR) {
                            member_var.value = *reinterpret_cast<char *>(
                                element_ptr + offset);
                            offset += sizeof(char);
                        } else {
                            member_var.value = *reinterpret_cast<int64_t *>(
                                element_ptr + offset);
                            offset += sizeof(long);
                        }

                        member_var.is_assigned = true;
                        result.struct_members[member_def.name] = member_var;

                        if (interpreter_.is_debug_mode()) {
                            size_t member_size = 0;
                            if (member_def.is_pointer) {
                                member_size = sizeof(void *);
                            } else if (member_def.type == TYPE_LONG) {
                                member_size = sizeof(long);
                            } else if (member_def.type == TYPE_INT) {
                                member_size = sizeof(int);
                            } else if (member_def.type == TYPE_FLOAT) {
                                member_size = sizeof(float);
                            } else if (member_def.type == TYPE_DOUBLE) {
                                member_size = sizeof(double);
                            } else if (member_def.type == TYPE_CHAR) {
                                member_size = sizeof(char);
                            } else {
                                member_size = sizeof(long);
                            }
                            std::cerr
                                << "[array_get]   Member " << member_def.name
                                << " at offset " << (offset - member_size)
                                << ": type="
                                << static_cast<int>(member_def.type)
                                << ", is_pointer=" << member_def.is_pointer
                                << ", value=" << member_var.value << " (0x"
                                << std::hex << member_var.value << std::dec
                                << ")\n";
                        }
                    }

                    // Deep copy for nested generic structs
                    // Vector<T>とQueue<T>のポインタメンバーをdeep copy
                    if (actual_type.find("Vector<") == 0) {
                        // Vector<T>のdeep copy: data配列をコピー
                        auto data_it = result.struct_members.find("data");
                        auto length_it = result.struct_members.find("length");
                        auto capacity_it =
                            result.struct_members.find("capacity");

                        if (data_it != result.struct_members.end() &&
                            length_it != result.struct_members.end() &&
                            capacity_it != result.struct_members.end()) {

                            void *original_data =
                                reinterpret_cast<void *>(data_it->second.value);
                            int length =
                                static_cast<int>(length_it->second.value);
                            int capacity =
                                static_cast<int>(capacity_it->second.value);

                            if (interpreter_.is_debug_mode()) {
                                std::cerr
                                    << "[array_get] Vector deep copy check: "
                                    << "data=" << original_data
                                    << ", length=" << length
                                    << ", capacity=" << capacity << "\n";
                            }

                            if (original_data != nullptr && capacity > 0) {
                                // 要素の型を取得（Vector<T>のT）
                                size_t start = actual_type.find('<') + 1;
                                size_t end = actual_type.find_last_of('>');
                                std::string element_type =
                                    actual_type.substr(start, end - start);

                                // 要素のサイズを計算
                                size_t element_size = 0;
                                auto element_struct_def =
                                    interpreter_.find_struct_definition(
                                        element_type);
                                if (element_struct_def != nullptr) {
                                    // 構造体の場合、8バイトアライメントで計算
                                    for (size_t i = 0;
                                         i < element_struct_def->members.size();
                                         ++i) {
                                        element_size += sizeof(long);
                                    }
                                } else {
                                    // プリミティブ型の場合
                                    element_size = sizeof(long);
                                }

                                // 新しいdata配列を割り当て
                                size_t total_bytes = capacity * element_size;

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr
                                        << "[array_get] About to malloc: "
                                           "capacity="
                                        << capacity
                                        << ", element_size=" << element_size
                                        << ", total_bytes=" << total_bytes
                                        << "\n";
                                }

                                void *new_data = malloc(total_bytes);

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr
                                        << "[array_get] malloc returned: 0x"
                                        << std::hex << new_data << std::dec
                                        << "\n";
                                }

                                if (new_data == nullptr) {
                                    throw std::runtime_error(
                                        "malloc failed in deep copy");
                                }

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr
                                        << "[array_get] About to memcpy: src=0x"
                                        << std::hex << original_data
                                        << ", dst=0x" << new_data << std::dec
                                        << ", bytes=" << total_bytes << "\n";
                                }

                                memcpy(new_data, original_data, total_bytes);

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr << "[array_get] memcpy completed "
                                                 "successfully\n";
                                }

                                // dataポインタを更新 + 要素型名を保存
                                data_it->second.value =
                                    reinterpret_cast<int64_t>(new_data);
                                data_it->second.type_name =
                                    element_type; // 要素型名を保存
                                data_it->second.pointer_base_type_name =
                                    element_type; // 念のため両方設定
                                result.struct_members["data"] = data_it->second;

                                // ポインタ要素型をグローバルマップに登録
                                if (interpreter_.is_debug_mode()) {
                                    std::cerr
                                        << "[array_get] Registering pointer 0x"
                                        << std::hex << new_data << std::dec
                                        << " with element type: "
                                        << element_type << "\n";
                                }
                                interpreter_.register_pointer_element_type(
                                    new_data, element_type);
                                if (interpreter_.is_debug_mode()) {
                                    std::cerr << "[array_get] Registration "
                                                 "completed\n";
                                }

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr << "[array_get] Updated "
                                                 "result.struct_members["
                                                 "\"data\"] to 0x"
                                              << std::hex << new_data
                                              << std::dec << " (was 0x"
                                              << std::hex << original_data
                                              << std::dec << ")\n";
                                }

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr << "[array_get] Deep copied "
                                                 "Vector data: "
                                              << total_bytes << " bytes from 0x"
                                              << std::hex << original_data
                                              << " to 0x" << new_data
                                              << std::dec << "\n";

                                    // メモリの内容を確認
                                    if (element_type == "long" &&
                                        capacity >= 3) {
                                        long *src = reinterpret_cast<long *>(
                                            original_data);
                                        long *dst =
                                            reinterpret_cast<long *>(new_data);
                                        std::cerr
                                            << "[array_get]   Original data[0]="
                                            << src[0] << ", [1]=" << src[1]
                                            << ", [2]=" << src[2] << "\n";
                                        std::cerr
                                            << "[array_get]   Copied data[0]="
                                            << dst[0] << ", [1]=" << dst[1]
                                            << ", [2]=" << dst[2] << "\n";
                                    }
                                }
                            }
                        }
                    } else if (actual_type.find("Queue<") == 0) {
                        // Queue<T>のdeep copy: リンクリストをコピー
                        if (interpreter_.is_debug_mode()) {
                            std::cerr << "[array_get] Starting Queue<T> deep "
                                         "copy for type: "
                                      << actual_type << "\n";
                        }
                        auto front_it = result.struct_members.find("front");
                        auto rear_it = result.struct_members.find("rear");
                        auto length_it = result.struct_members.find("length");

                        if (front_it != result.struct_members.end() &&
                            rear_it != result.struct_members.end() &&
                            length_it != result.struct_members.end()) {

                            void *original_front = reinterpret_cast<void *>(
                                front_it->second.value);
                            int length =
                                static_cast<int>(length_it->second.value);

                            if (original_front != nullptr && length > 0) {
                                // 要素の型を取得（Queue<T>のT）
                                size_t start = actual_type.find('<') + 1;
                                size_t end = actual_type.find_last_of('>');
                                std::string element_type =
                                    actual_type.substr(start, end - start);

                                // ノードのサイズを計算（T data + void* next）
                                // 注意:
                                // Cbインタープリタは全ての型を8バイトアライメントで扱うため、
                                // primitive型でも8バイトとして計算する
                                size_t data_size = 0;
                                auto element_struct_def =
                                    interpreter_.find_struct_definition(
                                        element_type);
                                if (element_struct_def != nullptr) {
                                    for (size_t i = 0;
                                         i < element_struct_def->members.size();
                                         ++i) {
                                        data_size += 8; // 常に8バイト
                                    }
                                } else {
                                    data_size = 8; // primitive型も8バイト
                                }
                                size_t node_size =
                                    data_size +
                                    8; // data + next (両方8バイトアライメント)

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr << "[array_get] Queue node "
                                                 "calculation: element_type="
                                              << element_type
                                              << ", data_size=" << data_size
                                              << ", node_size=" << node_size
                                              << ", length=" << length << "\n";
                                    std::cerr << "[array_get] original_front=0x"
                                              << std::hex << original_front
                                              << std::dec << "\n";
                                }

                                // リンクリストをコピー
                                void *new_front = nullptr;
                                void *new_rear = nullptr;
                                void *current_old = original_front;

                                while (current_old != nullptr) {
                                    if (interpreter_.is_debug_mode()) {
                                        std::cerr << "[array_get] Processing "
                                                     "node at 0x"
                                                  << std::hex << current_old
                                                  << std::dec << "\n";
                                        // ノードの内容をダンプ
                                        int64_t *node_data =
                                            reinterpret_cast<int64_t *>(
                                                current_old);
                                        std::cerr
                                            << "[array_get]   Node data[0]="
                                            << node_data[0] << ", data[1]=0x"
                                            << std::hex << node_data[1]
                                            << std::dec << "\n";
                                    }

                                    // 新しいノードを割り当て
                                    void *new_node = malloc(node_size);

                                    if (interpreter_.is_debug_mode()) {
                                        std::cerr << "[array_get] Allocated "
                                                     "new_node at 0x"
                                                  << std::hex << new_node
                                                  << std::dec << "\n";
                                    }

                                    // Vectorの場合は構造体全体（24バイト）をコピー、それ以外は8バイト
                                    size_t copy_size = data_size;
                                    if (element_type.find("Vector<") == 0) {
                                        copy_size =
                                            24; // Vector struct: data(8) +
                                                // length(8) + capacity(8)
                                    }
                                    memcpy(new_node, current_old, copy_size);

                                    if (interpreter_.is_debug_mode()) {
                                        std::cerr << "[array_get] Copied data ("
                                                  << copy_size << " bytes)\n";
                                    }

                                    // nextポインタを初期化（nullに設定）
                                    char *next_field_addr =
                                        reinterpret_cast<char *>(new_node) +
                                        data_size;
                                    *reinterpret_cast<void **>(
                                        next_field_addr) = nullptr;

                                    // ノード内の要素がVectorの場合、deep copy
                                    if (element_type.find("Vector<") == 0) {
                                        char *new_node_data =
                                            reinterpret_cast<char *>(new_node);

                                        // Vectorのメンバーを読み取り
                                        void *vec_data =
                                            *reinterpret_cast<void **>(
                                                new_node_data + 0); // data
                                        int vec_length =
                                            *reinterpret_cast<int *>(
                                                new_node_data + 8); // length
                                        int vec_capacity =
                                            *reinterpret_cast<int *>(
                                                new_node_data + 16); // capacity

                                        if (interpreter_.is_debug_mode()) {
                                            std::cerr
                                                << "[array_get] Vector in "
                                                   "Queue node: "
                                                << "data=0x" << std::hex
                                                << vec_data << std::dec
                                                << ", length=" << vec_length
                                                << ", capacity=" << vec_capacity
                                                << "\n";
                                        }

                                        if (vec_data != nullptr &&
                                            vec_capacity > 0) {
                                            // Vector内の要素型を取得
                                            size_t vec_start =
                                                element_type.find('<') + 1;
                                            size_t vec_end =
                                                element_type.find_last_of('>');
                                            std::string vec_element_type =
                                                element_type.substr(
                                                    vec_start,
                                                    vec_end - vec_start);

                                            // 要素サイズを計算
                                            size_t vec_element_size = 0;
                                            auto vec_element_struct_def =
                                                interpreter_
                                                    .find_struct_definition(
                                                        vec_element_type);
                                            if (vec_element_struct_def !=
                                                nullptr) {
                                                for (size_t i = 0;
                                                     i < vec_element_struct_def
                                                             ->members.size();
                                                     ++i) {
                                                    vec_element_size +=
                                                        sizeof(long);
                                                }
                                            } else {
                                                vec_element_size = sizeof(long);
                                            }

                                            // Vector data配列をコピー
                                            size_t vec_total_bytes =
                                                vec_capacity * vec_element_size;
                                            void *new_vec_data =
                                                malloc(vec_total_bytes);
                                            memcpy(new_vec_data, vec_data,
                                                   vec_total_bytes);

                                            // 新しいdataポインタを設定
                                            *reinterpret_cast<void **>(
                                                new_node_data + 0) =
                                                new_vec_data;

                                            if (interpreter_.is_debug_mode()) {
                                                std::cerr
                                                    << "[array_get] Deep "
                                                       "copied "
                                                       "Vector in Queue node: "
                                                    << vec_total_bytes
                                                    << " bytes\n";
                                            }
                                        }
                                    }

                                    // リンクリストを構築
                                    if (new_front == nullptr) {
                                        new_front = new_node;
                                    }
                                    if (new_rear != nullptr) {
                                        // 前のノードのnextを更新
                                        char *prev_next_field =
                                            reinterpret_cast<char *>(new_rear) +
                                            data_size;
                                        *reinterpret_cast<void **>(
                                            prev_next_field) = new_node;
                                    }
                                    new_rear = new_node;

                                    // 次のノードへ
                                    char *old_next_field =
                                        reinterpret_cast<char *>(current_old) +
                                        data_size;
                                    current_old = *reinterpret_cast<void **>(
                                        old_next_field);
                                }

                                // frontとrearポインタを更新
                                front_it->second.value =
                                    reinterpret_cast<int64_t>(new_front);
                                rear_it->second.value =
                                    reinterpret_cast<int64_t>(new_rear);
                                result.struct_members["front"] =
                                    front_it->second;
                                result.struct_members["rear"] = rear_it->second;

                                if (interpreter_.is_debug_mode()) {
                                    std::cerr
                                        << "[array_get] Deep copied Queue: "
                                        << length << " nodes from 0x"
                                        << std::hex << original_front
                                        << " to 0x" << new_front << std::dec
                                        << "\n";
                                }
                            }
                        }
                    }

                    // ReturnExceptionで構造体を返す
                    throw ReturnException(result);
                }

                // 型に応じて適切にメモリから読み取る
                if (actual_type == "short") {
                    short *arr = reinterpret_cast<short *>(ptr_value);
                    return static_cast<int64_t>(arr[index]);
                } else if (actual_type == "long") {
                    long *arr = reinterpret_cast<long *>(ptr_value);
                    if (interpreter_.is_debug_mode()) {
                        std::cerr << "[array_get] Reading long at ptr=0x"
                                  << std::hex << ptr_value << std::dec
                                  << ", index=" << index
                                  << ", offset=" << (index * sizeof(long))
                                  << ", value=" << arr[index] << "\n";
                        // メモリ内容も確認
                        std::cerr << "[array_get]   Memory: arr[0]=" << arr[0]
                                  << ", arr[1]=" << arr[1]
                                  << ", arr[2]=" << arr[2] << "\n";
                    }
                    return static_cast<int64_t>(arr[index]);
                } else if (actual_type == "char") {
                    char *arr = reinterpret_cast<char *>(ptr_value);
                    return static_cast<int64_t>(arr[index]);
                }
                // int, その他はデフォルトのint扱い
            } else {
                // 型コンテキストがない場合：ポインタ要素型マップから型名を取得
                std::string element_type_name =
                    interpreter_.get_pointer_element_type(
                        reinterpret_cast<void *>(ptr_value));

                if (interpreter_.is_debug_mode()) {
                    if (!element_type_name.empty()) {
                        std::cerr
                            << "[array_get] Got element type from pointer map: "
                            << element_type_name << " for ptr=0x" << std::hex
                            << ptr_value << std::dec << "\n";
                    } else {
                        std::cerr
                            << "[array_get] No element type in map for ptr=0x"
                            << std::hex << ptr_value << std::dec << "\n";
                    }
                }

                // 型名が取得できた場合、その型で読み取り
                if (!element_type_name.empty()) {
                    if (element_type_name == "long") {
                        long *arr = reinterpret_cast<long *>(ptr_value);
                        if (interpreter_.is_debug_mode()) {
                            std::cerr << "[array_get] Reading long (from "
                                         "pointer map) at ptr=0x"
                                      << std::hex << ptr_value << std::dec
                                      << ", index=" << index
                                      << ", value=" << arr[index] << "\n";
                        }
                        return static_cast<int64_t>(arr[index]);
                    } else if (element_type_name == "int") {
                        int *arr = reinterpret_cast<int *>(ptr_value);
                        return static_cast<int64_t>(arr[index]);
                    } else if (element_type_name == "short") {
                        short *arr = reinterpret_cast<short *>(ptr_value);
                        return static_cast<int64_t>(arr[index]);
                    } else if (element_type_name == "char") {
                        char *arr = reinterpret_cast<char *>(ptr_value);
                        return static_cast<int64_t>(arr[index]);
                    }
                }
            }

            // デフォルトはintとして扱う
            if (interpreter_.is_debug_mode()) {
                std::cerr
                    << "[array_get] WARNING: Fallback to int type for ptr=0x"
                    << std::hex << ptr_value << std::dec << ", index=" << index
                    << "\n";
            }
            int *arr = reinterpret_cast<int *>(ptr_value);
            return static_cast<int64_t>(arr[index]);
        }

        // array_set(ptr, index, value) - 汎用配列要素設定（型推論版）
        // ジェネリクス対応: 型パラメータTから適切なarray_set_Tを呼び出す
        if (node->name == "array_set" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error("array_set() requires 3 arguments: "
                                         "array_set(ptr, index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0 || index < 0)
                return 0;

            // v0.13.1: 型コンテキストからTの実際の型を取得
            const TypeContext *type_ctx =
                interpreter_.get_current_type_context();
            if (type_ctx && type_ctx->has_mapping_for("T")) {
                std::string actual_type = type_ctx->resolve_type("T");

                // 構造体型の場合、構造体全体をメモリにコピー
                auto struct_def =
                    interpreter_.find_struct_definition(actual_type);
                if (struct_def != nullptr) {
                    // 第3引数は構造体変数
                    const ASTNode *value_node = node->arguments[2].get();

                    // 構造体変数を評価して取得
                    try {
                        (void)interpreter_.eval_expression(value_node);
                        // 通常は数値が返るがエラー
                        throw std::runtime_error(
                            "array_set: expected struct but got numeric value");
                    } catch (const ReturnException &ret) {
                        if (ret.is_struct) {
                            // 構造体の実際のサイズを計算
                            // NOTE: sizeof(T)を使うのが正確だが、Cbインタプリタ内では
                            //       Cbコードで計算されたsizeof(T)を使う必要がある
                            //       ここではメンバーの実際の型サイズを使用
                            size_t total_size = 0;
                            for (const auto &member_def : struct_def->members) {
                                size_t member_size = sizeof(long); // デフォルト8バイト
                                
                                // メンバーの型に応じてサイズを調整
                                if (member_def.type == TYPE_INT || member_def.type == TYPE_FLOAT) {
                                    member_size = sizeof(int); // 4バイト
                                } else if (member_def.type == TYPE_SHORT) {
                                    member_size = sizeof(short); // 2バイト
                                } else if (member_def.type == TYPE_CHAR || member_def.type == TYPE_TINY) {
                                    member_size = sizeof(char); // 1バイト
                                } else if (member_def.type == TYPE_LONG || member_def.type == TYPE_DOUBLE ||
                                           member_def.type == TYPE_POINTER || member_def.type == TYPE_STRING) {
                                    member_size = sizeof(long); // 8バイト
                                }
                                // TODO: 構造体メンバーや配列の場合は再帰的にサイズ計算が必要
                                
                                total_size += member_size;
                            }

                            // 配列内の書き込み位置を計算
                            char *arr = reinterpret_cast<char *>(ptr_value);
                            char *element_ptr = arr + (index * total_size);

                            // 構造体メンバーをメモリに書き込み
                            // Vector<T>の場合はdataポインタをdeep copyする
                            bool is_vector = (actual_type.find("Vector<") == 0);
                            void *original_data_ptr = nullptr;
                            size_t vec_capacity = 0;
                            std::string vec_element_type;

                            if (is_vector) {
                                // Vector<T>の要素型を取得
                                size_t start = actual_type.find('<') + 1;
                                size_t end = actual_type.find_last_of('>');
                                vec_element_type =
                                    actual_type.substr(start, end - start);

                                // dataとcapacityを取得
                                auto data_it =
                                    ret.struct_value.struct_members.find(
                                        "data");
                                auto capacity_it =
                                    ret.struct_value.struct_members.find(
                                        "capacity");
                                if (data_it !=
                                        ret.struct_value.struct_members.end() &&
                                    capacity_it !=
                                        ret.struct_value.struct_members.end()) {
                                    original_data_ptr =
                                        reinterpret_cast<void *>(
                                            data_it->second.value);
                                    vec_capacity = static_cast<size_t>(
                                        capacity_it->second.value);
                                }
                            }

                            size_t offset = 0;
                            for (const auto &member_def : struct_def->members) {
                                auto it = ret.struct_value.struct_members.find(
                                    member_def.name);
                                if (it !=
                                    ret.struct_value.struct_members.end()) {
                                    int64_t value_to_write = it->second.value;

                                    // Vector<T>のdataメンバーの場合、deep
                                    // copyを行う
                                    if (is_vector &&
                                        member_def.name == "data" &&
                                        original_data_ptr != nullptr &&
                                        vec_capacity > 0) {
                                        // 要素サイズを計算
                                        size_t element_size = 0;
                                        auto element_struct_def =
                                            interpreter_.find_struct_definition(
                                                vec_element_type);
                                        if (element_struct_def != nullptr) {
                                            for (size_t i = 0;
                                                 i < element_struct_def->members
                                                         .size();
                                                 ++i) {
                                                element_size += sizeof(long);
                                            }
                                        } else {
                                            element_size = sizeof(long);
                                        }

                                        // 新しいdata配列を確保してコピー
                                        size_t total_bytes =
                                            vec_capacity * element_size;
                                        void *new_data = malloc(total_bytes);
                                        memcpy(new_data, original_data_ptr,
                                               total_bytes);

                                        // 新しいポインタを書き込む
                                        value_to_write =
                                            reinterpret_cast<int64_t>(new_data);

                                        // ポインタ要素型を登録
                                        interpreter_
                                            .register_pointer_element_type(
                                                new_data, vec_element_type);

                                        if (interpreter_.is_debug_mode()) {
                                            std::cerr
                                                << "[array_set] Deep copied "
                                                   "Vector data: "
                                                << total_bytes
                                                << " bytes from 0x" << std::hex
                                                << original_data_ptr << " to 0x"
                                                << new_data << std::dec << "\n";
                                        }
                                    }

                                    // メンバーサイズに応じて書き込み
                                    size_t member_size = sizeof(long); // デフォルト8バイト
                                    
                                    if (member_def.type == TYPE_INT || member_def.type == TYPE_FLOAT) {
                                        member_size = sizeof(int); // 4バイト
                                        *reinterpret_cast<int32_t *>(element_ptr + offset) =
                                            static_cast<int32_t>(value_to_write);
                                    } else if (member_def.type == TYPE_SHORT) {
                                        member_size = sizeof(short); // 2バイト
                                        *reinterpret_cast<int16_t *>(element_ptr + offset) =
                                            static_cast<int16_t>(value_to_write);
                                    } else if (member_def.type == TYPE_CHAR || member_def.type == TYPE_TINY) {
                                        member_size = sizeof(char); // 1バイト
                                        *reinterpret_cast<int8_t *>(element_ptr + offset) =
                                            static_cast<int8_t>(value_to_write);
                                    } else {
                                        member_size = sizeof(long); // 8バイト
                                        *reinterpret_cast<int64_t *>(element_ptr + offset) = value_to_write;
                                    }
                                    
                                    offset += member_size;
                                } else {
                                    // デフォルト値を書き込み
                                    size_t member_size = sizeof(long); // デフォルト8バイト
                                    
                                    if (member_def.type == TYPE_INT || member_def.type == TYPE_FLOAT) {
                                        member_size = sizeof(int); // 4バイト
                                        *reinterpret_cast<int32_t *>(element_ptr + offset) = 0;
                                    } else if (member_def.type == TYPE_SHORT) {
                                        member_size = sizeof(short); // 2バイト
                                        *reinterpret_cast<int16_t *>(element_ptr + offset) = 0;
                                    } else if (member_def.type == TYPE_CHAR || member_def.type == TYPE_TINY) {
                                        member_size = sizeof(char); // 1バイト
                                        *reinterpret_cast<int8_t *>(element_ptr + offset) = 0;
                                    } else {
                                        member_size = sizeof(long); // 8バイト
                                        *reinterpret_cast<int64_t *>(element_ptr + offset) = 0;
                                    }
                                    
                                    offset += member_size;
                                }
                            }

                            if (interpreter_.is_debug_mode()) {
                                std::cerr << "[array_set] Wrote struct "
                                          << actual_type
                                          << " to array at index " << index
                                          << ", total_size=" << total_size
                                          << "\n";
                                size_t debug_offset = 0;
                                for (const auto &member_def :
                                     struct_def->members) {
                                    size_t member_size = sizeof(long);
                                    if (member_def.type == TYPE_INT || member_def.type == TYPE_FLOAT) {
                                        member_size = sizeof(int);
                                    } else if (member_def.type == TYPE_SHORT) {
                                        member_size = sizeof(short);
                                    } else if (member_def.type == TYPE_CHAR || member_def.type == TYPE_TINY) {
                                        member_size = sizeof(char);
                                    }
                                    
                                    auto it =
                                        ret.struct_value.struct_members.find(
                                            member_def.name);
                                    if (it !=
                                        ret.struct_value.struct_members.end()) {
                                        std::cerr << "[array_set]   Member "
                                                  << member_def.name
                                                  << " at offset "
                                                  << debug_offset << ": "
                                                  << it->second.value << "\n";
                                    }
                                    debug_offset += member_size;
                                }
                            }

                            return 0;
                        }
                    }

                    // 構造体でなければ変数参照として処理
                    std::string var_name;
                    if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
                        var_name = value_node->name;
                        Variable *struct_var =
                            interpreter_.find_variable(var_name);
                        if (struct_var && struct_var->is_struct) {
                            // 構造体の実際のサイズを計算
                            size_t total_size = 0;
                            for (const auto &member_def : struct_def->members) {
                                size_t member_size = sizeof(long); // デフォルト8バイト
                                
                                // メンバーの型に応じてサイズを調整
                                if (member_def.type == TYPE_INT || member_def.type == TYPE_FLOAT) {
                                    member_size = sizeof(int); // 4バイト
                                } else if (member_def.type == TYPE_SHORT) {
                                    member_size = sizeof(short); // 2バイト
                                } else if (member_def.type == TYPE_CHAR || member_def.type == TYPE_TINY) {
                                    member_size = sizeof(char); // 1バイト
                                } else if (member_def.type == TYPE_LONG || member_def.type == TYPE_DOUBLE ||
                                           member_def.type == TYPE_POINTER || member_def.type == TYPE_STRING) {
                                    member_size = sizeof(long); // 8バイト
                                }
                                
                                total_size += member_size;
                            }

                            // 配列内の書き込み位置を計算
                            char *arr = reinterpret_cast<char *>(ptr_value);
                            char *element_ptr = arr + (index * total_size);

                            // 構造体メンバーをメモリに書き込み
                            size_t offset = 0;
                            for (const auto &member_def : struct_def->members) {
                                size_t member_size = sizeof(long); // デフォルト8バイト
                                
                                // メンバーの型に応じてサイズを調整
                                if (member_def.type == TYPE_INT || member_def.type == TYPE_FLOAT) {
                                    member_size = sizeof(int); // 4バイト
                                } else if (member_def.type == TYPE_SHORT) {
                                    member_size = sizeof(short); // 2バイト
                                } else if (member_def.type == TYPE_CHAR || member_def.type == TYPE_TINY) {
                                    member_size = sizeof(char); // 1バイト
                                } else if (member_def.type == TYPE_LONG || member_def.type == TYPE_DOUBLE ||
                                           member_def.type == TYPE_POINTER || member_def.type == TYPE_STRING) {
                                    member_size = sizeof(long); // 8バイト
                                }
                                
                                auto it = struct_var->struct_members.find(
                                    member_def.name);
                                if (it != struct_var->struct_members.end()) {
                                    // メンバーサイズに応じて書き込み
                                    if (member_size == 4) {
                                        *reinterpret_cast<int32_t *>(element_ptr + offset) =
                                            static_cast<int32_t>(it->second.value);
                                    } else {
                                        *reinterpret_cast<int64_t *>(element_ptr + offset) =
                                            it->second.value;
                                    }
                                } else {
                                    // デフォルト値を書き込み
                                    if (member_size == 4) {
                                        *reinterpret_cast<int32_t *>(element_ptr + offset) = 0;
                                    } else {
                                        *reinterpret_cast<int64_t *>(element_ptr + offset) = 0;
                                    }
                                }
                                offset += member_size;
                            }

                            if (interpreter_.is_debug_mode()) {
                                std::cerr << "[array_set] Wrote struct "
                                          << actual_type << " (" << var_name
                                          << ") to array at index " << index
                                          << ", total_size=" << total_size
                                          << "\n";
                                for (const auto &m :
                                     struct_var->struct_members) {
                                    std::cerr << "[array_set]   " << m.first
                                              << " = " << m.second.value
                                              << "\n";
                                }
                            }

                            return 0;
                        }
                    }
                }

                // プリミティブ型の処理
                int64_t value =
                    interpreter_.eval_expression(node->arguments[2].get());

                // 型に応じて適切にメモリに書き込む
                if (actual_type == "short") {
                    short *arr = reinterpret_cast<short *>(ptr_value);
                    arr[index] = static_cast<short>(value);
                    return 0;
                } else if (actual_type == "long") {
                    long *arr = reinterpret_cast<long *>(ptr_value);
                    arr[index] = static_cast<long>(value);
                    return 0;
                } else if (actual_type == "char") {
                    char *arr = reinterpret_cast<char *>(ptr_value);
                    arr[index] = static_cast<char>(value);
                    return 0;
                }
                // int, その他はデフォルトのint扱い
            }

            // デフォルトはintとして扱う
            int64_t value =
                interpreter_.eval_expression(node->arguments[2].get());
            int *arr = reinterpret_cast<int *>(ptr_value);
            arr[index] = static_cast<int>(value);
            return 0;
        }

        // default(T) - 型Tのデフォルト値を返す
        // ジェネリクス対応: 型パラメータTに応じたデフォルト値
        if (node->name == "default" && !is_method_call) {
            if (node->arguments.size() != 1) {
                throw std::runtime_error(
                    "default() requires 1 argument: default(T)");
            }

            // 型名を取得
            const ASTNode *arg = node->arguments[0].get();
            if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                std::string type_name = arg->name;

                // 型に応じたデフォルト値を返す
                if (type_name == "int" || type_name == "long" ||
                    type_name == "short" || type_name == "char") {
                    return 0;
                }
                if (type_name == "bool") {
                    return 0; // false
                }
                if (type_name == "double" || type_name == "float") {
                    // 0.0をint64_tビット表現で返す
                    union {
                        double d;
                        int64_t i;
                    } converter;
                    converter.d = 0.0;
                    return converter.i;
                }
                if (type_name == "string") {
                    return 0; // 空文字列（nullポインタ）
                }

                // その他の型はnullptrまたは0を返す
                return 0;
            }

            return 0;
        }

        // call_function_pointer(func_ptr, arg1, arg2, ...) -
        // 関数ポインタを呼び出す ジェネリック対応:
        // 任意の数の引数で関数ポインタを呼び出す
        if (node->name == "call_function_pointer" && !is_method_call) {
            if (node->arguments.size() < 1) {
                throw std::runtime_error(
                    "call_function_pointer() requires at least 1 argument: "
                    "call_function_pointer(func_ptr, arg1, arg2, ...)");
            }

            // 第1引数: 関数ポインタ（ASTNodeのアドレス）
            int64_t func_ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());

            if (func_ptr_value == 0) {
                throw std::runtime_error(
                    "call_function_pointer: function pointer is null");
            }

            // 関数ポインタの値はASTNode*のアドレス
            // まずVariable*として試してみて、is_function_pointerかチェック
            // そうでなければASTNode*として扱う
            const ASTNode *func_def = nullptr;
            std::string func_name;

            // Variable*として解釈を試みる
            Variable *func_ptr_var =
                reinterpret_cast<Variable *>(func_ptr_value);
            if (func_ptr_var->is_function_pointer) {
                // Variable経由の関数ポインタ（パラメータとして渡された場合など）
                func_name = func_ptr_var->function_pointer_name;
                func_def = interpreter_.find_function(func_name);
            } else {
                // 直接ASTNode*として渡された場合（&func形式）
                func_def = reinterpret_cast<const ASTNode *>(func_ptr_value);
                func_name = func_def->name;
            }

            if (!func_def) {
                throw std::runtime_error("call_function_pointer: function '" +
                                         func_name + "' not found");
            }

            // 残りの引数を評価
            std::vector<int64_t> arg_values;
            for (size_t i = 1; i < node->arguments.size(); ++i) {
                arg_values.push_back(
                    interpreter_.eval_expression(node->arguments[i].get()));
            }

            // 引数の数をチェック
            if (arg_values.size() != func_def->parameters.size()) {
                throw std::runtime_error(
                    "call_function_pointer: argument count mismatch for '" +
                    func_name + "': expected " +
                    std::to_string(func_def->parameters.size()) + ", got " +
                    std::to_string(arg_values.size()));
            }

            // 新しいスコープを作成
            interpreter_.push_scope();

            try {
                // パラメータをバインド
                for (size_t i = 0; i < func_def->parameters.size(); ++i) {
                    const ASTNode *param = func_def->parameters[i].get();
                    Variable var;
                    var.value = arg_values[i];
                    var.is_assigned = true;
                    var.type = param->type_info; // パラメータの型情報を使用
                    interpreter_.current_scope().variables[param->name] = var;
                }

                // 関数本体を実行
                try {
                    interpreter_.execute_statement(func_def->body.get());
                } catch (const ReturnException &re) {
                    // 戻り値を取得
                    interpreter_.pop_scope();
                    return re.value;
                }

                // 戻り値なし（voidまたは明示的なreturn文なし）
                interpreter_.pop_scope();
                return 0;

            } catch (...) {
                interpreter_.pop_scope();
                throw;
            }
        }

        // array_get_int(ptr, index) - 配列要素を取得
        if (node->name == "array_get_int" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error(
                    "array_get_int() requires 2 arguments: array_get_int(ptr, "
                    "index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0) {
                std::cerr << "[array_get_int] Error: null pointer" << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_get_int] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            int *arr = reinterpret_cast<int *>(ptr_value);
            return static_cast<int64_t>(arr[index]);
        }

        // array_set_int(ptr, index, value) - 配列要素を設定
        if (node->name == "array_set_int" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error(
                    "array_set_int() requires 3 arguments: array_set_int(ptr, "
                    "index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());
            int64_t value =
                interpreter_.eval_expression(node->arguments[2].get());

            if (ptr_value == 0) {
                std::cerr << "[array_set_int] Error: null pointer" << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_set_int] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            int *arr = reinterpret_cast<int *>(ptr_value);
            arr[index] = static_cast<int>(value);
            return 0;
        }

        // array_get_long(ptr, index) - long配列要素を取得
        if (node->name == "array_get_long" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error("array_get_long() requires 2 "
                                         "arguments: array_get_long(ptr, "
                                         "index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0) {
                std::cerr << "[array_get_long] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_get_long] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            long *arr = reinterpret_cast<long *>(ptr_value);
            return static_cast<int64_t>(arr[index]);
        }

        // array_set_long(ptr, index, value) - long配列要素を設定
        if (node->name == "array_set_long" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error("array_set_long() requires 3 "
                                         "arguments: array_set_long(ptr, "
                                         "index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());
            int64_t value =
                interpreter_.eval_expression(node->arguments[2].get());

            if (ptr_value == 0) {
                std::cerr << "[array_set_long] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_set_long] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            long *arr = reinterpret_cast<long *>(ptr_value);
            arr[index] = static_cast<long>(value);
            return 0;
        }

        // array_get_char(ptr, index) - char配列要素を取得
        if (node->name == "array_get_char" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error("array_get_char() requires 2 "
                                         "arguments: array_get_char(ptr, "
                                         "index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0) {
                std::cerr << "[array_get_char] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_get_char] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            char *arr = reinterpret_cast<char *>(ptr_value);
            return static_cast<int64_t>(arr[index]);
        }

        // array_set_char(ptr, index, value) - char配列要素を設定
        if (node->name == "array_set_char" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error("array_set_char() requires 3 "
                                         "arguments: array_set_char(ptr, "
                                         "index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());
            int64_t value =
                interpreter_.eval_expression(node->arguments[2].get());

            if (ptr_value == 0) {
                std::cerr << "[array_set_char] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_set_char] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            char *arr = reinterpret_cast<char *>(ptr_value);
            arr[index] = static_cast<char>(value);
            return 0;
        }

        // array_get_bool(ptr, index) - bool配列要素を取得
        if (node->name == "array_get_bool" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error("array_get_bool() requires 2 "
                                         "arguments: array_get_bool(ptr, "
                                         "index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0) {
                std::cerr << "[array_get_bool] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_get_bool] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            bool *arr = reinterpret_cast<bool *>(ptr_value);
            return static_cast<int64_t>(arr[index] ? 1 : 0);
        }

        // array_set_bool(ptr, index, value) - bool配列要素を設定
        if (node->name == "array_set_bool" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error("array_set_bool() requires 3 "
                                         "arguments: array_set_bool(ptr, "
                                         "index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());
            int64_t value =
                interpreter_.eval_expression(node->arguments[2].get());

            if (ptr_value == 0) {
                std::cerr << "[array_set_bool] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_set_bool] Error: negative index " << index
                          << std::endl;
                return 0;
            }

            bool *arr = reinterpret_cast<bool *>(ptr_value);
            arr[index] = (value != 0);
            return 0;
        }

        // malloc(size) - メモリ確保
        // sizeof(type) - 型のサイズを取得
        // 注: sizeof演算子として実装すべきだが、簡易版として組み込み関数で実装
        if (node->name == "sizeof" && !is_method_call) {
            if (node->arguments.size() != 1) {
                throw std::runtime_error(
                    "sizeof() requires 1 argument: sizeof(type_expression)");
            }

            // 引数の型を推論
            const ASTNode *arg = node->arguments[0].get();

            // 型名が直接渡された場合（AST_VARIABLEで型名として解釈）
            if (arg->node_type == ASTNodeType::AST_VARIABLE) {
                std::string name = arg->name;

                // まず変数として検索
                Variable *var = interpreter_.find_variable(name);
                if (var) {
                    // 変数が見つかった場合、その型のサイズを返す
                    switch (var->type) {
                    case TYPE_INT:
                        return sizeof(int);
                    case TYPE_LONG:
                        return sizeof(long);
                    case TYPE_SHORT:
                        return sizeof(short);
                    case TYPE_CHAR:
                        return sizeof(char);
                    case TYPE_BOOL:
                        return sizeof(bool);
                    case TYPE_FLOAT:
                        return sizeof(float);
                    case TYPE_DOUBLE:
                        return sizeof(double);
                    case TYPE_QUAD:
                        return sizeof(long double);
                    case TYPE_POINTER:
                        return sizeof(void *);
                    case TYPE_STRING:
                        return sizeof(void *);
                    default:
                        if (var->is_struct)
                            return sizeof(void *);
                        throw std::runtime_error(
                            "Cannot determine size of variable type");
                    }
                }

                // 変数でない場合は型名として解釈
                std::string type_name = name;

                // プリミティブ型のサイズを返す
                if (type_name == "int")
                    return sizeof(int);
                if (type_name == "long")
                    return sizeof(long);
                if (type_name == "short")
                    return sizeof(short);
                if (type_name == "char")
                    return sizeof(char);
                if (type_name == "bool")
                    return sizeof(bool);
                if (type_name == "float")
                    return sizeof(float);
                if (type_name == "double")
                    return sizeof(double);
                if (type_name == "quad")
                    return sizeof(long double);
                if (type_name == "void*")
                    return sizeof(void *);

                // 構造体のサイズを取得
                Variable *struct_def = interpreter_.find_variable(type_name);
                if (struct_def && struct_def->is_struct) {
                    // 構造体のサイズは、全メンバーのサイズの合計
                    // 簡易実装: ポインタサイズを返す（構造体は参照渡しのため）
                    return sizeof(void *);
                }

                throw std::runtime_error("Unknown type for sizeof: " +
                                         type_name);
            }

            // 式の型を推論
            TypedValue typed_val = interpreter_.evaluate_typed(arg);

            switch (typed_val.type.type_info) {
            case TYPE_INT:
                return sizeof(int);
            case TYPE_LONG:
                return sizeof(long);
            case TYPE_SHORT:
                return sizeof(short);
            case TYPE_CHAR:
                return sizeof(char);
            case TYPE_BOOL:
                return sizeof(bool);
            case TYPE_FLOAT:
                return sizeof(float);
            case TYPE_DOUBLE:
                return sizeof(double);
            case TYPE_QUAD:
                return sizeof(long double);
            case TYPE_POINTER:
                return sizeof(void *);
            case TYPE_STRING:
                return sizeof(void *); // 文字列ポインタ
            default:
                throw std::runtime_error("Cannot determine size of type");
            }
        }

        if (node->name == "malloc" && !is_method_call) {
            if (node->arguments.size() != 1) {
                throw std::runtime_error(
                    "malloc() requires 1 argument: malloc(size)");
            }

            int64_t size =
                interpreter_.eval_expression(node->arguments[0].get());

            if (size <= 0) {
                std::cerr << "[malloc] Error: invalid size " << size
                          << std::endl;
                return 0;
            }

            void *ptr = std::malloc(static_cast<size_t>(size));
            if (ptr == nullptr) {
                std::cerr << "[malloc] Error: allocation failed for size "
                          << size << std::endl;
                return 0;
            }

            return reinterpret_cast<int64_t>(ptr);
        }

        // free(ptr) - メモリ解放
        if (node->name == "free" && !is_method_call) {
            if (node->arguments.size() != 1) {
                throw std::runtime_error(
                    "free() requires 1 argument: free(ptr)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());

            if (ptr_value == 0) {
                // nullptr の解放は何もしない
                return 0;
            }

            std::free(reinterpret_cast<void *>(ptr_value));
            return 0;
        }

        // array_get_double(ptr, index) - double配列要素を取得
        if (node->name == "array_get_double" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error("array_get_double() requires 2 "
                                         "arguments: array_get_double(ptr, "
                                         "index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0) {
                std::cerr << "[array_get_double] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_get_double] Error: negative index "
                          << index << std::endl;
                return 0;
            }

            double *arr = reinterpret_cast<double *>(ptr_value);
            double value = arr[index];

            // doubleをint64_tのビット表現として返す
            union {
                double d;
                int64_t i;
            } converter;
            converter.d = value;

            return converter.i;
        }

        // array_set_double(ptr, index, value) - double配列要素を設定
        if (node->name == "array_set_double" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error("array_set_double() requires 3 "
                                         "arguments: array_set_double(ptr, "
                                         "index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            // doubleの値を取得するにはevaluate_typedを使用
            TypedValue typed_val =
                interpreter_.evaluate_typed(node->arguments[2].get());
            double value = typed_val.as_double();

            if (ptr_value == 0) {
                std::cerr << "[array_set_double] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_set_double] Error: negative index "
                          << index << std::endl;
                return 0;
            }

            double *arr = reinterpret_cast<double *>(ptr_value);
            arr[index] = value;
            return 0;
        }

        // array_get_string(ptr, index) - string配列要素を取得
        if (node->name == "array_get_string" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error("array_get_string() requires 2 "
                                         "arguments: array_get_string(ptr, "
                                         "index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0) {
                std::cerr << "[array_get_string] Error: null pointer"
                          << std::endl;
                return 0; // 空文字列として0を返す
            }

            if (index < 0) {
                std::cerr << "[array_get_string] Error: negative index "
                          << index << std::endl;
                return 0; // 空文字列として0を返す
            }

            std::string **arr = reinterpret_cast<std::string **>(ptr_value);
            return reinterpret_cast<int64_t>(arr[index]);
        }

        // array_set_string(ptr, index, value) - string配列要素を設定
        if (node->name == "array_set_string" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error("array_set_string() requires 3 "
                                         "arguments: array_set_string(ptr, "
                                         "index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());
            int64_t str_ptr =
                interpreter_.eval_expression(node->arguments[2].get());

            if (ptr_value == 0) {
                std::cerr << "[array_set_string] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_set_string] Error: negative index "
                          << index << std::endl;
                return 0;
            }

            std::string **arr = reinterpret_cast<std::string **>(ptr_value);
            arr[index] = reinterpret_cast<std::string *>(str_ptr);
            return 0;
        }

        // array_get_struct(ptr, index) -
        // 構造体配列要素を取得（memcpyで値をコピー）
        if (node->name == "array_get_struct" && !is_method_call) {
            if (node->arguments.size() != 2) {
                throw std::runtime_error(
                    "array_get_struct() requires 2 arguments: "
                    "array_get_struct(ptr, index)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());

            if (ptr_value == 0) {
                std::cerr << "[array_get_struct] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_get_struct] Error: negative index "
                          << index << std::endl;
                return 0;
            }

            // 構造体配列は連続したメモリとして扱う
            // 戻り値は構造体のコピーのアドレス（インタプリタが管理）
            return ptr_value +
                   index; // ポインタ演算は呼び出し側で型サイズを考慮する
        }

        // array_set_struct(ptr, index, value) -
        // 構造体配列要素を設定（memcpyで値をコピー）
        if (node->name == "array_set_struct" && !is_method_call) {
            if (node->arguments.size() != 3) {
                throw std::runtime_error(
                    "array_set_struct() requires 3 arguments: "
                    "array_set_struct(ptr, index, value)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());
            int64_t index =
                interpreter_.eval_expression(node->arguments[1].get());
            // struct_ptrは呼び出し側で処理されるため、ここでは評価のみ
            (void)interpreter_.eval_expression(node->arguments[2].get());

            if (ptr_value == 0) {
                std::cerr << "[array_set_struct] Error: null pointer"
                          << std::endl;
                return 0;
            }

            if (index < 0) {
                std::cerr << "[array_set_struct] Error: negative index "
                          << index << std::endl;
                return 0;
            }

            // 構造体のコピーは呼び出し側が適切に処理する
            // ここでは単にポインタ演算の結果を返す
            return ptr_value + index;
        }

        // v0.14.0: 組み込み関数のチェック前にエラーを投げないようにする
        // malloc, free, sizeof などの組み込み関数は functions
        // マップに登録されていないため、
        // ここでエラーにせず、後続の組み込み関数チェックに進む
        static const std::vector<std::string> builtin_function_names = {
            "malloc",
            "free",
            "sizeof",
            "array_get",
            "array_set",
            "array_get_double",
            "array_set_double",
            "array_get_bool",
            "array_set_bool",
            "array_get_string",
            "array_set_string",
            "array_get_struct",
            "array_set_struct",
            "println",
            "print",
            "printf",
            "sprintf",
            "strlen",
            "strcpy",
            "strcmp",
            "strcat",
            "memcpy",
            "memset",
            "memcmp"};

        bool is_builtin = false;
        for (const auto &builtin_name : builtin_function_names) {
            if (node->name == builtin_name) {
                is_builtin = true;
                break;
            }
        }

        if (!is_builtin) {
            if (is_method_call) {
                std::string debug_type_name;
                if (is_method_call) {
                    if (!receiver_name.empty()) {
                        Variable *debug_receiver =
                            interpreter_.find_variable(receiver_name);
                        if (!debug_receiver &&
                            receiver_resolution.variable_ptr) {
                            debug_receiver = receiver_resolution.variable_ptr;
                        }
                        if (debug_receiver) {
                            if (!debug_receiver->struct_type_name.empty()) {
                                debug_type_name =
                                    debug_receiver->struct_type_name;
                            } else {
                                debug_type_name =
                                    std::string(::type_info_to_string(
                                        debug_receiver->type));
                            }
                        }
                    }
                }
            }
            throw std::runtime_error("Undefined function: " + node->name);
        }

        // 組み込み関数の場合は、後続の処理（malloc, free などのチェック）に進む
        // funcはnullptrのままだが、組み込み関数チェックで処理される
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
                                std::cerr
                                    << "Error: Cannot access private method '"
                                    << node->name
                                    << "' from outside its impl block"
                                    << std::endl;
                                std::exit(1);
                            }
                        }
                        break;
                    }
                }
            }
        }
    }

    // v0.14.0: 組み込み関数の処理（funcがnullptrの場合）
    // malloc, free, sizeof などの組み込み関数を早期に処理
    if (interpreter_.is_debug_mode()) {
        debug_print(
            "[BUILTIN_CHECK] func=%p, is_method_call=%d, node->name=%s\n",
            (void *)func, is_method_call, node->name.c_str());
    }

    if (!func && !is_method_call) {
        if (interpreter_.is_debug_mode()) {
            debug_print("[BUILTIN_EARLY] Processing builtin function: %s\n",
                        node->name.c_str());
        }

        // malloc(size) - メモリ確保
        if (node->name == "malloc") {
            if (node->arguments.size() != 1) {
                throw std::runtime_error(
                    "malloc() requires 1 argument: malloc(size)");
            }

            int64_t size =
                interpreter_.eval_expression(node->arguments[0].get());

            if (size <= 0) {
                std::cerr << "[malloc] Error: invalid size " << size
                          << std::endl;
                return 0;
            }

            void *ptr = std::malloc(static_cast<size_t>(size));
            if (ptr == nullptr) {
                std::cerr << "[malloc] Error: allocation failed for size "
                          << size << std::endl;
                return 0;
            }

            if (interpreter_.is_debug_mode()) {
                debug_print("[malloc] Allocated %lld bytes at %p\n", size, ptr);
            }

            return reinterpret_cast<int64_t>(ptr);
        }

        // free(ptr) - メモリ解放
        if (node->name == "free") {
            if (node->arguments.size() != 1) {
                throw std::runtime_error(
                    "free() requires 1 argument: free(ptr)");
            }

            int64_t ptr_value =
                interpreter_.eval_expression(node->arguments[0].get());

            if (ptr_value == 0) {
                // nullptr の解放は何もしない
                return 0;
            }

            std::free(reinterpret_cast<void *>(ptr_value));
            return 0;
        }

        // その他の組み込み関数は後続の処理で対応
        // funcがnullptrのままの場合、この時点では処理できない組み込み関数
        // エラーにするか、後続の処理に任せる
    }

    // v0.14.0: funcがnullptrの場合、通常の関数処理をスキップ
    // 組み込み関数は上記で処理済み、または後続の組み込み関数チェックで処理
    if (!func) {
        // funcがnullptrなのに、ここまで到達した場合
        // 未実装の組み込み関数か、エラー
        // 後続の組み込み関数チェック（sizeof等）に進むため、ここでは何もしない
        // ただし、通常の関数処理（スコープ作成等）はスキップ
        if (interpreter_.is_debug_mode()) {
            debug_print("[BUILTIN_FALLTHROUGH] Function %s not handled in "
                        "early builtin check, "
                        "proceeding to legacy builtin checks\n",
                        node->name.c_str());
        }
        // 後続の組み込み関数チェックセクションまでジャンプする必要があるが、
        // C++ではgotoを使わずに、処理を別関数に分離するのが良い
        // 今回は時間の制約上、既存のmallocチェックを早期リターンで処理した
        // その他の組み込み関数も同様に早期リターンで処理すべき
        throw std::runtime_error(
            "Builtin function not fully implemented in早期 check: " +
            node->name);
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

    // コンストラクタの場合、selfコンテキストを設定
    bool is_constructor =
        (func && (func->node_type == ASTNodeType::AST_CONSTRUCTOR_DECL ||
                  func->is_constructor));
    if (is_constructor && func) {
        // コンストラクタの場合、空のselfを作成
        std::string struct_name = func->constructor_struct_name;
        if (struct_name.empty() && func->type_name == func->name) {
            struct_name = func->name; // Rectangle()の場合、関数名が構造体名
        }

        if (!struct_name.empty()) {
            // 構造体定義を取得してselfを作成
            const StructDefinition *struct_def =
                interpreter_.find_struct_definition(struct_name);
            if (struct_def) {
                Variable self_var;
                self_var.type = TYPE_STRUCT;
                self_var.is_struct = true;
                self_var.struct_type_name = struct_name;

                // メンバーを初期化
                for (const auto &member : struct_def->members) {
                    Variable member_var;
                    member_var.type = member.type;
                    member_var.is_assigned = false;
                    self_var.struct_members[member.name] = member_var;

                    // self.memberとしてもアクセス可能にする
                    std::string self_member_path = "self." + member.name;
                    interpreter_.get_current_scope()
                        .variables[self_member_path] = member_var;
                }

                interpreter_.get_current_scope().variables["self"] = self_var;

                if (debug_mode) {
                    debug_print("CONSTRUCTOR_SELF_SETUP: Created self for "
                                "struct %s with %zu members\n",
                                struct_name.c_str(),
                                self_var.struct_members.size());
                }
            }
        }
    }

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
            // v0.13.1: struct_members_refを考慮
            auto &receiver_members = receiver_var->get_struct_members();
            for (const auto &member_pair : receiver_members) {
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
        // パラメータの評価と設定（デフォルト引数対応）
        size_t num_params = func->parameters.size();
        size_t num_args = node->arguments.size();
        size_t required_args = (func->first_default_param_index >= 0)
                                   ? func->first_default_param_index
                                   : num_params;

        // 引数数の検証（デフォルト引数を考慮）
        if (num_args < required_args || num_args > num_params) {
            if (debug_mode) {
                std::cerr << "[FUNC_CALL] Argument count mismatch: function '"
                          << node->name << "' expected " << required_args
                          << " to " << num_params << " args, got " << num_args
                          << std::endl;
                std::cerr << "[FUNC_CALL] Parameters:" << std::endl;
                for (const auto &param : func->parameters) {
                    std::cerr << "  - " << param->name
                              << " type_info=" << param->type_info
                              << " is_array=" << param->is_array
                              << " is_reference=" << param->is_reference
                              << " has_default=" << param->has_default_value
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
            throw std::runtime_error(
                "Argument count mismatch for function: " + node->name +
                " (expected " + std::to_string(required_args) + " to " +
                std::to_string(num_params) + ", got " +
                std::to_string(num_args) + ")");
        }

        // ジェネリックメソッドの場合、パラメータの型を解決
        std::map<std::string, std::string> type_context;
        if (is_method_call && !receiver_name.empty()) {
            Variable *receiver_var = interpreter_.find_variable(receiver_name);
            if (!receiver_var && receiver_resolution.variable_ptr) {
                receiver_var = receiver_resolution.variable_ptr;
            }

            if (receiver_var) {
                std::string type_name = receiver_var->struct_type_name;

                // impl_defを探す
                for (const auto &impl : interpreter_.get_impl_definitions()) {
                    if (impl.struct_name == type_name &&
                        !impl.type_parameter_map.empty()) {
                        type_context = impl.type_parameter_map;

                        if (debug_mode) {
                            std::cerr
                                << "[GENERIC_PARAM] Found type context for "
                                << type_name << ":" << std::endl;
                            for (const auto &pair : type_context) {
                                std::cerr << "  " << pair.first << " -> "
                                          << pair.second << std::endl;
                            }
                        }
                        break;
                    }
                }
            }
        }

        for (size_t i = 0; i < num_params; i++) {
            const auto &param_orig = func->parameters[i];

            // ジェネリック型パラメータを解決
            TypeInfo resolved_type_info = param_orig->type_info;
            if (!type_context.empty() && !param_orig->type_name.empty()) {
                auto it = type_context.find(param_orig->type_name);
                if (it != type_context.end()) {
                    const std::string &resolved_type = it->second;

                    // 型情報を更新
                    if (resolved_type == "string") {
                        resolved_type_info = TYPE_STRING;
                    } else if (resolved_type == "int") {
                        resolved_type_info = TYPE_INT;
                    } else if (resolved_type == "float") {
                        resolved_type_info = TYPE_FLOAT;
                    } else if (resolved_type == "double") {
                        resolved_type_info = TYPE_DOUBLE;
                    } else if (resolved_type == "bool") {
                        resolved_type_info = TYPE_BOOL;
                    } else if (resolved_type == "char") {
                        resolved_type_info = TYPE_CHAR;
                    }

                    if (debug_mode) {
                        std::cerr << "[GENERIC_PARAM] Resolved param '"
                                  << param_orig->name
                                  << "' type: " << param_orig->type_name
                                  << " -> " << resolved_type << " (type_info="
                                  << static_cast<int>(resolved_type_info) << ")"
                                  << std::endl;
                    }
                }
            }

            // resolved_type_infoを使ってパラメータを処理
            const auto &param = param_orig;

            // 引数が提供されている場合
            if (i < num_args) {
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
                        TypedValue func_ptr_val(
                            func_address,
                            InferredType(TYPE_POINTER,
                                         type_info_to_string(TYPE_POINTER)));
                        interpreter_.assign_function_parameter(
                            param->name, func_ptr_val, TYPE_POINTER,
                            param->type_name, false);

                        // 変数に関数ポインタフラグを設定
                        Variable *param_var =
                            interpreter_.find_variable(param->name);
                        if (param_var) {
                            param_var->is_function_pointer = true;
                            param_var->function_pointer_name = func_name;
                        }

                        if (debug_mode) {
                            std::cerr
                                << "[FUNC_CALL] Registered function pointer "
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
                    Variable *source_var =
                        interpreter_.find_variable(arg->name);
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
                    ref_var.implementing_struct =
                        source_var->implementing_struct;

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
                    interpreter_.current_scope().variables[param->name] =
                        ref_var;
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
                        array_ref.array_dimensions =
                            source_var->array_dimensions;
                        array_ref.array_type_info = source_var->array_type_info;

                        // ポインタ配列情報もコピー
                        array_ref.is_pointer = source_var->is_pointer;
                        array_ref.pointer_depth = source_var->pointer_depth;
                        array_ref.pointer_base_type =
                            source_var->pointer_base_type;
                        array_ref.pointer_base_type_name =
                            source_var->pointer_base_type_name;

                        // struct配列情報もコピー
                        array_ref.is_struct = source_var->is_struct;
                        array_ref.struct_type_name =
                            source_var->struct_type_name;

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
                    } else if (arg->node_type ==
                               ASTNodeType::AST_ARRAY_LITERAL) {
                        // 配列リテラルとして直接渡された場合
                        debug_msg(DebugMsgId::ARRAY_LITERAL_INIT_PROCESSING,
                                  ("Processing array literal argument for "
                                   "parameter: " +
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
                                int64_t val =
                                    evaluate_expression(element.get());
                                values.push_back(val);
                            }
                        }

                        // 一時変数に値を設定
                        if (!str_values.empty()) {
                            temp_var.array_strings = str_values;
                            temp_var.array_size = str_values.size();
                            temp_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + TYPE_STRING);
                        } else {
                            temp_var.array_values = values;
                            temp_var.array_size = values.size();
                            temp_var.type = static_cast<TypeInfo>(
                                TYPE_ARRAY_BASE + TYPE_INT);
                        }
                        temp_var.is_assigned = true;

                        // パラメータに設定
                        interpreter_.assign_array_parameter(
                            param->name, temp_var, param->type_info);

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
                    // 引数の型を事前にチェック（resolved_type_infoを使用）
                    if (arg->node_type == ASTNodeType::AST_STRING_LITERAL &&
                        resolved_type_info != TYPE_STRING) {
                        throw std::runtime_error(
                            "Type mismatch: cannot pass string literal to "
                            "non-string parameter '" +
                            param->name + "'");
                    }

                    // 文字列パラメータの場合（resolved_type_infoを使用）
                    if (resolved_type_info == TYPE_STRING) {
                        if (arg->node_type == ASTNodeType::AST_STRING_LITERAL) {
                            // 文字列リテラルを直接代入
                            Variable param_var;
                            param_var.type = TYPE_STRING;
                            param_var.str_value = arg->str_value;
                            // value
                            // フィールドにもポインタを保存（generic型で使用される）
                            param_var.value = reinterpret_cast<int64_t>(
                                strdup(param_var.str_value.c_str()));
                            param_var.is_assigned = true;
                            param_var.is_const =
                                param->is_const; // パラメータのconst修飾を保持
                            interpreter_.current_scope()
                                .variables[param->name] = param_var;
                        } else if (arg->node_type ==
                                   ASTNodeType::AST_VARIABLE) {
                            // 文字列変数を代入
                            Variable *source_var =
                                interpreter_.find_variable(arg->name);
                            if (!source_var ||
                                source_var->type != TYPE_STRING) {
                                throw std::runtime_error(
                                    "Type mismatch: expected string variable "
                                    "for "
                                    "parameter '" +
                                    param->name + "'");
                            }
                            Variable param_var;
                            param_var.type = TYPE_STRING;
                            param_var.str_value = source_var->str_value;
                            // value フィールドもコピー（generic型で使用される）
                            param_var.value = source_var->value;
                            param_var.is_assigned = true;
                            param_var.is_const =
                                param->is_const; // パラメータのconst修飾を保持
                            interpreter_.current_scope()
                                .variables[param->name] = param_var;
                        } else {
                            throw std::runtime_error(
                                "Type mismatch: cannot pass non-string "
                                "expression "
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
                                        "Cannot pass non-struct/non-primitive "
                                        "to "
                                        "interface parameter '" +
                                        param->name + "'");
                                }
                                assign_interface_argument(*source_var,
                                                          source_name);
                            } else if (arg->node_type ==
                                       ASTNodeType::AST_STRING_LITERAL) {
                                Variable temp;
                                temp.type = TYPE_STRING;
                                temp.str_value = arg->str_value;
                                // value
                                // フィールドにもポインタを保存（generic型で使用される）
                                temp.value = reinterpret_cast<int64_t>(
                                    strdup(temp.str_value.c_str()));
                                temp.is_assigned = true;
                                temp.struct_type_name = "string";
                                assign_interface_argument(temp, "");
                            } else {
                                auto build_temp_from_primitive =
                                    [&](TypeInfo value_type,
                                        int64_t numeric_value,
                                        const std::string &string_value) {
                                        Variable temp;
                                        temp.type = value_type;
                                        temp.is_assigned = true;
                                        if (!arg->type_name.empty()) {
                                            temp.struct_type_name =
                                                arg->type_name;
                                        } else {
                                            temp.struct_type_name = std::string(
                                                ::type_info_to_string(
                                                    value_type));
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
                                        Variable temp =
                                            build_temp_from_primitive(
                                                TYPE_STRING, 0, arg->str_value);
                                        assign_interface_argument(temp, "");
                                    } else {
                                        Variable temp =
                                            build_temp_from_primitive(
                                                resolved_type, numeric_value,
                                                "");
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
                                        Variable temp =
                                            build_temp_from_primitive(
                                                TYPE_STRING, 0, ret.str_value);
                                        assign_interface_argument(temp, "");
                                    } else if (!ret.is_struct) {
                                        Variable temp =
                                            build_temp_from_primitive(
                                                ret.type, ret.value,
                                                ret.str_value);
                                        assign_interface_argument(temp, "");
                                    } else {
                                        assign_interface_argument(
                                            ret.struct_value, "");
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
                                source_var =
                                    interpreter_.find_variable(arg->name);
                            } else if (arg->node_type ==
                                       ASTNodeType::AST_ARRAY_REF) {
                                // 構造体配列要素を引数として渡す場合
                                // (struct_array[0])
                                std::string array_name = arg->left->name;
                                int64_t index =
                                    evaluate_expression(arg->array_index.get());
                                source_var_name = array_name + "[" +
                                                  std::to_string(index) + "]";

                                // 配列要素の最新状態を同期
                                interpreter_
                                    .sync_struct_members_from_direct_access(
                                        source_var_name);
                                // 同期後に再度取得
                                source_var =
                                    interpreter_.find_variable(source_var_name);
                            }

                            if (source_var && source_var->is_struct) {

                                // typedef名を実際のstruct名に解決
                                std::string resolved_struct_type =
                                    interpreter_.resolve_typedef(
                                        param->type_name);
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
                                if (normalized_resolved.substr(0, 7) ==
                                        "struct " &&
                                    normalized_resolved.length() > 7) {
                                    normalized_resolved =
                                        normalized_resolved.substr(7);
                                }
                                if (normalized_source.substr(0, 7) ==
                                        "struct " &&
                                    normalized_source.length() > 7) {
                                    normalized_source =
                                        normalized_source.substr(7);
                                }

                                if (normalized_resolved != normalized_source) {
                                    throw std::runtime_error(
                                        "Type mismatch: cannot pass struct "
                                        "type '" +
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
                                        interpreter_.find_variable(
                                            source_var_name);
                                } else {
                                    debug_print(
                                        "WARNING: Empty source_var_name, "
                                        "skipping sync\n");
                                }

                                if (!sync_source_var) {
                                    throw std::runtime_error(
                                        "Source struct variable not found: " +
                                        source_var_name);
                                }

                                // v0.13.1:
                                // 文字列配列メンバの場合、追加で確実にarray_stringsを同期
                                auto &sync_source_members =
                                    sync_source_var->get_struct_members();
                                for (auto &source_member_pair :
                                     sync_source_members) {
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
                                             i < source_member_pair.second
                                                     .array_size;
                                             i++) {
                                            std::string element_name =
                                                source_member_name + "[" +
                                                std::to_string(i) + "]";
                                            Variable *element_var =
                                                interpreter_.find_variable(
                                                    element_name);
                                            if (element_var &&
                                                element_var->type ==
                                                    TYPE_STRING) {
                                                if (source_member_pair.second
                                                        .array_strings.size() <=
                                                    static_cast<size_t>(i)) {
                                                    source_member_pair.second
                                                        .array_strings.resize(
                                                            i + 1);
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
                                    param
                                        ->is_const; // パラメータのconst修飾を保持
                                param_var.is_struct =
                                    true; // 明示的にstructフラグを設定
                                param_var.type = TYPE_STRUCT; // 型情報も設定
                                // 解決されたstruct型名を設定
                                param_var.struct_type_name =
                                    resolved_struct_type;

                                // struct_membersの配列要素も確実にコピー
                                for (auto &member_pair :
                                     param_var.struct_members) {
                                    if (member_pair.second.is_array &&
                                        TypeHelpers::isString(
                                            member_pair.second.type)) {
                                        // 文字列配列の場合、array_stringsを確実にコピー
                                        const auto &source_member =
                                            sync_source_var->struct_members
                                                .find(member_pair.first);
                                        if (source_member !=
                                            sync_source_var->struct_members
                                                .end()) {
                                            debug_print(
                                                "DEBUG: Copying string array "
                                                "%s: "
                                                "size=%d\n",
                                                member_pair.first.c_str(),
                                                static_cast<int>(
                                                    source_member->second
                                                        .array_strings.size()));
                                            member_pair.second.array_strings =
                                                source_member->second
                                                    .array_strings;
                                            if (!source_member->second
                                                     .array_strings.empty()) {
                                                debug_print(
                                                    "DEBUG: First element: "
                                                    "'%s'\n",
                                                    source_member->second
                                                        .array_strings[0]
                                                        .c_str());
                                            }
                                        }
                                    }
                                }

                                interpreter_.current_scope()
                                    .variables[param->name] = param_var;

                                // v0.13.1:
                                // 個別メンバー変数も作成（値を正しく設定）
                                // 元の構造体定義から type_name 情報を取得
                                const StructDefinition *struct_def =
                                    interpreter_.find_struct_definition(
                                        resolved_struct_type);
                                auto &sync_source_members_2 =
                                    sync_source_var->get_struct_members();
                                for (const auto &member_pair :
                                     sync_source_members_2) {
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

                                    // 元の構造体定義から type_name
                                    // を取得して設定
                                    if (struct_def) {
                                        for (const auto &member :
                                             struct_def->members) {
                                            if (member.name ==
                                                member_pair.first) {
                                                member_var.type_name =
                                                    member.type_alias;
                                                member_var.is_pointer =
                                                    member.is_pointer;
                                                member_var.pointer_depth =
                                                    member.pointer_depth;
                                                member_var
                                                    .pointer_base_type_name =
                                                    member
                                                        .pointer_base_type_name;
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
                                        .variables[full_member_name] =
                                        member_var;

                                    // 配列メンバの場合、個別要素変数も作成
                                    if (member_var.is_array) {
                                        // ソース側の配列要素変数をコピー
                                        std::string source_member_name =
                                            source_var_name + "." +
                                            member_pair.first;
                                        for (int i = 0;
                                             i < member_var.array_size; i++) {
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
                                                    .variables
                                                        [param_element_name] =
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
                                                    element_var.type =
                                                        TYPE_STRING;
                                                    element_var.str_value =
                                                        sync_source_var
                                                            ->struct_members
                                                                [member_pair
                                                                     .first]
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
                                                                [member_pair
                                                                     .first]
                                                            .array_values[i];
                                                } else {
                                                    // デフォルト値を設定
                                                    element_var.type =
                                                        member_var.type;
                                                    if (member_var.type ==
                                                        TYPE_STRING) {
                                                        element_var.str_value =
                                                            "";
                                                    } else {
                                                        element_var.value = 0;
                                                    }
                                                }
                                                element_var.is_assigned = true;
                                                interpreter_.current_scope()
                                                    .variables
                                                        [param_element_name] =
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
                            if (arg->node_type ==
                                ASTNodeType::AST_STRING_LITERAL) {
                                throw std::runtime_error(
                                    "Type mismatch: cannot pass string literal "
                                    "to "
                                    "numeric parameter '" +
                                    param->name + "'");
                            }

                            // ポインタパラメータの場合、引数のconst情報を事前に取得
                            bool arg_is_pointer = false;
                            bool arg_is_pointee_const = false;
                            bool arg_is_pointer_const = false;
                            int arg_pointer_depth = 0;
                            TypeInfo arg_pointer_base_type = TYPE_UNKNOWN;
                            std::string arg_pointer_base_type_name;

                            if (param->is_pointer &&
                                (arg->node_type == ASTNodeType::AST_VARIABLE ||
                                 arg->node_type ==
                                     ASTNodeType::AST_IDENTIFIER)) {
                                // 引数変数のconst情報を取得（関数スコープ作成前に）
                                Variable *arg_var =
                                    interpreter_.find_variable(arg->name);
                                if (arg_var && arg_var->is_pointer) {
                                    arg_is_pointer = true;
                                    arg_is_pointee_const =
                                        arg_var->is_pointee_const;
                                    arg_is_pointer_const =
                                        arg_var->is_pointer_const;
                                    arg_pointer_depth = arg_var->pointer_depth;
                                    arg_pointer_base_type =
                                        arg_var->pointer_base_type;
                                    arg_pointer_base_type_name =
                                        arg_var->pointer_base_type_name;
                                }
                            }

                            TypedValue arg_value =
                                evaluate_typed_expression(arg.get());
                            interpreter_.assign_function_parameter(
                                param->name, arg_value, param->type_info,
                                param->type_name, param->is_unsigned);

                            // const修飾を設定
                            if (param->is_const) {
                                Variable *param_var =
                                    interpreter_.find_variable(param->name);
                                if (param_var) {
                                    param_var->is_const = true;
                                }
                            }

                            // ポインタパラメータの場合、const情報を保持・チェック
                            if (param->is_pointer && arg_is_pointer) {
                                // 型安全性チェック: const → non-const は禁止
                                // const T* → T* への変換をチェック
                                if (arg_is_pointee_const &&
                                    !param->is_pointee_const_qualifier) {
                                    throw std::runtime_error(
                                        "Type mismatch in function call to '" +
                                        node->name + "':\n" +
                                        "  Cannot pass pointer to const (" +
                                        (arg_pointer_base_type_name.empty()
                                             ? "const T*"
                                             : "const " +
                                                   arg_pointer_base_type_name +
                                                   "*") +
                                        ") to parameter of type pointer to "
                                        "non-const (" +
                                        (param->type_name.empty()
                                             ? "T*"
                                             : param->type_name) +
                                        ")\n" +
                                        "  Cannot discard const qualifier from "
                                        "pointed-to type");
                                }

                                // T* const → T* への変換をチェック
                                if (arg_is_pointer_const &&
                                    !param->is_pointer_const_qualifier) {
                                    throw std::runtime_error(
                                        "Type mismatch in function call to '" +
                                        node->name + "':\n" +
                                        "  Cannot pass const pointer (" +
                                        (arg_pointer_base_type_name.empty()
                                             ? "T* const"
                                             : arg_pointer_base_type_name +
                                                   "* const") +
                                        ") to parameter of type non-const "
                                        "pointer\n" +
                                        "  Cannot discard const qualifier from "
                                        "pointer itself");
                                }

                                Variable *param_var =
                                    interpreter_.find_variable(param->name);
                                if (param_var) {
                                    // const情報を伝播
                                    param_var->is_pointee_const =
                                        arg_is_pointee_const;
                                    param_var->is_pointer_const =
                                        arg_is_pointer_const;
                                    param_var->pointer_depth =
                                        arg_pointer_depth;
                                    param_var->pointer_base_type =
                                        arg_pointer_base_type;
                                    param_var->pointer_base_type_name =
                                        arg_pointer_base_type_name;
                                    param_var->is_pointer = true;
                                }
                            }
                        }
                    }
                }
            } else {
                // 引数が提供されていない場合、デフォルト値を使用
                if (!param->has_default_value) {
                    throw std::runtime_error(
                        "Missing required argument for parameter: " +
                        param->name);
                }

                // デフォルト値を評価
                TypedValue default_val =
                    evaluate_typed_expression(param->default_value.get());

                // パラメータに設定
                interpreter_.assign_function_parameter(
                    param->name, default_val, param->type_info,
                    param->type_name, param->is_unsigned);

                // const修飾を設定
                if (param->is_const) {
                    Variable *param_var =
                        interpreter_.find_variable(param->name);
                    if (param_var) {
                        param_var->is_const = true;
                    }
                }

                if (debug_mode) {
                    std::cerr
                        << "[FUNC_CALL] Used default value for parameter: "
                        << param->name << std::endl;
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

        // v0.11.0: ジェネリックメソッド呼び出しの型コンテキスト管理
        bool type_context_pushed = false;
        std::string receiver_type_name;

        if (is_method_call && !receiver_name.empty()) {
            Variable *receiver_var = nullptr;
            if (used_resolution_ptr && dereferenced_struct_ptr) {
                receiver_var = dereferenced_struct_ptr;
            } else {
                receiver_var = interpreter_.find_variable(receiver_name);
            }

            if (receiver_var && receiver_var->type == TYPE_STRUCT) {
                receiver_type_name = receiver_var->struct_type_name;

                // Queue<int>のようなジェネリック型のメソッド呼び出し
                if (receiver_type_name.find('<') != std::string::npos) {
                    const ImplDefinition *impl_def =
                        interpreter_.find_impl_for_struct(receiver_type_name,
                                                          "");

                    if (impl_def && impl_def->is_generic_instance) {
                        interpreter_.push_type_context(
                            impl_def->get_type_context());
                        type_context_pushed = true;

                        if (interpreter_.is_debug_mode()) {
                            debug_print("[TYPE_CONTEXT] Pushed for %s::%s\n",
                                        receiver_type_name.c_str(),
                                        node->name.c_str());
                        }
                    }
                }
            }
        }

        // 関数本体を実行
        try {
            if (interpreter_.is_debug_mode()) {
                debug_print(
                    "[METHOD_EXEC] func->name='%s', body=%p, statements=%zu\n",
                    func->name.c_str(), (void *)func->body.get(),
                    func->body ? func->body->statements.size() : 0);
            }
            if (func->body) {
                interpreter_.execute_statement(func->body.get());
            } else {
                if (interpreter_.is_debug_mode()) {
                    debug_print("[METHOD_EXEC] Warning: func->body is null!\n");
                }
            }

            // v0.11.0: 型コンテキストをクリア
            if (type_context_pushed) {
                interpreter_.pop_type_context();
                if (interpreter_.is_debug_mode()) {
                    debug_print("[TYPE_CONTEXT] Popped after %s::%s\n",
                                receiver_type_name.c_str(), node->name.c_str());
                }
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
                    // v0.13.0:
                    // まず、全てのself.member変数をself.struct_membersにマージ
                    auto &current_scope = interpreter_.get_current_scope();
                    if (current_scope.variables.find("self") !=
                        current_scope.variables.end()) {
                        Variable &self_var = current_scope.variables["self"];

                        // Step 1:
                        // self.で始まる全ての変数をselfのstruct_membersに書き戻す
                        for (const auto &var_pair : current_scope.variables) {
                            const std::string &var_name = var_pair.first;
                            if (var_name.find("self.") == 0 &&
                                var_name.find('.', 5) == std::string::npos) {
                                // self.memberの形式（ネストしていない）
                                std::string member_name = var_name.substr(5);
                                const Variable &member_var = var_pair.second;

                                // selfのstruct_membersに反映
                                if (self_var.struct_members.find(member_name) !=
                                    self_var.struct_members.end()) {
                                    self_var.struct_members[member_name] =
                                        member_var;
                                    if (debug_mode) {
                                        debug_print("SELF_MERGE: %s -> "
                                                    "self.struct_members[%s] "
                                                    "(value=%lld)\n",
                                                    var_name.c_str(),
                                                    member_name.c_str(),
                                                    member_var.value);
                                    }
                                }
                            }
                        }

                        // Step 2: selfの全フィールドをreceiver_varにコピー
                        receiver_var->struct_members = self_var.struct_members;
                        receiver_var->value = self_var.value;
                        receiver_var->str_value = self_var.str_value;
                        receiver_var->float_value = self_var.float_value;
                        receiver_var->double_value = self_var.double_value;
                        receiver_var->quad_value = self_var.quad_value;
                        receiver_var->big_value = self_var.big_value;
                        receiver_var->array_values = self_var.array_values;
                        receiver_var->array_float_values =
                            self_var.array_float_values;
                        receiver_var->array_double_values =
                            self_var.array_double_values;
                        receiver_var->array_quad_values =
                            self_var.array_quad_values;
                        receiver_var->array_strings = self_var.array_strings;
                        receiver_var->is_assigned = self_var.is_assigned;

                        if (debug_mode) {
                            debug_print("SELF_WRITEBACK_FULL: Copied all "
                                        "fields from self to %s\n",
                                        receiver_name.c_str());
                        }
                    }

                    // すべての self.* 変数を検索して書き戻し
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

            // v0.13.1: メソッド内でメソッドを呼んだ場合、parent
            // scopeのselfも更新する
            // これにより、vec.push()内でself.reserve()を呼んだ後、push()内のselfが更新される
            if (has_receiver && !receiver_name.empty()) {
                auto &scope_stack = interpreter_.get_scope_stack();
                // 現在のスコープ(呼ばれたメソッドのスコープ)の1つ上を確認
                if (scope_stack.size() >= 2) {
                    auto &parent_scope = scope_stack[scope_stack.size() - 2];
                    // parent
                    // scopeにselfが存在し、同じreceiverを参照している場合
                    if (parent_scope.variables.find("self") !=
                        parent_scope.variables.end()) {
                        Variable &parent_self = parent_scope.variables["self"];
                        // receiverと同じstruct_type_nameを持つ場合、更新する
                        Variable *receiver_var_for_parent = nullptr;
                        if (used_resolution_ptr && dereferenced_struct_ptr) {
                            receiver_var_for_parent = dereferenced_struct_ptr;
                        } else {
                            receiver_var_for_parent =
                                interpreter_.find_variable(receiver_name);
                        }

                        if (receiver_var_for_parent &&
                            parent_self.struct_type_name ==
                                receiver_var_for_parent->struct_type_name) {
                            // parent scopeのselfを更新
                            parent_self.struct_members =
                                receiver_var_for_parent->struct_members;
                            parent_self.value = receiver_var_for_parent->value;
                            parent_self.str_value =
                                receiver_var_for_parent->str_value;
                            parent_self.float_value =
                                receiver_var_for_parent->float_value;
                            parent_self.double_value =
                                receiver_var_for_parent->double_value;
                            parent_self.quad_value =
                                receiver_var_for_parent->quad_value;
                            parent_self.big_value =
                                receiver_var_for_parent->big_value;

                            // parent scopeのself.member変数も更新
                            for (const auto &member_pair :
                                 parent_self.struct_members) {
                                const std::string &member_name =
                                    member_pair.first;
                                const Variable &member_var = member_pair.second;
                                std::string var_name = "self." + member_name;

                                if (parent_scope.variables.find(var_name) !=
                                    parent_scope.variables.end()) {
                                    parent_scope.variables[var_name] =
                                        member_var;
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

            // メソッド通常終了時も、selfの変更をレシーバーに同期
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

            cleanup_method_context();
            interpreter_.pop_scope();
            method_scope_active = false;
            interpreter_.current_function_name = prev_function_name;
            return 0;
        } catch (const ReturnException &ret) {
            // v0.11.0: 型コンテキストをクリア（例外時）
            if (type_context_pushed) {
                interpreter_.pop_type_context();
                if (interpreter_.is_debug_mode()) {
                    debug_print(
                        "[TYPE_CONTEXT] Popped (exception) after %s::%s\n",
                        receiver_type_name.c_str(), node->name.c_str());
                }
            }

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
                    // v0.13.0:
                    // まず、全てのself.member変数をself.struct_membersにマージ
                    auto &current_scope = interpreter_.get_current_scope();
                    if (current_scope.variables.find("self") !=
                        current_scope.variables.end()) {
                        Variable &self_var = current_scope.variables["self"];

                        // Step 1:
                        // self.で始まる全ての変数をselfのstruct_membersに書き戻す
                        for (const auto &var_pair : current_scope.variables) {
                            const std::string &var_name = var_pair.first;
                            if (var_name.find("self.") == 0 &&
                                var_name.find('.', 5) == std::string::npos) {
                                // self.memberの形式（ネストしていない）
                                std::string member_name = var_name.substr(5);
                                const Variable &member_var = var_pair.second;

                                // selfのstruct_membersに反映
                                if (self_var.struct_members.find(member_name) !=
                                    self_var.struct_members.end()) {
                                    self_var.struct_members[member_name] =
                                        member_var;
                                    if (debug_mode) {
                                        debug_print("SELF_MERGE: %s -> "
                                                    "self.struct_members[%s] "
                                                    "(value=%lld)\n",
                                                    var_name.c_str(),
                                                    member_name.c_str(),
                                                    member_var.value);
                                    }
                                }
                            }
                        }

                        // Step 2: selfの全フィールドをreceiver_varにコピー
                        receiver_var->struct_members = self_var.struct_members;
                        receiver_var->value = self_var.value;
                        receiver_var->str_value = self_var.str_value;
                        receiver_var->float_value = self_var.float_value;
                        receiver_var->double_value = self_var.double_value;
                        receiver_var->quad_value = self_var.quad_value;
                        receiver_var->big_value = self_var.big_value;
                        receiver_var->array_values = self_var.array_values;
                        receiver_var->array_float_values =
                            self_var.array_float_values;
                        receiver_var->array_double_values =
                            self_var.array_double_values;
                        receiver_var->array_quad_values =
                            self_var.array_quad_values;
                        receiver_var->array_strings = self_var.array_strings;
                        receiver_var->is_assigned = self_var.is_assigned;

                        if (debug_mode) {
                            debug_print("SELF_WRITEBACK_FULL: Copied all "
                                        "fields from self to %s\n",
                                        receiver_name.c_str());
                        }
                    }

                    // すべての self.* 変数を検索して書き戻し
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
