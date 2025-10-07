#include "expression_function_call.h"
#include "../../../common/ast.h"
#include "../../../common/debug.h"
#include "core/error_handler.h"
#include "core/interpreter.h"
#include <iostream>
#include <stdexcept>

namespace FunctionCallHelpers {

// ========================================================================
// 関数ポインタ呼び出しの評価
// ========================================================================
int64_t evaluate_function_pointer_call(const ASTNode *node,
                                       Interpreter &interpreter) {
    bool debug_mode = interpreter.is_debug_mode();

    // 関数ポインタ呼び出し: (*funcPtr)(args) 形式
    if (!node->left) {
        throw std::runtime_error(
            "Function pointer call requires a pointer variable");
    }

    // ポインタ変数名を取得（leftはVARIABLEノード）
    std::string ptr_var_name;
    if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
        ptr_var_name = node->left->name;
    } else {
        throw std::runtime_error("Function pointer call requires a variable");
    }

    // 関数ポインタを検索
    auto &func_ptrs = interpreter.current_scope().function_pointers;
    auto it = func_ptrs.find(ptr_var_name);
    if (it == func_ptrs.end()) {
        // グローバルスコープも確認
        auto &global_func_ptrs =
            interpreter.get_global_scope().function_pointers;
        it = global_func_ptrs.find(ptr_var_name);
        if (it == global_func_ptrs.end()) {
            throw std::runtime_error("Not a function pointer: " + ptr_var_name);
        }
    }

    const FunctionPointer &func_ptr = it->second;
    const ASTNode *func_node = func_ptr.function_node;

    if (debug_mode) {
        std::cerr << "[FUNC_PTR] Calling function pointer: " << ptr_var_name
                  << " -> " << func_ptr.function_name << std::endl;
    }

    // 引数を評価
    std::vector<int64_t> arg_values;
    std::vector<std::string> arg_strings;
    std::vector<TypeInfo> arg_types;

    if (debug_mode) {
        std::cerr << "[FUNC_PTR] node->arguments size: "
                  << node->arguments.size() << std::endl;
    }

    for (const auto &arg : node->arguments) {
        TypedValue typed_val = interpreter.evaluate_typed(arg.get());
        arg_values.push_back(typed_val.value);
        if (typed_val.type.type_info == TYPE_STRING) {
            arg_strings.push_back(typed_val.string_value);
        }
    }

    if (debug_mode) {
        std::cerr << "[FUNC_PTR] Evaluated " << arg_values.size()
                  << " arguments" << std::endl;
        for (size_t i = 0; i < arg_values.size(); ++i) {
            std::cerr << "[FUNC_PTR] arg[" << i << "] = " << arg_values[i]
                      << std::endl;
        }
    }

    // 関数を呼び出し（既存の関数呼び出し機構を再利用）
    // パラメータを設定
    interpreter.push_interpreter_scope();

    // 仮引数と実引数をバインド
    size_t param_idx = 0;
    if (debug_mode) {
        std::cerr << "[FUNC_PTR] Binding parameters: function has "
                  << func_node->parameters.size() << " parameters" << std::endl;
    }
    for (const auto &param : func_node->parameters) {
        if (param_idx >= arg_values.size()) {
            throw std::runtime_error(
                "Too few arguments for function pointer call");
        }

        std::string param_name = param->name;
        TypeInfo param_type = param->type_info;
        bool is_unsigned = param->is_unsigned;

        if (debug_mode) {
            std::cerr << "[FUNC_PTR] Binding param[" << param_idx
                      << "]: " << param_name << " = " << arg_values[param_idx]
                      << std::endl;
        }

        if (param_type == TYPE_STRING) {
            interpreter.assign_variable(param_name, arg_strings[param_idx]);
        } else {
            interpreter.assign_function_parameter(
                param_name, arg_values[param_idx], param_type, is_unsigned);
        }

        param_idx++;
    }

    // 関数本体を実行
    int64_t result = 0;
    if (debug_mode) {
        std::cerr << "[FUNC_PTR] func_node->body exists: "
                  << (func_node->body ? "yes" : "no") << std::endl;
        if (func_node->body) {
            std::cerr << "[FUNC_PTR] func_node->body type: "
                      << static_cast<int>(func_node->body->node_type)
                      << std::endl;
        }
    }
    try {
        if (func_node->body) {
            if (debug_mode) {
                std::cerr << "[FUNC_PTR] Executing function body: node_type="
                          << static_cast<int>(func_node->body->node_type)
                          << std::endl;
            }
            interpreter.exec_statement(func_node->body.get());
            if (debug_mode) {
                std::cerr << "[FUNC_PTR] Function body execution finished "
                             "without exception"
                          << std::endl;
            }
        } else {
            if (debug_mode) {
                std::cerr
                    << "[FUNC_PTR] No function body (func_node->body is null)"
                    << std::endl;
            }
        }
    } catch (const ReturnException &ret) {
        if (debug_mode) {
            std::cerr << "[FUNC_PTR] Caught ReturnException: value="
                      << ret.value << std::endl;
        }
        // 戻り値を取得（スコープをpopする前に）
        if (ret.type == TYPE_STRING) {
            interpreter.pop_interpreter_scope();
            throw ret; // 文字列の場合はexceptionとして伝播
        } else {
            result = ret.value;
            if (debug_mode) {
                std::cerr << "[FUNC_PTR] Function returned: " << result
                          << std::endl;
            }
            interpreter.pop_interpreter_scope();
            if (debug_mode) {
                std::cerr << "[FUNC_PTR] Scope popped, returning result="
                          << result << std::endl;
            }
            return result;
        }
    } catch (const std::exception &e) {
        if (debug_mode) {
            std::cerr << "[FUNC_PTR] Caught unexpected exception: " << e.what()
                      << std::endl;
        }
        interpreter.pop_interpreter_scope();
        throw;
    }

    interpreter.pop_interpreter_scope();
    if (debug_mode) {
        std::cerr << "[FUNC_PTR] Function completed without return, result="
                  << result << std::endl;
    }
    return result;
}

} // namespace FunctionCallHelpers
