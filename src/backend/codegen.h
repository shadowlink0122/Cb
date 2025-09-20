#pragma once
#include "../common/ast.h"
#include <sstream>
#include <string>

// コード生成器の基底クラス
class CodeGenerator : public CodeGeneratorInterface {
  public:
    virtual ~CodeGenerator() = default;

    // 各バックエンドで実装すべき純粋仮想関数
    virtual std::string generate_code(const ASTNode *ast) override = 0;
    virtual void emit_to_file(const ASTNode *ast,
                              const std::string &filename) override = 0;

  protected:
    // 共通のヘルパー関数
    std::string type_to_string(TypeInfo type);
    std::string operator_to_string(const std::string &op);
    void generate_statement(const ASTNode *node, std::ostream &out,
                            int indent = 0);
    void generate_expression(const ASTNode *node, std::ostream &out);
    std::string indent_string(int level);
};

// C言語コード生成器
class CCodeGenerator : public CodeGenerator {
  public:
    std::string generate_code(const ASTNode *ast) override;
    void emit_to_file(const ASTNode *ast, const std::string &filename) override;

  private:
    void generate_c_headers(std::ostream &out);
    void generate_c_function(const ASTNode *func, std::ostream &out);
    void generate_c_statement(const ASTNode *stmt, std::ostream &out,
                              int indent = 0);
    void generate_c_expression(const ASTNode *expr, std::ostream &out);
    std::string c_type_name(TypeInfo type);
};

// マルチターゲット対応
class LLVMCodeGenerator : public CodeGenerator {
  public:
    std::string generate_code(const ASTNode *ast) override;
    void emit_to_file(const ASTNode *ast, const std::string &filename) override;

    // ターゲット指定
    void set_target(const std::string &target); // "x86_64", "arm64", "wasm32"

  private:
    std::string target_triple_ = "x86_64-unknown-linux-gnu";
};

class WebAssemblyCodeGenerator : public CodeGenerator {
  public:
    std::string generate_code(const ASTNode *ast) override;
    void emit_to_file(const ASTNode *ast, const std::string &filename) override;

    // WASM特有機能
    void enable_browser_features(bool enable = true);
    void set_memory_pages(uint32_t pages);

  private:
    void generate_wasm_header(std::ostream &out);
    void generate_wasm_imports(std::ostream &out);
    void generate_wasm_exports(std::ostream &out);
    std::string wasm_type_name(TypeInfo type);
    bool browser_features_ = true;
    uint32_t memory_pages_ = 1;
};

class AssemblyCodeGenerator : public CodeGenerator {
  public:
    std::string generate_code(const ASTNode *ast) override;
    void emit_to_file(const ASTNode *ast, const std::string &filename) override;
};
