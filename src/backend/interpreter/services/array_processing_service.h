#pragma once
#include "../../../common/ast.h"
#include "managers/common_operations.h"
#include <vector>
#include <string>

// 前方宣言
struct Variable;
class Interpreter;
class CommonOperations;

/**
 * 配列処理統合サービス - すべての配列操作を統一
 * DRY原則に基づく完全統合アーキテクチャ
 */
class ArrayProcessingService {
public:
    // 配列操作コンテキスト
    enum class ArrayContext {
        GLOBAL_VARIABLE,        // グローバル変数
        LOCAL_VARIABLE,         // ローカル変数
        FUNCTION_PARAMETER,     // 関数引数
        FUNCTION_RETURN,        // 関数戻り値
        STRUCT_MEMBER,          // 構造体メンバー
        STRUCT_ARRAY_ELEMENT,   // 構造体配列要素
        MULTIDIMENSIONAL       // 多次元配列
    };

    struct ArrayOperationResult {
        bool success;
        std::string error_message;
        TypeInfo inferred_type;
        size_t element_count;
    };

    ArrayProcessingService(Interpreter* interpreter, CommonOperations* common_ops);
    
    // 統一配列処理エントリーポイント
    ArrayOperationResult processArrayLiteral(
        const std::string& target_name,
        const ASTNode* literal_node,
        ArrayContext context
    );
    
    // 統一配列宣言処理
    ArrayOperationResult processArrayDeclaration(
        const ASTNode* node,
        ArrayContext context
    );
    
    // 統一配列代入処理
    ArrayOperationResult assignArrayLiteral(
        Variable* target_var,
        const ASTNode* literal_node,
        ArrayContext context
    );
    
    // 統一配列要素アクセス
    int64_t getArrayElement(
        const std::string& var_name,
        const std::vector<int64_t>& indices,
        ArrayContext context
    );
    
    void setArrayElement(
        const std::string& var_name,
        const std::vector<int64_t>& indices,
        int64_t value,
        ArrayContext context
    );
    
    // 統一文字列配列要素アクセス
    std::string getStringArrayElement(
        const std::string& var_name,
        const std::vector<int64_t>& indices,
        ArrayContext context
    );
    
    void setStringArrayElement(
        const std::string& var_name,
        const std::vector<int64_t>& indices,
        const std::string& value,
        ArrayContext context
    );
    
    // 統一型推論
    TypeInfo inferArrayElementType(const ASTNode* literal_node);
    
    // 統一バリデーション
    bool validateArrayOperation(
        const Variable* var,
        const ASTNode* node,
        ArrayContext context
    );

private:
    Interpreter* interpreter_;
    CommonOperations* common_operations_;
    
    // 内部統一処理
    ArrayOperationResult processVariableArray(const std::string& name, const ASTNode* node);
    ArrayOperationResult processStructMemberArray(const std::string& name, const ASTNode* node);
    ArrayOperationResult processStructArrayElement(const std::string& name, const ASTNode* node);
    ArrayOperationResult processFunctionParameterArray(const std::string& name, const ASTNode* node);
    ArrayOperationResult processFunctionReturnArray(const std::string& name, const ASTNode* node);
    ArrayOperationResult processMultidimensionalArray(const std::string& name, const ASTNode* node);
    
    // 統一バリデーション・ヘルパー
    bool validateArrayContext(const std::string& name, ArrayContext context);
    void performContextSpecificPostProcessing(Variable* var, const CommonOperations::ArrayLiteralResult& result, ArrayContext context);
    void updateStructMemberElements(Variable* var, const CommonOperations::ArrayLiteralResult& result);
    void logArrayOperation(ArrayContext context, const std::string& details);
    std::string contextToString(ArrayContext context);
};
