#pragma once
#include "../../common/ast.h"
#include "../../common/type_alias.h"
#include <string>
#include <vector>
#include <memory>

// 前方宣言
class Interpreter;
class VariableManager;

// 意味解析結果
struct SemanticAnalysisResult {
    bool success;
    std::string error_message;
    std::string error_location;
    
    SemanticAnalysisResult(bool s = true) : success(s) {}
    SemanticAnalysisResult(const std::string& error, const std::string& location = "") 
        : success(false), error_message(error), error_location(location) {}
};

// 配列リテラル解析結果
struct ArrayLiteralAnalysis {
    bool valid;
    TypeInfo inferred_element_type;
    std::vector<std::unique_ptr<ASTNode>> elements;
    std::vector<std::string> type_errors;
    
    ArrayLiteralAnalysis() : valid(false), inferred_element_type(TYPE_UNKNOWN) {}
};

// 意味解析エンジン
class SemanticAnalyzer {
public:
    SemanticAnalyzer(Interpreter& interpreter, VariableManager& var_manager);
    
    // メイン解析フェーズ
    SemanticAnalysisResult analyze_declarations(ASTNode* program_ast);
    
    // typedef解析とregistration
    SemanticAnalysisResult analyze_typedef(ASTNode* typedef_node);
    
    // 変数宣言の意味解析（初期化値含む）
    SemanticAnalysisResult analyze_variable_declaration(ASTNode* var_decl);
    
    // 配列リテラルの深度解析
    ArrayLiteralAnalysis analyze_array_literal(ASTNode* array_literal, TypeInfo expected_type);
    
    // 型互換性チェック
    bool is_type_compatible(TypeInfo source, TypeInfo target);
    
    // 値の型推論
    TypeInfo infer_value_type(ASTNode* value_node);
    
    // エラー報告
    void report_semantic_error(const std::string& message, const std::string& location = "");

private:
    Interpreter& interpreter_;
    VariableManager& variable_manager_;
    TypeAliasRegistry& type_registry_;
    
    std::vector<std::string> semantic_errors_;
    
    // 内部解析ヘルパー
    SemanticAnalysisResult analyze_array_typedef(const std::string& alias_name, ASTNode* type_spec, const std::vector<std::unique_ptr<ASTNode>>* dimensions);
    bool validate_array_literal_elements(const std::vector<std::unique_ptr<ASTNode>>& elements, TypeInfo target_type);
};
