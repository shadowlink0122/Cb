#pragma once

#include "../../../common/ast.h"
#include "hir_node.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace cb {
namespace ir {

// HIRGenerator: ASTからHIRへの変換を行うクラス
class HIRGenerator {
  public:
    HIRGenerator();
    ~HIRGenerator();

    // ASTからHIRProgramを生成
    std::unique_ptr<hir::HIRProgram>
    generate(const std::vector<std::unique_ptr<ASTNode>> &ast_nodes);

    // For testing purposes
    friend class HIRGeneratorTest;

  private:
    // 式の変換
    hir::HIRExpr convert_expr(const ASTNode *node);

    // 文の変換
    hir::HIRStmt convert_stmt(const ASTNode *node);

    // 関数定義の変換
    hir::HIRFunction convert_function(const ASTNode *node);

    // 構造体定義の変換
    hir::HIRStruct convert_struct(const ASTNode *node);

    // Enum定義の変換
    hir::HIREnum convert_enum(const ASTNode *node);

    // Interface定義の変換
    hir::HIRInterface convert_interface(const ASTNode *node);

    // Impl定義の変換
    hir::HIRImpl convert_impl(const ASTNode *node);

    // 型情報の変換
    hir::HIRType convert_type(TypeInfo type_info,
                              const std::string &type_name = "");

    // ソース位置情報の変換
    SourceLocation convert_location(const ::SourceLocation &ast_loc);

    // エラーレポート
    void report_error(const std::string &message,
                      const ::SourceLocation &location);

    // 変数IDカウンタ（SSA形式のための準備）
    uint32_t next_var_id = 0;

    // エラーカウンタ
    int error_count = 0;
};

} // namespace ir
} // namespace cb
