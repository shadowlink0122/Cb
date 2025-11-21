#include "executors/statement_executor.h"
#include "../../../common/debug.h"
#include "../../../common/type_alias.h"
#include "../../../common/type_helpers.h"
#include "core/error_handler.h"
#include "core/interpreter.h"
#include "core/pointer_metadata.h"
#include "core/type_inference.h"
#include "evaluator/core/evaluator.h"
#include "event_loop/simple_event_loop.h" // v0.12.0: バックグラウンドタスク実行
#include "executors/assignments/member_assignment.h"
#include "executors/assignments/simple_assignment.h"
#include "executors/declarations/array_declaration.h"
#include "executors/declarations/variable_declaration.h"
#include "managers/arrays/manager.h"
#include "managers/types/manager.h"
#include "managers/variables/manager.h"
#include "services/array_processing_service.h"
#include <inttypes.h>

StatementExecutor::StatementExecutor(Interpreter &interpreter)
    : interpreter_(interpreter) {}

void StatementExecutor::execute_statement(const ASTNode *node) {
    execute(node);

    // v0.12.0: バックグラウンドタスクを1サイクル実行
    // async関数呼び出し（awaitなし）時に、ラウンドロビンでタスクを進める
    if (interpreter_.get_simple_event_loop().has_tasks()) {
        interpreter_.get_simple_event_loop().run_one_cycle();
    }
}

void StatementExecutor::execute(const ASTNode *node) {
    if (!node)
        return;

    // ASTNodeTypeが異常な値でないことを確認
    int node_type_int = static_cast<int>(node->node_type);
    if (node_type_int < 0 || node_type_int > 100) {
        if (debug_mode) {
            std::cerr << "[CRITICAL] Abnormal node_type detected in "
                         "StatementExecutor: "
                      << node_type_int
                      << " (ptr: " << static_cast<const void *>(node) << ")"
                      << std::endl;
            std::cerr << "[CRITICAL] Node name: '" << node->name << "'"
                      << std::endl;
        }
        return;
    }

    if (debug_mode) {
        std::cerr << "[DEBUG_EXECUTE] Executing node type: " << node_type_int
                  << std::endl;
    }

    switch (node->node_type) {
    case ASTNodeType::AST_ASSIGN: {
        execute_assignment(node);
        break;
    }
    case ASTNodeType::AST_VAR_DECL: {
        // Debug output removed - use --debug option if needed
        execute_variable_declaration(node);
        break;
    }
    case ASTNodeType::AST_MULTIPLE_VAR_DECL: {
        execute_multiple_var_decl(node);
        break;
    }
    case ASTNodeType::AST_ARRAY_DECL: {
        execute_array_decl(node);
        break;
    }
    case ASTNodeType::AST_PRE_INCDEC:
    case ASTNodeType::AST_POST_INCDEC: {
        // インクリメント/デクリメントをステートメントとして実行
        // expression_evaluatorで評価するだけで副作用（変数の変更）が発生する
        interpreter_.evaluate(node);
        break;
    }
    // 他のstatement types（AST_FUNC_DECL, AST_IF_STMT等）は
    // Interpreterクラスで直接処理されるため、ここでは未対応
    default:
        // StatementExecutorが対応していないノード型は
        // Interpreterで処理される想定
        break;
    }
}

void StatementExecutor::execute_assignment(const ASTNode *node) {
    AssignmentHandlers::execute_assignment(this, interpreter_, node);
}

void StatementExecutor::execute_variable_declaration(const ASTNode *node) {
    DeclarationHandlers::execute_variable_declaration(this, interpreter_, node);
}

void StatementExecutor::execute_multiple_var_decl(const ASTNode *node) {
    DeclarationHandlers::execute_multiple_var_decl(this, interpreter_, node);
}

void StatementExecutor::execute_array_decl(const ASTNode *node) {
    DeclarationHandlers::execute_array_decl(this, interpreter_, node);
}

void StatementExecutor::execute_struct_array_literal_init(
    const std::string &array_name, const ASTNode *array_literal,
    const std::string &struct_type) {
    DeclarationHandlers::execute_struct_array_literal_init(
        interpreter_, array_name, array_literal, struct_type);
}

void StatementExecutor::execute_member_array_assignment(const ASTNode *node) {
    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "DEBUG: execute_member_array_assignment called");
    // obj.member[index] = value の処理
    const ASTNode *member_array_access = node->left.get();

    if (!member_array_access || member_array_access->node_type !=
                                    ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "DEBUG: Not AST_MEMBER_ARRAY_ACCESS, node_type=%d",
                     member_array_access
                         ? static_cast<int>(member_array_access->node_type)
                         : -1);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        throw std::runtime_error("Invalid member array access in assignment");
    }

    // オブジェクト名を取得
    std::string obj_name;
    std::string array_member_name; // obj.array[idx].member の "array" 部分
    bool is_nested_struct_array_access = false;

    if (member_array_access->left &&
        (member_array_access->left->node_type == ASTNodeType::AST_VARIABLE ||
         member_array_access->left->node_type == ASTNodeType::AST_IDENTIFIER)) {
        obj_name = member_array_access->left->name;
    } else if (member_array_access->left &&
               member_array_access->left->node_type ==
                   ASTNodeType::AST_MEMBER_ACCESS) {
        // 2つのケースをチェック:
        // 1. s.grades[0] = 85 (構造体メンバーの配列へのアクセス)
        // 2. triangle.points[0].x = 1
        // (構造体配列メンバーの要素へのメンバーアクセス)

        if (member_array_access->left->left &&
            (member_array_access->left->left->node_type ==
                 ASTNodeType::AST_VARIABLE ||
             member_array_access->left->left->node_type ==
                 ASTNodeType::AST_IDENTIFIER)) {
            obj_name = member_array_access->left->left->name;
            array_member_name = member_array_access->left->name;

            // member_array_access->name が設定されている場合、これは
            // obj.array[idx].member のパターン
            if (!member_array_access->name.empty() &&
                member_array_access->name != array_member_name) {
                is_nested_struct_array_access = true;
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "DEBUG: Detected nested struct array member ");
            }
        } else {
            debug_msg(DebugMsgId::GENERIC_DEBUG, "ERROR: Nested ");
            throw std::runtime_error(
                "Invalid nested object reference in member array access");
        }
    } else {
        if (member_array_access->left) {
            {
                char dbg_buf[512];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "ERROR: member_array_access->left->node_type = %d",
                    static_cast<int>(member_array_access->left->node_type));
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        } else {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "ERROR: member_array_access->left is null");
        }
        throw std::runtime_error(
            "Invalid object reference in member array access");
    }

    // メンバ名を取得
    std::string member_name;
    if (is_nested_struct_array_access) {
        // triangle.points[0].x = 1 の場合
        member_name = member_array_access->name; // "x"
    } else if (member_array_access->left &&
               member_array_access->left->node_type ==
                   ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストされた場合: s.grades[0] の "grades" 部分
        member_name = member_array_access->left->name;
    } else {
        // 直接の場合
        member_name = member_array_access->name;
    }

    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "DEBUG: obj_name='%s', member_name='%s', array_member='%s', ");

    // インデックス値を評価（多次元対応）
    std::vector<int64_t> indices;
    if (member_array_access->right) {
        // 1次元の場合（従来通り）
        int64_t index = interpreter_.evaluate(member_array_access->right.get());
        indices.push_back(index);
    } else if (!member_array_access->arguments.empty()) {
        // 多次元の場合
        for (const auto &arg : member_array_access->arguments) {
            int64_t index = interpreter_.evaluate(arg.get());
            indices.push_back(index);
        }
    } else {
        throw std::runtime_error(
            "No indices found for array access in member array assignment");
    }

    // ネストされた構造体配列メンバーアクセスの処理: obj.array[idx].member =
    // value
    if (is_nested_struct_array_access) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "DEBUG: Processing nested struct array member assignment");
        int array_index = static_cast<int>(indices[0]);

        // array_member_name の配列から要素を取得
        Variable *array_member =
            interpreter_.get_struct_member(obj_name, array_member_name);
        if (!array_member) {
            throw std::runtime_error("Struct member not found: " +
                                     array_member_name);
        }

        if (!array_member->is_array) {
            throw std::runtime_error("Member is not an array: " +
                                     array_member_name);
        }

        // 配列インデックスの境界チェック
        if (array_index < 0 || array_index >= array_member->array_size) {
            throw std::runtime_error("Array index out of bounds: " +
                                     std::to_string(array_index));
        }

        // struct_members内で配列要素にアクセス
        // 構造体配列の要素は "array_member_name[index]"
        // という名前で個別に格納されている まず array_member の struct_members
        // を調べる
        std::string element_key =
            array_member_name + "[" + std::to_string(array_index) + "]";

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "DEBUG: Looking for struct array element: %s",
                     element_key.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        // 親構造体から配列要素を探す（最初に親のstruct_membersを確認）
        Variable *parent_struct = interpreter_.find_variable(obj_name);
        // v0.11.0: enum型もチェック
        if (!parent_struct ||
            (!parent_struct->is_struct && !parent_struct->is_enum)) {
            throw std::runtime_error(
                "Parent variable is not a struct or enum: " + obj_name);
        }

        // 配列要素の構造体を探す - まず親のstruct_membersから
        auto element_it = parent_struct->struct_members.find(element_key);
        if (element_it == parent_struct->struct_members.end()) {
            // 配列メンバー自体のstruct_membersから探す
            element_it = array_member->struct_members.find(element_key);
            if (element_it == array_member->struct_members.end()) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "DEBUG: Available keys in parent struct_members:");
                for (const auto &pair : parent_struct->struct_members) {
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf), "  - %s",
                                 pair.first.c_str());
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                }
                debug_msg(
                    DebugMsgId::GENERIC_DEBUG,
                    "DEBUG: Available keys in array_member struct_members:");
                for (const auto &pair : array_member->struct_members) {
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf), "  - %s",
                                 pair.first.c_str());
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                }
                throw std::runtime_error("Struct array element not found: " +
                                         element_key);
            }
        }

        Variable &struct_element = element_it->second;
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "DEBUG: Found struct array element, is_struct=%s, ");
        if (!struct_element.is_struct) {
            throw std::runtime_error("Array element is not a struct");
        }

        // 構造体要素のメンバーに値を代入
        auto member_it = struct_element.struct_members.find(member_name);
        if (member_it == struct_element.struct_members.end()) {
            throw std::runtime_error(
                "Struct member not found in array element: " + member_name);
        }

        // 右辺の値を評価して代入
        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            member_it->second.str_value = node->right->str_value;
            member_it->second.type = TYPE_STRING;
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "DEBUG_ASSIGN: Assigned string '%s' to %s.%s[%d].%s",
                         node->right->str_value.c_str(), obj_name.c_str(),
                         array_member_name.c_str(), array_index,
                         member_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        } else {
            TypedValue typed_value =
                interpreter_.evaluate_typed(node->right.get());
            if (typed_value.is_floating()) {
                member_it->second.double_value = typed_value.as_double();
                member_it->second.type = typed_value.type.type_info;
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "DEBUG_ASSIGN: Assigned double %f to %s.%s[%d].%s",
                             typed_value.as_double(), obj_name.c_str(),
                             array_member_name.c_str(), array_index,
                             member_name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            } else {
                int64_t value = typed_value.as_numeric();
                member_it->second.value = value;
                member_it->second.type = typed_value.type.type_info;
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "DEBUG_ASSIGN: Assigned integer %" PRId64
                             " to %s.%s[%d].%s",
                             value, obj_name.c_str(), array_member_name.c_str(),
                             array_index, member_name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
        }
        member_it->second.is_assigned = true;

        // ダイレクトアクセス変数も更新
        std::string direct_access_name =
            obj_name + "." + element_key + "." + member_name;
        Variable *direct_var = interpreter_.find_variable(direct_access_name);
        if (direct_var) {
            if (TypeHelpers::isString(member_it->second.type)) {
                direct_var->str_value = member_it->second.str_value;
            } else if (member_it->second.type == TYPE_FLOAT ||
                       member_it->second.type == TYPE_DOUBLE) {
                direct_var->double_value = member_it->second.double_value;
            } else {
                direct_var->value = member_it->second.value;
            }
            direct_var->type = member_it->second.type;
            direct_var->is_assigned = true;
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "DEBUG_ASSIGN: Updated direct access variable: %s = "
                         "%" PRId64,
                         direct_access_name.c_str(), direct_var->value);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }

        // 構造体配列要素変数自体の struct_members も更新
        std::string element_var_name = obj_name + "." + element_key;
        Variable *element_variable =
            interpreter_.find_variable(element_var_name);
        if (element_variable && element_variable->is_struct) {
            auto elem_member_it =
                element_variable->struct_members.find(member_name);
            if (elem_member_it != element_variable->struct_members.end()) {
                elem_member_it->second = member_it->second;
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "DEBUG_ASSIGN: Updated element variable ");
            }
        }

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "DEBUG: Nested struct array member assigned: %s.%s[%d].%s",
                     obj_name.c_str(), array_member_name.c_str(), array_index,
                     member_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        return;
    }

    // 右辺の値を評価して構造体メンバー配列要素に代入
    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "DEBUG: execute_member_array_assignment - right type=%d, ");

    if (indices.size() > 1) {
        // 多次元配列の場合
        Variable *member_var =
            interpreter_.get_struct_member(obj_name, member_name);
        if (!member_var) {
            throw std::runtime_error("Struct member not found: " + member_name);
        }

        if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
            interpreter_.setMultidimensionalStringArrayElement(
                *member_var, indices, node->right->str_value);
        } else {
            int64_t value = interpreter_.evaluate(node->right.get());
            interpreter_.setMultidimensionalArrayElement(*member_var, indices,
                                                         value);
        }
        return; // 多次元処理完了
    }

    // 1次元配列の場合（従来処理）
    int index = static_cast<int>(indices[0]);
    if (node->right->node_type == ASTNodeType::AST_STRING_LITERAL) {
        interpreter_.assign_struct_member_array_element(
            obj_name, member_name, index, node->right->str_value);
    } else if (node->right->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体メンバ配列アクセスがAST_ARRAY_REFとして解析される場合
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "DEBUG: Processing AST_ARRAY_REF on right-hand side in ");
        if (node->right->left &&
            node->right->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // original.tags[0] の形式
            std::string right_obj_name = node->right->left->left->name;
            std::string right_member_name = node->right->left->name;
            int64_t array_index =
                interpreter_.evaluate(node->right->array_index.get());

            Variable *right_member_var = interpreter_.get_struct_member(
                right_obj_name, right_member_name);
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "DEBUG: AST_ARRAY_REF right_member_var type=%d, "
                         "is_array=%d",
                         static_cast<int>(right_member_var->type),
                         right_member_var->is_array ? 1 : 0);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            if ((right_member_var->type == TYPE_STRING &&
                 right_member_var->is_array) ||
                right_member_var->type ==
                    static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "DEBUG: Using string array element access via ");
                std::string str_value =
                    interpreter_.get_struct_member_array_string_element(
                        right_obj_name, right_member_name,
                        static_cast<int>(array_index));
                interpreter_.assign_struct_member_array_element(
                    obj_name, member_name, index, str_value);
            } else {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "DEBUG: Using numeric array element access via ");
                int64_t value = interpreter_.get_struct_member_array_element(
                    right_obj_name, right_member_name,
                    static_cast<int>(array_index));
                interpreter_.assign_struct_member_array_element(
                    obj_name, member_name, index, value);
            }
        } else {
            // 通常の配列参照として処理
            int64_t value = interpreter_.evaluate(node->right.get());
            interpreter_.assign_struct_member_array_element(
                obj_name, member_name, index, value);
        }
    } else if (node->right->node_type == ASTNodeType::AST_MEMBER_ARRAY_ACCESS) {
        // 構造体メンバ配列アクセスの場合（original.tags[0]等）
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "DEBUG: Processing AST_MEMBER_ARRAY_ACCESS on right-hand ");
        std::string right_obj_name;
        std::string right_member_name = node->right->name;

        if (node->right->left->node_type == ASTNodeType::AST_VARIABLE) {
            right_obj_name = node->right->left->name;
        } else if (node->right->left->node_type == ASTNodeType::AST_ARRAY_REF) {
            // struct配列要素の場合
            std::string array_name = node->right->left->left->name;
            int64_t idx =
                interpreter_.evaluate(node->right->left->array_index.get());
            right_obj_name = array_name + "[" + std::to_string(idx) + "]";
        } else {
            throw std::runtime_error("Invalid right-hand member array access");
        }

        // インデックスを評価
        int64_t array_index = interpreter_.evaluate(node->right->right.get());

        // 右辺の構造体メンバ配列要素を取得
        Variable *right_member_var =
            interpreter_.get_struct_member(right_obj_name, right_member_name);
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "DEBUG: right_member_var type=%d, is_array=%d in array ");
        if ((right_member_var->type == TYPE_STRING &&
             right_member_var->is_array) ||
            right_member_var->type ==
                static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "DEBUG: Using string array element access in array ");
            std::string str_value =
                interpreter_.get_struct_member_array_string_element(
                    right_obj_name, right_member_name,
                    static_cast<int>(array_index));
            interpreter_.assign_struct_member_array_element(
                obj_name, member_name, index, str_value);
        } else {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "DEBUG: Using numeric array element access in array ");
            int64_t value = interpreter_.get_struct_member_array_element(
                right_obj_name, right_member_name,
                static_cast<int>(array_index));
            interpreter_.assign_struct_member_array_element(
                obj_name, member_name, index, value);
        }
    } else {
        int64_t value = interpreter_.evaluate(node->right.get());
        interpreter_.assign_struct_member_array_element(obj_name, member_name,
                                                        index, value);
    }
}

void StatementExecutor::execute_member_assignment(const ASTNode *node) {
    AssignmentHandlers::execute_member_assignment(this, interpreter_, node);
}

void StatementExecutor::execute_arrow_assignment(const ASTNode *node) {
    AssignmentHandlers::execute_arrow_assignment(this, interpreter_, node);
}
void StatementExecutor::execute_member_array_literal_assignment(
    const ASTNode *node) {
    // obj.member = [1, 2, 3] または array[index].member = [1, 2, 3] の処理
    const ASTNode *member_access = node->left.get();

    if (!member_access ||
        member_access->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        throw std::runtime_error(
            "Invalid member access in array literal assignment");
    }

    // オブジェクト名を取得
    std::string obj_name;
    if (member_access->left &&
        member_access->left->node_type == ASTNodeType::AST_VARIABLE) {
        // 通常の構造体変数: obj.member
        obj_name = member_access->left->name;
    } else if (member_access->left &&
               member_access->left->node_type == ASTNodeType::AST_ARRAY_REF) {
        // 構造体配列要素: array[index].member
        std::string array_name = member_access->left->left->name;
        int64_t index =
            interpreter_.evaluate(member_access->left->array_index.get());
        obj_name = array_name + "[" + std::to_string(index) + "]";
    } else {
        throw std::runtime_error(
            "Invalid object reference in member array literal assignment");
    }

    // メンバ名を取得
    std::string member_name = member_access->name;

    if (debug_mode) {
        std::cerr << "DEBUG: Member array literal assignment: " << obj_name
                  << "." << member_name << std::endl;
    }

    // 構造体メンバー配列への配列リテラル代入
    interpreter_.assign_struct_member_array_literal(obj_name, member_name,
                                                    node->right.get());
}

void StatementExecutor::execute_union_assignment(const std::string &var_name,
                                                 const ASTNode *value_node) {
    // union型変数への代入を実行
    auto &var = interpreter_.current_scope().variables[var_name];

    if (var.type != TYPE_UNION) {
        throw std::runtime_error("Variable is not a union type: " + var_name);
    }

    std::string union_type_name = var.type_name;

    // 値の型に応じて検証と代入を実行
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列値
        std::string str_value = value_node->str_value;
        if (interpreter_.get_type_manager()->is_value_allowed_for_union(
                union_type_name, str_value)) {
            var.str_value = str_value;
            var.current_type = TYPE_STRING;
        } else {
            throw std::runtime_error("String value '" + str_value +
                                     "' is not allowed for union type " +
                                     union_type_name);
        }
    } else if (value_node->node_type == ASTNodeType::AST_NUMBER) {
        // 数値
        int64_t int_value = value_node->int_value;
        if (interpreter_.get_type_manager()->is_value_allowed_for_union(
                union_type_name, int_value)) {
            var.value = int_value;
            var.current_type = TYPE_INT;
        } else {
            throw std::runtime_error(
                "Integer value " + std::to_string(int_value) +
                " is not allowed for union type " + union_type_name);
        }
    } else {
        // 式の評価
        try {
            // まず文字列として評価してみる
            if (value_node->node_type == ASTNodeType::AST_VARIABLE) {
                // 変数参照の場合、変数の値を取得
                auto &source_var =
                    interpreter_.current_scope().variables[value_node->name];
                if (source_var.current_type == TYPE_STRING) {
                    if (interpreter_.get_type_manager()
                            ->is_value_allowed_for_union(
                                union_type_name, source_var.str_value)) {
                        var.str_value = source_var.str_value;
                        var.current_type = TYPE_STRING;
                        return;
                    }
                } else {
                    int64_t int_value = source_var.value;
                    if (interpreter_.get_type_manager()
                            ->is_value_allowed_for_union(union_type_name,
                                                         int_value)) {
                        var.value = int_value;
                        var.current_type = TYPE_INT;
                        return;
                    }
                }
            }

            // 数値として評価
            int64_t int_value = interpreter_.evaluate(value_node);
            if (interpreter_.get_type_manager()->is_value_allowed_for_union(
                    union_type_name, int_value)) {
                var.value = int_value;
                var.current_type = TYPE_INT;
            } else {
                throw std::runtime_error("Value " + std::to_string(int_value) +
                                         " is not allowed for union type " +
                                         union_type_name);
            }
        } catch (const std::exception &e) {
            throw std::runtime_error(
                "Failed to assign value to union variable " + var_name + ": " +
                e.what());
        }
    }
}

void StatementExecutor::execute_self_member_assignment(
    const std::string &member_name, const ASTNode *value_node) {
    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_START, member_name.c_str());

    // まず、self変数を取得
    Variable *self_var = interpreter_.find_variable("self");
    if (!self_var) {
        throw std::runtime_error("Self variable not found");
    }

    // selfのstruct_membersからメンバーを取得
    auto it = self_var->struct_members.find(member_name);
    if (it == self_var->struct_members.end()) {
        // struct_membersになければ、直接アクセス変数を探す
        std::string self_member_path = "self." + member_name;
        Variable *self_member = interpreter_.find_variable(self_member_path);
        if (!self_member) {
            throw std::runtime_error("Self member not found: " + member_name);
        }
        // 見つかった場合は、そのまま使用
        it = self_var->struct_members.find(member_name);
        if (it == self_var->struct_members.end()) {
            throw std::runtime_error("Self member not found: " + member_name);
        }
    }

    Variable *self_member = &(it->second);

    // constメンバへの代入チェック
    if (self_member->is_const && self_member->is_assigned) {
        std::string self_member_path = "self." + member_name;
        error_msg(DebugMsgId::CONST_REASSIGN_ERROR, self_member_path.c_str());
        throw std::runtime_error("Cannot assign to const self member: " +
                                 member_name);
    }

    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_FOUND, member_name.c_str());

    // 元のレシーバー変数からselfメンバーのパスを取得
    Variable *receiver_info = interpreter_.find_variable("__self_receiver__");
    std::string original_receiver_path;

    if (self_var && receiver_info && !receiver_info->str_value.empty()) {
        original_receiver_path = receiver_info->str_value + "." + member_name;
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "SELF_ASSIGN_DEBUG: Original receiver path: %s",
                     original_receiver_path.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    // 値の型に応じて代入処理
    if (value_node->node_type == ASTNodeType::AST_STRING_LITERAL) {
        self_member->str_value = value_node->str_value;
        self_member->type = TYPE_STRING;
        self_member->is_assigned = true;

        // 元の変数のメンバーも同時に更新
        if (!original_receiver_path.empty()) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "SELF_ASSIGN_DEBUG: Looking for original member: %s",
                         original_receiver_path.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            Variable *original_member =
                interpreter_.find_variable(original_receiver_path);
            if (original_member) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "SELF_ASSIGN_DEBUG: Found original member, ");
                original_member->str_value = value_node->str_value;
                original_member->type = TYPE_STRING;
                original_member->is_assigned = true;
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "SELF_ASSIGN_SYNC: %s = \"%s\"",
                             original_receiver_path.c_str(),
                             value_node->str_value.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            } else {
                {
                    char dbg_buf[512];
                    snprintf(
                        dbg_buf, sizeof(dbg_buf),
                        "SELF_ASSIGN_DEBUG: Could not find original member: %s",
                        original_receiver_path.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
        }

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf), "SELF_ASSIGN: %s = \"%s\"",
                     member_name.c_str(), value_node->str_value.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    } else if (value_node->node_type == ASTNodeType::AST_VARIABLE ||
               value_node->node_type == ASTNodeType::AST_IDENTIFIER) {
        // 変数参照の場合
        Variable *source_var = interpreter_.find_variable(value_node->name);

        // 構造体の場合の特別処理
        if (source_var && source_var->type == TYPE_STRUCT) {
            if (debug_mode) {
                std::cerr
                    << "[SELF_ASSIGN_STRUCT] Assigning struct from variable: "
                    << value_node->name << " to self." << member_name
                    << std::endl;
            }

            // 構造体データをコピー
            bool was_const = self_member->is_const;
            bool was_unsigned = self_member->is_unsigned;
            *self_member = *source_var;
            self_member->is_const = was_const;
            self_member->is_unsigned = was_unsigned;
            self_member->is_assigned = true;

            // 元の変数のメンバーも同時に更新
            if (!original_receiver_path.empty()) {
                Variable *original_member =
                    interpreter_.find_variable(original_receiver_path);
                if (original_member) {
                    bool orig_was_const = original_member->is_const;
                    bool orig_was_unsigned = original_member->is_unsigned;
                    *original_member = *source_var;
                    original_member->is_const = orig_was_const;
                    original_member->is_unsigned = orig_was_unsigned;
                    original_member->is_assigned = true;
                }
            }

            // ダイレクトアクセス変数も更新
            interpreter_.sync_direct_access_from_struct_value(
                "self." + member_name, *self_member);

            if (debug_mode) {
                std::cerr << "[SELF_ASSIGN_STRUCT] Successfully assigned struct"
                          << std::endl;
            }
        } else if (source_var && source_var->type == TYPE_STRING) {
            self_member->str_value = source_var->str_value;
            self_member->type = TYPE_STRING;

            // 元の変数のメンバーも同時に更新
            if (!original_receiver_path.empty()) {
                {
                    char dbg_buf[512];
                    snprintf(
                        dbg_buf, sizeof(dbg_buf),
                        "SELF_ASSIGN_DEBUG: Looking for original member: %s",
                        original_receiver_path.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                Variable *original_member =
                    interpreter_.find_variable(original_receiver_path);
                if (original_member) {
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "SELF_ASSIGN_DEBUG: Found original member, ");
                    original_member->str_value = source_var->str_value;
                    original_member->type = TYPE_STRING;
                    original_member->is_assigned = true;
                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "SELF_ASSIGN_SYNC: %s = \"%s\" (from variable)",
                            original_receiver_path.c_str(),
                            source_var->str_value.c_str());
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                } else {
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "SELF_ASSIGN_DEBUG: Could not find original ");
                }
            }

            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "SELF_ASSIGN: %s = \"%s\" (from variable)",
                         member_name.c_str(), source_var->str_value.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        } else {
            // 右辺の型情報を取得
            bool is_nullptr =
                (value_node->node_type == ASTNodeType::AST_NULLPTR);

            int64_t value = interpreter_.evaluate(value_node);
            self_member->value = value;

            // nullptr の場合、または元の型が TYPE_POINTER の場合は型を保持
            if (self_member->type != TYPE_STRING && !is_nullptr &&
                self_member->type != TYPE_POINTER) {
                self_member->type = TYPE_INT; // デフォルトはint型
            }

            // 元の変数のメンバーも同時に更新
            if (!original_receiver_path.empty()) {
                {
                    char dbg_buf[512];
                    snprintf(
                        dbg_buf, sizeof(dbg_buf),
                        "SELF_ASSIGN_DEBUG: Looking for original member: %s",
                        original_receiver_path.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                Variable *original_member =
                    interpreter_.find_variable(original_receiver_path);
                if (original_member) {
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "SELF_ASSIGN_DEBUG: Found original member, ");
                    original_member->value = value;
                    if (original_member->type != TYPE_STRING && !is_nullptr &&
                        original_member->type != TYPE_POINTER) {
                        original_member->type = TYPE_INT;
                    }
                    original_member->is_assigned = true;
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf),
                                 "SELF_ASSIGN_SYNC: %s = %lld (from variable)",
                                 original_receiver_path.c_str(),
                                 (long long)value);
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                } else {
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "SELF_ASSIGN_DEBUG: Could not find original ");
                }
            }

            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "SELF_ASSIGN: %s = %lld (from variable)",
                         member_name.c_str(), (long long)value);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
        self_member->is_assigned = true;
    } else {
        // 式の評価
        int64_t value = interpreter_.evaluate(value_node);

        // 複合代入演算子の処理
        if (value_node->node_type == ASTNodeType::AST_BINARY_OP) {
            // += -= *= /= などの複合代入かチェック
            if (value_node->name == "+=" || value_node->name == "-=" ||
                value_node->name == "*=" || value_node->name == "/=") {
                // 複合代入は既に評価済みの値として処理
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "SELF_COMPOUND_ASSIGN: %s %s= %lld",
                             member_name.c_str(), value_node->name.c_str(),
                             (long long)value);
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
        }

        // nullptr または TYPE_POINTER の場合は型を保持
        bool is_nullptr = (value_node->node_type == ASTNodeType::AST_NULLPTR);

        self_member->value = value;
        if (self_member->type != TYPE_STRING && !is_nullptr &&
            self_member->type != TYPE_POINTER) {
            self_member->type = TYPE_INT;
        }
        self_member->is_assigned = true;

        // 元の変数のメンバーも同時に更新
        if (!original_receiver_path.empty()) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "SELF_ASSIGN_DEBUG: Looking for original member: %s",
                         original_receiver_path.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            Variable *original_member =
                interpreter_.find_variable(original_receiver_path);
            if (original_member) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "SELF_ASSIGN_DEBUG: Found original member, ");
                original_member->value = value;
                if (original_member->type != TYPE_STRING && !is_nullptr &&
                    original_member->type != TYPE_POINTER) {
                    original_member->type = TYPE_INT;
                }
                original_member->is_assigned = true;
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "SELF_ASSIGN_SYNC: %s = %lld",
                             original_receiver_path.c_str(), (long long)value);
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            } else {
                {
                    char dbg_buf[512];
                    snprintf(
                        dbg_buf, sizeof(dbg_buf),
                        "SELF_ASSIGN_DEBUG: Could not find original member: %s",
                        original_receiver_path.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
        }

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf), "SELF_ASSIGN: %s = %lld",
                     member_name.c_str(), (long long)value);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    // self.member個別変数も同時に更新（sync_struct_members_from_direct_accessで上書きされないように）
    std::string self_member_path = "self." + member_name;
    Variable *self_member_var = interpreter_.find_variable(self_member_path);
    if (self_member_var) {
        self_member_var->value = self_member->value;
        self_member_var->str_value = self_member->str_value;
        self_member_var->type = self_member->type;
        self_member_var->is_assigned = true;
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf), "SELF_ASSIGN_DIRECT: %s = %lld",
                     self_member_path.c_str(),
                     (long long)self_member_var->value);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    std::string self_value_str = std::to_string(self_member->value);
    debug_msg(DebugMsgId::SELF_MEMBER_ACCESS_VALUE, self_value_str.c_str());
}

void StatementExecutor::execute_ternary_assignment(const ASTNode *node) {
    // 三項演算子の条件を評価
    int64_t condition = interpreter_.evaluate(node->right->left.get());

    // 条件に基づいて選択される分岐を決定
    const ASTNode *selected_branch =
        condition ? node->right->right.get() : node->right->third.get();

    // 選択された分岐の型に基づいて処理を分岐
    if (selected_branch->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 配列リテラルの代入
        if (!node->name.empty()) {
            interpreter_.assign_array_literal(node->name, selected_branch);
            return;
        }
    } else if (selected_branch->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        // 構造体リテラルの代入
        if (!node->name.empty()) {
            interpreter_.assign_struct_literal(node->name, selected_branch);
            return;
        }
    } else if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの代入
        if (!node->name.empty()) {
            Variable *var = interpreter_.get_variable(node->name);
            if (var) {
                var->str_value = selected_branch->str_value;
                var->type = TYPE_STRING;
                var->is_assigned = true;
            }
        }
        return;
    } else {
        // その他（数値、関数呼び出しなど）の場合は通常の評価
        try {
            TypedValue typed_value =
                interpreter_.evaluate_typed_expression(selected_branch);
            interpreter_.assign_variable(node->name, typed_value,
                                         typed_value.type.type_info, false);
        } catch (const ReturnException &ret) {
            if (!node->name.empty()) {
                if (TypeHelpers::isString(ret.type)) {
                    TypedValue typed_value(ret.str_value,
                                           InferredType(TYPE_STRING, "string"));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 TYPE_STRING, false);
                } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE ||
                           ret.type == TYPE_QUAD) {
                    TypeInfo numeric_type = ret.type;
                    long double quad_value =
                        (ret.type == TYPE_FLOAT)
                            ? static_cast<long double>(ret.double_value)
                        : (ret.type == TYPE_DOUBLE)
                            ? static_cast<long double>(ret.double_value)
                            : ret.quad_value;
                    TypedValue typed_value(
                        quad_value,
                        InferredType(numeric_type,
                                     type_info_to_string(numeric_type)));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 numeric_type, false);
                } else if (ret.is_struct) {
                    Variable struct_var = ret.struct_value;
                    TypedValue typed_value(
                        struct_var,
                        InferredType(TYPE_STRUCT, struct_var.struct_type_name));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 TYPE_STRUCT, false);
                } else {
                    TypedValue typed_value(
                        ret.value,
                        InferredType(ret.type, type_info_to_string(ret.type)));
                    interpreter_.assign_variable(node->name, typed_value,
                                                 ret.type, false);
                }
            }
        }
    }
}

void StatementExecutor::execute_ternary_variable_initialization(
    const ASTNode *var_decl_node, const ASTNode *ternary_node) {
    debug_msg(DebugMsgId::TERNARY_VAR_INIT_START,
              "execute_ternary_variable_initialization");

    // 三項演算子の条件を評価
    int64_t condition = interpreter_.evaluate(ternary_node->left.get());
    debug_msg(DebugMsgId::TERNARY_VAR_CONDITION,
              std::to_string(condition).c_str());

    // 条件に基づいて選択される分岐を決定
    const ASTNode *selected_branch =
        condition ? ternary_node->right.get() : ternary_node->third.get();
    debug_msg(
        DebugMsgId::TERNARY_VAR_BRANCH_TYPE,
        std::to_string(static_cast<int>(selected_branch->node_type)).c_str());

    std::string var_name = var_decl_node->name;
    Variable *var = interpreter_.get_variable(var_name);

    if (!var) {
        throw std::runtime_error(
            "Variable not found during ternary initialization: " + var_name);
    }

    // 選択された分岐の型に基づいて処理
    if (selected_branch->node_type == ASTNodeType::AST_ARRAY_LITERAL) {
        // 配列リテラルの初期化
        interpreter_.assign_array_literal(var_name, selected_branch);
        var->is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_STRUCT_LITERAL) {
        // 構造体リテラルの初期化
        interpreter_.assign_struct_literal(var_name, selected_branch);
        var->is_assigned = true;
    } else if (selected_branch->node_type == ASTNodeType::AST_STRING_LITERAL) {
        // 文字列リテラルの初期化
        var->str_value = selected_branch->str_value;
        var->type = TYPE_STRING;
        var->is_assigned = true;
    } else {
        try {
            TypedValue typed_value =
                interpreter_.evaluate_typed_expression(selected_branch);
            interpreter_.assign_variable(var_name, typed_value,
                                         typed_value.type.type_info, false);
        } catch (const ReturnException &ret) {
            if (TypeHelpers::isString(ret.type)) {
                TypedValue typed_value(ret.str_value,
                                       InferredType(TYPE_STRING, "string"));
                interpreter_.assign_variable(var_name, typed_value, TYPE_STRING,
                                             false);
            } else if (ret.type == TYPE_FLOAT || ret.type == TYPE_DOUBLE ||
                       ret.type == TYPE_QUAD) {
                TypeInfo numeric_type = ret.type;
                long double quad_value =
                    (ret.type == TYPE_FLOAT)
                        ? static_cast<long double>(ret.double_value)
                    : (ret.type == TYPE_DOUBLE)
                        ? static_cast<long double>(ret.double_value)
                        : ret.quad_value;
                TypedValue typed_value(
                    quad_value, InferredType(numeric_type, type_info_to_string(
                                                               numeric_type)));
                interpreter_.assign_variable(var_name, typed_value,
                                             numeric_type, false);
            } else if (ret.is_struct) {
                Variable struct_var = ret.struct_value;
                TypedValue typed_value(
                    struct_var,
                    InferredType(TYPE_STRUCT, struct_var.struct_type_name));
                interpreter_.assign_variable(var_name, typed_value, TYPE_STRUCT,
                                             false);
            } else {
                TypedValue typed_value(
                    ret.value,
                    InferredType(ret.type, type_info_to_string(ret.type)));
                interpreter_.assign_variable(var_name, typed_value, ret.type,
                                             false);
            }
        }
    }
}

Variable *StatementExecutor::evaluate_nested_member_access(
    const ASTNode *member_access_node) {
    // ネストメンバアクセス (obj.mid.data)
    // を再帰的に評価して、最終的なメンバを含む親構造体を返す
    if (!member_access_node ||
        member_access_node->node_type != ASTNodeType::AST_MEMBER_ACCESS) {
        return nullptr;
    }

    // 左側を取得
    if (!member_access_node->left) {
        return nullptr;
    }

    Variable *parent_struct = nullptr;

    if (member_access_node->left->node_type == ASTNodeType::AST_VARIABLE ||
        member_access_node->left->node_type == ASTNodeType::AST_IDENTIFIER) {
        // 基底オブジェクト: obj または self
        std::string obj_name = member_access_node->left->name;
        parent_struct = interpreter_.find_variable(obj_name);

        if (!parent_struct || parent_struct->type != TYPE_STRUCT) {
            throw std::runtime_error("Base object is not a struct: " +
                                     obj_name);
        }
    } else if (member_access_node->left->node_type ==
               ASTNodeType::AST_MEMBER_ACCESS) {
        // ネストメンバアクセス: obj.mid (さらに再帰)
        // 左側のメンバアクセスを評価して、その親構造体を取得
        Variable *intermediate_struct =
            evaluate_nested_member_access(member_access_node->left.get());
        if (!intermediate_struct) {
            return nullptr;
        }

        // 左側のメンバ名を取得
        std::string intermediate_member = member_access_node->left->name;

        // 親構造体から中間メンバを取得
        auto it = intermediate_struct->struct_members.find(intermediate_member);
        if (it == intermediate_struct->struct_members.end()) {
            throw std::runtime_error("Intermediate member not found: " +
                                     intermediate_member);
        }

        parent_struct = &it->second;
        if (parent_struct->type != TYPE_STRUCT) {
            throw std::runtime_error("Intermediate member is not a struct: " +
                                     intermediate_member);
        }
    } else if (member_access_node->left->node_type ==
               ASTNodeType::AST_ARRAY_REF) {
        // 配列アクセスを含むネストメンバアクセス: obj.arr[0].member
        // AST_ARRAY_REFの構造: left = 配列（変数 or
        // メンバアクセス）、array_index = インデックス
        const ASTNode *array_ref = member_access_node->left.get();

        // 配列参照の左側を再帰的に処理
        if (array_ref->left->node_type == ASTNodeType::AST_MEMBER_ACCESS) {
            // struct.array[index] の場合 - 再帰的にstructを取得
            Variable *intermediate_struct =
                evaluate_nested_member_access(array_ref->left.get());
            if (!intermediate_struct) {
                return nullptr;
            }

            // メンバ名（配列名）を取得
            std::string array_member = array_ref->left->name;

            // 配列メンバを取得
            auto it = intermediate_struct->struct_members.find(array_member);
            if (it == intermediate_struct->struct_members.end()) {
                throw std::runtime_error("Array member not found: " +
                                         array_member);
            }

            Variable &array_var = it->second;
            if (!array_var.is_array) {
                throw std::runtime_error("Member is not an array: " +
                                         array_member);
            }

            // インデックスを評価
            int64_t index = interpreter_.evaluate(array_ref->array_index.get());

            // 配列要素の変数名を構築
            // 構造体配列の要素は "struct_name.array_name[index]"
            // という変数名で管理される
            std::string struct_name;
            if (array_ref->left->left->node_type == ASTNodeType::AST_VARIABLE ||
                array_ref->left->left->node_type ==
                    ASTNodeType::AST_IDENTIFIER) {
                struct_name = array_ref->left->left->name;
            } else if (array_ref->left->left->node_type ==
                       ASTNodeType::AST_ARRAY_REF) {
                // さらにネストした配列アクセス: container.shapes[0].edges[0]
                // このケースは再帰的に処理する必要があるため、一旦配列要素を評価
                // 親構造体の完全な名前を構築
                std::function<std::string(const ASTNode *)> build_full_path;
                build_full_path = [&](const ASTNode *node) -> std::string {
                    if (!node)
                        return "";

                    if (node->node_type == ASTNodeType::AST_VARIABLE ||
                        node->node_type == ASTNodeType::AST_IDENTIFIER) {
                        return node->name;
                    } else if (node->node_type ==
                               ASTNodeType::AST_MEMBER_ACCESS) {
                        std::string left_path =
                            build_full_path(node->left.get());
                        return left_path + "." + node->name;
                    } else if (node->node_type == ASTNodeType::AST_ARRAY_REF) {
                        std::string left_path =
                            build_full_path(node->left.get());
                        int64_t idx =
                            interpreter_.evaluate(node->array_index.get());
                        return left_path + "[" + std::to_string(idx) + "]";
                    }
                    return "";
                };

                struct_name = build_full_path(array_ref->left->left.get());
            } else {
                throw std::runtime_error(
                    "Complex struct access not yet supported in nested member");
            }

            // 配列要素の変数名を構築
            std::string element_name = struct_name + "." + array_member + "[" +
                                       std::to_string(index) + "]";

            // 要素変数を取得
            parent_struct = interpreter_.find_variable(element_name);
            if (!parent_struct) {
                throw std::runtime_error("Struct array element not found: " +
                                         element_name);
            }
        } else {
            // 単純な配列[index]の場合
            throw std::runtime_error(
                "Simple array access not supported in this context");
        }
    } else {
        throw std::runtime_error(
            "Unsupported nested member access left node type");
    }

    return parent_struct;
}
