// v0.14.0: HIR Builder
// HIRノードを簡単に構築するためのビルダーパターン実装

#pragma once

#include "hir_node.h"
#include <memory>
#include <string>
#include <vector>

namespace cb {
namespace ir {
namespace hir {

// HIRビルダー: HIRノードを構築するためのヘルパークラス
class HIRBuilder {
  public:
    HIRBuilder() = default;

    // 式の構築
    static HIRExpr make_literal(const std::string &value, const HIRType &type);
    static HIRExpr make_variable(const std::string &name, const HIRType &type);
    static HIRExpr make_binary_op(const std::string &op, HIRExpr left,
                                  HIRExpr right, const HIRType &result_type);
    static HIRExpr make_unary_op(const std::string &op, HIRExpr operand,
                                 const HIRType &result_type);
    static HIRExpr make_function_call(const std::string &func_name,
                                      std::vector<HIRExpr> args,
                                      const HIRType &return_type);
    static HIRExpr make_method_call(HIRExpr receiver,
                                    const std::string &method_name,
                                    std::vector<HIRExpr> args,
                                    const HIRType &return_type);
    static HIRExpr make_member_access(HIRExpr object,
                                      const std::string &member_name,
                                      bool is_arrow,
                                      const HIRType &member_type);
    static HIRExpr make_array_access(HIRExpr array, HIRExpr index,
                                     const HIRType &element_type);
    static HIRExpr make_cast(HIRExpr expr, const HIRType &target_type);
    static HIRExpr make_ternary(HIRExpr condition, HIRExpr then_expr,
                                HIRExpr else_expr, const HIRType &result_type);
    static HIRExpr make_struct_literal(const std::string &struct_name,
                                       std::vector<std::string> field_names,
                                       std::vector<HIRExpr> field_values);
    static HIRExpr make_array_literal(std::vector<HIRExpr> elements,
                                      const HIRType &array_type);
    static HIRExpr make_address_of(HIRExpr expr, const HIRType &pointer_type);
    static HIRExpr make_dereference(HIRExpr expr, const HIRType &value_type);
    static HIRExpr make_sizeof(const HIRType &type);
    static HIRExpr make_new(const HIRType &type, std::vector<HIRExpr> args);
    static HIRExpr make_await(HIRExpr expr, const HIRType &result_type);

    // 文の構築
    static HIRStmt make_var_decl(const std::string &name, const HIRType &type,
                                 bool is_const, HIRExpr init_expr);
    static HIRStmt make_assignment(HIRExpr lhs, HIRExpr rhs);
    static HIRStmt make_expr_stmt(HIRExpr expr);
    static HIRStmt make_if(HIRExpr condition, HIRStmt then_body,
                           HIRStmt else_body);
    static HIRStmt make_while(HIRExpr condition, HIRStmt body);
    static HIRStmt make_for(HIRStmt init, HIRExpr condition, HIRStmt update,
                            HIRStmt body);
    static HIRStmt make_return(HIRExpr expr);
    static HIRStmt make_break();
    static HIRStmt make_continue();
    static HIRStmt make_block(std::vector<HIRStmt> stmts);
    static HIRStmt make_defer(HIRStmt stmt);
    static HIRStmt make_delete(HIRExpr expr);

    // 型の構築
    static HIRType make_basic_type(HIRType::TypeKind kind);
    static HIRType make_pointer_type(const HIRType &inner);
    static HIRType make_reference_type(const HIRType &inner);
    static HIRType make_array_type(const HIRType &element_type, int size = -1);
    static HIRType make_struct_type(const std::string &name);
    static HIRType make_enum_type(const std::string &name);
    static HIRType make_interface_type(const std::string &name);
    static HIRType make_function_type(std::vector<HIRType> param_types,
                                      const HIRType &return_type);
    static HIRType make_generic_type(const std::string &name);
    static HIRType make_optional_type(const HIRType &inner);

  private:
    // ソース位置情報（デフォルト）
    static SourceLocation default_location();
};

} // namespace hir
} // namespace ir
} // namespace cb
