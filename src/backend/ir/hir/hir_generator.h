#pragma once

#include "../../../common/ast.h"
#include "hir_node.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cb {
namespace ir {

// Forward declarations
class HIRExprConverter;
class HIRStmtConverter;
class HIRDeclTypeConverter;

/**
 * @brief HIR Generator - Main coordinator for AST to HIR conversion
 *
 * This class coordinates the conversion by delegating to specialized
 * converters:
 * - HIRExprConverter: Expression conversion
 * - HIRStmtConverter: Statement conversion
 * - HIRDeclTypeConverter: Declaration and type conversion
 */
class HIRGenerator {
  public:
    HIRGenerator();
    ~HIRGenerator();

    std::unique_ptr<hir::HIRProgram>
    generate(const std::vector<std::unique_ptr<ASTNode>> &ast_nodes);

    std::unique_ptr<hir::HIRProgram> generate_with_parser_definitions(
        const std::vector<std::unique_ptr<ASTNode>> &ast_nodes,
        const std::unordered_map<std::string, StructDefinition> &struct_defs,
        const std::unordered_map<std::string, InterfaceDefinition>
            &interface_defs,
        const std::vector<ImplDefinition> &impl_defs);

    // For testing
    friend class HIRGeneratorTest;
    friend class HIRExprConverter;
    friend class HIRStmtConverter;
    friend class HIRDeclTypeConverter;

    // Accessors for converters
    const std::unordered_set<std::string> &get_interface_names() const {
        return interface_names_;
    }

    // Function lookup for type inference
    const ASTNode *lookup_function(const std::string &name) const;

    // Analyze if a function returns a function pointer
    bool
    analyze_function_returns_function_pointer(const ASTNode *func_node) const;

  private:
    // Converter instances
    std::unique_ptr<HIRExprConverter> expr_converter_;
    std::unique_ptr<HIRStmtConverter> stmt_converter_;
    std::unique_ptr<HIRDeclTypeConverter> decl_type_converter_;

    // Delegation methods (called by main generate functions)
    hir::HIRExpr convert_expr(const ASTNode *node);
    hir::HIRStmt convert_stmt(const ASTNode *node);
    hir::HIRFunction convert_function(const ASTNode *node);
    hir::HIRStruct convert_struct(const ASTNode *node);
    hir::HIREnum convert_enum(const ASTNode *node);
    hir::HIRInterface convert_interface(const ASTNode *node);
    hir::HIRImpl convert_impl(const ASTNode *node);
    hir::HIRUnion convert_union(const ASTNode *node);
    hir::HIRType convert_type(TypeInfo type_info,
                              const std::string &type_name = "");
    hir::HIRType convert_array_type(const ArrayTypeInfo &array_info);

    // Utility methods
    SourceLocation convert_location(const ::SourceLocation &ast_loc);
    void report_error(const std::string &message,
                      const ::SourceLocation &location);

    // State
    uint32_t next_var_id = 0;
    int error_count = 0;
    std::unordered_set<std::string> interface_names_;

    // AST nodes for lookup during conversion
    const std::vector<std::unique_ptr<ASTNode>> *ast_nodes_ = nullptr;
};

} // namespace ir
} // namespace cb
