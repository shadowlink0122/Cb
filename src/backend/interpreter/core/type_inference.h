#pragma once
#include "../../../common/ast.h"
#include <memory>
#include <string>
#include <vector>

// 前方宣言
struct Variable;

// 型推論の結果を表現する構造体
struct InferredType {
    TypeInfo type_info;
    std::string type_name;
    bool is_array;
    int array_dimensions;

    InferredType(TypeInfo info = TYPE_UNKNOWN, const std::string &name = "",
                 bool array = false, int dims = 0)
        : type_info(info), type_name(name), is_array(array),
          array_dimensions(dims) {}

    bool is_compatible_with(const InferredType &other) const {
        if (type_info == other.type_info && type_name == other.type_name) {
            return is_array == other.is_array &&
                   array_dimensions == other.array_dimensions;
        }
        return false;
    }

    std::string to_string() const {
        std::string result = type_name.empty()
                                 ? std::to_string(static_cast<int>(type_info))
                                 : type_name;
        if (is_array) {
            result += "[" + std::string(array_dimensions, ']');
        }
        return result;
    }
};

// 式評価の結果（値 + 型情報 + ASTノード参照）
struct TypedValue {
    int64_t value;          // 整数表現
    double double_value;    // 浮動小数点（二倍精度）表現
    long double quad_value; // 128bit相当の拡張精度
    std::string string_value;
    bool is_numeric_result;
    bool is_float_result;
    TypeInfo numeric_type; // 数値型（int/float/double/quad等）
    InferredType type;

    // 複雑な型（配列、構造体など）の場合、実際の評価を遅延するためのASTノード参照
    const ASTNode *deferred_node;
    bool is_deferred;

    // 構造体結果用フィールド（関数戻り値処理用）
    bool is_struct_result;
    std::shared_ptr<Variable> struct_data; // 構造体データを共有ポインタで保持

    // 関数ポインタ結果用フィールド
    bool is_function_pointer;
    std::string function_pointer_name;
    const ASTNode *function_pointer_node;

    TypedValue(int64_t val, const InferredType &t)
        : value(val), double_value(static_cast<double>(val)),
          quad_value(static_cast<long double>(val)), string_value(""),
          is_numeric_result(true), is_float_result(false),
          numeric_type(t.type_info), type(t), deferred_node(nullptr),
          is_deferred(false), is_struct_result(false), struct_data(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr) {
        // デバッグ: ポインタ型の場合のみ出力
        extern bool debug_mode;
        if (debug_mode && t.type_info == TYPE_POINTER) {
            fprintf(stderr,
                    "[TypedValue int64_t constructor] value=%lld (0x%llx), "
                    "type=POINTER\n",
                    (long long)val, (unsigned long long)val);
        }
    }

    TypedValue(double val, const InferredType &t)
        : value(static_cast<int64_t>(val)), double_value(val),
          quad_value(static_cast<long double>(val)), string_value(""),
          is_numeric_result(true), is_float_result(true),
          numeric_type(t.type_info), type(t), deferred_node(nullptr),
          is_deferred(false), is_struct_result(false), struct_data(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr) {}

    TypedValue(long double val, const InferredType &t)
        : value((t.type_info == TYPE_POINTER)
                    ? *reinterpret_cast<const int64_t *>(
                          &val) // ポインタの場合はビット再解釈
                    : static_cast<int64_t>(val)), // それ以外は通常のキャスト
          double_value(static_cast<double>(val)), quad_value(val),
          string_value(""), is_numeric_result(true),
          is_float_result((t.type_info != TYPE_POINTER)),
          numeric_type(t.type_info), type(t), deferred_node(nullptr),
          is_deferred(false), is_struct_result(false), struct_data(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr) {
        // デバッグ: ポインタ型の場合のみ出力
        extern bool debug_mode;
        if (debug_mode && t.type_info == TYPE_POINTER) {
            fprintf(stderr,
                    "[TypedValue long double constructor] val=%Lf, "
                    "reinterpreted value=%lld (0x%llx)\n",
                    val, (long long)value, (unsigned long long)value);
        }
    }

    TypedValue(const std::string &val, const InferredType &t)
        : value(0), double_value(0.0), quad_value(0.0L), string_value(val),
          is_numeric_result(false), is_float_result(false),
          numeric_type(TYPE_UNKNOWN), type(t), deferred_node(nullptr),
          is_deferred(false), is_struct_result(false), struct_data(nullptr),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr) {}

    // 構造体用コンストラクタ
    TypedValue(const Variable &struct_var, const InferredType &t)
        : value(0), string_value(""), is_numeric_result(false), type(t),
          deferred_node(nullptr), is_deferred(false), is_struct_result(true),
          struct_data(std::make_shared<Variable>(struct_var)),
          is_function_pointer(false), function_pointer_name(""),
          function_pointer_node(nullptr) {}

    // 遅延評価用コンストラクタ
    static TypedValue deferred(const ASTNode *node, const InferredType &t) {
        TypedValue result(static_cast<int64_t>(0), t);
        result.double_value = 0.0;
        result.quad_value = 0.0L;
        result.numeric_type = t.type_info;
        result.deferred_node = node;
        result.is_deferred = true;
        return result;
    }

    // 関数ポインタ用静的ファクトリーメソッド
    static TypedValue function_pointer(int64_t val,
                                       const std::string &func_name,
                                       const ASTNode *func_node,
                                       const InferredType &t) {
        TypedValue result(val, t);
        result.is_function_pointer = true;
        result.function_pointer_name = func_name;
        result.function_pointer_node = func_node;
        return result;
    }

    bool is_numeric() const {
        return is_numeric_result && !is_deferred && !is_struct_result;
    }
    bool is_floating() const { return is_numeric() && is_float_result; }
    bool is_string() const {
        return !is_numeric_result && !is_deferred && !is_struct_result;
    }
    bool is_struct() const {
        return is_struct_result && struct_data != nullptr;
    }
    bool needs_deferred_evaluation() const { return is_deferred; }

    int64_t as_numeric() const {
        if (!is_numeric() || is_deferred) {
            return 0;
        }
        if (is_float_result) {
            return static_cast<int64_t>(double_value);
        }
        // デバッグ: ポインタ型の場合のみ出力
        extern bool debug_mode;
        if (debug_mode && numeric_type == TYPE_POINTER) {
            fprintf(stderr,
                    "[TypedValue::as_numeric] Returning pointer value=%lld "
                    "(0x%llx)\n",
                    (long long)value, (unsigned long long)value);
        }
        return value;
    }

    double as_double() const {
        if (!is_numeric() || is_deferred) {
            return 0.0;
        }
        return is_float_result ? double_value : static_cast<double>(value);
    }

    long double as_quad() const {
        if (!is_numeric() || is_deferred) {
            return 0.0L;
        }
        return is_float_result ? quad_value : static_cast<long double>(value);
    }

    std::string as_string() const {
        if (is_string() && !is_deferred) {
            return string_value;
        } else if (is_numeric() && !is_deferred) {
            if (is_float_result) {
                return std::to_string(double_value);
            }
            return std::to_string(value);
        }
        return "";
    }
};

// 前方宣言
class Interpreter;

// 型推論エンジン
class TypeInferenceEngine {
  public:
    TypeInferenceEngine(Interpreter &interpreter);

    // ASTノードから型を推論
    InferredType infer_type(const ASTNode *node);

    // 三項演算子の型推論
    InferredType infer_ternary_type(const ASTNode *condition,
                                    const ASTNode *true_expr,
                                    const ASTNode *false_expr);

    // 関数呼び出しの戻り値型推論
    InferredType
    infer_function_return_type(const std::string &func_name,
                               const std::vector<InferredType> &arg_types);

    // メンバアクセスの型推論
    InferredType infer_member_type(const InferredType &object_type,
                                   const std::string &member_name);

    // 配列アクセスの型推論
    InferredType infer_array_element_type(const InferredType &array_type);

    // typedef型の再帰的解決
    InferredType resolve_typedef_type(const std::string &typedef_name);

    // 型エラーチェック（チェーン処理用）
    bool
    validate_chain_compatibility(const InferredType &object_type,
                                 const std::string &method_name,
                                 const std::vector<InferredType> &arg_types);

  private:
    Interpreter &interpreter_;

    // ヘルパー関数
    InferredType get_common_type(const InferredType &type1,
                                 const InferredType &type2);
    InferredType literal_to_type(const ASTNode *node);

    // 型定義検索ヘルパー
    const ASTNode *find_struct_definition(const std::string &struct_name);
    const ASTNode *find_union_definition(const std::string &union_name);
    const ASTNode *find_typedef_definition(const std::string &typedef_name);
};
