#pragma once

#include "../../../common/ast.h"
#include "hir_node.h"
#include <memory>
#include <string>

namespace cb {
namespace ir {

class HIRGenerator;

class HIRExprConverter {
  public:
    explicit HIRExprConverter(HIRGenerator *generator);
    ~HIRExprConverter();
    
    hir::HIRExpr convert_expr(const ASTNode *node);

  private:
    HIRGenerator *generator_;
};

} // namespace ir
} // namespace cb
