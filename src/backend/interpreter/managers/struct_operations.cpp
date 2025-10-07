// struct_operations.cpp
// Struct関連の操作をinterpreter.cppから抽出
// 構造体の定義、初期化、メンバーアクセス、同期処理を管理

#include "struct_operations.h"
#include "../../../common/debug.h"
#include "../../../common/debug_messages.h"
#include "../core/interpreter.h"
#include "type_manager.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

// ヘルパー関数（interpreter.cppの無名名前空間から移動）
namespace {

std::string trim_copy(const std::string &text) {
    auto begin =
        std::find_if_not(text.begin(), text.end(),
                         [](unsigned char ch) { return std::isspace(ch); });
    auto end =
        std::find_if_not(text.rbegin(), text.rend(), [](unsigned char ch) {
            return std::isspace(ch);
        }).base();

    if (begin >= end) {
        return "";
    }

    return std::string(begin, end);
}

std::string normalize_struct_type_name(const std::string &raw_name) {
    std::string normalized = trim_copy(raw_name);
    if (normalized.empty()) {
        return normalized;
    }

    if (normalized.rfind("struct ", 0) == 0) {
        normalized = trim_copy(normalized.substr(7));
    }

    while (!normalized.empty() && normalized.back() == '*') {
        normalized.pop_back();
    }
    normalized = trim_copy(normalized);

    auto bracket_pos = normalized.find('[');
    if (bracket_pos != std::string::npos) {
        normalized = trim_copy(normalized.substr(0, bracket_pos));
    }

    return normalized;
}

} // namespace

StructOperations::StructOperations(Interpreter *interpreter)
    : interpreter_(interpreter) {}

// 以下、interpreter.cppからstruct関連のメソッドをコピーして実装
// TODO: 実装を追加

