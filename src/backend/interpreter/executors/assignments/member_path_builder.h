#pragma once

#include "../../../../common/ast.h"
#include <functional>
#include <string>

namespace AssignmentHelpers {

// 再帰的にメンバーアクセスのパスを構築する
// 例: container.shapes[0].edges[1].start.x ->
// "container.shapes[0].edges[1].start.x"
inline std::string
build_member_path(const ASTNode *node,
                  std::function<int64_t(const ASTNode *)> evaluate_index) {

    if (!node) {
        return "";
    }

    switch (node->node_type) {
    case ASTNodeType::AST_VARIABLE:
    case ASTNodeType::AST_IDENTIFIER:
        return node->name;

    case ASTNodeType::AST_MEMBER_ACCESS: {
        std::string base = build_member_path(node->left.get(), evaluate_index);
        if (base.empty()) {
            return node->name;
        }
        return base + "." + node->name;
    }

    case ASTNodeType::AST_ARRAY_REF: {
        std::string base = build_member_path(node->left.get(), evaluate_index);
        int64_t index = evaluate_index(node->array_index.get());
        return base + "[" + std::to_string(index) + "]";
    }

    default:
        throw std::runtime_error(
            "Unsupported node type in member path: " +
            std::to_string(static_cast<int>(node->node_type)));
    }
}

// 左辺のパスを構築する（代入文用）
// 例: container.shapes[0].edges[0].start.x = 10;
//     -> "container.shapes[0].edges[0].start.x"
inline std::string build_assignment_target_path(
    const ASTNode *left_node,
    std::function<int64_t(const ASTNode *)> evaluate_index) {

    return build_member_path(left_node, evaluate_index);
}

// パスを分解してベースオブジェクトとメンバー名を取得
// 例: "container.shapes[0].edges[0].start.x" ->
//     base: "container.shapes[0].edges[0].start", member: "x"
inline std::pair<std::string, std::string>
split_member_path(const std::string &path) {
    size_t last_dot = path.rfind('.');
    if (last_dot == std::string::npos) {
        // ドットがない場合は単純な変数
        return {"", path};
    }

    std::string base = path.substr(0, last_dot);
    std::string member = path.substr(last_dot + 1);
    return {base, member};
}

} // namespace AssignmentHelpers
