#pragma once
#include "../../../common/ast.h"
#include "../../../common/io_interface.h"
#include <string>
#include <memory> // スマートポインタ用

// 前方宣言
class Interpreter;
struct Variable;
class ReturnException;
class ExpressionService; // DRY効率化: 式評価サービス
class Interpreter;
struct Variable;
class ReturnException;
// class ExpressionService; // DRY効率化: 式評価サービス（一時コメントアウト）

// 出力管理クラス
class OutputManager {
public:
    OutputManager(Interpreter* interpreter);
    // ~OutputManager(); // 一時コメントアウト
    
    // IOInterface設定（抽象化レイヤー）
    void set_io_interface(IOInterface* io) { io_interface_ = io; }
    IOInterface* get_io_interface() const { return io_interface_; }
    
    // 基本出力機能
    void print_value(const ASTNode *expr);
    void print_value_with_newline(const ASTNode *expr);
    void print_newline();
    void print_multiple_with_newline(const ASTNode *arg_list);
    void print_formatted_with_newline(const ASTNode *format_str, const ASTNode *arg_list);
    void print_formatted(const ASTNode *format_str, const ASTNode *arg_list);
    void print_formatted(const ASTNode *format_str, const ASTNode *arg_list, size_t start_index);
    void print_multiple(const ASTNode *arg_list);
    
    // 文字列フォーマット機能（戻り値として返す）
    std::string format_string(const ASTNode *format_str, const ASTNode *arg_list);

private:
    Interpreter* interpreter_;  // インタープリターへの参照
    IOInterface* io_interface_; // 出力抽象化レイヤー
    ExpressionService* expression_service_; // DRY効率化: 統一式評価サービス（Interpreterから取得）
    
    // ヘルパーメソッド
    Variable* find_variable(const std::string& name);
    int64_t evaluate_expression(const ASTNode* node);
    const ASTNode* find_function(const std::string& name);
    
    // エスケープ処理とフォーマット解析
    std::string process_escape_sequences(const std::string& input);
    bool has_unescaped_format_specifiers(const std::string& str);
    size_t count_format_specifiers(const std::string& str);
};
