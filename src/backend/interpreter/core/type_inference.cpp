#include "type_inference.h"
#include "interpreter.h"
#include "../managers/variable_manager.h"
#include "../managers/type_manager.h"

TypeInferenceEngine::TypeInferenceEngine(Interpreter& interpreter) 
    : interpreter_(interpreter) {
}

InferredType TypeInferenceEngine::infer_type(const ASTNode* node) {
    if (!node) return InferredType();
    
    switch (node->node_type) {
        case ASTNodeType::AST_NUMBER:
            return InferredType(TYPE_INT, "int");
            
        case ASTNodeType::AST_STRING_LITERAL:
            return InferredType(TYPE_STRING, "string");
            
        case ASTNodeType::AST_ARRAY_LITERAL: {
            // 配列リテラルの要素から型を推論
            if (!node->arguments.empty()) {
                InferredType element_type = infer_type(node->arguments[0].get());
                // 配列として返す
                return InferredType(element_type.type_info, element_type.type_name, true, 1);
            }
            return InferredType(TYPE_INT, "int", true, 1); // デフォルトはint配列
        }
            
        case ASTNodeType::AST_VARIABLE: {
            // 変数の型を取得
            auto* var = interpreter_.get_variable_manager()->find_variable(node->name);
            if (var) {
                int dims = var->is_multidimensional ? 2 : (var->is_array ? 1 : 0);
                return InferredType(var->type, var->type_name, var->is_array, dims);
            }
            return InferredType();
        }
        
        case ASTNodeType::AST_TERNARY_OP:
            return infer_ternary_type(node->left.get(), node->right.get(), node->third.get());
            
        case ASTNodeType::AST_FUNC_CALL: {
            // 引数の型を推論
            std::vector<InferredType> arg_types;
            for (const auto& arg : node->arguments) {
                arg_types.push_back(infer_type(arg.get()));
            }
            return infer_function_return_type(node->name, arg_types);
        }
        
        case ASTNodeType::AST_MEMBER_ACCESS: {
            InferredType object_type = infer_type(node->left.get());
            return infer_member_type(object_type, node->name);
        }
        
        case ASTNodeType::AST_ARRAY_REF: {
            InferredType array_type = infer_type(node->left.get());
            return infer_array_element_type(array_type);
        }
        
        case ASTNodeType::AST_BINARY_OP: {
            InferredType left_type = infer_type(node->left.get());
            InferredType right_type = infer_type(node->right.get());
            
            // 比較演算子は常にboolを返す
            if (node->op == "==" || node->op == "!=" || node->op == "<" || 
                node->op == ">" || node->op == "<=" || node->op == ">=") {
                return InferredType(TYPE_BOOL, "bool");
            }
            
            // 算術演算子は共通型を返す
            return get_common_type(left_type, right_type);
        }
        
        case ASTNodeType::AST_UNARY_OP: {
            // 単項演算子の場合、オペランドの型を基に推論
            InferredType operand_type = infer_type(node->left.get());
            
            // 論理否定演算子(!)の場合はboolを返す
            if (node->op == "!") {
                return InferredType(TYPE_BOOL, "bool");
            }
            
            // 算術単項演算子(+, -, ++, --)の場合はオペランドと同じ型を返す
            if (node->op == "+" || node->op == "-" || node->op == "++" || node->op == "--") {
                return operand_type;
            }
            
            // その他の単項演算子もオペランドと同じ型を返す
            return operand_type;
        }
        
        default:
            return InferredType();
    }
}

InferredType TypeInferenceEngine::infer_ternary_type(const ASTNode* condition, const ASTNode* true_expr, const ASTNode* false_expr) {
    InferredType true_type = infer_type(true_expr);
    InferredType false_type = infer_type(false_expr);
    
    // 両方の分岐が同じ型なら、その型を返す
    if (true_type.is_compatible_with(false_type)) {
        return true_type;
    }
    
    // 型が異なる場合は共通型を探す
    return get_common_type(true_type, false_type);
}

InferredType TypeInferenceEngine::infer_function_return_type(const std::string& func_name, const std::vector<InferredType>& arg_types) {
    // 関数定義を検索して戻り値型を取得
    const ASTNode* func_def = interpreter_.find_function_definition(func_name);
    if (func_def) {
        // 関数定義から戻り値型を取得
        TypeInfo return_type = func_def->type_info;
        std::string return_type_name = func_def->type_name;
        return InferredType(return_type, return_type_name);
    }
    
    // ビルトイン関数の場合
    if (func_name == "println" || func_name == "printf") {
        return InferredType(TYPE_VOID, "void");
    }
    
    return InferredType();
}

InferredType TypeInferenceEngine::infer_member_type(const InferredType& object_type, const std::string& member_name) {
    // 構造体の場合 - 実際の変数から型を推論
    if (object_type.type_info == TYPE_STRUCT || !object_type.type_name.empty()) {
        // 実際の構造体変数からメンバの型を推論する
        // ここでは一般的な推論を行う（実装を簡素化）
        if (member_name == "name") {
            return InferredType(TYPE_STRING, "string");
        } else if (member_name == "age" || member_name == "value" || member_name == "count" || member_name == "x" || member_name == "y") {
            return InferredType(TYPE_INT, "int");
        } else if (member_name.find("str") != std::string::npos || member_name.find("text") != std::string::npos) {
            return InferredType(TYPE_STRING, "string");
        }
    }
    
    // インターフェース変数の場合
    if (object_type.type_info == TYPE_INTERFACE) {
        // メソッド呼び出しの場合、実装を検索して戻り値型を推論
        // 簡単のため、ここではINTを返す（実際にはより複雑な処理が必要）
        return InferredType(TYPE_INT, "int");
    }
    
    // デフォルトはINT型を推定
    return InferredType(TYPE_INT, "int");
}

InferredType TypeInferenceEngine::infer_array_element_type(const InferredType& array_type) {
    if (array_type.is_array && array_type.array_dimensions > 0) {
        InferredType element_type = array_type;
        element_type.array_dimensions--;
        if (element_type.array_dimensions == 0) {
            element_type.is_array = false;
        }
        return element_type;
    }
    return InferredType();
}

InferredType TypeInferenceEngine::get_common_type(const InferredType& type1, const InferredType& type2) {
    // 同じ型の場合
    if (type1.is_compatible_with(type2)) {
        return type1;
    }
    
    // 文字列が関わる場合は文字列型を優先
    if (type1.type_info == TYPE_STRING || type2.type_info == TYPE_STRING) {
        return InferredType(TYPE_STRING, "string");
    }
    
    // 数値型の場合、より大きな型を選択
    if ((type1.type_info == TYPE_INT || type1.type_info == TYPE_LONG) &&
        (type2.type_info == TYPE_INT || type2.type_info == TYPE_LONG)) {
        if (type1.type_info == TYPE_LONG || type2.type_info == TYPE_LONG) {
            return InferredType(TYPE_LONG, "long");
        } else {
            return InferredType(TYPE_INT, "int");
        }
    }
    
    // デフォルトは最初の型
    return type1;
}

InferredType TypeInferenceEngine::literal_to_type(const ASTNode* node) {
    if (!node) return InferredType();
    
    switch (node->node_type) {
        case ASTNodeType::AST_NUMBER:
            return InferredType(TYPE_INT, "int");
        case ASTNodeType::AST_STRING_LITERAL:
            return InferredType(TYPE_STRING, "string");
        default:
            return InferredType();
    }
}
