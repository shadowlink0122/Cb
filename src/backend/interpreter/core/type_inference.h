#pragma once
#include "../../../common/ast.h"
#include <string>

// 型推論の結果を表現する構造体
struct InferredType {
    TypeInfo type_info;
    std::string type_name;
    bool is_array;
    int array_dimensions;
    
    InferredType(TypeInfo info = TYPE_UNKNOWN, const std::string& name = "", bool array = false, int dims = 0) 
        : type_info(info), type_name(name), is_array(array), array_dimensions(dims) {}
    
    bool is_compatible_with(const InferredType& other) const {
        if (type_info == other.type_info && type_name == other.type_name) {
            return is_array == other.is_array && array_dimensions == other.array_dimensions;
        }
        return false;
    }
    
    std::string to_string() const {
        std::string result = type_name.empty() ? std::to_string(static_cast<int>(type_info)) : type_name;
        if (is_array) {
            result += "[" + std::string(array_dimensions, ']');
        }
        return result;
    }
};

// 式評価の結果（値 + 型情報）
struct TypedValue {
    int64_t numeric_value;
    std::string string_value;
    bool is_numeric_result;
    InferredType type;
    
    TypedValue(int64_t val, const InferredType& t) 
        : numeric_value(val), string_value(""), is_numeric_result(true), type(t) {}
    
    TypedValue(const std::string& val, const InferredType& t) 
        : numeric_value(0), string_value(val), is_numeric_result(false), type(t) {}
    
    bool is_numeric() const { return is_numeric_result; }
    bool is_string() const { return !is_numeric_result; }
    
    int64_t as_numeric() const { 
        return is_numeric() ? numeric_value : 0; 
    }
    
    std::string as_string() const { 
        if (is_string()) {
            return string_value;
        } else {
            return std::to_string(numeric_value);
        }
    }
};

// 前方宣言
class Interpreter;

// 型推論エンジン
class TypeInferenceEngine {
public:
    TypeInferenceEngine(Interpreter& interpreter);
    
    // ASTノードから型を推論
    InferredType infer_type(const ASTNode* node);
    
    // 三項演算子の型推論
    InferredType infer_ternary_type(const ASTNode* condition, const ASTNode* true_expr, const ASTNode* false_expr);
    
    // 関数呼び出しの戻り値型推論
    InferredType infer_function_return_type(const std::string& func_name, const std::vector<InferredType>& arg_types);
    
    // メンバアクセスの型推論
    InferredType infer_member_type(const InferredType& object_type, const std::string& member_name);
    
    // 配列アクセスの型推論
    InferredType infer_array_element_type(const InferredType& array_type);
    
private:
    Interpreter& interpreter_;
    
    // ヘルパー関数
    InferredType get_common_type(const InferredType& type1, const InferredType& type2);
    InferredType literal_to_type(const ASTNode* node);
};
