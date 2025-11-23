#pragma once

#include "../../../common/ast.h"
#include "hir_node.h"
#include <memory>
#include <string>

namespace cb {
namespace ir {

class HIRGenerator;

class HIRDeclTypeConverter {
  public:
    explicit HIRDeclTypeConverter(HIRGenerator *generator);
    ~HIRDeclTypeConverter();

    hir::HIRFunction convert_function(const ASTNode *node);
    hir::HIRStruct convert_struct(const ASTNode *node);
    hir::HIREnum convert_enum(const ASTNode *node);
    hir::HIRUnion convert_union(const ASTNode *node);
    hir::HIRInterface convert_interface(const ASTNode *node);
    hir::HIRImpl convert_impl(const ASTNode *node);
    hir::HIRType convert_type(TypeInfo type_info,
                              const std::string &type_name = "");
    hir::HIRType convert_array_type(const ArrayTypeInfo &array_info);

  private:
    HIRGenerator *generator_;

    // v0.14.0: Helper to resolve array size (handles both literals and const
    // variables)
    int resolve_array_size(const std::string &size_str);
};

} // namespace ir
} // namespace cb
