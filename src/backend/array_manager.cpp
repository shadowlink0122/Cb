#include "array_manager.h"
#include "../backend/interpreter.h" // Variable の定義のため
#include "../common/debug_messages.h"
#include "error_handler.h"
#include "variable_manager.h"
#include <stdexcept>

void ArrayManager::processMultidimensionalArrayLiteral(
    Variable &var, const ASTNode *literal_node, TypeInfo elem_type) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error("Invalid array literal node");
    }

    // 空の配列リテラルのチェック
    if (literal_node->arguments.empty()) {
        throw std::runtime_error(
            "Empty array literal for multidimensional array");
    }

    // 多次元配列の次元を検証
    std::vector<int> dimensions;
    dimensions.push_back(literal_node->arguments.size()); // 最外側の次元

    // 最初のサブ配列から内部次元を取得
    if (literal_node->arguments[0]->node_type ==
        ASTNodeType::AST_ARRAY_LITERAL) {
        dimensions.push_back(literal_node->arguments[0]->arguments.size());
    } else {
        throw std::runtime_error(
            "Expected nested array literal for multidimensional array");
    }

    // 宣言された次元と一致するかチェック
    if (dimensions.size() != var.array_type_info.dimensions.size()) {
        debug_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                  ("Dimension mismatch: literal=" +
                   std::to_string(dimensions.size()) + ", declared=" +
                   std::to_string(var.array_type_info.dimensions.size()))
                      .c_str());
        throw std::runtime_error(
            "Array literal dimensions don't match declaration");
    }

    for (size_t i = 0; i < dimensions.size(); ++i) {
        if (dimensions[i] != var.array_type_info.dimensions[i].size) {
            debug_msg(DebugMsgId::TYPE_MISMATCH_ERROR,
                      ("Size mismatch at dimension " + std::to_string(i) +
                       ": literal=" + std::to_string(dimensions[i]) +
                       ", declared=" +
                       std::to_string(var.array_type_info.dimensions[i].size))
                          .c_str());
            throw std::runtime_error(
                "Array literal size doesn't match declaration");
        }
    }

    // 総要素数を計算
    std::vector<int> dim_sizes;
    for (const auto &dim : var.array_type_info.dimensions) {
        dim_sizes.push_back(dim.size);
    }
    int total_size = calculateTotalSize(dim_sizes);

    // データ配列を初期化
    if (elem_type == TYPE_STRING) {
        var.multidim_array_strings.resize(total_size);
    } else {
        var.multidim_array_values.resize(total_size);
    }

    // 配列リテラルの値を設定
    std::vector<int> current_indices;
    processArrayLiteralRecursive(var, literal_node, elem_type, 0,
                                 current_indices);
}

void ArrayManager::processNDimensionalArrayLiteral(Variable &var,
                                                   const ASTNode *literal_node,
                                                   TypeInfo base_type) {
    if (!literal_node ||
        literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        throw std::runtime_error(
            "Invalid array literal for N-dimensional array");
    }

    // 空配列の場合
    if (literal_node->arguments.empty()) {
        throw std::runtime_error(
            "Empty array literal not allowed for N-dimensional arrays");
    }

    // リテラルから次元情報を推定
    std::vector<int> inferred_dimensions;
    const ASTNode *current = literal_node;

    while (current && current->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        inferred_dimensions.push_back(current->arguments.size());
        if (!current->arguments.empty() && current->arguments[0]->node_type ==
                                               ASTNodeType::AST_ARRAY_LITERAL) {
            current = current->arguments[0].get();
        } else {
            break;
        }
    }

    // 宣言された次元との照合
    validateArrayDimensions(var.array_dimensions, inferred_dimensions);

    // 総要素数計算と初期化
    int total_size = calculateTotalSize(inferred_dimensions);
    var.array_dimensions = inferred_dimensions;

    if (base_type == TYPE_STRING) {
        var.multidim_array_strings.resize(total_size);
    } else {
        var.multidim_array_values.resize(total_size);
    }

    // データを再帰的に処理
    std::vector<int> current_indices;
    processArrayLiteralRecursive(var, literal_node, base_type, 0,
                                 current_indices);
}

void ArrayManager::processArrayLiteralRecursive(
    Variable &var, const ASTNode *node, TypeInfo base_type, int current_dim,
    std::vector<int> &current_indices) {
    if (!node)
        return;

    if (node->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 現在の次元のインデックスをセット
        for (int i = 0; i < static_cast<int>(node->arguments.size()); ++i) {
            current_indices.resize(current_dim + 1);
            current_indices[current_dim] = i;

            // 再帰的に処理
            processArrayLiteralRecursive(var, node->arguments[i].get(),
                                         base_type, current_dim + 1,
                                         current_indices);
        }
    } else {
        // 葉ノード（実際の値）
        int flat_index = var.calculate_flat_index(current_indices);

        if (base_type == TYPE_STRING) {
            var.multidim_array_strings[flat_index] = node->str_value;
        } else {
            var.multidim_array_values[flat_index] = node->int_value;
        }
    }
}

int64_t ArrayManager::getMultidimensionalArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type == TYPE_STRING) {
        // 文字列配列の場合、文字列を数値として扱うのは適切でない
        throw std::runtime_error("Cannot get string array element as integer");
    }

    return var.multidim_array_values[flat_index];
}

void ArrayManager::setMultidimensionalArrayElement(
    Variable &var, const std::vector<int64_t> &indices, int64_t value) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    // const配列への書き込みチェック
    if (var.is_const) {
        throw std::runtime_error(
            "Cannot assign to const multidimensional array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type == TYPE_STRING) {
        throw std::runtime_error(
            "Cannot set string array element with integer value");
    }

    var.multidim_array_values[flat_index] = value;
}

std::string ArrayManager::getMultidimensionalStringArrayElement(
    const Variable &var, const std::vector<int64_t> &indices) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type != TYPE_STRING) {
        throw std::runtime_error(
            "Cannot get non-string array element as string");
    }

    return var.multidim_array_strings[flat_index];
}

void ArrayManager::setMultidimensionalStringArrayElement(
    Variable &var, const std::vector<int64_t> &indices,
    const std::string &value) {
    if (!var.is_multidimensional) {
        throw std::runtime_error("Variable is not a multidimensional array");
    }

    // const配列への書き込みチェック
    if (var.is_const) {
        throw std::runtime_error(
            "Cannot assign to const multidimensional string array");
    }

    std::vector<int> int_indices;
    for (int64_t idx : indices) {
        int_indices.push_back(static_cast<int>(idx));
    }

    int flat_index = var.calculate_flat_index(int_indices);

    if (var.array_type_info.base_type != TYPE_STRING) {
        throw std::runtime_error(
            "Cannot set non-string array element with string value");
    }

    var.multidim_array_strings[flat_index] = value;
}

void ArrayManager::initializeArray(Variable &var, TypeInfo base_type,
                                   const std::vector<int> &dimensions) {
    var.is_array = true;
    var.array_dimensions = dimensions;

    if (dimensions.size() > 1) {
        var.is_multidimensional = true;
        // ArrayTypeInfoを設定
        var.array_type_info.base_type = base_type;
        var.array_type_info.dimensions.clear();
        for (int dim : dimensions) {
            ArrayDimension array_dim;
            array_dim.size = dim;
            var.array_type_info.dimensions.push_back(array_dim);
        }
    }

    int total_size = calculateTotalSize(dimensions);

    if (base_type == TYPE_STRING) {
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
}

void ArrayManager::initializeMultidimensionalArray(
    Variable &var, const ArrayTypeInfo &array_info) {
    var.is_array = true;
    var.is_multidimensional = true;
    var.array_type_info = array_info;

    // 次元サイズをコピー
    var.array_dimensions.clear();
    for (const auto &dim : array_info.dimensions) {
        var.array_dimensions.push_back(dim.size);
    }

    int total_size = calculateTotalSize(var.array_dimensions);

    if (array_info.base_type == TYPE_STRING) {
        var.multidim_array_strings.resize(total_size, "");
    } else {
        var.multidim_array_values.resize(total_size, 0);
    }
}

int ArrayManager::calculateTotalSize(const std::vector<int> &dimensions) {
    int total = 1;
    for (int dim : dimensions) {
        total *= dim;
    }
    return total;
}

std::vector<int> ArrayManager::extractDimensionSizes(
    const std::vector<ArrayDimension> &dimensions) {
    std::vector<int> sizes;
    for (const auto &dim : dimensions) {
        sizes.push_back(dim.size);
    }
    return sizes;
}

void ArrayManager::validateArrayDimensions(const std::vector<int> &expected,
                                           const std::vector<int> &actual) {
    if (expected.size() != actual.size()) {
        throw std::runtime_error("Array dimension count mismatch");
    }

    for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] != actual[i]) {
            throw std::runtime_error(
                "Array dimension size mismatch at dimension " +
                std::to_string(i));
        }
    }
}

void ArrayManager::declare_array(const ASTNode *node) {
    Variable var;

    debug_msg(DebugMsgId::ARRAY_DECL_START, node->name.c_str());

    // 多次元配列かどうかを確認
    if (node->array_type_info.dimensions.size() > 1) {
        debug_msg(DebugMsgId::MULTIDIM_ARRAY_DECL_INFO,
                  static_cast<int>(node->array_type_info.dimensions.size()));

        // 多次元配列の場合
        var.is_array = true;
        var.is_multidimensional = true;
        var.array_type_info = node->array_type_info;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                         node->array_type_info.base_type);
        var.is_const = node->is_const;
        var.is_assigned = false;

        // 全次元のサイズを計算して平坦化された配列を作成
        int total_size = 1;
        var.array_dimensions.clear();
        for (const ArrayDimension &dim : node->array_type_info.dimensions) {
            total_size *= dim.size;
            var.array_dimensions.push_back(dim.size);
        }

        var.array_size = total_size; // array_sizeを設定

        debug_msg(DebugMsgId::ARRAY_TOTAL_SIZE, total_size);

        // 多次元配列用のストレージを初期化
        if (node->array_type_info.base_type == TYPE_STRING) {
            var.multidim_array_strings.resize(total_size, "");
        } else {
            var.multidim_array_values.resize(total_size, 0);
        }

        // グローバルスコープに保存（AST_ARRAY_DECLはグローバル配列宣言のみ）
        variable_manager_->getInterpreter()
            ->global_scope.variables[node->name] = var;
        debug_msg(DebugMsgId::MULTIDIM_ARRAY_DECL_SUCCESS, node->name.c_str());
    } else {
        // 単一次元配列の場合
        debug_msg(DebugMsgId::ARRAY_DECL_DEBUG);

        var.is_array = true;
        var.is_multidimensional = false;
        var.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                         node->array_type_info.base_type);
        var.is_const = node->is_const;
        var.is_assigned = false;

        int size = node->array_type_info.dimensions[0].size;
        var.array_size = size; // array_sizeを設定
        debug_msg(DebugMsgId::ARRAY_TOTAL_SIZE, size);

        // 単一次元配列の場合もarray_dimensionsを設定
        var.array_dimensions.clear();
        var.array_dimensions.push_back(size);

        // 単一次元配列用のストレージを初期化
        if (node->array_type_info.base_type == TYPE_STRING) {
            var.array_strings.resize(size, "");
        } else {
            var.array_values.resize(size, 0);
        }

        // グローバルスコープに保存（AST_ARRAY_DECLはグローバル配列宣言のみ）
        variable_manager_->getInterpreter()
            ->global_scope.variables[node->name] = var;
        debug_msg(DebugMsgId::ARRAY_DECL_SUCCESS, node->name.c_str());
    }
}

// 配列コピー機能の実装
void ArrayManager::copyArray(Variable &dest, const Variable &src) {
    if (!isCompatibleArrayType(dest, src)) {
        throw std::runtime_error("Incompatible array types for copy operation");
    }

    // 型情報をコピー
    dest.type = src.type;
    dest.is_array = src.is_array;
    dest.is_multidimensional = src.is_multidimensional;
    dest.array_dimensions = src.array_dimensions;
    dest.array_size = src.array_size;
    dest.array_type_info = src.array_type_info;

    // データをコピー
    if (src.is_multidimensional) {
        if (src.array_type_info.base_type == TYPE_STRING) {
            dest.multidim_array_strings = src.multidim_array_strings;
        } else {
            dest.multidim_array_values = src.multidim_array_values;
        }
    } else {
        if (static_cast<int>(src.type) == TYPE_ARRAY_BASE + TYPE_STRING) {
            dest.array_strings = src.array_strings;
        } else {
            dest.array_values = src.array_values;
        }
    }

    dest.is_assigned = true;
}

void ArrayManager::copyArraySlice(Variable &dest, const Variable &src,
                                  const std::vector<int64_t> &slice_indices) {
    if (!src.is_multidimensional ||
        slice_indices.size() >= src.array_dimensions.size()) {
        throw std::runtime_error("Invalid array slice operation");
    }

    // スライスのサイズを計算
    std::vector<int> slice_dimensions;
    for (size_t i = slice_indices.size(); i < src.array_dimensions.size();
         i++) {
        slice_dimensions.push_back(src.array_dimensions[i]);
    }

    // 目的配列が単一次元の場合
    if (slice_dimensions.size() == 1) {
        dest.is_array = true;
        dest.is_multidimensional = false;
        dest.array_size = slice_dimensions[0];
        dest.array_dimensions = slice_dimensions;
        dest.type = static_cast<TypeInfo>(TYPE_ARRAY_BASE +
                                          src.array_type_info.base_type);

        // データをコピー
        if (src.array_type_info.base_type == TYPE_STRING) {
            dest.array_strings.resize(slice_dimensions[0]);
            for (int i = 0; i < slice_dimensions[0]; i++) {
                std::vector<int64_t> full_indices = slice_indices;
                full_indices.push_back(i);
                dest.array_strings[i] =
                    getMultidimensionalStringArrayElement(src, full_indices);
            }
        } else {
            dest.array_values.resize(slice_dimensions[0]);
            for (int i = 0; i < slice_dimensions[0]; i++) {
                std::vector<int64_t> full_indices = slice_indices;
                full_indices.push_back(i);
                dest.array_values[i] =
                    getMultidimensionalArrayElement(src, full_indices);
            }
        }
    } else {
        // 多次元スライス（将来の拡張用）
        throw std::runtime_error(
            "Multi-dimensional array slicing not yet supported");
    }

    dest.is_assigned = true;
}

bool ArrayManager::isCompatibleArrayType(const Variable &dest,
                                         const Variable &src) {
    // 基本的な配列型チェック
    if (!dest.is_array || !src.is_array) {
        return false;
    }

    // 配列スライスの場合は次元数チェックを緩和
    // 例: int[3] = int[3][3][0] のような場合を許可

    // 基本型チェック
    TypeInfo dest_base =
        dest.is_multidimensional
            ? dest.array_type_info.base_type
            : static_cast<TypeInfo>(dest.type - TYPE_ARRAY_BASE);
    TypeInfo src_base = src.is_multidimensional
                            ? src.array_type_info.base_type
                            : static_cast<TypeInfo>(src.type - TYPE_ARRAY_BASE);

    return dest_base == src_base;
}
