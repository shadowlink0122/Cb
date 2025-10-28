#include "special.h"
#include "../../../../common/debug.h"
#include "../../core/pointer_metadata.h"
#include "../../managers/types/enums.h"
#include "../../managers/types/manager.h"
#include "../core/evaluator.h"
#include <functional>
#include <iostream>
#include <stdexcept>

namespace SpecialAccessHelpers {

int64_t evaluate_arrow_access(
    const ASTNode *node, Interpreter &interpreter,
    ExpressionEvaluator &evaluator,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<Variable(const Variable &, const std::string &)>
        get_struct_member_func) {
    // アロー演算子アクセス: ptr->member は (*ptr).member と等価
    // まず左側のポインタを評価
    debug_msg(DebugMsgId::EXPR_EVAL_START, "Arrow operator member access");

    std::string member_name = node->name;

    // v0.11.0 Week 2 Day 3: ptr[index]->member パターン対応
    // 左側がポインタ配列アクセス (ptr[0])
    // の場合、ReturnExceptionで構造体が返される
    Variable *struct_var = nullptr;
    int64_t ptr_value = 0;

    try {
        // ポインタを評価して値を取得
        ptr_value = evaluate_expression_func(node->left.get());
    } catch (const ReturnException &ret) {
        // 構造体が返された場合（ptr[index]からの構造体）
        if (ret.is_struct) {
            if (interpreter.is_debug_mode()) {
                std::cerr << "[ARROW_DEBUG] Caught struct from ptr[index]"
                          << " struct_type="
                          << ret.struct_value.struct_type_name << std::endl;
            }

            // 構造体からメンバーを取得
            Variable member_var =
                get_struct_member_func(ret.struct_value, member_name);

            if (member_var.type == TYPE_STRING) {
                TypedValue typed_result(static_cast<int64_t>(0),
                                        InferredType(TYPE_STRING, "string"));
                typed_result.string_value = member_var.str_value;
                typed_result.is_numeric_result = false;
                evaluator.set_last_typed_result(typed_result);
                return 0;
            } else if (member_var.type == TYPE_POINTER) {
                return member_var.value;
            } else if (member_var.type == TYPE_FLOAT ||
                       member_var.type == TYPE_DOUBLE ||
                       member_var.type == TYPE_QUAD) {
                return static_cast<int64_t>(member_var.float_value);
            } else {
                return member_var.value;
            }
        } else {
            // その他のReturnExceptionは再投げ
            throw;
        }
    }

    if (interpreter.is_debug_mode()) {
        std::cerr << "[ARROW_DEBUG] ptr_value=" << ptr_value
                  << " has_meta=" << ((ptr_value & (1LL << 63)) ? "yes" : "no")
                  << std::endl;
    }

    if (ptr_value == 0) {
        throw std::runtime_error("Null pointer dereference in arrow operator");
    }

    // メタデータポインタかどうかをチェック（最上位ビットが1）
    bool has_metadata = (ptr_value & (1LL << 63)) != 0;

    if (has_metadata) {
        // メタデータポインタの場合、最上位ビットをクリアして実際のポインタを取得
        int64_t meta_ptr = ptr_value & ~(1LL << 63);
        PointerSystem::PointerMetadata *metadata =
            reinterpret_cast<PointerSystem::PointerMetadata *>(meta_ptr);

        if (!metadata) {
            throw std::runtime_error(
                "Invalid metadata pointer in arrow operator");
        }

        // メタデータの種類に応じて処理
        if (metadata->target_type ==
                PointerSystem::PointerTargetType::VARIABLE &&
            metadata->var_ptr) {
            struct_var = metadata->var_ptr;
        } else if (metadata->target_type ==
                   PointerSystem::PointerTargetType::ARRAY_ELEMENT) {
            // 配列要素の場合、array_valuesから実際の要素ポインタを取得
            // 構造体配列の場合、array_name[index] という変数名を構築
            if (!metadata->array_var) {
                throw std::runtime_error(
                    "Invalid array metadata in arrow operator");
            }

            // 配列要素の変数名を構築: "rectangles[0]" など
            std::string element_name = metadata->array_name + "[" +
                                       std::to_string(metadata->element_index) +
                                       "]";
            struct_var = interpreter.find_variable(element_name);

            if (!struct_var) {
                throw std::runtime_error("Struct array element not found: " +
                                         element_name);
            }
        } else {
            throw std::runtime_error(
                "Unsupported metadata type in arrow operator");
        }
    } else {
        // 通常のポインタの場合
        struct_var = reinterpret_cast<Variable *>(ptr_value);
    }

    if (!struct_var) {
        throw std::runtime_error("Invalid pointer in arrow operator");
    }

    if (interpreter.is_debug_mode()) {
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it != struct_var->struct_members.end()) {
            std::cerr << "[ARROW_DEBUG] struct_var=" << struct_var
                      << " member=" << member_name
                      << " value=" << member_it->second.value << " is_assigned="
                      << (member_it->second.is_assigned ? "true" : "false")
                      << std::endl;
        } else {
            std::cerr << "[ARROW_DEBUG] struct_var=" << struct_var
                      << " member=" << member_name << " not found" << std::endl;
        }
    }

    // 構造体型またはInterface型をチェック
    if (struct_var->type != TYPE_STRUCT && struct_var->type != TYPE_INTERFACE) {
        throw std::runtime_error(
            "Arrow operator requires struct or interface pointer");
    }

    // メンバーを取得
    Variable member_var = get_struct_member_func(*struct_var, member_name);

    if (member_var.type == TYPE_STRING) {
        if (interpreter.is_debug_mode()) {
            std::cerr << "[ARROW_DEBUG] STRING member found: str_value='"
                      << member_var.str_value << "'" << std::endl;
        }
        TypedValue typed_result(static_cast<int64_t>(0),
                                InferredType(TYPE_STRING, "string"));
        typed_result.string_value = member_var.str_value;
        typed_result.is_numeric_result = false;
        // last_typed_result_に設定
        evaluator.set_last_typed_result(typed_result);
        if (interpreter.is_debug_mode()) {
            std::cerr
                << "[ARROW_DEBUG] set_last_typed_result called with string: '"
                << typed_result.string_value << "'" << std::endl;
        }
        return 0;
    } else if (member_var.type == TYPE_POINTER) {
        // ポインタメンバの場合はそのまま値を返す
        return member_var.value;
    } else if (member_var.type == TYPE_STRUCT ||
               member_var.type == TYPE_INTERFACE) {
        // 構造体メンバの場合は、その構造体へのポインタを返す
        // struct_membersから実際の変数へのポインタを取得
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it != struct_var->struct_members.end()) {
            return reinterpret_cast<int64_t>(&member_it->second);
        }
        // fallback: member_varのアドレスを返す(コピーなので注意)
        throw std::runtime_error(
            "Cannot get address of temporary struct member");
    } else if (member_var.type == TYPE_FLOAT ||
               member_var.type == TYPE_DOUBLE || member_var.type == TYPE_QUAD) {
        // 浮動小数点数の場合
        // Note: last_typed_result_の設定は呼び出し側で処理
        return static_cast<int64_t>(member_var.float_value);
    } else {
        return member_var.value;
    }
}

int64_t evaluate_member_array_access(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func,
    std::function<Variable(const Variable &, const std::string &)>
        get_struct_member_func) {
    // メンバの配列アクセス: obj.member[index] または func().member[index]
    std::string obj_name;
    Variable base_struct;
    bool is_function_call = false;

    if (node->left->node_type == ASTNodeType::AST_VARIABLE ||
        node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        obj_name = node->left->name;
    } else if (node->left->node_type == ASTNodeType::AST_FUNC_CALL) {
        // 関数呼び出し結果でのメンバー配列アクセス: func().member[index]
        is_function_call = true;
        debug_msg(DebugMsgId::EXPR_EVAL_START,
                  "Function call member array access");

        try {
            evaluate_expression_func(node->left.get());
            throw std::runtime_error(
                "Function did not return a struct for member array access");
        } catch (const ReturnException &ret_ex) {
            if (ret_ex.is_struct_array && ret_ex.struct_array_3d.size() > 0) {
                throw std::runtime_error("Struct array function return member "
                                         "array access not yet supported");
            } else {
                base_struct = ret_ex.struct_value;
                obj_name = "func_result"; // 仮の名前
            }
        }
    } else {
        throw std::runtime_error(
            "Invalid object reference in member array access");
    }

    std::string member_name = node->name;

    // インデックスを評価（多次元対応）
    std::vector<int64_t> indices;
    if (node->right) {
        // 1次元の場合（従来通り）
        int64_t index = evaluate_expression_func(node->right.get());
        indices.push_back(index);
    } else if (!node->arguments.empty()) {
        // 多次元の場合
        for (const auto &arg : node->arguments) {
            int64_t index = evaluate_expression_func(arg.get());
            indices.push_back(index);
        }
    } else {
        throw std::runtime_error("No indices found for array access");
    }

    // 構造体メンバー変数を取得
    Variable member_var_copy; // 関数呼び出しの場合用のコピー
    Variable *member_var;

    if (is_function_call) {
        // 関数戻り値からメンバーを取得
        member_var_copy = get_struct_member_func(base_struct, member_name);
        member_var = &member_var_copy;
    } else {
        member_var = interpreter.get_struct_member(obj_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }
    }

    // 多次元配列の場合
    if (member_var->is_multidimensional && indices.size() > 1) {
        if (is_function_call) {
            // 関数戻り値の場合は直接配列要素を取得
            if (!member_var->is_array || member_var->array_values.empty()) {
                throw std::runtime_error(
                    "Member is not a valid array for multi-dimensional access");
            }
            // 多次元インデックス計算（簡易版）
            int64_t flat_index = indices[0];
            if (indices.size() > 1 && member_var->is_multidimensional) {
                // 簡易的な多次元計算（正確には別の実装が必要）
                flat_index = indices[0] * 10 + indices[1]; // 仮の計算
            }
            if (flat_index >= 0 &&
                flat_index < (int64_t)member_var->array_values.size()) {
                return member_var->array_values[flat_index];
            } else {
                throw std::runtime_error("Array index out of bounds in "
                                         "function member array access");
            }
        } else {
            return interpreter.getMultidimensionalArrayElement(*member_var,
                                                               indices);
        }
    }

    // 1次元配列の場合
    int64_t index = indices[0];
    if (is_function_call) {
        // 関数戻り値の場合
        if (!member_var->is_array || member_var->array_values.empty()) {
            throw std::runtime_error("Member is not a valid array");
        }
        if (index >= 0 && index < (int64_t)member_var->array_values.size()) {
            return member_var->array_values[index];
        } else {
            throw std::runtime_error(
                "Array index out of bounds in function member array access");
        }
    } else {
        return interpreter.get_struct_member_array_element(
            obj_name, member_name, static_cast<int>(index));
    }
}

int64_t evaluate_enum_access(const ASTNode *node, Interpreter &interpreter) {
    // enum値アクセス (EnumName::member)
    EnumManager *enum_manager = interpreter.get_enum_manager();
    int64_t enum_value;

    std::string enum_name = node->enum_name;

    // ジェネリック型の場合（Option<int>）、インスタンス化された名前に変換
    // Option<int> -> Option_int
    if (enum_name.find('<') != std::string::npos) {
        // 型名の正規化（< > , を _ に置換）
        std::string instantiated_name;
        bool in_type_args = false;

        for (char c : enum_name) {
            if (c == '<') {
                in_type_args = true;
                instantiated_name += '_';
            } else if (c == '>') {
                in_type_args = false;
                // 末尾の _ は追加しない
            } else if (c == ',' || c == ' ') {
                if (in_type_args) {
                    instantiated_name += '_';
                }
            } else if (c == '*') {
                instantiated_name += "_ptr";
            } else if (c == '[') {
                instantiated_name += "_array";
            } else if (c == ']') {
                // 配列サイズは既に処理済み
            } else {
                instantiated_name += c;
            }
        }

        enum_name = instantiated_name;
    }

    // typedef名を実際のenum名に解決
    std::string resolved_enum_name =
        interpreter.get_type_manager()->resolve_typedef(enum_name);

    if (enum_manager->get_enum_value(resolved_enum_name, node->enum_member,
                                     enum_value)) {
        debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, enum_value);
        return enum_value;
    } else {
        std::string error_message = "Undefined enum value: " + node->enum_name +
                                    "::" + node->enum_member;
        throw std::runtime_error(error_message);
    }
}

// v0.11.0: enum値の構築 (EnumName::member(value))
int64_t evaluate_enum_construct(const ASTNode *node, Interpreter &interpreter) {
    // enum値の構築: Option<int>::Some(42)

    std::string enum_name = node->enum_name;
    std::string original_enum_name = enum_name; // デバッグ用に元の名前を保存
    std::vector<std::string> type_arguments;
    std::string base_enum_name;

    // ジェネリック型の場合（Option<int>）、インスタンス化が必要
    if (enum_name.find('<') != std::string::npos) {
        // 基底名と型引数を抽出
        size_t lt_pos = enum_name.find('<');
        base_enum_name = enum_name.substr(0, lt_pos);

        // 型引数を抽出（簡易版: カンマで分割）
        size_t start = lt_pos + 1;
        size_t end = enum_name.find_last_of('>');
        if (end != std::string::npos && end > start) {
            std::string args_str = enum_name.substr(start, end - start);

            // カンマで分割（ネストした<>は考慮しない簡易版）
            std::string current_arg;
            for (char c : args_str) {
                if (c == ',') {
                    if (!current_arg.empty()) {
                        // 空白を削除
                        current_arg.erase(0,
                                          current_arg.find_first_not_of(" \t"));
                        current_arg.erase(current_arg.find_last_not_of(" \t") +
                                          1);
                        type_arguments.push_back(current_arg);
                        current_arg.clear();
                    }
                } else {
                    current_arg += c;
                }
            }
            if (!current_arg.empty()) {
                current_arg.erase(0, current_arg.find_first_not_of(" \t"));
                current_arg.erase(current_arg.find_last_not_of(" \t") + 1);
                type_arguments.push_back(current_arg);
            }
        }

        // インスタンス化された型名を生成
        std::string instantiated_name = base_enum_name;
        for (const auto &arg : type_arguments) {
            // 型名の正規化を適用
            std::string normalized_arg = arg;
            std::string temp;
            for (char c : normalized_arg) {
                if (c == '*') {
                    temp += "_ptr";
                } else if (c == '[') {
                    temp += "_array";
                } else if (c == ']') {
                    // skip
                } else if (c == ' ') {
                    // skip
                } else {
                    temp += c;
                }
            }
            instantiated_name += "_" + temp;
        }

        enum_name = instantiated_name;

        // ジェネリックenumのインスタンス化を試みる
        // TODO:
        // Parser経由でインスタンス化すべきだが、現時点ではinterpreterから直接アクセスできない
        // この時点でインスタンス化されていない場合はエラーになる
    }

    // typedef名を実際のenum名に解決
    std::string resolved_enum_name =
        interpreter.get_type_manager()->resolve_typedef(enum_name);

    // enum定義を取得
    EnumManager *enum_manager = interpreter.get_enum_manager();
    const EnumDefinition *enum_def =
        enum_manager->get_enum_definition(resolved_enum_name);

    if (!enum_def) {
        std::string error_message = "Undefined enum: " + original_enum_name +
                                    " (resolved to: " + resolved_enum_name +
                                    ")";
        if (!type_arguments.empty()) {
            error_message += "\nHint: Generic enum '" + base_enum_name +
                             "' needs to be instantiated before use.";
            error_message += "\nTry using it in a type context first (e.g., "
                             "variable declaration).";
        }
        throw std::runtime_error(error_message);
    }

    // メンバーを検索
    const EnumMember *member = enum_def->find_member(node->enum_member);
    if (!member) {
        std::string error_message =
            "Undefined enum member: " + node->enum_name +
            "::" + node->enum_member;
        throw std::runtime_error(error_message);
    }

    // 関連値の有無をチェック
    if (!member->has_associated_value) {
        std::string error_message = "Enum member " + node->enum_name +
                                    "::" + node->enum_member +
                                    " does not have an associated value";
        throw std::runtime_error(error_message);
    }

    // 引数を評価（現時点では単純な値のみサポート）
    if (node->arguments.empty()) {
        std::string error_message = "Enum constructor " + node->enum_name +
                                    "::" + node->enum_member +
                                    " requires an argument";
        throw std::runtime_error(error_message);
    }

    // 関連値を評価して返す
    // Option<int>::Some(42) の場合、42 を返す
    int64_t arg_value = interpreter.eval_expression(node->arguments[0].get());

    // デバッグ出力
    debug_msg(DebugMsgId::EXPR_EVAL_NUMBER, arg_value);

    // v0.11.0: 簡易実装として関連値を直接返す
    // enum型変数への代入時は、variable_declaration.cppで特別処理される
    // TODO: 将来的にはenum値オブジェクトを返し、型に応じて自動変換する
    return arg_value;
}

} // namespace SpecialAccessHelpers
