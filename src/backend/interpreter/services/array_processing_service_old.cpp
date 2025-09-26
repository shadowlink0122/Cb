#include "services/array_processing_service.h"
#include "managers/common_operations.h"
#include "core/interpreter.h"
#include "managers/common_operations.h"
#include "managers/variable_manager.h"
#include "managers/array_manager.h"
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
        common_operations_->assign_array_literal_to_variable(target_var, array_result);
        
        // コンテキスト固有の後処理
        // performContextSpecificPostProcessing(target_var, array_result, context);
        
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
    
    if (var->is_const) {
        throw std::runtime_error("Cannot assign to const array: " + var_name);
    }
    
    if (var->is_multidimensional) {
        // 多次元配列は直接ArrayManagerに委譲
        interpreter_->get_array_manager()->setMultidimensionalArrayElement(*var, indices, value);
    } else {
        // 1次元配列の場合
        if (indices.size() != 1) {
            throw std::runtime_error("Invalid index count for 1D array");
        }
        
        int64_t index = indices[0];
        if (index < 0 || index >= static_cast<int64_t>(var->array_values.size())) {
            throw std::runtime_error("Array index out of bounds");
        }
        
        var->array_values[index] = value;
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
    
    if (var->is_const) {
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
            }
        }
    }
    
    return true;
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
    ArrayOperationResult result = {};
    
    // 構造体メンバー名を解析（"var.member"形式）
    size_t dot_pos = name.find('.');
    if (dot_pos == std::string::npos) {
        result.success = false;
        result.error_message = "Invalid struct member name format: " + name;
        return result;
    }
    
    std::string var_name = name.substr(0, dot_pos);
    std::string member_name = name.substr(dot_pos + 1);
    
    // 既存の統一処理を活用
    try {
        interpreter_->assign_struct_member_array_literal(var_name, member_name, literal_node);
        result.success = true;
        result.inferred_type = inferArrayElementType(literal_node);
        result.element_count = literal_node->arguments.size();
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    return result;
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processStructArrayElement(const std::string& name, const ASTNode* literal_node) {
    ArrayOperationResult result = {};
    
    try {
        interpreter_->assign_struct_literal(name, literal_node);
        result.success = true;
        result.inferred_type = TYPE_STRUCT;
        result.element_count = 1;
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    return result;
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processFunctionParameterArray(const std::string& name, const ASTNode* literal_node) {
    // 関数パラメータは通常の変数配列として処理
    return processVariableArray(name, literal_node);
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processFunctionReturnArray(const std::string& name, const ASTNode* literal_node) {
    // 関数戻り値配列は通常の変数配列として処理
    return processVariableArray(name, literal_node);
}

ArrayProcessingService::ArrayOperationResult 
ArrayProcessingService::processMultidimensionalArray(const std::string& name, const ASTNode* literal_node) {
    ArrayOperationResult result = {};
    
    Variable* var = interpreter_->find_variable(name);
    if (!var) {
        result.success = false;
        result.error_message = "Variable not found: " + name;
        return result;
    }
    
    try {
        TypeInfo element_type = inferArrayElementType(literal_node);
        interpreter_->get_array_manager()->processMultidimensionalArrayLiteral(*var, literal_node, element_type);
        result.success = true;
        result.inferred_type = element_type;
        result.element_count = literal_node->arguments.size();
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }
    
    return result;
}

// ヘルパー関数の実装
bool ArrayProcessingService::validateArrayContext(const std::string& name, ArrayContext context) {
    // コンテキストに応じた名前の妥当性チェック
    switch (context) {
        case ArrayContext::STRUCT_MEMBER:
            return name.find('.') != std::string::npos;
            
        case ArrayContext::STRUCT_ARRAY_ELEMENT:
            return name.find('[') != std::string::npos;
            
        default:
            return !name.empty();
    }
}

void ArrayProcessingService::performContextSpecificPostProcessing(
    Variable* var, const CommonOperations::ArrayLiteralResult& result, ArrayContext context) {
    
    // コンテキスト別の後処理
    switch (context) {
        case ArrayContext::STRUCT_MEMBER:
            // 構造体メンバー配列の場合、個別要素変数も更新
            updateStructMemberElements(var, result);
            break;
            
        default:
            // 通常の配列では特別な後処理は不要
            break;
    }
}

void ArrayProcessingService::updateStructMemberElements(
    Variable* var, const CommonOperations::ArrayLiteralResult& result) {
    
    // 構造体メンバー配列の個別要素を更新（既存ロジックを統合）
    if (!result.is_string_array) {
        for (size_t i = 0; i < result.size && i < result.int_values.size(); i++) {
            // 個別要素変数名を生成して更新
            // 実装は既存のassign_struct_member_array_literalと同等
        }
    }
}

void ArrayProcessingService::logArrayOperation(ArrayContext context, const std::string& details) {
    if (interpreter_->is_debug_mode()) {
        debug_print("[ArrayProcessingService] [%s] %s\n", 
                   contextToString(context).c_str(), details.c_str());
    }
}

std::string ArrayProcessingService::contextToString(ArrayContext context) {
    switch (context) {
        case ArrayContext::GLOBAL_VARIABLE: return "GLOBAL_VAR";
        case ArrayContext::LOCAL_VARIABLE: return "LOCAL_VAR";
        case ArrayContext::FUNCTION_PARAMETER: return "FUNC_PARAM";
        case ArrayContext::FUNCTION_RETURN: return "FUNC_RETURN";
        case ArrayContext::STRUCT_MEMBER: return "STRUCT_MEMBER";
        case ArrayContext::STRUCT_ARRAY_ELEMENT: return "STRUCT_ARRAY";
        case ArrayContext::MULTIDIMENSIONAL: return "MULTIDIM";
        default: return "UNKNOWN";
    }
}

ArrayProcessingService::ArrayOperationResult ArrayProcessingService::processArrayDeclaration(
    const ASTNode* node,
    ArrayContext context
) {
    if (!node) {
        return {false, "Invalid array declaration node", TYPE_INT, 0};
    }
    
    // 既存のArrayManagerの処理に移譲
    Variable var;
    try {
        interpreter_->array_manager_->processArrayDeclaration(var, node);
        
        // 結果を適切なスコープに配置
        if (context == ArrayContext::GLOBAL_VARIABLE) {
            interpreter_->global_scope.variables[node->name] = var;
        } else {
            interpreter_->current_scope().variables[node->name] = var;
        }
        
        return {true, "", var.type, static_cast<size_t>(var.array_size)};
    } catch (const std::exception& e) {
        return {false, e.what(), TYPE_INT, 0};
    }
}
