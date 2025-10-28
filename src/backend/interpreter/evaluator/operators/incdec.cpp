#include "incdec.h"
#include "../../../../common/debug.h"
#include "../../core/interpreter.h"
#include "../../core/pointer_metadata.h"
#include <cinttypes>
#include <cstdio>
#include <stdexcept>

namespace IncDecHelpers {

int64_t evaluate_incdec(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func) {
    if (!node->left) {
        error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR);
        throw std::runtime_error("Invalid increment/decrement operation");
    }

    // (*ptr)++ の場合：デリファレンス演算子の処理
    if (node->left->node_type == ASTNodeType::AST_UNARY_OP &&
        node->left->op == "DEREFERENCE") {
        if (!node->left->left) {
            throw std::runtime_error(
                "Invalid dereference in increment/decrement");
        }

        // ポインタ変数を取得
        int64_t ptr_value = 0;
        if (node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
            Variable *ptr_var =
                interpreter.find_variable(node->left->left->name);
            if (!ptr_var || ptr_var->type != TYPE_POINTER) {
                throw std::runtime_error("Not a pointer variable");
            }
            ptr_value = ptr_var->value;
        } else {
            ptr_value = evaluate_expression_func(node->left->left.get());
        }

        // メタデータポインタの場合
        bool is_metadata = (ptr_value & (1LL << 63)) != 0;
        Variable *target_var = nullptr;
        size_t array_index = 0;
        bool is_array_element = false;
        TypeInfo element_type = TYPE_INT;

        if (is_metadata) {
            int64_t clean_ptr = ptr_value & ~(1LL << 63);
            using namespace PointerSystem;
            PointerMetadata *meta =
                reinterpret_cast<PointerMetadata *>(clean_ptr);

            if (!meta) {
                throw std::runtime_error("Invalid pointer metadata");
            }

            // ポインタが指す値を取得して変更
            if (meta->target_type == PointerTargetType::VARIABLE) {
                target_var = meta->var_ptr;
            } else if (meta->target_type == PointerTargetType::ARRAY_ELEMENT) {
                target_var = meta->array_var;
                array_index = meta->element_index;
                is_array_element = true;
                element_type = meta->element_type;
            }

            if (!target_var) {
                throw std::runtime_error("Invalid pointer target");
            }
        } else {
            // 従来のポインタ形式（Variable*）
            target_var = reinterpret_cast<Variable *>(ptr_value);
            if (!target_var) {
                throw std::runtime_error("Null pointer dereference");
            }
        }

        // 型に応じてインクリメント/デクリメント
        int64_t old_value = 0;
        int64_t new_value = 0;

        if (!is_array_element) {
            // 変数へのポインタ
            if (target_var->type == TYPE_INT || target_var->type == TYPE_TINY ||
                target_var->type == TYPE_SHORT ||
                target_var->type == TYPE_LONG ||
                target_var->type == TYPE_CHAR) {
                old_value = target_var->value;
                if (node->op == "++") {
                    target_var->value += 1;
                } else {
                    target_var->value -= 1;
                }
                new_value = target_var->value;
            } else if (target_var->type == TYPE_FLOAT) {
                old_value = static_cast<int64_t>(target_var->float_value);
                if (node->op == "++") {
                    target_var->float_value += 1.0f;
                } else {
                    target_var->float_value -= 1.0f;
                }
                new_value = static_cast<int64_t>(target_var->float_value);
            } else if (target_var->type == TYPE_DOUBLE) {
                old_value = static_cast<int64_t>(target_var->double_value);
                if (node->op == "++") {
                    target_var->double_value += 1.0;
                } else {
                    target_var->double_value -= 1.0;
                }
                new_value = static_cast<int64_t>(target_var->double_value);
            } else {
                throw std::runtime_error("Unsupported type for pointer "
                                         "dereference increment/decrement");
            }
        } else {
            // 配列要素へのポインタ
            if (element_type == TYPE_INT || element_type == TYPE_CHAR) {
                auto &values = target_var->is_multidimensional
                                   ? target_var->multidim_array_values
                                   : target_var->array_values;
                if (array_index >= values.size()) {
                    throw std::runtime_error("Array index out of bounds");
                }
                old_value = values[array_index];
                if (node->op == "++") {
                    values[array_index] += 1;
                } else {
                    values[array_index] -= 1;
                }
                new_value = values[array_index];
            } else if (element_type == TYPE_FLOAT) {
                auto &values = target_var->is_multidimensional
                                   ? target_var->multidim_array_float_values
                                   : target_var->array_float_values;
                if (array_index >= values.size()) {
                    throw std::runtime_error("Array index out of bounds");
                }
                old_value = static_cast<int64_t>(values[array_index]);
                if (node->op == "++") {
                    values[array_index] += 1.0f;
                } else {
                    values[array_index] -= 1.0f;
                }
                new_value = static_cast<int64_t>(values[array_index]);
            } else if (element_type == TYPE_DOUBLE) {
                auto &values = target_var->is_multidimensional
                                   ? target_var->multidim_array_double_values
                                   : target_var->array_double_values;
                if (array_index >= values.size()) {
                    throw std::runtime_error("Array index out of bounds");
                }
                old_value = static_cast<int64_t>(values[array_index]);
                if (node->op == "++") {
                    values[array_index] += 1.0;
                } else {
                    values[array_index] -= 1.0;
                }
                new_value = static_cast<int64_t>(values[array_index]);
            } else {
                throw std::runtime_error("Unsupported array element type for "
                                         "dereference increment/decrement");
            }
        }

        // プレフィックスは新しい値、ポストフィックスは古い値を返す
        if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
            return new_value;
        } else {
            return old_value;
        }
    }

    // 変数の場合
    if (node->left->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *var = interpreter.find_variable(node->left->name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR,
                      node->left->name.c_str());
            throw std::runtime_error("Undefined variable");
        }

        // 型に応じた処理
        if (var->type == TYPE_FLOAT) {
            float old_value = var->float_value;
            if (node->op == "++") {
                var->float_value += 1.0f;
            } else if (node->op == "--") {
                var->float_value -= 1.0f;
            }
            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return static_cast<int64_t>(var->float_value);
            } else {
                return static_cast<int64_t>(old_value);
            }
        } else if (var->type == TYPE_DOUBLE) {
            double old_value = var->double_value;
            if (node->op == "++") {
                var->double_value += 1.0;
            } else if (node->op == "--") {
                var->double_value -= 1.0;
            }
            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return static_cast<int64_t>(var->double_value);
            } else {
                return static_cast<int64_t>(old_value);
            }
        } else if (var->type == TYPE_QUAD) {
            long double old_value = var->quad_value;
            if (node->op == "++") {
                var->quad_value += 1.0L;
            } else if (node->op == "--") {
                var->quad_value -= 1.0L;
            }
            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return static_cast<int64_t>(var->quad_value);
            } else {
                return static_cast<int64_t>(old_value);
            }
        } else if (var->type == TYPE_POINTER) {
            // ポインタ型のインクリメント/デクリメント
            int64_t old_ptr_value = var->value;

            // メタデータポインタの場合
            if (old_ptr_value & (1LL << 63)) {
                int64_t clean_ptr = old_ptr_value & ~(1LL << 63);
                using namespace PointerSystem;
                PointerMetadata *meta =
                    reinterpret_cast<PointerMetadata *>(clean_ptr);

                if (meta &&
                    meta->target_type == PointerTargetType::ARRAY_ELEMENT) {
                    // 新しいインデックスを計算
                    size_t new_index = meta->element_index;

                    if (node->op == "++") {
                        new_index += 1;
                    } else { // "--"
                        if (new_index == 0) {
                            throw std::runtime_error(
                                "Pointer decrement resulted in negative index");
                        }
                        new_index -= 1;
                    }

                    // 範囲チェック
                    if (new_index >=
                        static_cast<size_t>(meta->array_var->array_size)) {
                        throw std::runtime_error(
                            "Pointer increment/decrement out of array bounds");
                    }

                    // 新しいメタデータを作成
                    PointerMetadata temp_meta =
                        PointerMetadata::create_array_element_pointer(
                            meta->array_var, new_index, meta->element_type,
                            meta->array_name);
                    PointerMetadata *new_meta = new PointerMetadata(temp_meta);

                    // タグ付きポインタ
                    int64_t new_ptr_value = reinterpret_cast<int64_t>(new_meta);
                    new_ptr_value |= (1LL << 63);

                    // 変数を更新
                    var->value = new_ptr_value;

                    // プレフィックスは新しい値、ポストフィックスは古い値を返す
                    if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                        return new_ptr_value;
                    } else {
                        return old_ptr_value;
                    }
                }
            }

            // 従来の方式（Variable*）またはサポートされていないポインタ
            // 単純にポインタ値をインクリメント/デクリメント（警告：安全ではない）
            if (node->op == "++") {
                var->value += 1;
            } else {
                var->value -= 1;
            }

            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return var->value;
            } else {
                return old_ptr_value;
            }
        } else {
            // 整数型
            int64_t old_value = var->value;

            if (node->op == "++") {
                var->value += 1;
            } else if (node->op == "--") {
                var->value -= 1;
            }

            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return var->value;
            } else {
                return old_value;
            }
        }
    }
    // 構造体メンバーアクセスの場合
    else if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // メンバーアクセスからオブジェクト名とメンバー名を取得
        if (!node->left->left ||
            node->left->left->node_type != ASTNodeType::AST_VARIABLE) {
            throw std::runtime_error(
                "Invalid member access in increment/decrement");
        }

        std::string obj_name = node->left->left->name;
        std::string member_name = node->left->name;

        Variable *var = interpreter.find_variable(obj_name);
        if (!var || var->struct_members.empty()) {
            throw std::runtime_error("Undefined struct variable: " + obj_name);
        }

        auto it = var->struct_members.find(member_name);
        if (it == var->struct_members.end()) {
            throw std::runtime_error("Undefined struct member: " + member_name);
        }

        // 型に応じた処理
        if (it->second.type == TYPE_FLOAT) {
            float old_value = it->second.float_value;
            if (node->op == "++") {
                it->second.float_value += 1.0f;
            } else if (node->op == "--") {
                it->second.float_value -= 1.0f;
            }
            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return static_cast<int64_t>(it->second.float_value);
            } else {
                return static_cast<int64_t>(old_value);
            }
        } else if (it->second.type == TYPE_DOUBLE) {
            double old_value = it->second.double_value;
            if (node->op == "++") {
                it->second.double_value += 1.0;
            } else if (node->op == "--") {
                it->second.double_value -= 1.0;
            }
            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return static_cast<int64_t>(it->second.double_value);
            } else {
                return static_cast<int64_t>(old_value);
            }
        } else if (it->second.type == TYPE_QUAD) {
            long double old_value = it->second.quad_value;
            if (node->op == "++") {
                it->second.quad_value += 1.0L;
            } else if (node->op == "--") {
                it->second.quad_value -= 1.0L;
            }
            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return static_cast<int64_t>(it->second.quad_value);
            } else {
                return static_cast<int64_t>(old_value);
            }
        } else {
            // 整数型
            int64_t old_value = it->second.value;

            if (node->op == "++") {
                it->second.value += 1;
            } else if (node->op == "--") {
                it->second.value -= 1;
            }

            if (node->node_type == ASTNodeType::AST_PRE_INCDEC) {
                return it->second.value;
            } else {
                return old_value;
            }
        }
    }
    // 配列要素アクセスの場合
    else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        debug_msg(DebugMsgId::INCDEC_ARRAY_ELEMENT_START);

        // 配列アクセスを評価して配列要素のポインタを取得
        if (!node->left->left ||
            node->left->left->node_type != ASTNodeType::AST_VARIABLE) {
            throw std::runtime_error(
                "Invalid array access in increment/decrement");
        }

        std::string array_name = node->left->left->name;
        debug_msg(DebugMsgId::INCDEC_ARRAY_NAME_FOUND, array_name.c_str());

        Variable *array_var = interpreter.find_variable(array_name);
        if (!array_var) {
            throw std::runtime_error("Undefined array variable: " + array_name);
        }

        // インデックスを評価
        int64_t index = evaluate_expression_func(node->left->array_index.get());
        debug_msg(DebugMsgId::INCDEC_ARRAY_INDEX_EVAL, index);

        // 配列の型は、どのvectorにデータが格納されているかで判定
        bool is_multidim = array_var->is_multidimensional;
        bool has_int =
            (!is_multidim && !array_var->array_values.empty()) ||
            (is_multidim && !array_var->multidim_array_values.empty());
        bool has_float =
            (!is_multidim && !array_var->array_float_values.empty()) ||
            (is_multidim && !array_var->multidim_array_float_values.empty());
        bool has_double =
            (!is_multidim && !array_var->array_double_values.empty()) ||
            (is_multidim && !array_var->multidim_array_double_values.empty());

        debug_msg(DebugMsgId::INCDEC_ELEMENT_TYPE_CHECK, is_multidim, has_int,
                  has_float, has_double);

        // 整数配列の場合
        if (has_int) {
            debug_msg(DebugMsgId::INCDEC_INT_ARRAY_PROCESSING);
            auto &values = is_multidim ? array_var->multidim_array_values
                                       : array_var->array_values;

            if (index < 0 || static_cast<size_t>(index) >= values.size()) {
                throw std::runtime_error("Array index out of bounds");
            }

            int64_t old_value = values[index];
            char old_str[32];
            snprintf(old_str, sizeof(old_str), "%" PRId64, old_value);
            debug_msg(DebugMsgId::INCDEC_OLD_VALUE, old_str);

            if (node->op == "++") {
                values[index] += 1;
            } else if (node->op == "--") {
                values[index] -= 1;
            }

            char new_str[32];
            snprintf(new_str, sizeof(new_str), "%" PRId64, values[index]);
            debug_msg(DebugMsgId::INCDEC_NEW_VALUE, new_str);

            int64_t result = (node->node_type == ASTNodeType::AST_PRE_INCDEC)
                                 ? values[index]
                                 : old_value;
            debug_msg(DebugMsgId::INCDEC_OPERATION_COMPLETE, node->op.c_str(),
                      result);
            return result;
        }
        // float配列の場合
        else if (has_float) {
            debug_msg(DebugMsgId::INCDEC_FLOAT_ARRAY_PROCESSING);
            auto &values = is_multidim ? array_var->multidim_array_float_values
                                       : array_var->array_float_values;

            if (index < 0 || static_cast<size_t>(index) >= values.size()) {
                throw std::runtime_error("Array index out of bounds");
            }

            float old_value = values[index];
            char old_str[32];
            snprintf(old_str, sizeof(old_str), "%f", old_value);
            debug_msg(DebugMsgId::INCDEC_OLD_VALUE, old_str);

            if (node->op == "++") {
                values[index] += 1.0f;
            } else if (node->op == "--") {
                values[index] -= 1.0f;
            }

            char new_str[32];
            snprintf(new_str, sizeof(new_str), "%f", values[index]);
            debug_msg(DebugMsgId::INCDEC_NEW_VALUE, new_str);

            int64_t result = static_cast<int64_t>(
                (node->node_type == ASTNodeType::AST_PRE_INCDEC) ? values[index]
                                                                 : old_value);
            debug_msg(DebugMsgId::INCDEC_OPERATION_COMPLETE, node->op.c_str(),
                      result);
            return result;
        }
        // double配列の場合
        else if (has_double) {
            debug_msg(DebugMsgId::INCDEC_DOUBLE_ARRAY_PROCESSING);
            auto &values = is_multidim ? array_var->multidim_array_double_values
                                       : array_var->array_double_values;

            if (index < 0 || static_cast<size_t>(index) >= values.size()) {
                throw std::runtime_error("Array index out of bounds");
            }

            double old_value = values[index];
            char old_str[32];
            snprintf(old_str, sizeof(old_str), "%f", old_value);
            debug_msg(DebugMsgId::INCDEC_OLD_VALUE, old_str);

            if (node->op == "++") {
                values[index] += 1.0;
            } else if (node->op == "--") {
                values[index] -= 1.0;
            }

            char new_str[32];
            snprintf(new_str, sizeof(new_str), "%f", values[index]);
            debug_msg(DebugMsgId::INCDEC_NEW_VALUE, new_str);

            int64_t result = static_cast<int64_t>(
                (node->node_type == ASTNodeType::AST_PRE_INCDEC) ? values[index]
                                                                 : old_value);
            debug_msg(DebugMsgId::INCDEC_OPERATION_COMPLETE, node->op.c_str(),
                      result);
            return result;
        } else {
            error_msg(DebugMsgId::INCDEC_UNSUPPORTED_TYPE_ERROR);
            throw std::runtime_error(
                "Unsupported array type for increment/decrement");
        }
    } else {
        error_msg(DebugMsgId::DIRECT_ARRAY_ASSIGN_ERROR);
        throw std::runtime_error("Invalid increment/decrement operation");
    }
}

} // namespace IncDecHelpers
