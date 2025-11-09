#include "binary_unary.h"
#include "../../../../common/debug.h"
#include "../../../../common/type_helpers.h"
#include "../../core/pointer_metadata.h"
#include "../../event_loop/event_loop.h"        // v0.12.0 Phase 8: EventLoop
#include "../../event_loop/simple_event_loop.h" // v0.13.0 Phase 2.0: SimpleEventLoop
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>

// デバッグモードフラグ
extern bool debug_mode;

namespace BinaryUnaryTypedHelpers {

// ヘルパー関数：型情報から文字列への変換（簡易版）
static const char *type_info_to_string_simple(TypeInfo type) {
    switch (type) {
    case TYPE_BOOL:
        return "bool";
    case TYPE_CHAR:
        return "char";
    case TYPE_TINY:
        return "tiny";
    case TYPE_SHORT:
        return "short";
    case TYPE_INT:
        return "int";
    case TYPE_LONG:
        return "long";
    case TYPE_BIG:
        return "big";
    case TYPE_FLOAT:
        return "float";
    case TYPE_DOUBLE:
        return "double";
    case TYPE_QUAD:
        return "quad";
    case TYPE_STRING:
        return "string";
    case TYPE_VOID:
        return "void";
    case TYPE_POINTER:
        return "pointer";
    default:
        return "unknown";
    }
}

// ヘルパー関数：型の確保
static InferredType ensure_type(const InferredType &inferred, TypeInfo type,
                                const std::string &name) {
    if (inferred.type_info != TYPE_UNKNOWN) {
        return inferred;
    }
    return InferredType(type, name);
}

TypedValue evaluate_binary_op_typed(
    const ASTNode *node, Interpreter &interpreter,
    const InferredType &inferred_type,
    std::function<TypedValue(const ASTNode *)> evaluate_typed_func) {
    TypedValue left_value = evaluate_typed_func(node->left.get());
    TypedValue right_value = evaluate_typed_func(node->right.get());

    // ポインタ演算のチェック（加算のみ）
    if (node->op == "+") {
        bool left_is_pointer = false;
        bool right_is_pointer = false;

        // 左オペランドがポインタかチェック
        if (node->left->node_type == ASTNodeType::AST_VARIABLE ||
            node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            Variable *left_var = interpreter.find_variable(node->left->name);
            if (left_var && left_var->is_pointer) {
                left_is_pointer = true;
            }
        }

        // 右オペランドがポインタかチェック
        if (node->right->node_type == ASTNodeType::AST_VARIABLE ||
            node->right->node_type == ASTNodeType::AST_IDENTIFIER) {
            Variable *right_var = interpreter.find_variable(node->right->name);
            if (right_var && right_var->is_pointer) {
                right_is_pointer = true;
            }
        }

        // ポインタ同士の加算を禁止
        if (left_is_pointer && right_is_pointer) {
            throw std::runtime_error(
                "Cannot add two pointers together. Pointer arithmetic only "
                "supports: pointer + integer, integer + pointer");
        }
    }

    auto is_integral_type_info = [](TypeInfo type) {
        switch (type) {
        case TYPE_BOOL:
        case TYPE_CHAR:
        case TYPE_TINY:
        case TYPE_SHORT:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_BIG:
            return true;
        default:
            return false;
        }
    };

    auto normalize_type = [](TypeInfo type) {
        if (type >= TYPE_ARRAY_BASE) {
            return static_cast<TypeInfo>(type - TYPE_ARRAY_BASE);
        }
        return type;
    };

    auto integral_rank = [](TypeInfo type) {
        switch (type) {
        case TYPE_BOOL:
            return 0;
        case TYPE_CHAR:
        case TYPE_TINY:
            return 1;
        case TYPE_SHORT:
            return 2;
        case TYPE_INT:
            return 3;
        case TYPE_LONG:
            return 4;
        case TYPE_BIG:
            return 5;
        default:
            return -1;
        }
    };

    auto determine_integral_result_type = [&]() -> TypeInfo {
        int best_rank = -1;
        TypeInfo best_type = TYPE_UNKNOWN;
        auto consider = [&](TypeInfo candidate) {
            candidate = normalize_type(candidate);
            int rank = integral_rank(candidate);
            if (rank > best_rank) {
                best_rank = rank;
                best_type = candidate;
            }
        };

        consider(inferred_type.type_info);
        consider(left_value.type.type_info);
        consider(left_value.numeric_type);
        consider(right_value.type.type_info);
        consider(right_value.numeric_type);

        if (!is_integral_type_info(best_type)) {
            best_type = TYPE_INT;
        }
        return best_type;
    };

    auto make_numeric_typed_value = [&](long double quad_value,
                                        bool prefer_integral) -> TypedValue {
        if (prefer_integral) {
            TypeInfo integer_type = determine_integral_result_type();
            return TypedValue(
                static_cast<int64_t>(quad_value),
                ensure_type(
                    inferred_type, integer_type,
                    std::string(type_info_to_string_simple(integer_type))));
        }

        // prefer_integral が false の場合、オペランドの型も考慮
        TypeInfo result_type = inferred_type.type_info;

        // オペランドから浮動小数点型を検出
        if (result_type == TYPE_UNKNOWN || is_integral_type_info(result_type)) {
            // left_value または right_value が浮動小数点の場合、その型を使用
            if (left_value.numeric_type == TYPE_QUAD ||
                right_value.numeric_type == TYPE_QUAD) {
                result_type = TYPE_QUAD;
            } else if (left_value.numeric_type == TYPE_DOUBLE ||
                       right_value.numeric_type == TYPE_DOUBLE) {
                result_type = TYPE_DOUBLE;
            } else if (left_value.numeric_type == TYPE_FLOAT ||
                       right_value.numeric_type == TYPE_FLOAT) {
                result_type = TYPE_FLOAT;
            } else if (left_value.type.type_info == TYPE_QUAD ||
                       right_value.type.type_info == TYPE_QUAD) {
                result_type = TYPE_QUAD;
            } else if (TypeHelpers::isFloating(left_value) ||
                       TypeHelpers::isFloating(right_value)) {
                // DOUBLEまたはFLOATのいずれかが含まれる
                if (left_value.type.type_info == TYPE_DOUBLE ||
                    right_value.type.type_info == TYPE_DOUBLE) {
                    result_type = TYPE_DOUBLE;
                } else {
                    result_type = TYPE_FLOAT;
                }
            }
        }

        if (result_type == TYPE_QUAD) {
            return TypedValue(quad_value,
                              ensure_type(inferred_type, TYPE_QUAD, "quad"));
        }
        if (result_type == TYPE_DOUBLE) {
            return TypedValue(
                static_cast<double>(quad_value),
                ensure_type(inferred_type, TYPE_DOUBLE, "double"));
        }
        if (result_type == TYPE_FLOAT) {
            return TypedValue(static_cast<double>(quad_value),
                              ensure_type(inferred_type, TYPE_FLOAT, "float"));
        }

        // それでも浮動小数点型が検出されない場合、整数型として処理
        TypeInfo effective = result_type != TYPE_UNKNOWN
                                 ? result_type
                                 : determine_integral_result_type();
        if (!is_integral_type_info(effective)) {
            effective = determine_integral_result_type();
        }
        return TypedValue(
            static_cast<int64_t>(quad_value),
            ensure_type(inferred_type, effective,
                        std::string(type_info_to_string_simple(effective))));
    };

    auto make_integer_typed_value = [&](int64_t int_value) -> TypedValue {
        TypeInfo integer_type = determine_integral_result_type();
        return TypedValue(
            int_value,
            ensure_type(inferred_type, integer_type,
                        std::string(type_info_to_string_simple(integer_type))));
    };

    auto operands_are_integral =
        left_value.is_numeric() && !left_value.is_floating() &&
        right_value.is_numeric() && !right_value.is_floating();
    bool prefer_integral_result = operands_are_integral;

    auto make_bool_typed_value = [&](bool value) -> TypedValue {
        return TypedValue(static_cast<int64_t>(value ? 1 : 0),
                          ensure_type(inferred_type, TYPE_BOOL, "bool"));
    };

    auto left_quad = left_value.as_quad();
    auto right_quad = right_value.as_quad();
    auto left_int = left_value.as_numeric();
    auto right_int = right_value.as_numeric();
    auto truthy = [&](const TypedValue &v) {
        if (v.is_floating()) {
            return v.as_double() != 0.0;
        }
        return v.as_numeric() != 0;
    };

    // 文字列連結の処理（+演算子）
    if (node->op == "+" && left_value.is_string() && right_value.is_string()) {
        std::string result_str =
            left_value.string_value + right_value.string_value;
        return TypedValue(result_str, InferredType(TYPE_STRING, "string"));
    }

    // ポインタ演算の特別処理
    if (node->op == "+" || node->op == "-") {
        // 左オペランドがポインタの場合
        if (left_value.numeric_type == TYPE_POINTER ||
            TypeHelpers::isPointer(left_value)) {
            int64_t left_ptr = left_value.as_numeric();
            int64_t offset = right_value.as_numeric();

            // メタデータポインタの場合
            if (left_ptr & (1LL << 63)) {
                int64_t clean_ptr = left_ptr & ~(1LL << 63);
                using namespace PointerSystem;
                PointerMetadata *meta =
                    reinterpret_cast<PointerMetadata *>(clean_ptr);

                if (meta) {
                    // 真のポインタ演算：アドレス = アドレス + (オフセット ×
                    // sizeof(要素型))
                    // 配列要素はint64_t（8バイト）として保存されているため、実際のメモリレイアウトに合わせる
                    ptrdiff_t offset_value = static_cast<ptrdiff_t>(offset);
                    uintptr_t new_address;
                    size_t actual_element_size =
                        sizeof(int64_t); // 配列要素は常にint64_tで保存

                    if (node->op == "+") {
                        new_address = meta->address +
                                      (offset_value * actual_element_size);
                    } else { // "-"
                        new_address = meta->address -
                                      (offset_value * actual_element_size);
                    }

                    // 範囲チェック（配列ポインタの場合）
                    if (meta->array_var) {
                        if (new_address < meta->array_start_addr ||
                            new_address >= meta->array_end_addr) {
                            throw std::runtime_error(
                                "Pointer arithmetic out of array bounds");
                        }
                    }

                    // 新しいメタデータを作成
                    PointerMetadata *new_meta = new PointerMetadata();

                    // グローバルプールに追加（自動管理）
                    using namespace PointerSystem;
                    global_metadata_pool.push_back(new_meta);

                    new_meta->target_type = meta->target_type;
                    new_meta->address = new_address;
                    new_meta->pointed_type = meta->pointed_type;
                    new_meta->type_size = meta->type_size;
                    new_meta->element_type = meta->element_type;

                    // 範囲チェック情報をコピー
                    new_meta->array_var = meta->array_var;
                    new_meta->array_start_addr = meta->array_start_addr;
                    new_meta->array_end_addr = meta->array_end_addr;

                    // インデックスを更新（レガシー互換性のため）
                    if (meta->array_var && actual_element_size > 0) {
                        new_meta->element_index =
                            (new_address - meta->array_start_addr) /
                            actual_element_size;
                    }

                    // タグ付きポインタを返す
                    int64_t ptr_value = reinterpret_cast<int64_t>(new_meta);
                    ptr_value |= (1LL << 63);

                    InferredType ptr_type(TYPE_POINTER,
                                          left_value.type.type_name);
                    TypedValue result(ptr_value, ptr_type);
                    result.numeric_type = TYPE_POINTER;
                    return result;
                }
            }
        }
    }

    if (node->op == "+") {
        return make_numeric_typed_value(left_quad + right_quad,
                                        prefer_integral_result);
    } else if (node->op == "-") {
        return make_numeric_typed_value(left_quad - right_quad,
                                        prefer_integral_result);
    } else if (node->op == "*") {
        return make_numeric_typed_value(left_quad * right_quad,
                                        prefer_integral_result);
    } else if (node->op == "/") {
        bool treat_as_float_division =
            !prefer_integral_result &&
            (inferred_type.type_info == TYPE_QUAD ||
             inferred_type.type_info == TYPE_DOUBLE ||
             inferred_type.type_info == TYPE_FLOAT);
        if (treat_as_float_division) {
            if (right_quad == 0.0L) {
                error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
                throw std::runtime_error("Division by zero");
            }
            return make_numeric_typed_value(left_quad / right_quad, false);
        } else {
            if (right_int == 0) {
                error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
                throw std::runtime_error("Division by zero");
            }
            return make_integer_typed_value(left_int / right_int);
        }
    } else if (node->op == "%") {
        if (right_int == 0) {
            error_msg(DebugMsgId::ZERO_DIVISION_ERROR);
            throw std::runtime_error("Modulo by zero");
        }
        return make_integer_typed_value(left_int % right_int);
    } else if (node->op == "==") {
        // 文字列比較
        if (left_value.is_string() || right_value.is_string()) {
            return make_bool_typed_value(left_value.string_value ==
                                         right_value.string_value);
        }
        // 浮動小数点比較
        if (inferred_type.type_info == TYPE_QUAD ||
            inferred_type.type_info == TYPE_DOUBLE ||
            inferred_type.type_info == TYPE_FLOAT || left_value.is_floating() ||
            right_value.is_floating()) {
            return make_bool_typed_value(left_quad == right_quad);
        }
        return make_bool_typed_value(left_int == right_int);
    } else if (node->op == "!=") {
        // 文字列比較
        if (left_value.is_string() || right_value.is_string()) {
            return make_bool_typed_value(left_value.string_value !=
                                         right_value.string_value);
        }
        // 浮動小数点比較
        if (inferred_type.type_info == TYPE_QUAD ||
            inferred_type.type_info == TYPE_DOUBLE ||
            inferred_type.type_info == TYPE_FLOAT || left_value.is_floating() ||
            right_value.is_floating()) {
            return make_bool_typed_value(left_quad != right_quad);
        }
        return make_bool_typed_value(left_int != right_int);
    } else if (node->op == "<") {
        // 文字列比較
        if (left_value.is_string() || right_value.is_string()) {
            return make_bool_typed_value(left_value.string_value <
                                         right_value.string_value);
        }
        // 浮動小数点比較
        if (inferred_type.type_info == TYPE_QUAD ||
            inferred_type.type_info == TYPE_DOUBLE ||
            inferred_type.type_info == TYPE_FLOAT || left_value.is_floating() ||
            right_value.is_floating()) {
            return make_bool_typed_value(left_quad < right_quad);
        }
        return make_bool_typed_value(left_int < right_int);
    } else if (node->op == ">") {
        // 文字列比較
        if (left_value.is_string() || right_value.is_string()) {
            return make_bool_typed_value(left_value.string_value >
                                         right_value.string_value);
        }
        // オペランドのいずれかが浮動小数点型なら浮動小数点比較
        if (inferred_type.type_info == TYPE_QUAD ||
            inferred_type.type_info == TYPE_DOUBLE ||
            inferred_type.type_info == TYPE_FLOAT || left_value.is_floating() ||
            right_value.is_floating()) {
            return make_bool_typed_value(left_quad > right_quad);
        }
        return make_bool_typed_value(left_int > right_int);
    } else if (node->op == "<=") {
        // 文字列比較
        if (left_value.is_string() || right_value.is_string()) {
            return make_bool_typed_value(left_value.string_value <=
                                         right_value.string_value);
        }
        // 浮動小数点比較
        if (inferred_type.type_info == TYPE_QUAD ||
            inferred_type.type_info == TYPE_DOUBLE ||
            inferred_type.type_info == TYPE_FLOAT || left_value.is_floating() ||
            right_value.is_floating()) {
            return make_bool_typed_value(left_quad <= right_quad);
        }
        return make_bool_typed_value(left_int <= right_int);
    } else if (node->op == ">=") {
        // 文字列比較
        if (left_value.is_string() || right_value.is_string()) {
            return make_bool_typed_value(left_value.string_value >=
                                         right_value.string_value);
        }
        // 浮動小数点比較
        if (inferred_type.type_info == TYPE_QUAD ||
            inferred_type.type_info == TYPE_DOUBLE ||
            inferred_type.type_info == TYPE_FLOAT || left_value.is_floating() ||
            right_value.is_floating()) {
            return make_bool_typed_value(left_quad >= right_quad);
        }
        return make_bool_typed_value(left_int >= right_int);
    } else if (node->op == "&&") {
        return make_bool_typed_value(truthy(left_value) && truthy(right_value));
    } else if (node->op == "||") {
        return make_bool_typed_value(truthy(left_value) || truthy(right_value));
    } else if (node->op == "&") {
        return make_integer_typed_value(left_int & right_int);
    } else if (node->op == "|") {
        return make_integer_typed_value(left_int | right_int);
    } else if (node->op == "^") {
        return make_integer_typed_value(left_int ^ right_int);
    } else if (node->op == "<<") {
        return make_integer_typed_value(left_int << right_int);
    } else if (node->op == ">>") {
        return make_integer_typed_value(left_int >> right_int);
    }

    // 未対応の演算子の場合は例外
    throw std::runtime_error(
        "Unsupported binary operator in typed evaluation: " + node->op);
}

TypedValue evaluate_unary_op_typed(
    const ASTNode *node, Interpreter &interpreter,
    const InferredType &inferred_type,
    std::function<TypedValue(const ASTNode *)> evaluate_typed_func,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func) {
    bool debug_mode = interpreter.is_debug_mode();

    // アドレス演算子 (&)
    if (node->op == "ADDRESS_OF") {
        if (debug_mode) {
            std::cerr << "[ADDRESS_OF evaluate_typed] is_function_address="
                      << node->is_function_address
                      << ", function_address_name='"
                      << node->function_address_name << "'"
                      << ", has_left=" << (node->left != nullptr) << std::endl;
        }

        // 関数アドレスの特別処理
        // is_function_addressがtrueで関数名がある場合、まず関数として検索
        // ただし、leftがAST_ARRAY_REFの場合は配列要素なので除外
        bool is_array_element =
            node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF;

        if (node->is_function_address && !node->function_address_name.empty() &&
            !is_array_element) {
            const ASTNode *func_node =
                interpreter.find_function(node->function_address_name);

            // 関数が見つかった場合のみ関数ポインタとして処理
            if (func_node) {
                // 関数ポインタ値として関数ノードの実際のメモリアドレスを返す
                int64_t func_address = reinterpret_cast<int64_t>(func_node);

                // 戻り値型を取得してポインタ型を構築
                std::string type_name;
                switch (func_node->type_info) {
                case TYPE_INT:
                    type_name = "int";
                    break;
                case TYPE_FLOAT:
                    type_name = "float";
                    break;
                case TYPE_DOUBLE:
                    type_name = "double";
                    break;
                case TYPE_STRING:
                    type_name = "string";
                    break;
                case TYPE_VOID:
                    type_name = "void";
                    break;
                default:
                    type_name = "int";
                    break;
                }
                std::string func_ptr_type = type_name + "*";
                InferredType pointer_type(TYPE_POINTER, func_ptr_type);

                if (debug_mode) {
                    std::cerr << "[FUNC_PTR evaluate_typed] Taking address of "
                                 "function: "
                              << node->function_address_name << " -> "
                              << func_address << ", type: " << func_ptr_type
                              << std::endl;
                }

                // 関数ポインタ情報を含むTypedValueを返す
                return TypedValue::function_pointer(func_address,
                                                    node->function_address_name,
                                                    func_node, pointer_type);
            }
            // 関数が見つからない場合は変数として処理
            if (debug_mode) {
                std::cerr << "[ADDRESS_OF evaluate_typed] Not a function, "
                             "treating as variable address: "
                          << node->function_address_name << std::endl;
            }

            // function_address_nameを使って変数を検索
            Variable *var =
                interpreter.find_variable(node->function_address_name);
            if (!var) {
                error_msg(DebugMsgId::UNDEFINED_VAR_ERROR,
                          node->function_address_name.c_str());
                throw std::runtime_error("Undefined variable");
            }

            std::string ptr_type = var->type_name + "*";
            InferredType pointer_type(TYPE_POINTER, ptr_type);

            // Variable*を返す（従来の方式）
            int64_t addr = reinterpret_cast<int64_t>(var);

            if (debug_mode) {
                std::cerr << "[ADDRESS_OF evaluate_typed] var=" << var
                          << ", is_struct=" << var->is_struct
                          << ", is_array=" << var->is_array
                          << ", returning Variable* addr=" << addr << std::endl;
            }

            return TypedValue(addr, pointer_type);
        }

        if (!node->left) {
            throw std::runtime_error("Address-of operator requires an operand");
        }

        // 変数のアドレス取得
        if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = interpreter.find_variable(node->left->name);
            if (!var) {
                error_msg(DebugMsgId::UNDEFINED_VAR_ERROR,
                          node->left->name.c_str());
                throw std::runtime_error("Undefined variable");
            }

            // ポインタ型として返す
            std::string ptr_type = var->type_name + "*";
            InferredType pointer_type(TYPE_POINTER, ptr_type);

            // Variable*を返す（従来の方式）
            int64_t addr = reinterpret_cast<int64_t>(var);

            if (debug_mode) {
                std::cerr << "[ADDRESS_OF evaluate_typed AST_VARIABLE] var="
                          << var << ", is_struct=" << var->is_struct
                          << ", is_array=" << var->is_array
                          << ", returning Variable* addr=" << addr << std::endl;
            }

            return TypedValue(addr, pointer_type);
        }
        // 配列要素や構造体メンバーの場合は通常評価にフォールバック
        else {
            int64_t address = evaluate_expression_func(node);
            if (debug_mode) {
                std::cerr << "[ADDRESS_OF evaluate_typed] evaluate_expression "
                             "returned: "
                          << address << " (0x" << std::hex << address
                          << std::dec << ")" << std::endl;
            }
            InferredType pointer_type(TYPE_POINTER, "int*"); // 暫定的にint*
            TypedValue result(address, pointer_type);
            if (debug_mode) {
                std::cerr
                    << "[ADDRESS_OF evaluate_typed] Created TypedValue: value="
                    << result.value << " (0x" << std::hex << result.value
                    << std::dec << ")" << std::endl;
                std::cerr << "[ADDRESS_OF evaluate_typed] TypedValue fields: "
                             "numeric_type="
                          << static_cast<int>(result.numeric_type)
                          << ", is_numeric=" << result.is_numeric()
                          << ", is_float=" << result.is_float_result
                          << std::endl;
            }
            return result;
        }
    }

    // 間接参照演算子 (*)
    if (node->op == "DEREFERENCE") {
        TypedValue ptr_value = evaluate_typed_func(node->left.get());
        int64_t ptr_int = ptr_value.as_numeric();

        // 変数参照の場合、変数の型情報も取得
        std::string var_type_name;
        if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *var = interpreter.find_variable(node->left->name);
            if (var) {
                var_type_name = var->type_name;
            }
        }

        if (debug_mode) {
            std::cerr << "[DEREFERENCE] ptr_int=0x" << std::hex << ptr_int
                      << std::dec
                      << ", has_meta=" << ((ptr_int & (1LL << 63)) != 0)
                      << ", type_name='" << ptr_value.type.type_name << "'"
                      << ", var_type_name='" << var_type_name << "'"
                      << std::endl;
        }

        if (ptr_int == 0) {
            throw std::runtime_error("Null pointer dereference");
        }

        // 構造体ポインタのチェック（型名から判定）
        // 1. TypedValueの型名をチェック
        // 2. 変数の型名をチェック（キャスト式の結果が変数に保存されている場合）
        std::string check_type_name = ptr_value.type.type_name;
        if (check_type_name == "pointer" && !var_type_name.empty()) {
            check_type_name = var_type_name;
        }

        if (!check_type_name.empty() &&
            check_type_name.find('*') != std::string::npos) {
            // 構造体型名から'*'を除去
            std::string struct_name = check_type_name;
            size_t star_pos = struct_name.find('*');
            if (star_pos != std::string::npos) {
                struct_name = struct_name.substr(0, star_pos);
            }

            // スペースを除去（"Point *" -> "Point"）
            struct_name.erase(std::remove_if(struct_name.begin(),
                                             struct_name.end(), ::isspace),
                              struct_name.end());

            // 構造体定義を確認
            const StructDefinition *struct_def =
                interpreter.find_struct_definition(struct_name);

            if (struct_def) {
                // 構造体ポインタのデリファレンス
                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] Struct pointer: " << struct_name
                              << ", address=0x" << std::hex << ptr_int
                              << std::dec << std::endl;
                }

                // Variable*としてデリファレンス
                Variable *var = reinterpret_cast<Variable *>(ptr_int);
                InferredType struct_type(TYPE_STRUCT, struct_name);
                return TypedValue(*var, struct_type);
            }
        }

        // ポインタがメタデータを持つかチェック（最上位ビット）
        if (ptr_int & (1LL << 63)) {
            // メタデータポインタの場合
            int64_t clean_ptr = ptr_int & ~(1LL << 63); // タグを除去

            using namespace PointerSystem;
            PointerMetadata *meta =
                reinterpret_cast<PointerMetadata *>(clean_ptr);

            if (!meta) {
                throw std::runtime_error("Invalid pointer metadata");
            }

            // 構造体ポインタ（型キャスト済み）の場合
            if (debug_mode) {
                std::cerr << "[DEREFERENCE] Checking struct_type_name: '"
                          << meta->struct_type_name << "'" << std::endl;
            }

            if (!meta->struct_type_name.empty()) {
                // 構造体型名から'*'を除去
                std::string struct_name = meta->struct_type_name;
                size_t star_pos = struct_name.find('*');
                if (star_pos != std::string::npos) {
                    struct_name = struct_name.substr(0, star_pos);
                }

                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] Struct pointer detected: "
                              << struct_name << ", address=0x" << std::hex
                              << meta->address << std::dec << std::endl;
                }

                // 構造体定義を取得
                const StructDefinition *struct_def =
                    interpreter.find_struct_definition(struct_name);
                if (!struct_def) {
                    throw std::runtime_error(
                        "Dereference requires struct or interface pointer");
                }

                // 構造体ポインタのデリファレンスは、構造体インスタンスを返す
                // 実際には、生メモリから構造体全体を読み取るのは複雑なため、
                // メンバーアクセス時に処理する
                // ここでは、構造体型情報を持つTypedValueを返す
                InferredType struct_type(TYPE_STRUCT, struct_name);

                // メタデータのポインタ値（生メモリのアドレス）を返す
                // これはメンバーアクセス時に使用される
                void *base_ptr = reinterpret_cast<void *>(meta->address);

                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] Returning TypedValue: "
                              << "base_ptr=0x" << std::hex
                              << reinterpret_cast<int64_t>(base_ptr) << std::dec
                              << ", type=TYPE_STRUCT(" << struct_name << ")"
                              << std::endl;
                }

                return TypedValue(reinterpret_cast<int64_t>(base_ptr),
                                  struct_type);
            }

            // メタデータから型に応じて値を読み取り
            TypeInfo elem_type = meta->pointed_type;
            InferredType deref_type(elem_type,
                                    type_info_to_string_simple(elem_type));

            if (elem_type == TYPE_FLOAT || elem_type == TYPE_DOUBLE ||
                elem_type == TYPE_QUAD) {
                // 浮動小数点数の場合
                double float_value = meta->read_float_value();
                return TypedValue(float_value, deref_type);
            } else {
                // 整数型の場合
                int64_t value = meta->read_int_value();
                return TypedValue(value, deref_type);
            }
        } else {
            // v0.13.1: void* と void** の場合は生のポインタ値として扱う
            // Variable* として reinterpret_cast するのは危険
            if (check_type_name == "void**" || var_type_name == "void**") {
                // void** をデリファレンスした結果は void*
                // （メモリから読み取る）
                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] void** dereference: reading "
                                 "void* from address 0x"
                              << std::hex << ptr_int << std::dec << std::endl;
                }
                // ptr_int は void** のアドレス、そこから void* を読み取る
                void **ptr_to_ptr = reinterpret_cast<void **>(ptr_int);
                void *ptr_value = *ptr_to_ptr;
                int64_t ptr_as_int = reinterpret_cast<int64_t>(ptr_value);
                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] void** -> void*: value=0x"
                              << std::hex << ptr_as_int << std::dec
                              << std::endl;
                }
                InferredType void_ptr_type(TYPE_POINTER, "void*");
                return TypedValue(ptr_as_int, void_ptr_type);
            }

            if (check_type_name == "void*" || var_type_name == "void*") {
                // void* のデリファレンスは未定義動作だが、
                // 暫定的に int として読み取る（malloc からの読み取りなど）
                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] void* dereference: reading as "
                                 "int from 0x"
                              << std::hex << ptr_int << std::dec << std::endl;
                }
                int *int_ptr = reinterpret_cast<int *>(ptr_int);
                int value = *int_ptr;
                InferredType int_type(TYPE_INT, "int");
                return TypedValue(static_cast<int64_t>(value), int_type);
            }

            // 元のポインタ変数の型情報を確認
            Variable *ptr_var = nullptr;
            if (node->left &&
                node->left->node_type == ASTNodeType::AST_VARIABLE) {
                ptr_var = interpreter.find_variable(node->left->name);
            }

            // プリミティブ型のポインタ（int*, float*など）かVariable*かを判別
            // points_to_heap_memoryフラグで判定（new式で作成されたか）
            bool is_raw_memory_ptr = false;
            TypeInfo deref_elem_type = TYPE_INT; // デフォルト
            if (ptr_var && ptr_var->points_to_heap_memory) {
                // new式で作成された生メモリポインタ
                is_raw_memory_ptr = true;
                deref_elem_type = ptr_var->type;
                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] Heap memory pointer detected"
                              << ", type=" << static_cast<int>(deref_elem_type)
                              << std::endl;
                }
            }

            if (is_raw_memory_ptr) {
                // 生メモリからの読み取り（new int等で割り当てられたメモリ）
                if (debug_mode) {
                    std::cerr << "[DEREFERENCE] Raw memory read: ptr=0x"
                              << std::hex << ptr_int << std::dec
                              << ", type=" << static_cast<int>(deref_elem_type)
                              << std::endl;
                }

                // 型に応じて適切なサイズで読み取る
                if (deref_elem_type == TYPE_FLOAT) {
                    float *target = reinterpret_cast<float *>(ptr_int);
                    InferredType float_type(TYPE_FLOAT, "float");
                    return TypedValue(static_cast<double>(*target), float_type);
                } else if (deref_elem_type == TYPE_DOUBLE) {
                    double *target = reinterpret_cast<double *>(ptr_int);
                    InferredType double_type(TYPE_DOUBLE, "double");
                    return TypedValue(*target, double_type);
                } else if (deref_elem_type == TYPE_QUAD) {
                    long double *target =
                        reinterpret_cast<long double *>(ptr_int);
                    InferredType quad_type(TYPE_QUAD, "quad");
                    return TypedValue(static_cast<double>(*target), quad_type);
                } else {
                    // 整数型
                    std::string type_name =
                        type_info_to_string_simple(deref_elem_type);
                    InferredType int_type(deref_elem_type, type_name);
                    if (deref_elem_type == TYPE_LONG ||
                        deref_elem_type == TYPE_BIG) {
                        long *target = reinterpret_cast<long *>(ptr_int);
                        return TypedValue(static_cast<int64_t>(*target),
                                          int_type);
                    } else if (deref_elem_type == TYPE_CHAR) {
                        char *target = reinterpret_cast<char *>(ptr_int);
                        return TypedValue(static_cast<int64_t>(*target),
                                          int_type);
                    } else {
                        // デフォルトは4バイト整数
                        int *target = reinterpret_cast<int *>(ptr_int);
                        return TypedValue(static_cast<int64_t>(*target),
                                          int_type);
                    }
                }
            } else {
                // 従来の方式（変数ポインタ）
                Variable *var = reinterpret_cast<Variable *>(ptr_int);

                // 参照先の変数の型で返す
                if (var->type == TYPE_STRUCT || var->is_struct) {
                    // 構造体の場合
                    InferredType deref_type(TYPE_STRUCT, var->struct_type_name);
                    return TypedValue(*var, deref_type);
                } else if (var->type == TYPE_STRING) {
                    // 文字列の場合
                    InferredType deref_type(TYPE_STRING, "string");
                    return TypedValue(var->str_value, deref_type);
                } else if (var->type == TYPE_FLOAT ||
                           var->type == TYPE_DOUBLE || var->type == TYPE_QUAD) {
                    // 浮動小数点数の場合
                    InferredType deref_type(
                        var->type, type_info_to_string_simple(var->type));
                    return TypedValue(var->double_value, deref_type);
                } else {
                    // その他（整数型など）
                    InferredType deref_type(var->type, var->type_name);
                    return TypedValue(var->value, deref_type);
                }
            }
        }
    }

    // v0.12.0: await式の評価
    if (node->is_await_expression || node->op == "await") {
        // オペランドを評価（Future<T>構造体を期待）
        TypedValue future_value = evaluate_typed_func(node->left.get());

        if (future_value.is_struct() && future_value.struct_data) {
            Variable &future_var =
                const_cast<Variable &>(*future_value.struct_data);

            // Future<T>構造体であることを確認（型名が"Future"で始まる）
            if (future_var.struct_type_name.find("Future") == 0) {
                // is_readyフラグをチェック
                auto ready_it = future_var.struct_members.find("is_ready");
                if (ready_it != future_var.struct_members.end()) {
                    bool is_ready = (ready_it->second.value != 0);

                    debug_msg(DebugMsgId::AWAIT_FUTURE_READY_CHECK,
                              is_ready ? "true" : "false");

                    // v0.12.0:
                    // Futureが未完了の場合、run_until_complete()で完了まで待機
                    // task_idを取得して、そのタスクが完了するまでイベントループを実行
                    if (!is_ready) {
                        auto task_id_it =
                            future_var.struct_members.find("task_id");
                        if (task_id_it != future_var.struct_members.end()) {
                            int task_id = task_id_it->second.value;

                            debug_msg(DebugMsgId::AWAIT_RUN_UNTIL_COMPLETE,
                                      std::to_string(task_id).c_str());

                            // 指定タスクが完了するまでSimpleEventLoopを実行
                            interpreter.get_simple_event_loop()
                                .run_until_complete(task_id);

                            // タスクからinternal_futureを取得（更新された値を取得）
                            AsyncTask *task =
                                interpreter.get_simple_event_loop().get_task(
                                    task_id);
                            if (task && task->use_internal_future) {
                                future_var = task->internal_future;
                            }

                            // 実行後、is_readyを再確認
                            ready_it =
                                future_var.struct_members.find("is_ready");
                            if (ready_it != future_var.struct_members.end()) {
                                is_ready = (ready_it->second.value != 0);
                            }
                        }
                    }

                    // v0.12.0 Phase 7: (Phase 1用のフォールバック)
                    // まだ実行されていない場合、ここで実行する（遅延実行）
                    // Phase
                    // 2の場合はtask_idがないので、この処理はスキップされる
                    if (!is_ready) {

                        // task_idを取得（Phase 1のみ）
                        auto task_id_it =
                            future_var.struct_members.find("task_id");
                        if (task_id_it != future_var.struct_members.end()) {
                            int task_id = task_id_it->second.value;

                            if (debug_mode) {
                                std::cerr
                                    << "[AWAIT_DEFERRED] Looking for task_id="
                                    << task_id << std::endl;
                            }

                            // タスクを取得
                            AsyncTask *task =
                                interpreter.get_async_task(task_id);
                            if (task && !task->is_executed) {
                                if (debug_mode) {
                                    std::cerr
                                        << "[AWAIT_DEFERRED] Executing task: "
                                        << task->function_name << " with "
                                        << task->args.size() << " args"
                                        << std::endl;
                                }

                                // v0.12.0 Phase 7: タスクを実行
                                const ASTNode *func = task->function_node;
                                if (!func || !func->body) {
                                    throw std::runtime_error(
                                        "Task has no function body (task_id=" +
                                        std::to_string(task_id) + ")");
                                }

                                // 新しいスコープを作成して引数を復元
                                interpreter.push_scope();

                                // 引数をスコープに設定
                                for (size_t i = 0; i < task->args.size() &&
                                                   i < func->parameters.size();
                                     i++) {
                                    const auto &param = func->parameters[i];
                                    Variable arg_var = task->args[i]; // コピー
                                    interpreter.current_scope()
                                        .variables[param->name] = arg_var;

                                    if (debug_mode) {
                                        std::cerr
                                            << "[AWAIT_DEFERRED] Restored arg["
                                            << i << "]: " << param->name
                                            << " = " << arg_var.value
                                            << std::endl;
                                    }
                                }

                                // 関数本体を実行
                                try {
                                    interpreter.execute_statement(
                                        func->body.get());

                                    // 正常終了（return文なし）の場合はvoid扱い
                                    interpreter.pop_scope();
                                    task->is_executed = true;

                                    // Future.is_readyをtrueに設定
                                    ready_it->second.value = 1;

                                    // valueメンバーを0に設定（void関数）
                                    auto value_it =
                                        future_var.struct_members.find("value");
                                    if (value_it !=
                                        future_var.struct_members.end()) {
                                        value_it->second.type = TYPE_INT;
                                        value_it->second.value = 0;
                                        value_it->second.is_assigned = true;
                                    }

                                    if (debug_mode) {
                                        std::cerr
                                            << "[AWAIT_DEFERRED] Task executed "
                                               "successfully (void)"
                                            << std::endl;
                                    }
                                } catch (const ReturnException &ret) {
                                    // return文で値が返された場合
                                    interpreter.pop_scope();
                                    task->is_executed = true;

                                    // Future.is_readyをtrueに設定
                                    ready_it->second.value = 1;

                                    // Future.valueメンバーに戻り値を設定
                                    auto value_it =
                                        future_var.struct_members.find("value");
                                    if (value_it !=
                                        future_var.struct_members.end()) {
                                        if (ret.type == TYPE_STRING) {
                                            value_it->second.type = TYPE_STRING;
                                            value_it->second.str_value =
                                                ret.str_value;
                                        } else if (ret.type == TYPE_FLOAT ||
                                                   ret.type == TYPE_DOUBLE ||
                                                   ret.type == TYPE_QUAD) {
                                            value_it->second.type = ret.type;
                                            value_it->second.double_value =
                                                ret.double_value;
                                        } else if (ret.is_struct) {
                                            value_it->second = ret.struct_value;
                                        } else {
                                            value_it->second.type = TYPE_INT;
                                            value_it->second.value = ret.value;
                                        }
                                        value_it->second.is_assigned = true;
                                    }

                                    if (debug_mode) {
                                        std::cerr
                                            << "[AWAIT_DEFERRED] Task executed "
                                               "with return value: "
                                            << ret.value << std::endl;
                                    }
                                }
                            } else {
                                throw std::runtime_error(
                                    "Task not found or already executed "
                                    "(task_id=" +
                                    std::to_string(task_id) + ")");
                            }
                        }
                        // Phase
                        // 2の場合はtask_idがないが、SimpleEventLoopで処理済みなのでOK
                        // エラーを投げずに続行して値を取得する
                    }
                }

                // Future構造体からvalueメンバーを取得
                auto value_it = future_var.struct_members.find("value");
                if (value_it != future_var.struct_members.end()) {
                    const Variable &value_member = value_it->second;

                    // 抽出値のデバッグ表示
                    int64_t display_value = 0;
                    if (value_member.type == TYPE_FLOAT ||
                        value_member.type == TYPE_DOUBLE ||
                        value_member.type == TYPE_QUAD) {
                        display_value =
                            static_cast<int64_t>(value_member.double_value);
                    } else if (!value_member.is_struct) {
                        display_value = value_member.value;
                    }
                    debug_msg(DebugMsgId::AWAIT_VALUE_EXTRACTED, display_value,
                              static_cast<int>(value_member.type));

                    // valueメンバーの型に応じて適切なTypedValueを返す
                    if (value_member.type == TYPE_STRING) {
                        return TypedValue(value_member.str_value,
                                          InferredType(TYPE_STRING, "string"));
                    } else if (value_member.type == TYPE_FLOAT ||
                               value_member.type == TYPE_DOUBLE ||
                               value_member.type == TYPE_QUAD) {
                        return TypedValue(
                            value_member.double_value,
                            InferredType(
                                value_member.type,
                                type_info_to_string_simple(value_member.type)));
                    } else if (value_member.type == TYPE_STRUCT ||
                               value_member.is_struct) {
                        return TypedValue(
                            value_member,
                            InferredType(TYPE_STRUCT,
                                         value_member.struct_type_name));
                    } else {
                        // 整数型
                        return TypedValue(
                            value_member.value,
                            InferredType(
                                value_member.type,
                                type_info_to_string_simple(value_member.type)));
                    }
                } else {
                    throw std::runtime_error(
                        "Future struct has no 'value' member");
                }
            } else {
                throw std::runtime_error(
                    "await expression requires Future<T> struct, got " +
                    future_var.struct_type_name);
            }
        } else {
            throw std::runtime_error(
                "await expression requires Future<T> operand");
        }
    }

    if (node->op == "+" || node->op == "-") {
        TypedValue operand_value = evaluate_typed_func(node->left.get());
        long double operand_quad = operand_value.as_quad();
        if (node->op == "-") {
            operand_quad = -operand_quad;
        }
        if (inferred_type.type_info == TYPE_QUAD) {
            return TypedValue(operand_quad,
                              ensure_type(inferred_type, TYPE_QUAD, "quad"));
        }
        if (inferred_type.type_info == TYPE_DOUBLE ||
            inferred_type.type_info == TYPE_FLOAT) {
            TypeInfo info = (inferred_type.type_info == TYPE_FLOAT)
                                ? TYPE_FLOAT
                                : TYPE_DOUBLE;
            return TypedValue(
                static_cast<double>(operand_quad),
                ensure_type(inferred_type, info,
                            info == TYPE_FLOAT ? "float" : "double"));
        }
        TypeInfo int_like = inferred_type.type_info == TYPE_UNKNOWN
                                ? TYPE_INT
                                : inferred_type.type_info;
        return TypedValue(
            static_cast<int64_t>(operand_quad),
            ensure_type(inferred_type, int_like,
                        std::string(type_info_to_string_simple(int_like))));
    } else if (node->op == "!") {
        TypedValue operand_value = evaluate_typed_func(node->left.get());
        bool operand_truthy = operand_value.is_floating()
                                  ? (operand_value.as_double() != 0.0)
                                  : (operand_value.as_numeric() != 0);
        return TypedValue(static_cast<int64_t>(operand_truthy ? 0 : 1),
                          ensure_type(inferred_type, TYPE_BOOL, "bool"));
    }

    // フォールバック
    int64_t numeric_result = evaluate_expression_func(node);

    // consume_numeric_typed_value相当の処理
    InferredType result_type = inferred_type;
    if (result_type.type_info == TYPE_UNKNOWN) {
        result_type = InferredType(TYPE_INT, "int");
    }
    return TypedValue(numeric_result, result_type);
}

} // namespace BinaryUnaryTypedHelpers

namespace BinaryAndUnaryOperators {

// v0.12.0: await式の評価
int64_t evaluate_await(
    const ASTNode *node, Interpreter &interpreter,
    const std::function<TypedValue(const ASTNode *)> &evaluate_typed_func) {

    // オペランドを評価（Future<T>構造体を期待）
    TypedValue future_value = evaluate_typed_func(node->left.get());

    // Phase 7 Fix: 変数名の場合は直接Interpreterから取得
    Variable *future_var_ptr = nullptr;
    std::string var_name;
    if (node->left && node->left->node_type == ASTNodeType::AST_VARIABLE) {
        // 変数名から直接取得
        var_name = node->left->name;
        Variable *var = interpreter.get_variable(var_name);
        if (var) {
            future_var_ptr = var;
        }
    }

    // TypedValueからも取得を試みる
    Variable future_var_copy; // ローカルコピーを作成
    if (!future_var_ptr && future_value.is_struct() &&
        future_value.struct_data) {
        future_var_copy = *future_value.struct_data; // 完全にコピー
        future_var_ptr = &future_var_copy;
    }

    if (future_var_ptr) {
        Variable &future_var = *future_var_ptr;

        // Future<T>構造体であることを確認（型名が"Future"で始まる）
        if (future_var.struct_type_name.find("Future") == 0) {
            if (!var_name.empty()) {
                debug_msg(DebugMsgId::AWAIT_EXPRESSION_START, var_name.c_str());
            }

            // is_readyフラグをチェック
            auto ready_it = future_var.struct_members.find("is_ready");
            if (ready_it != future_var.struct_members.end()) {
                bool is_ready = (ready_it->second.value != 0);

                debug_msg(DebugMsgId::AWAIT_FUTURE_READY_CHECK,
                          is_ready ? "true" : "false");

                // v0.12.0 Phase 9:
                // まだ実行されていない場合、このタスクだけを実行
                if (!is_ready) {

                    // awaited タスクのIDを取得
                    auto task_id_it = future_var.struct_members.find("task_id");
                    if (task_id_it == future_var.struct_members.end()) {
                        throw std::runtime_error(
                            "Future struct has no 'task_id' member");
                    }
                    int awaited_task_id = task_id_it->second.value;

                    // v0.13.0: 親タスクを「待機中」に設定してから
                    // run_until_complete
                    int current_task_id =
                        interpreter.get_current_executing_task_id();
                    if (current_task_id >= 0) {
                        AsyncTask *current_task =
                            interpreter.get_simple_event_loop().get_task(
                                current_task_id);
                        if (current_task) {
                            current_task->is_waiting = true;
                            current_task->waiting_for_task_id = awaited_task_id;
                            debug_msg(DebugMsgId::AWAIT_TASK_WAITING,
                                      current_task_id, awaited_task_id);
                        }
                    }

                    // run_until_complete
                    // で待機（待機中の親タスクはスキップされる）
                    interpreter.get_simple_event_loop().run_until_complete(
                        awaited_task_id);

                    // run_until_complete後、最新のFutureから値を取得
                    Variable value_member_updated;
                    bool value_found = false;

                    // まず、タスクIDからタスクを取得して、
                    // internal_futureまたはfuture_varから値を取得を試みる
                    const AsyncTask *task =
                        interpreter.get_simple_event_loop().get_task(
                            awaited_task_id);
                    if (task) {
                        if (task->use_internal_future) {
                            auto value_it_internal =
                                task->internal_future.struct_members.find(
                                    "value");
                            if (value_it_internal !=
                                task->internal_future.struct_members.end()) {
                                value_member_updated =
                                    value_it_internal->second;
                                value_found = true;
                                debug_msg(DebugMsgId::AWAIT_INTERNAL_FUTURE);
                            }
                        } else if (task->future_var) {
                            auto value_it_future =
                                task->future_var->struct_members.find("value");
                            if (value_it_future !=
                                task->future_var->struct_members.end()) {
                                value_member_updated = value_it_future->second;
                                value_found = true;
                            }
                        }
                    }

                    // フォールバック:
                    // 変数名から取得を試みる（スコープが保持されている場合）
                    if (!value_found && !var_name.empty()) {
                        Variable *updated_future =
                            interpreter.get_variable(var_name);
                        if (updated_future) {
                            auto value_it_after =
                                updated_future->struct_members.find("value");
                            if (value_it_after !=
                                updated_future->struct_members.end()) {
                                value_member_updated = value_it_after->second;
                                value_found = true;
                            }
                        }
                    }

                    if (!value_found) {
                        throw std::runtime_error(
                            "Cannot retrieve value from Future after "
                            "run_until_complete");
                    }

                    // valueメンバーの値を直接返す
                    int64_t result;
                    if (value_member_updated.type == TYPE_STRING) {
                        result = 0;
                    } else if (value_member_updated.type == TYPE_FLOAT ||
                               value_member_updated.type == TYPE_DOUBLE ||
                               value_member_updated.type == TYPE_QUAD) {
                        result = static_cast<int64_t>(
                            value_member_updated.double_value);
                    } else if (value_member_updated.is_struct) {
                        result = 0;
                    } else {
                        result = value_member_updated.value;
                    }

                    debug_msg(DebugMsgId::AWAIT_VALUE_EXTRACTED, result,
                              static_cast<int>(value_member_updated.type));

                    return result;
                }

                // is_ready == true の場合: タスクは既に完了しているので、
                // タスクIDを取得してタスクの internal_future から値を取得
                auto task_id_it_ready =
                    future_var.struct_members.find("task_id");
                if (task_id_it_ready != future_var.struct_members.end()) {
                    int task_id_ready = task_id_it_ready->second.value;

                    const AsyncTask *task_ready =
                        interpreter.get_simple_event_loop().get_task(
                            task_id_ready);

                    if (task_ready) {
                        if (task_ready->use_internal_future) {
                            auto value_it_ready =
                                task_ready->internal_future.struct_members.find(
                                    "value");
                            if (value_it_ready != task_ready->internal_future
                                                      .struct_members.end()) {
                                const Variable &value_member =
                                    value_it_ready->second;

                                int64_t result;
                                if (value_member.type == TYPE_STRING) {
                                    result = 0;
                                } else if (value_member.type == TYPE_FLOAT ||
                                           value_member.type == TYPE_DOUBLE ||
                                           value_member.type == TYPE_QUAD) {
                                    result = static_cast<int64_t>(
                                        value_member.double_value);
                                } else if (value_member.is_struct) {
                                    result = 0;
                                } else {
                                    result = value_member.value;
                                }

                                debug_msg(DebugMsgId::AWAIT_TASK_COMPLETED,
                                          task_id_ready);

                                return result;
                            }
                        }
                    }
                }
            }

            // is_ready
            // フィールドがない場合、またはタスクから値が取得できなかった場合のフォールバック
            // Future構造体からvalueメンバーを取得
            auto value_it = future_var.struct_members.find("value");
            if (value_it != future_var.struct_members.end()) {
                const Variable &value_member = value_it->second;

                // valueメンバーの型に応じて適切な値を返す
                int64_t result;
                if (value_member.type == TYPE_STRING) {
                    // 文字列は数値変換できないので、とりあえず0を返す
                    result = 0;
                } else if (value_member.type == TYPE_FLOAT ||
                           value_member.type == TYPE_DOUBLE ||
                           value_member.type == TYPE_QUAD) {
                    result = static_cast<int64_t>(value_member.double_value);
                } else if (value_member.is_struct) {
                    // 構造体は数値変換できないので0を返す
                    result = 0;
                } else {
                    // 整数値を返す
                    result = value_member.value;
                }

                debug_msg(DebugMsgId::AWAIT_VALUE_EXTRACTED, result,
                          static_cast<int>(value_member.type));

                return result;
            } else {
                throw std::runtime_error("Future struct has no 'value' member");
            }
        } else {
            throw std::runtime_error(
                "await operand must be a Future<T> (got: " +
                future_var.struct_type_name + ")");
        }
    }

    throw std::runtime_error("await operand must be a Future<T>");
}

} // namespace BinaryAndUnaryOperators
