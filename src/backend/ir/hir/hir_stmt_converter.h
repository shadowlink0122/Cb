#pragma once

#include "../../../common/ast.h"
#include "hir_node.h"
#include <memory>
#include <string>

namespace cb {
namespace ir {

class HIRGenerator;

class HIRStmtConverter {
  public:
    explicit HIRStmtConverter(HIRGenerator *generator);
    ~HIRStmtConverter();
    
    hir::HIRStmt convert_stmt(const ASTNode *node);

  private:
    HIRGenerator *generator_;
};

} // namespace ir
} // namespace cb
