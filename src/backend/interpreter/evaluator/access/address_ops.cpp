#include "address_ops.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../core/interpreter.h"
#include "../../core/pointer_metadata.h"
#include "recursive_member_evaluator.h"
#include <iostream>
#include <stdexcept>

using namespace PointerSystem;

namespace AddressOperationHelpers {

// ========================================================================
// アドレス演算子 (&) の評価
// ========================================================================
int64_t evaluate_address_of(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func) {
    bool debug_mode = interpreter.is_debug_mode();

    // デバッグ情報
    if (debug_mode) {
        std::cerr << "[ADDRESS_OF] is_function_address="
                  << node->is_function_address
                  << ", function_address_name=" << node->function_address_name
                  << ", has_left=" << (node->left != nullptr) << std::endl;
    }

    // 関数アドレスの特別処理
    // is_function_addressがtrueで関数名がある場合、まず関数として検索
    // ただし、leftがAST_ARRAY_REFの場合は配列要素なので除外
    bool is_array_element =
        node->left && node->left->node_type == ASTNodeType::AST_ARRAY_REF;

    if (node->is_function_address && !node->function_address_name.empty() &&
        !is_array_element) {
        if (debug_mode) {
            std::cerr << "[ADDRESS_OF] Looking for function: "
                      << node->function_address_name << std::endl;
        }

        const ASTNode *func_node =
            interpreter.find_function(node->function_address_name);

        if (debug_mode) {
            std::cerr << "[ADDRESS_OF] Function found: "
                      << (func_node ? "YES" : "NO") << std::endl;
        }

        // 関数が見つかった場合のみ関数ポインタとして処理
        if (func_node) {
            // 関数ポインタ値として関数ノードの実際のメモリアドレスを返す
            // 関数も変数と同じくメモリ上に存在するという概念を反映
            int64_t func_address = reinterpret_cast<int64_t>(func_node);

            if (debug_mode) {
                std::cerr << "[FUNC_PTR] Taking address of function: "
                          << node->function_address_name << " -> 0x" << std::hex
                          << func_address << std::dec << std::endl;
            }

            return func_address;
        }
        // 関数が見つからない場合は変数として処理
        if (debug_mode) {
            std::cerr
                << "[ADDRESS_OF] Not a function, treating as variable address: "
                << node->function_address_name << std::endl;
        }

        // function_address_nameを使って変数を検索
        Variable *var = interpreter.find_variable(node->function_address_name);
        if (!var) {
            error_msg(DebugMsgId::UNDEFINED_VAR_ERROR,
                      node->function_address_name.c_str());
            throw std::runtime_error("Undefined variable");
        }
        return reinterpret_cast<int64_t>(var);
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

        // 変数のアドレスを返す（従来の方式: Variable*をint64_tとして返す）
        // メタデータは不要（変数ポインタは後方互換性のため従来の方式を維持）
        return reinterpret_cast<int64_t>(var);
    }
    // 配列要素のアドレス取得: &arr[index]
    else if (node->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        std::string array_name =
            interpreter.extract_array_name(node->left.get());
        std::vector<int64_t> indices =
            interpreter.extract_array_indices(node->left.get());

        if (array_name.empty() || indices.empty()) {
            throw std::runtime_error(
                "Invalid array reference in address-of operator");
        }

        Variable *array_var = interpreter.find_variable(array_name);
        if (!array_var) {
            throw std::runtime_error("Undefined array: " + array_name);
        }

        // 配列参照の場合、元の配列を取得
        // これにより、関数から戻った後もポインタが有効になる
        if (array_var->is_reference && array_var->is_array) {
            array_var = reinterpret_cast<Variable *>(array_var->value);
            if (!array_var) {
                throw std::runtime_error(
                    "Invalid array reference in address-of");
            }
        }

        // 配列要素のアドレス取得（1次元・多次元両対応）
        // 要素の型を判定
        TypeInfo elem_type = TYPE_INT; // デフォルト
        if (array_var->type >= TYPE_ARRAY_BASE) {
            elem_type =
                static_cast<TypeInfo>(array_var->type - TYPE_ARRAY_BASE);
        }

        // フラットインデックスを計算
        size_t flat_index;
        if (array_var->is_multidimensional && indices.size() > 1) {
            // 多次元配列の場合: calculate_flat_index()を使用
            std::vector<int> int_indices(indices.begin(), indices.end());
            flat_index = static_cast<size_t>(
                array_var->calculate_flat_index(int_indices));

            if (debug_mode) {
                std::cerr << "[ADDRESS_OF] Multi-dimensional array access:"
                          << std::endl;
                std::cerr << "  Indices: [";
                for (size_t i = 0; i < indices.size(); ++i) {
                    std::cerr << indices[i];
                    if (i < indices.size() - 1)
                        std::cerr << ", ";
                }
                std::cerr << "]" << std::endl;
                std::cerr << "  Flat index: " << flat_index << std::endl;
            }
        } else {
            // 1次元配列の場合: 従来の処理
            int64_t index = indices[0];
            if (index < 0 || index >= array_var->array_size) {
                throw std::runtime_error(
                    "Array index out of bounds in address-of");
            }
            flat_index = static_cast<size_t>(index);
        }

        // 構造体/インターフェース配列の場合は、要素変数への直接ポインタを返す
        bool is_struct_like_element =
            (array_var->is_struct && !array_var->struct_type_name.empty()) ||
            elem_type == TYPE_STRUCT || elem_type == TYPE_INTERFACE;

        if (is_struct_like_element) {
            std::string element_name =
                interpreter.extract_array_element_name(node->left.get());
            Variable *element_var = interpreter.find_variable(element_name);

            if (!element_var && array_var->is_struct &&
                !array_var->struct_type_name.empty()) {
                interpreter.create_struct_variable(element_name,
                                                   array_var->struct_type_name);
                element_var = interpreter.find_variable(element_name);
            }

            if (!element_var) {
                throw std::runtime_error("Struct array element not found: " +
                                         element_name);
            }

            if (debug_mode) {
                std::cerr << "[ADDRESS_OF] Returning struct element pointer: "
                          << element_name << " -> " << element_var << std::endl;
            }

            return reinterpret_cast<int64_t>(element_var);
        }

        // メタデータを作成してヒープに配置
        PointerMetadata *meta = new PointerMetadata();
        *meta = PointerMetadata::create_array_element_pointer(
            array_var, flat_index, elem_type);

        if (debug_mode) {
            std::cerr << "[POINTER_METADATA] Created array element pointer: "
                      << meta->to_string() << std::endl;
            std::cerr << "[ADDRESS_OF] meta address="
                      << static_cast<void *>(meta) << std::endl;
            std::cerr << "[ADDRESS_OF] meta->target_type="
                      << static_cast<int>(meta->target_type) << std::endl;
            std::cerr << "[ADDRESS_OF] meta->array_var="
                      << static_cast<void *>(meta->array_var) << std::endl;
            std::cerr << "[ADDRESS_OF] meta->element_index="
                      << meta->element_index << std::endl;
        }

        // メタデータのアドレスをint64_tとして返す
        // 注意: 最上位ビットを1にしてメタデータポインタであることを示す
        int64_t ptr_value = reinterpret_cast<int64_t>(meta);
        // タグを設定（最上位ビット）
        ptr_value |= (1LL << 63);

        if (debug_mode) {
            std::cerr << "[ADDRESS_OF] Returning ptr_value=" << ptr_value
                      << " (0x" << std::hex << ptr_value << std::dec << ")"
                      << std::endl;
        }

        return ptr_value;
    }
    // 構造体メンバーのアドレス取得: &obj.member または
    // &container.shapes[0].edges[0].start.x
    else if (node->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
        // 深いネスト対応: 再帰的リゾルバを使用
        Variable *member_var = nullptr;
        std::string member_path;

        // leftがMEMBER_ACCESSまたはARRAY_REFを含む場合、再帰リゾルバを使用
        if (node->left->left &&
            (node->left->left->node_type == ASTNodeType::AST_MEMBER_ACCESS ||
             node->left->left->node_type == ASTNodeType::AST_ARRAY_REF)) {

            // 再帰的に解決
            auto evaluate_index = [&evaluate_expression_func](
                                      const ASTNode *idx_node) -> int64_t {
                return evaluate_expression_func(idx_node);
            };

            member_var =
                MemberEvaluationHelpers::resolve_nested_member_for_evaluation(
                    interpreter, node->left.get(), evaluate_index);

            // member_pathを再構築（デバッグ用）
            member_path = node->left->name + " (nested)";

        } else {
            // 単純な obj.member の場合: 従来の方法
            std::string obj_name = node->left->left->name;
            std::string member_name = node->left->name;
            member_path = obj_name + "." + member_name;
            member_var = interpreter.find_variable(member_path);
        }

        if (!member_var) {
            throw std::runtime_error("Undefined member: " + member_path);
        }

        if (debug_mode) {
            std::cerr << "[ADDRESS_OF] member_var found: " << member_var
                      << ", is_assigned=" << member_var->is_assigned
                      << ", value=" << member_var->value << std::endl;
        }

        // 構造体メンバーへのポインタもメタデータで表現
        PointerMetadata *meta = new PointerMetadata();
        *meta = PointerMetadata::create_struct_member_pointer(member_var,
                                                              member_path);

        if (debug_mode) {
            std::cerr << "[POINTER_METADATA] Created struct member pointer: "
                      << meta->to_string() << std::endl;
            std::cerr << "[ADDRESS_OF] meta->member_var = " << meta->member_var
                      << std::endl;
        }

        // メタデータのアドレスをタグ付きで返す
        int64_t ptr_value = reinterpret_cast<int64_t>(meta);
        ptr_value |= (1LL << 63);
        return ptr_value;
    } else {
        throw std::runtime_error("Address-of operator requires a variable, "
                                 "array element, or struct member");
    }
}

// ========================================================================
// 間接参照演算子 (*) の評価
// ========================================================================
int64_t evaluate_dereference(
    const ASTNode *node, Interpreter &interpreter,
    std::function<int64_t(const ASTNode *)> evaluate_expression_func) {
    bool debug_mode = interpreter.is_debug_mode();

    int64_t ptr_value = evaluate_expression_func(node->left.get());
    if (ptr_value == 0) {
        throw std::runtime_error("Null pointer dereference");
    }

    // ポインタがメタデータを持つかチェック（最上位ビット）
    if (ptr_value & (1LL << 63)) {
        // メタデータポインタの場合
        int64_t clean_ptr = ptr_value & ~(1LL << 63); // タグを除去

        if (debug_mode) {
            std::cerr << "[DEREFERENCE] ptr_value=" << ptr_value << std::endl;
            std::cerr << "[DEREFERENCE] clean_ptr=" << clean_ptr << " (0x"
                      << std::hex << clean_ptr << std::dec << ")" << std::endl;
        }

        PointerMetadata *meta = reinterpret_cast<PointerMetadata *>(clean_ptr);

        if (!meta) {
            throw std::runtime_error("Invalid pointer metadata");
        }

        if (debug_mode) {
            std::cerr << "[DEREFERENCE] meta address="
                      << static_cast<void *>(meta) << std::endl;
            std::cerr << "[DEREFERENCE] meta->target_type="
                      << static_cast<int>(meta->target_type) << std::endl;
            std::cerr << "[DEREFERENCE] meta->array_var="
                      << static_cast<void *>(meta->array_var) << std::endl;
            std::cerr << "[DEREFERENCE] meta->element_index="
                      << meta->element_index << std::endl;
            std::cerr << "[POINTER_METADATA] Dereferencing: "
                      << meta->to_string() << std::endl;
        }

        // メタデータから値を読み取り
        return meta->read_int_value();
    } else {
        // 従来の方式（変数ポインタまたは構造体ポインタ）
        Variable *var = reinterpret_cast<Variable *>(ptr_value);

        // If the pointer points to a struct, return the struct pointer itself
        // so that subsequent member access (*ptr).member can work
        if (var && (var->type == TYPE_STRUCT || var->is_struct ||
                    !var->struct_members.empty())) {
            // For struct pointers, return the pointer to the struct variable
            // This allows (*struct_ptr).member patterns to work
            return ptr_value; // Return the struct pointer itself, not its value
        }

        // For primitive types, return the value
        return var->value;
    }
}

} // namespace AddressOperationHelpers
