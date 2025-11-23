// v0.14.0: HIR Builder Implementation
// HIRノードを簡単に構築するためのビルダーパターン実装

#include "hir_builder.h"

namespace cb {
namespace ir {
namespace hir {

SourceLocation HIRBuilder::default_location() {
    return SourceLocation("", 0, 0);
}

// === 式の構築 ===

HIRExpr HIRBuilder::make_literal(const std::string &value,
                                 const HIRType &type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::Literal;
    expr.literal_value = value;
    expr.literal_type = type;
    expr.type = type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_variable(const std::string &name,
                                  const HIRType &type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::Variable;
    expr.var_name = name;
    expr.type = type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_binary_op(const std::string &op, HIRExpr left,
                                   HIRExpr right, const HIRType &result_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::BinaryOp;
    expr.op = op;
    expr.left = std::make_unique<HIRExpr>(std::move(left));
    expr.right = std::make_unique<HIRExpr>(std::move(right));
    expr.type = result_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_unary_op(const std::string &op, HIRExpr operand,
                                  const HIRType &result_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::UnaryOp;
    expr.op = op;
    expr.operand = std::make_unique<HIRExpr>(std::move(operand));
    expr.type = result_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_function_call(const std::string &func_name,
                                       std::vector<HIRExpr> args,
                                       const HIRType &return_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::FunctionCall;
    expr.func_name = func_name;
    expr.arguments = std::move(args);
    expr.type = return_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_method_call(HIRExpr receiver,
                                     const std::string &method_name,
                                     std::vector<HIRExpr> args,
                                     const HIRType &return_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::MethodCall;
    expr.receiver = std::make_unique<HIRExpr>(std::move(receiver));
    expr.method_name = method_name;
    expr.arguments = std::move(args);
    expr.type = return_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_member_access(HIRExpr object,
                                       const std::string &member_name,
                                       bool is_arrow,
                                       const HIRType &member_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::MemberAccess;
    expr.object = std::make_unique<HIRExpr>(std::move(object));
    expr.member_name = member_name;
    expr.is_arrow = is_arrow;
    expr.type = member_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_array_access(HIRExpr array, HIRExpr index,
                                      const HIRType &element_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::ArrayAccess;
    expr.array = std::make_unique<HIRExpr>(std::move(array));
    expr.index = std::make_unique<HIRExpr>(std::move(index));
    expr.type = element_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_cast(HIRExpr expr_to_cast,
                              const HIRType &target_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::Cast;
    expr.cast_expr = std::make_unique<HIRExpr>(std::move(expr_to_cast));
    expr.cast_type = target_type;
    expr.type = target_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_ternary(HIRExpr condition, HIRExpr then_expr,
                                 HIRExpr else_expr,
                                 const HIRType &result_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::Ternary;
    expr.condition = std::make_unique<HIRExpr>(std::move(condition));
    expr.then_expr = std::make_unique<HIRExpr>(std::move(then_expr));
    expr.else_expr = std::make_unique<HIRExpr>(std::move(else_expr));
    expr.type = result_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_struct_literal(const std::string &struct_name,
                                        std::vector<std::string> field_names,
                                        std::vector<HIRExpr> field_values) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::StructLiteral;
    expr.struct_type_name = struct_name;
    expr.field_names = std::move(field_names);
    expr.field_values = std::move(field_values);
    expr.type = make_struct_type(struct_name);
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_array_literal(std::vector<HIRExpr> elements,
                                       const HIRType &array_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::ArrayLiteral;
    expr.array_elements = std::move(elements);
    expr.type = array_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_address_of(HIRExpr expr_val,
                                    const HIRType &pointer_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::AddressOf;
    expr.operand = std::make_unique<HIRExpr>(std::move(expr_val));
    expr.type = pointer_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_dereference(HIRExpr expr_val,
                                     const HIRType &value_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::Dereference;
    expr.operand = std::make_unique<HIRExpr>(std::move(expr_val));
    expr.type = value_type;
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_sizeof(const HIRType &type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::SizeOf;
    expr.sizeof_type = type;
    expr.type = make_basic_type(
        HIRType::TypeKind::Long); // sizeof returns size_t (long)
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_new(const HIRType &type, std::vector<HIRExpr> args) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::New;
    expr.new_type = type;
    expr.new_args = std::move(args);
    expr.type = make_pointer_type(type);
    expr.location = default_location();
    return expr;
}

HIRExpr HIRBuilder::make_await(HIRExpr expr_val, const HIRType &result_type) {
    HIRExpr expr;
    expr.kind = HIRExpr::ExprKind::Await;
    expr.operand = std::make_unique<HIRExpr>(std::move(expr_val));
    expr.type = result_type;
    expr.location = default_location();
    return expr;
}

// === 文の構築 ===

HIRStmt HIRBuilder::make_var_decl(const std::string &name, const HIRType &type,
                                  bool is_const, HIRExpr init_expr) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::VarDecl;
    stmt.var_name = name;
    stmt.var_type = type;
    stmt.is_const = is_const;
    stmt.init_expr = std::make_unique<HIRExpr>(std::move(init_expr));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_assignment(HIRExpr lhs, HIRExpr rhs) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::Assignment;
    stmt.lhs = std::make_unique<HIRExpr>(std::move(lhs));
    stmt.rhs = std::make_unique<HIRExpr>(std::move(rhs));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_expr_stmt(HIRExpr expr) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::ExprStmt;
    stmt.expr = std::make_unique<HIRExpr>(std::move(expr));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_if(HIRExpr condition, HIRStmt then_body,
                            HIRStmt else_body) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::If;
    stmt.condition = std::make_unique<HIRExpr>(std::move(condition));
    stmt.then_body = std::make_unique<HIRStmt>(std::move(then_body));
    stmt.else_body = std::make_unique<HIRStmt>(std::move(else_body));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_while(HIRExpr condition, HIRStmt body) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::While;
    stmt.condition = std::make_unique<HIRExpr>(std::move(condition));
    stmt.body = std::make_unique<HIRStmt>(std::move(body));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_for(HIRStmt init, HIRExpr condition, HIRStmt update,
                             HIRStmt body) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::For;
    stmt.init = std::make_unique<HIRStmt>(std::move(init));
    stmt.condition = std::make_unique<HIRExpr>(std::move(condition));
    stmt.update = std::make_unique<HIRStmt>(std::move(update));
    stmt.body = std::make_unique<HIRStmt>(std::move(body));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_return(HIRExpr expr) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::Return;
    stmt.return_expr = std::make_unique<HIRExpr>(std::move(expr));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_break() {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::Break;
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_continue() {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::Continue;
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_block(std::vector<HIRStmt> stmts) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::Block;
    stmt.block_stmts = std::move(stmts);
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_defer(HIRStmt deferred_stmt) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::Defer;
    stmt.defer_stmt = std::make_unique<HIRStmt>(std::move(deferred_stmt));
    stmt.location = default_location();
    return stmt;
}

HIRStmt HIRBuilder::make_delete(HIRExpr expr) {
    HIRStmt stmt;
    stmt.kind = HIRStmt::StmtKind::Delete;
    stmt.delete_expr = std::make_unique<HIRExpr>(std::move(expr));
    stmt.location = default_location();
    return stmt;
}

// === 型の構築 ===

HIRType HIRBuilder::make_basic_type(HIRType::TypeKind kind) {
    HIRType type;
    type.kind = kind;
    return type;
}

HIRType HIRBuilder::make_pointer_type(const HIRType &inner) {
    HIRType type;
    type.kind = HIRType::TypeKind::Pointer;
    type.inner_type = std::make_unique<HIRType>(inner);
    return type;
}

HIRType HIRBuilder::make_reference_type(const HIRType &inner) {
    HIRType type;
    type.kind = HIRType::TypeKind::Reference;
    type.inner_type = std::make_unique<HIRType>(inner);
    return type;
}

HIRType HIRBuilder::make_array_type(const HIRType &element_type, int size) {
    HIRType type;
    type.kind = HIRType::TypeKind::Array;
    type.inner_type = std::make_unique<HIRType>(element_type);
    type.array_size = size;
    return type;
}

HIRType HIRBuilder::make_struct_type(const std::string &name) {
    HIRType type;
    type.kind = HIRType::TypeKind::Struct;
    type.name = name;
    return type;
}

HIRType HIRBuilder::make_enum_type(const std::string &name) {
    HIRType type;
    type.kind = HIRType::TypeKind::Enum;
    type.name = name;
    return type;
}

HIRType HIRBuilder::make_interface_type(const std::string &name) {
    HIRType type;
    type.kind = HIRType::TypeKind::Interface;
    type.name = name;
    return type;
}

HIRType HIRBuilder::make_function_type(std::vector<HIRType> param_types,
                                       const HIRType &return_type) {
    HIRType type;
    type.kind = HIRType::TypeKind::Function;
    type.param_types = std::move(param_types);
    type.return_type = std::make_unique<HIRType>(return_type);
    return type;
}

HIRType HIRBuilder::make_generic_type(const std::string &name) {
    HIRType type;
    type.kind = HIRType::TypeKind::Generic;
    type.name = name;
    return type;
}

HIRType HIRBuilder::make_optional_type(const HIRType &inner) {
    HIRType type;
    type.kind = HIRType::TypeKind::Optional;
    type.inner_type = std::make_unique<HIRType>(inner);
    return type;
}

} // namespace hir
} // namespace ir
} // namespace cb
