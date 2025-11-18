// v0.14.0: HIR to C++ Transpiler
// HIRからC++コードを生成するバックエンド

#pragma once

#include "../ir/hir/hir_node.h"
#include <sstream>
#include <string>
#include <unordered_map>

namespace cb {
namespace codegen {

// HIR → C++トランスパイラ
class HIRToCpp {
  public:
    HIRToCpp();
    ~HIRToCpp();

    // HIRProgramからC++コードを生成
    std::string generate(const ir::hir::HIRProgram &program);

  private:
    std::stringstream output;
    int indent_level = 0;
    const ir::hir::HIRProgram *current_program =
        nullptr; // 現在のプログラム参照
    std::unordered_map<std::string, ir::hir::HIRType>
        current_function_params;            // 現在の関数パラメータ
    bool current_function_is_async = false; // 現在の関数がasyncかどうか
    ir::hir::HIRType current_function_return_type; // 現在の関数の戻り値型
    std::vector<std::string> current_generic_params; // 現在のジェネリック型パラメータ
                                                     // (T, K, V, etc.)

    // ヘルパーメソッド
    void emit(const std::string &code);
    void emit_line(const std::string &code);
    void emit_indent();
    void increase_indent();
    void decrease_indent();

    // トップレベル定義の生成
    void generate_imports(const ir::hir::HIRProgram &program);
    void generate_typedefs(const std::vector<ir::hir::HIRTypedef> &typedefs);
    void generate_foreign_functions(
        const std::vector<ir::hir::HIRForeignFunction> &foreign_funcs);
    void generate_forward_declarations(const ir::hir::HIRProgram &program);
    void generate_structs(const std::vector<ir::hir::HIRStruct> &structs);
    void generate_enums(const std::vector<ir::hir::HIREnum> &enums);
    void
    generate_interfaces(const std::vector<ir::hir::HIRInterface> &interfaces);
    void
    generate_global_vars(const std::vector<ir::hir::HIRGlobalVar> &globals);
    void generate_functions(const std::vector<ir::hir::HIRFunction> &functions);
    void generate_impls(const std::vector<ir::hir::HIRImpl> &impls);

    // 個別の定義生成
    void generate_struct(const ir::hir::HIRStruct &struct_def);
    void generate_enum(const ir::hir::HIREnum &enum_def);
    void generate_function(const ir::hir::HIRFunction &func);
    void generate_impl(const ir::hir::HIRImpl &impl);

    // 文の生成
    void generate_stmt(const ir::hir::HIRStmt &stmt);
    void generate_var_decl(const ir::hir::HIRStmt &stmt);
    void generate_assignment(const ir::hir::HIRStmt &stmt);
    void generate_if(const ir::hir::HIRStmt &stmt);
    void generate_while(const ir::hir::HIRStmt &stmt);
    void generate_for(const ir::hir::HIRStmt &stmt);
    void generate_return(const ir::hir::HIRStmt &stmt);
    void generate_block(const ir::hir::HIRStmt &stmt);
    void generate_switch(const ir::hir::HIRStmt &stmt);
    void generate_defer(const ir::hir::HIRStmt &stmt);
    void generate_delete(const ir::hir::HIRStmt &stmt);
    void generate_try_catch(const ir::hir::HIRStmt &stmt);

    // 式の生成
    std::string generate_expr(const ir::hir::HIRExpr &expr);
    std::string generate_literal(const ir::hir::HIRExpr &expr);
    std::string generate_variable(const ir::hir::HIRExpr &expr);
    std::string generate_binary_op(const ir::hir::HIRExpr &expr);
    std::string generate_unary_op(const ir::hir::HIRExpr &expr);
    std::string generate_function_call(const ir::hir::HIRExpr &expr);
    std::string generate_method_call(const ir::hir::HIRExpr &expr);
    std::string generate_member_access(const ir::hir::HIRExpr &expr);
    std::string generate_array_access(const ir::hir::HIRExpr &expr);
    std::string generate_cast(const ir::hir::HIRExpr &expr);
    std::string generate_ternary(const ir::hir::HIRExpr &expr);
    std::string generate_lambda(const ir::hir::HIRExpr &expr);
    std::string generate_struct_literal(const ir::hir::HIRExpr &expr);
    std::string generate_array_literal(const ir::hir::HIRExpr &expr);
    std::string generate_address_of(const ir::hir::HIRExpr &expr);
    std::string generate_dereference(const ir::hir::HIRExpr &expr);
    std::string generate_sizeof(const ir::hir::HIRExpr &expr);
    std::string generate_new(const ir::hir::HIRExpr &expr);
    std::string generate_await(const ir::hir::HIRExpr &expr);
    std::string generate_pre_incdec(const ir::hir::HIRExpr &expr);
    std::string generate_post_incdec(const ir::hir::HIRExpr &expr);

    // 文の生成（追加）
    void generate_assert(const ir::hir::HIRStmt &stmt);

    // 型の生成
    std::string generate_type(const ir::hir::HIRType &type);
    std::string generate_basic_type(const ir::hir::HIRType &type);
    std::string generate_pointer_type(const ir::hir::HIRType &type);
    std::string generate_reference_type(const ir::hir::HIRType &type);
    std::string generate_array_type(const ir::hir::HIRType &type);
    std::string generate_function_type(const ir::hir::HIRType &type);

    // ユーティリティ
    std::string escape_string(const std::string &str);
    std::string
    mangle_generic_name(const std::string &base_name,
                        const std::vector<ir::hir::HIRType> &generic_args);
    std::string add_hir_prefix(
        const std::string &name); // v0.14.0: HIR変数名にプレフィックス追加
};

} // namespace codegen
} // namespace cb
