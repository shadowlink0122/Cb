#include "semantic_analyzer.h"
#include "../interpreter.h"
#include "variable_manager.h"
#include "../../common/debug_messages.h"
#include <iostream>
#include <algorithm>

SemanticAnalyzer::SemanticAnalyzer(Interpreter& interpreter, VariableManager& var_manager)
    : interpreter_(interpreter), variable_manager_(var_manager), 
      type_registry_(get_global_type_alias_registry()) {
}

SemanticAnalysisResult SemanticAnalyzer::analyze_declarations(ASTNode* program_ast) {
    if (!program_ast || program_ast->node_type != ASTNodeType::AST_STMT_LIST) {
        return SemanticAnalysisResult("Invalid program AST structure");
    }
    
    debug_print("[DEBUG] Starting semantic analysis phase");
    
    // 全宣言を走査してセマンティック解析
    for (auto& statement : program_ast->statements) {
        if (!statement) continue;
        
        SemanticAnalysisResult result;
        
        switch (statement->node_type) {
            case ASTNodeType::AST_TYPEDEF_DECL:
                result = analyze_typedef(statement.get());
                break;
                
            case ASTNodeType::AST_VAR_DECL:
                result = analyze_variable_declaration(statement.get());
                break;
                
            // 他の宣言タイプも追加可能
            default:
                continue; // 式文等はスキップ
        }
        
        if (!result.success) {
            return result;
        }
    }
    
    debug_print("[DEBUG] Semantic analysis complete");
    return SemanticAnalysisResult(true);
}

SemanticAnalysisResult SemanticAnalyzer::analyze_typedef(ASTNode* typedef_node) {
    if (!typedef_node || typedef_node->node_type != ASTNodeType::AST_TYPEDEF_DECL) {
        return SemanticAnalysisResult("Invalid typedef node");
    }
    
    std::string alias_name = typedef_node->name;
    debug_print("[DEBUG] Analyzing typedef: %s", alias_name.c_str());
    
    // 配列typedef の場合の処理
    if (!typedef_node->array_dimensions.empty()) {
        return analyze_array_typedef(alias_name, nullptr, &typedef_node->array_dimensions);
    }
    
    // 単純型aliasの処理 - type_info から型を取得
    TypeInfo base_type = typedef_node->type_info;
    type_registry_.register_alias(alias_name, base_type);
    debug_print("[DEBUG] Typedef registered: %s -> type %d", alias_name.c_str(), (int)base_type);
    return SemanticAnalysisResult(true);
    
    return SemanticAnalysisResult("Invalid typedef structure");
}

SemanticAnalysisResult SemanticAnalyzer::analyze_array_typedef(const std::string& alias_name, 
                                                             ASTNode* type_spec, 
                                                             const std::vector<std::unique_ptr<ASTNode>>* dimensions) {
    // 基本型を取得 (簡易実装：int固定)
    TypeInfo base_type = TYPE_INT;
    
    // 配列次元を解析
    std::vector<ArrayDimension> dims;
    for (const auto& dim_node : *dimensions) {
        ArrayDimension dim;
        if (dim_node && dim_node->node_type == ASTNodeType::AST_NUMBER) {
            dim.size = static_cast<int>(dim_node->int_value);
            dim.is_dynamic = false;
        } else {
            dim.size = -1; // 動的サイズ
            dim.is_dynamic = true;
        }
        dims.push_back(dim);
    }
    
    // ArrayTypeInfoを作成して登録
    ArrayTypeInfo array_info;
    array_info.base_type = base_type;
    array_info.dimensions = dims;
    
    type_registry_.register_array_alias(alias_name, array_info);
    debug_print("[DEBUG] Array typedef registered: %s with %d dimensions", alias_name.c_str(), (int)dims.size());
    
    return SemanticAnalysisResult(true);
}

SemanticAnalysisResult SemanticAnalyzer::analyze_variable_declaration(ASTNode* var_decl) {
    if (!var_decl || var_decl->node_type != ASTNodeType::AST_VAR_DECL) {
        return SemanticAnalysisResult("Invalid variable declaration node");
    }
    
    std::string var_name = var_decl->name;
    debug_print("[DEBUG] Analyzing variable declaration: %s", var_name.c_str());
    
    // 型情報を解決
    TypeInfo declared_type = TYPE_UNKNOWN;
    std::string type_name;
    
    if (var_decl->type_info != TYPE_UNKNOWN) {
        declared_type = var_decl->type_info;
    } else if (!var_decl->type_name.empty()) {
        type_name = var_decl->type_name;
        declared_type = type_registry_.resolve_alias(type_name);
        
        if (declared_type == TYPE_UNKNOWN && type_registry_.is_array_alias(type_name)) {
            // 配列型エイリアスの場合
            ArrayTypeInfo array_info = type_registry_.resolve_array_alias(type_name);
            // declared_type は配列として扱うためのプレースホルダー
            declared_type = array_info.base_type; 
        }
    }
    
    // 初期化値がある場合の解析 (rightノードを使用)
    if (var_decl->right) {
        if (var_decl->right->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
            // 配列リテラル初期化の詳細解析
            ArrayLiteralAnalysis analysis = analyze_array_literal(var_decl->right.get(), declared_type);
            
            if (!analysis.valid) {
                std::string error = "Array literal initialization failed for variable '" + var_name + "'";
                for (const std::string& type_error : analysis.type_errors) {
                    error += "\n  - " + type_error;
                }
                return SemanticAnalysisResult(error);
            }
            
            debug_print("[DEBUG] Array literal validated: %s (%d elements)", var_name.c_str(), (int)analysis.elements.size());
        } else {
            // 単一値初期化の型チェック
            TypeInfo init_type = infer_value_type(var_decl->right.get());
            if (!is_type_compatible(init_type, declared_type)) {
                return SemanticAnalysisResult("Type mismatch in initialization of variable '" + var_name + "'");
            }
        }
    }
    
    debug_print("[DEBUG] Variable semantic analysis complete: %s", var_name.c_str());
    return SemanticAnalysisResult(true);
}

ArrayLiteralAnalysis SemanticAnalyzer::analyze_array_literal(ASTNode* array_literal, TypeInfo expected_type) {
    ArrayLiteralAnalysis analysis;
    
    if (!array_literal || array_literal->node_type != ASTNodeType::AST_ARRAY_LITERAL) {
        analysis.type_errors.push_back("Invalid array literal node");
        return analysis;
    }
    
    if (array_literal->statements.empty()) {
        // 空配列
        analysis.valid = true;
        analysis.inferred_element_type = expected_type;
        return analysis;
    }
    
    // 各要素の型を推論・検証
    TypeInfo inferred_type = TYPE_UNKNOWN;
    
    for (size_t i = 0; i < array_literal->statements.size(); ++i) {
        auto& element = array_literal->statements[i];
        TypeInfo element_type = infer_value_type(element.get());
        
        if (i == 0) {
            inferred_type = element_type;
        } else {
            // 型の統一性をチェック
            if (!is_type_compatible(element_type, inferred_type)) {
                analysis.type_errors.push_back("Element " + std::to_string(i) + " has incompatible type");
            }
        }
        
        // 要素の参照を保存（コピーではなく）
        analysis.elements.push_back(std::unique_ptr<ASTNode>(nullptr)); // プレースホルダー
    }
    
    analysis.inferred_element_type = inferred_type;
    analysis.valid = analysis.type_errors.empty();
    
    return analysis;
}

bool SemanticAnalyzer::is_type_compatible(TypeInfo source, TypeInfo target) {
    // 基本的な型互換性チェック
    if (source == target) return true;
    if (target == TYPE_UNKNOWN) return true; // 型推論の場合
    
    // 数値型間の互換性
    if ((source >= TYPE_TINY && source <= TYPE_LONG) && 
        (target >= TYPE_TINY && target <= TYPE_LONG)) {
        return true; // 数値型同士は相互変換可能
    }
    
    // 文字列とブール値の特別処理（配列リテラルでの混合型対応）
    return false;
}

TypeInfo SemanticAnalyzer::infer_value_type(ASTNode* value_node) {
    if (!value_node) return TYPE_UNKNOWN;
    
    switch (value_node->node_type) {
        case ASTNodeType::AST_NUMBER:
            return value_node->type_info; // ASTNodeのtype_infoを使用
        case ASTNodeType::AST_STRING_LITERAL:
            return TYPE_STRING;
        case ASTNodeType::AST_VARIABLE:
            // 変数参照の場合、変数管理システムから型を取得
            return TYPE_UNKNOWN; // 簡単化のため
        default:
            return TYPE_UNKNOWN;
    }
}

void SemanticAnalyzer::report_semantic_error(const std::string& message, const std::string& location) {
    std::string full_error = message;
    if (!location.empty()) {
        full_error += " (at " + location + ")";
    }
    semantic_errors_.push_back(full_error);
    std::cerr << "Semantic Error: " << full_error << std::endl;
}
