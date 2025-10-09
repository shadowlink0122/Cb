#pragma once

#include "../../../../common/ast.h"
#include "../../core/interpreter.h"
#include <string>
#include <vector>

namespace AssignmentHelpers {

// パス文字列からセグメントのリストを抽出
// 例: "container.shapes[0].edges[1].start" ->
//     ["container", "shapes[0]", "edges[1]", "start"]
inline std::vector<std::string> parse_path_segments(const std::string &path) {
    std::vector<std::string> segments;
    std::string current;

    for (size_t i = 0; i < path.length(); ++i) {
        char c = path[i];

        if (c == '.') {
            if (!current.empty()) {
                segments.push_back(current);
                current.clear();
            }
        } else if (c == '[') {
            // 配列アクセスの開始 - ]まで読む
            size_t close_bracket = path.find(']', i);
            if (close_bracket != std::string::npos) {
                current += path.substr(i, close_bracket - i + 1);
                i = close_bracket;
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) {
        segments.push_back(current);
    }

    return segments;
}

// ネストした構造体へのパスを解決し、必要に応じて中間構造体を作成
// 戻り値: {親構造体への参照, 最終メンバー名}
inline std::pair<Variable *, std::string>
resolve_or_create_nested_path(Interpreter &interpreter,
                              const std::string &full_path) {

    auto segments = parse_path_segments(full_path);

    if (segments.empty()) {
        return {nullptr, ""};
    }

    // 最初のセグメント（ルート変数）を取得
    std::string current_path = segments[0];
    Variable *current_var = interpreter.find_variable(current_path);

    if (!current_var) {
        throw std::runtime_error("Root variable not found: " + current_path);
    }

    // 中間のセグメントを順に処理
    for (size_t i = 1; i < segments.size() - 1; ++i) {
        std::string segment = segments[i];
        std::string next_path = current_path + "." + segment;

        // 配列アクセスの場合
        if (segment.find('[') != std::string::npos) {
            size_t bracket_pos = segment.find('[');
            std::string array_member = segment.substr(0, bracket_pos);

            // 親のstruct_membersから配列メンバーを取得
            Variable *array_var = nullptr;
            if (current_var->is_struct) {
                auto it = current_var->struct_members.find(array_member);
                if (it != current_var->struct_members.end()) {
                    array_var = &it->second;
                }
            }

            if (!array_var || !array_var->is_array) {
                throw std::runtime_error(
                    "Array member not found or not an array: " + array_member +
                    " in " + current_path);
            }

            // 配列要素変数を検索または作成
            Variable *elem_var = interpreter.find_variable(next_path);

            if (!elem_var && array_var->is_struct) {
                // 構造体配列の要素変数を作成
                interpreter.create_struct_variable(next_path,
                                                   array_var->struct_type_name);
                elem_var = interpreter.find_variable(next_path);

                if (!elem_var) {
                    throw std::runtime_error(
                        "Failed to create array element variable: " +
                        next_path);
                }
            }

            if (!elem_var) {
                // 非構造体配列の場合、struct_membersから直接参照
                // ただし、ここでは構造体のネストを想定しているのでエラー
                throw std::runtime_error(
                    "Array element variable not available: " + next_path);
            }

            current_var = elem_var;
        } else {
            // 通常のメンバーアクセス
            if (!current_var->is_struct) {
                throw std::runtime_error(
                    "Cannot access member of non-struct: " + current_path);
            }

            // struct_membersから次のメンバーを取得
            auto it = current_var->struct_members.find(segment);
            if (it == current_var->struct_members.end()) {
                throw std::runtime_error("Member not found: " + segment +
                                         " in " + current_path);
            }

            current_var = &it->second;
        }

        current_path = next_path;
    }

    // 最後のセグメントはメンバー名として返す
    std::string final_member = segments.back();

    return {current_var, final_member};
}

} // namespace AssignmentHelpers
