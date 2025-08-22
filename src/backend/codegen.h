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

// 将来の拡張用
class LLVMCodeGenerator : public CodeGenerator {
  public:
    std::string generate_code(const ASTNode *ast) override;
    void emit_to_file(const ASTNode *ast, const std::string &filename) override;
};

class AssemblyCodeGenerator : public CodeGenerator {
  public:
    std::string generate_code(const ASTNode *ast) override;
    void emit_to_file(const ASTNode *ast, const std::string &filename) override;
};
