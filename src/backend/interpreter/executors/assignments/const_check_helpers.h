#ifndef CONST_CHECK_HELPERS_H
#define CONST_CHECK_HELPERS_H

#include "core/interpreter.h"

namespace AssignmentHelpers {

/**
 * constポインタ経由での値変更をチェック
 * const T* 経由で *ptr = value や (*ptr).member = value を禁止する
 */
inline void check_const_pointer_modification(Interpreter &interpreter,
                                             const ASTNode *ptr_node) {
    if (ptr_node && ptr_node->node_type == ASTNodeType::AST_VARIABLE) {
        Variable *ptr_var = interpreter.find_variable(ptr_node->name);
        if (ptr_var && ptr_var->is_pointee_const) {
            throw std::runtime_error(
                "Cannot modify value through pointer to const (const T*)");
        }
    }
}

/**
 * constポインタ自体への再代入をチェック
 * T* const ptr の場合、ptr = ... を禁止する
 */
inline void check_const_pointer_reassignment(const Variable *target_var) {
    if (target_var && target_var->is_pointer && target_var->is_pointer_const) {
        throw std::runtime_error("Cannot reassign const pointer (T* const)");
    }
}

} // namespace AssignmentHelpers

#endif // CONST_CHECK_HELPERS_H
