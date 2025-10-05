#include "services/array_processing_service.h"
#include "managers/common_operations.h"
#include "core/interpreter.h"
#include "managers/variable_manager.h"
#include "managers/array_manager.h"
#include "services/debug_service.h"
#include "../../../common/debug_messages.h"
#include <iostream>
#include <stdexcept>

ArrayProcessingService::ArrayProcessingService(Interpreter* interpreter, CommonOperations* common_ops)
    : interpreter_(interpreter), common_operations_(common_ops) {}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processArrayLiteral(
    const std::string& target_name,
    const ASTNode* literal_node,
    ArrayContext context) {
    
    ArrayOperationResult result = {};
    result.success = false;
    
    try {
        if (!literal_node || literal_node->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
            result.error_message = "Invalid array literal";
            return result;
        }
        
        logArrayOperation(context, "Processing array literal for: " + target_name);
        
        // 統一的な前処理バリデーション
        if (!validateArrayContext(target_name, context)) {
            result.error_message = "Invalid array context for: " + target_name;
            return result;
        }
        
        // コンテキスト別処理の統一ディスパッチ
        switch (context) {
            case ArrayContext::GLOBAL_VARIABLE:
            case ArrayContext::LOCAL_VARIABLE:
                result = processVariableArray(target_name, literal_node);
                break;
                
            case ArrayContext::STRUCT_MEMBER:
                result = processStructMemberArray(target_name, literal_node);
                break;
                
            case ArrayContext::STRUCT_ARRAY_ELEMENT:
                result = processStructArrayElement(target_name, literal_node);
                break;
                
            case ArrayContext::FUNCTION_PARAMETER:
                result = processFunctionParameterArray(target_name, literal_node);
                break;
                
            case ArrayContext::FUNCTION_RETURN:
                result = processFunctionReturnArray(target_name, literal_node);
                break;
                
            case ArrayContext::MULTIDIMENSIONAL:
                result = processMultidimensionalArray(target_name, literal_node);
                break;
                
            default:
                result.error_message = "Unsupported array context";
                return result;
        }
        
        if (result.success) {
            logArrayOperation(context, "Successfully processed array for: " + target_name);
        }
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Array processing error: " + std::string(e.what());
    }
    
    return result;
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processArrayDeclaration(
    const ASTNode* node,
    ArrayContext context) {
    
    ArrayOperationResult result = {};
    result.success = true; // 基本実装では成功とする
    
    return result;
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::assignArrayLiteral(
    Variable* target_var,
    const ASTNode* literal_node,
    ArrayContext context) {
    
    ArrayOperationResult result = {};
    result.success = false;
    
    try {
        if (!target_var) {
            result.error_message = "Null target variable";
            return result;
        }
        
        if (!validateArrayOperation(target_var, literal_node, context)) {
            result.error_message = "Array operation validation failed";
            return result;
        }
        
        // 統一的な配列リテラル解析
        CommonOperations::ArrayLiteralResult array_result =
            common_operations_->parse_array_literal(literal_node);
            
        // 変数に配列を代入
        std::string resolved_name;
        if (interpreter_) {
            resolved_name = interpreter_->find_variable_name(target_var);
        }
        common_operations_->assign_array_literal_to_variable(target_var, array_result, resolved_name);
        
        result.success = true;
        result.inferred_type = array_result.element_type;
        result.element_count = array_result.size;
        
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = "Array assignment error: " + std::string(e.what());
    }
    
    return result;
}

// Priority 3: 統一配列要素アクセス機能
int64_t ArrayProcessingService::getArrayElement(
    const std::string& var_name,
    const std::vector<int64_t>& indices,
    ArrayContext context) {
    
    Variable* var = interpreter_->find_variable(var_name);
    if (!var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }
    
    if (!var->is_array) {
        throw std::runtime_error("Variable is not an array: " + var_name);
    }
    
    if (var->is_multidimensional) {
        // 多次元配列は直接ArrayManagerに委譲
        return interpreter_->get_array_manager()->getMultidimensionalArrayElement(*var, indices);
    } else {
        // 1次元配列の場合
        if (indices.size() != 1) {
            throw std::runtime_error("Invalid index count for 1D array");
        }
        
        int64_t index = indices[0];
        if (index < 0 || index >= static_cast<int64_t>(var->array_values.size())) {
            throw std::runtime_error("Array index out of bounds");
        }
        
        return var->array_values[index];
    }
}

void ArrayProcessingService::setArrayElement(
    const std::string& var_name,
    const std::vector<int64_t>& indices,
    int64_t value,
    ArrayContext context) {
    
    Variable* var = interpreter_->find_variable(var_name);
    if (!var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }
    
    if (!var->is_array) {
        throw std::runtime_error("Variable is not an array: " + var_name);
    }
    
    if (var->is_const && var->is_assigned) {
        throw std::runtime_error("Cannot assign to const array: " + var_name);
    }
    
    int64_t adjusted_value = value;
    if (var->is_unsigned && adjusted_value < 0) {
        DEBUG_WARN(
            VARIABLE,
            "Unsigned array %s element assignment with negative value (%lld); clamping to 0",
            var_name.c_str(), static_cast<long long>(adjusted_value));
        adjusted_value = 0;
    }

    if (var->is_multidimensional) {
        // 多次元配列は直接ArrayManagerに委譲
        interpreter_->get_array_manager()->setMultidimensionalArrayElement(*var, indices, adjusted_value);
    } else {
        // 1次元配列の場合
        if (indices.size() != 1) {
            throw std::runtime_error("Invalid index count for 1D array");
        }
        
        int64_t index = indices[0];
        if (index < 0 || index >= static_cast<int64_t>(var->array_values.size())) {
            throw std::runtime_error("Array index out of bounds");
        }
        
        var->array_values[index] = adjusted_value;
    }
}

std::string ArrayProcessingService::getStringArrayElement(
    const std::string& var_name,
    const std::vector<int64_t>& indices,
    ArrayContext context) {
    
    Variable* var = interpreter_->find_variable(var_name);
    if (!var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }
    
    if (!var->is_array) {
        throw std::runtime_error("Variable is not an array: " + var_name);
    }
    
    if (var->is_multidimensional) {
        // 多次元配列は直接ArrayManagerに委譲
        return interpreter_->get_array_manager()->getMultidimensionalStringArrayElement(*var, indices);
    } else {
        // 1次元配列の場合
        if (indices.size() != 1) {
            throw std::runtime_error("Invalid index count for 1D array");
        }
        
        int64_t index = indices[0];
        if (index < 0 || index >= static_cast<int64_t>(var->array_strings.size())) {
            throw std::runtime_error("String array index out of bounds");
        }
        
        return var->array_strings[index];
    }
}

void ArrayProcessingService::setStringArrayElement(
    const std::string& var_name,
    const std::vector<int64_t>& indices,
    const std::string& value,
    ArrayContext context) {
    
    Variable* var = interpreter_->find_variable(var_name);
    if (!var) {
        throw std::runtime_error("Variable not found: " + var_name);
    }
    
    if (!var->is_array) {
        throw std::runtime_error("Variable is not an array: " + var_name);
    }
    
    if (var->is_const && var->is_assigned) {
        throw std::runtime_error("Cannot assign to const string array: " + var_name);
    }
    
    if (var->is_multidimensional) {
        // 多次元配列は直接ArrayManagerに委譲  
        interpreter_->get_array_manager()->setMultidimensionalStringArrayElement(*var, indices, value);
    } else {
        // 1次元配列の場合
        if (indices.size() != 1) {
            throw std::runtime_error("Invalid index count for 1D array");
        }
        
        int64_t index = indices[0];
        if (index < 0 || index >= static_cast<int64_t>(var->array_strings.size())) {
            throw std::runtime_error("String array index out of bounds");
        }
        
        var->array_strings[index] = value;
    }
}

TypeInfo ArrayProcessingService::inferArrayElementType(const ASTNode* literal_node) {
    if (!literal_node || literal_node->arguments.empty()) {
        return TYPE_UNKNOWN;
    }
    
    // 最初の要素から型を推論
    const ASTNode* first_element = literal_node->arguments[0].get();
    
    // 多次元配列の場合、再帰的に最深レベルの要素まで辿る
    while (first_element && first_element->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        if (first_element->arguments.empty()) {
            return TYPE_UNKNOWN;
        }
        first_element = first_element->arguments[0].get();
    }
    
    if (!first_element) {
        return TYPE_UNKNOWN;
    }
    
    if (first_element->node_type == ASTNodeType::AST_STRING_LITERAL) {
        return TYPE_STRING;
    } else if (first_element->node_type == ASTNodeType::AST_NUMBER) {
        return TYPE_INT; // 簡素化
    }
    
    return TYPE_INT; // デフォルト
}

bool ArrayProcessingService::validateArrayOperation(
    const Variable* var,
    const ASTNode* node,
    ArrayContext context) {
    return var && node; // 簡素化
}

// コンテキスト別処理実装
ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processVariableArray(const std::string& name, const ASTNode* literal_node) {
    ArrayOperationResult result = {};
    
    Variable* var = interpreter_->find_variable(name);
    if (!var) {
        result.success = false;
        result.error_message = "Variable not found: " + name;
        return result;
    }
    
    return assignArrayLiteral(var, literal_node, ArrayContext::LOCAL_VARIABLE);
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processStructMemberArray(const std::string& name, const ASTNode* literal_node) {
    return processVariableArray(name, literal_node); // 簡素化
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processStructArrayElement(const std::string& name, const ASTNode* literal_node) {
    return processVariableArray(name, literal_node); // 簡素化
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processFunctionParameterArray(const std::string& name, const ASTNode* literal_node) {
    return processVariableArray(name, literal_node); // 簡素化
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processFunctionReturnArray(const std::string& name, const ASTNode* literal_node) {
    return processVariableArray(name, literal_node); // 簡素化
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processMultidimensionalArray(const std::string& name, const ASTNode* literal_node) {
    return processVariableArray(name, literal_node); // 簡素化
}

bool ArrayProcessingService::validateArrayContext(const std::string& name, ArrayContext context) {
    return true; // 簡素化
}

void ArrayProcessingService::performContextSpecificPostProcessing(
    Variable* var, const CommonOperations::ArrayLiteralResult& result, ArrayContext context) {
    // 簡素化のため空実装
}

void ArrayProcessingService::updateStructMemberElements(
    Variable* var, const CommonOperations::ArrayLiteralResult& result) {
    // 簡素化のため空実装
}

void ArrayProcessingService::logArrayOperation(ArrayContext context, const std::string& details) {
    // デバッグ出力（必要に応じて）
}

std::string ArrayProcessingService::contextToString(ArrayContext context) {
    switch (context) {
        case ArrayContext::GLOBAL_VARIABLE: return "GLOBAL_VARIABLE";
        case ArrayContext::LOCAL_VARIABLE: return "LOCAL_VARIABLE";
        case ArrayContext::FUNCTION_PARAMETER: return "FUNCTION_PARAMETER";
        case ArrayContext::FUNCTION_RETURN: return "FUNCTION_RETURN";
        case ArrayContext::STRUCT_MEMBER: return "STRUCT_MEMBER";
        case ArrayContext::STRUCT_ARRAY_ELEMENT: return "STRUCT_ARRAY_ELEMENT";
        case ArrayContext::MULTIDIMENSIONAL: return "MULTIDIMENSIONAL";
        default: return "UNKNOWN";
    }
}
