// struct_operations.cpp (Phase 3.4a-b)
// Struct関連の操作をinterpreter.cppから抽出
// Phase 3.4a: 構造体定義の登録と再帰検証のみ実装
// Phase 3.4b: 定義検索とパーサー同期

#include "operations.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../common/type_helpers.h"
#include "../../../../frontend/recursive_parser/recursive_parser.h"
#include "../../core/interpreter.h"
#include "../types/interfaces.h"
#include "../types/manager.h"
#include "../variables/static.h"
#include <algorithm>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

// ヘルパー関数
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

std::string build_cycle_path(const std::vector<std::string> &cycle) {
    std::ostringstream oss;
    for (size_t i = 0; i < cycle.size(); ++i) {
        if (i > 0) {
            oss << " -> ";
        }
        oss << cycle[i];
    }
    return oss.str();
}

std::string strip_array_suffix(const std::string &name) {
    auto bracket_pos = name.find('[');
    if (bracket_pos == std::string::npos) {
        return name;
    }
    return name.substr(0, bracket_pos);
}

} // namespace

StructOperations::StructOperations(Interpreter *interpreter)
    : interpreter_(interpreter) {}

// struct定義を登録
void StructOperations::register_struct_definition(
    const std::string &struct_name, const StructDefinition &definition) {
    debug_msg(DebugMsgId::INTERPRETER_STRUCT_DEFINITION_STORED,
              struct_name.c_str());

    if (interpreter_->is_debug_mode()) {
        std::cerr << "[REGISTER_STRUCT] " << struct_name
                  << ": has_default_member=" << definition.has_default_member
                  << ", default_member_name=" << definition.default_member_name
                  << std::endl;
    }

    // v0.11.0 Phase 1a: インターフェース境界の検証は遅延
    // すべてのグローバル宣言後にInterpreter::validate_all_interface_bounds()で実行

    interpreter_->struct_definitions_[struct_name] = definition;
    validate_struct_recursion_rules();
}

void StructOperations::validate_struct_recursion_rules() {
    if (!interpreter_->type_manager_ ||
        interpreter_->struct_definitions_.empty()) {
        return;
    }

    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    adjacency.reserve(interpreter_->struct_definitions_.size());
    for (const auto &entry : interpreter_->struct_definitions_) {
        adjacency.emplace(entry.first, std::vector<std::string>{});
    }

    auto gather_forms = [&](const std::string &raw,
                            std::vector<std::string> &collector) {
        if (raw.empty()) {
            return;
        }

        std::string trimmed = trim_copy(raw);
        if (trimmed.empty()) {
            return;
        }

        collector.push_back(trimmed);

        std::string normalized = normalize_struct_type_name(trimmed);
        if (!normalized.empty()) {
            collector.push_back(normalized);
        }

        std::string resolved =
            interpreter_->type_manager_->resolve_typedef(trimmed);
        if (!resolved.empty()) {
            collector.push_back(resolved);

            std::string normalized_resolved =
                normalize_struct_type_name(resolved);
            if (!normalized_resolved.empty()) {
                collector.push_back(normalized_resolved);
            }
        }

        if (!normalized.empty()) {
            std::string resolved_from_normalized =
                interpreter_->type_manager_->resolve_typedef(normalized);
            if (!resolved_from_normalized.empty()) {
                collector.push_back(resolved_from_normalized);

                std::string normalized_twice =
                    normalize_struct_type_name(resolved_from_normalized);
                if (!normalized_twice.empty()) {
                    collector.push_back(normalized_twice);
                }
            }
        }
    };

    auto resolve_member_target =
        [&](const StructMember &member) -> std::string {
        std::vector<std::string> candidates;
        candidates.reserve(8);
        gather_forms(member.pointer_base_type_name, candidates);
        gather_forms(member.type_alias, candidates);

        std::unordered_set<std::string> seen;
        for (const auto &candidate : candidates) {
            std::string normalized = normalize_struct_type_name(candidate);
            if (normalized.empty()) {
                continue;
            }

            if (!seen.insert(normalized).second) {
                continue;
            }

            if (interpreter_->struct_definitions_.count(normalized)) {
                return normalized;
            }

            std::string resolved = normalize_struct_type_name(
                interpreter_->type_manager_->resolve_typedef(normalized));
            if (!resolved.empty() && seen.insert(resolved).second &&
                interpreter_->struct_definitions_.count(resolved)) {
                return resolved;
            }
        }

        return "";
    };

    for (const auto &entry : interpreter_->struct_definitions_) {
        const std::string &struct_name = entry.first;
        const StructDefinition &definition = entry.second;

        for (const auto &member : definition.members) {
            bool is_struct_value_member =
                !member.is_pointer && (TypeHelpers::isStruct(member.type) ||
                                       member.pointer_base_type == TYPE_STRUCT);

            if (!is_struct_value_member) {
                continue;
            }

            std::string target = resolve_member_target(member);
            if (target.empty()) {
                continue;
            }

            adjacency[struct_name].push_back(target);
        }
    }

    std::unordered_set<std::string> visiting;
    std::unordered_set<std::string> visited;
    std::vector<std::string> path;

    std::function<void(const std::string &)> dfs = [&](const std::string
                                                           &node) {
        if (visiting.count(node)) {
            auto cycle_start = std::find(path.begin(), path.end(), node);
            std::vector<std::string> cycle;
            if (cycle_start != path.end()) {
                cycle.assign(cycle_start, path.end());
            }
            cycle.push_back(node);

            std::ostringstream oss;
            oss << "Recursive struct value member cycle detected: "
                << build_cycle_path(cycle)
                << ". Recursive struct relationships must use pointer members.";
            throw std::runtime_error(oss.str());
        }

        if (visited.count(node)) {
            return;
        }

        visiting.insert(node);
        path.push_back(node);

        auto it = adjacency.find(node);
        if (it != adjacency.end()) {
            for (const auto &next : it->second) {
                if (!interpreter_->struct_definitions_.count(next)) {
                    continue;
                }
                dfs(next);
            }
        }

        visiting.erase(node);
        visited.insert(node);
        path.pop_back();
    };

    for (const auto &entry : interpreter_->struct_definitions_) {
        dfs(entry.first);
    }
}

// ========================================================================
// Phase 3.4b: 定義検索とパーサー同期
// ========================================================================

const StructDefinition *
StructOperations::find_struct_definition(const std::string &struct_name) {
    auto it = interpreter_->struct_definitions_.find(struct_name);
    if (it != interpreter_->struct_definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

void StructOperations::sync_struct_definitions_from_parser(
    RecursiveParser *parser) {
    if (!parser)
        return;

    auto &parser_structs = parser->get_struct_definitions();
    for (const auto &pair : parser_structs) {
        const std::string &struct_name = pair.first;
        const StructDefinition &struct_def = pair.second;

        // v0.11.0 Phase 1a: インターフェース境界の検証は遅延
        // すべてのグローバル宣言(interface/impl)が登録された後に
        // Interpreter::validate_all_interface_bounds()で一括チェック

        // Interpreterのstruct_definitions_に登録
        interpreter_->struct_definitions_[struct_name] = struct_def;

        debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED, struct_name.c_str(),
                  struct_def.members.size());
    }
}

// ========================================================================
// Phase 3.4c: アクセス制御
// ========================================================================

bool StructOperations::is_current_impl_context_for(
    const std::string &struct_type_name) {
    if (struct_type_name.empty()) {
        return false;
    }

    auto resolve_struct_name = [&](const std::string &name) -> std::string {
        if (name.empty()) {
            return std::string();
        }
        std::string resolved =
            interpreter_->type_manager_->resolve_typedef(name);
        if (!resolved.empty()) {
            return normalize_struct_type_name(resolved);
        }
        return normalize_struct_type_name(name);
    };

    Variable *self_var = interpreter_->find_variable("self");
    if (!self_var) {
        return false;
    }

    auto extract_struct_type = [&](const Variable *var) -> std::string {
        if (!var) {
            return std::string();
        }
        if (!var->struct_type_name.empty()) {
            auto resolved = resolve_struct_name(var->struct_type_name);
            if (!resolved.empty()) {
                return resolved;
            }
        }
        if (!var->implementing_struct.empty()) {
            auto resolved = resolve_struct_name(var->implementing_struct);
            if (!resolved.empty()) {
                return resolved;
            }
        }
        return std::string();
    };

    std::string self_struct = extract_struct_type(self_var);
    if (self_struct.empty()) {
        return false;
    }

    std::string target_struct = resolve_struct_name(struct_type_name);
    if (target_struct.empty()) {
        target_struct = normalize_struct_type_name(struct_type_name);
    }

    return !target_struct.empty() && target_struct == self_struct;
}

void StructOperations::ensure_struct_member_access_allowed(
    const std::string &accessor_name, const std::string &member_name) {
    if (accessor_name.empty()) {
        return;
    }

    Variable *struct_var = interpreter_->find_variable(accessor_name);
    if (!struct_var) {
        return;
    }

    const bool is_struct_like = struct_var->is_struct ||
                                struct_var->type == TYPE_STRUCT ||
                                struct_var->type == TYPE_INTERFACE;
    if (!is_struct_like) {
        return;
    }

    auto member_is_private = [&]() -> bool {
        auto member_it = struct_var->struct_members.find(member_name);
        if (member_it != struct_var->struct_members.end()) {
            return member_it->second.is_private_member;
        }

        std::string full_member_name = accessor_name + "." + member_name;
        if (Variable *direct_member =
                interpreter_->find_variable(full_member_name)) {
            if (direct_member->is_private_member) {
                return true;
            }
        }

        std::string struct_type = struct_var->struct_type_name;
        if (struct_type.empty() && !struct_var->implementing_struct.empty()) {
            struct_type = struct_var->implementing_struct;
        }

        if (struct_type.empty()) {
            return false;
        }

        std::string resolved =
            interpreter_->type_manager_->resolve_typedef(struct_type);
        if (resolved.empty()) {
            resolved = struct_type;
        }

        const StructDefinition *struct_def = find_struct_definition(resolved);
        if (!struct_def) {
            std::string normalized = normalize_struct_type_name(resolved);
            if (normalized != resolved) {
                struct_def = find_struct_definition(normalized);
            }
        }

        if (struct_def) {
            for (const auto &member : struct_def->members) {
                if (member.name == member_name) {
                    return member.is_private;
                }
            }
        }
        return false;
    }();

    if (!member_is_private) {
        return;
    }

    std::string sanitized_accessor = strip_array_suffix(accessor_name);
    if (sanitized_accessor == "self") {
        return;
    }

    std::string struct_type = struct_var->struct_type_name;
    if (struct_type.empty() && !struct_var->implementing_struct.empty()) {
        struct_type = struct_var->implementing_struct;
    }

    if (!is_current_impl_context_for(struct_type)) {
        std::cerr << "Error: Cannot access private member '" << accessor_name
                  << "." << member_name << "' from outside its impl block"
                  << std::endl;
        std::exit(1);
    }
}

// ========================================================================
// Phase 3.4d: メンバーアクセス
// ========================================================================

Variable *StructOperations::get_struct_member(const std::string &var_name,
                                              const std::string &member_name) {
    debug_msg(DebugMsgId::EXPR_EVAL_STRUCT_MEMBER, member_name.c_str());
    debug_msg(DebugMsgId::INTERPRETER_GET_STRUCT_MEMBER, var_name.c_str(),
              member_name.c_str());

    Variable *var = interpreter_->find_variable(var_name);

    // v0.11.0: enum型の場合は、この関数を使わずにevaluatorで処理すべき
    // しかし、ここに到達した場合でもエラーにしないで、ダミーを返す
    if (var && var->is_enum) {
        // enum型のメンバーアクセスはevaluatorで処理される
        // ここに到達するのは想定外だが、エラーを避けるためダミーを返す
        static Variable dummy;
        dummy = *var; // 元の変数をコピー
        return &dummy;
    }

    // v0.11.0: enum型もチェックを緩和
    if (!var || (!var->is_struct && !var->is_enum)) {
        // 配列要素の場合、自動的に作成を試みる (例: people[0])
        size_t bracket_pos = var_name.find('[');
        if (bracket_pos != std::string::npos) {
            std::string array_name = var_name.substr(0, bracket_pos);
            Variable *array_var = interpreter_->find_variable(array_name);

            if (array_var && array_var->is_array && array_var->is_struct &&
                !array_var->struct_type_name.empty()) {
                // 親配列が存在する場合、要素変数を作成
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "[DEBUG] Auto-creating struct array element: %s",
                             var_name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                interpreter_->create_struct_variable(
                    var_name, array_var->struct_type_name);
                var = interpreter_->find_variable(var_name);
            }
        }

        // v0.11.0: enum型もメンバーアクセスをサポート
        if (!var || (!var->is_struct && !var->is_enum)) {
            debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_STRUCT, var_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[DEBUG] get_struct_member: var=%p, var_name='%s', ");
            throw std::runtime_error("Variable is not a struct or enum: " +
                                     var_name);
        }
    }

    // enum型の場合は、evaluator側で処理されるのでここには来ないはず
    if (var && var->is_enum) {
        throw std::runtime_error(
            "Enum member access should be handled in evaluator: " + var_name);
    }

    // 参照型の場合、参照先の変数を取得
    Variable *actual_var = var;
    if (var->is_reference) {
        actual_var = reinterpret_cast<Variable *>(var->value);
        if (!actual_var) {
            throw std::runtime_error(
                "Invalid reference in struct member access: " + var_name);
        }
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[DEBUG] get_struct_member: resolving reference %s to ");
    }

    // 構造体メンバーアクセス前に最新状態を同期
    // 注: 参照の場合、参照先で同期する必要があるが、
    // 参照先は名前がないため、元の変数名で同期を試みる
    interpreter_->sync_struct_members_from_direct_access(var_name);

    ensure_struct_member_access_allowed(var_name, member_name);

    debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBERS_FOUND,
              actual_var->struct_members.size());

    auto it = actual_var->struct_members.find(member_name);
    if (it != actual_var->struct_members.end()) {
        // 親変数がconstの場合、メンバーもconstにする
        if (actual_var->is_const && !it->second.is_const) {
            it->second.is_const = true;
        }

        debug_msg(DebugMsgId::EXPR_EVAL_MULTIDIM_ACCESS,
                  it->second.is_multidimensional ? 1 : 0,
                  it->second.array_dimensions.size(),
                  (size_t)2); // 固定で2インデックス（i,j）
        debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBER_FOUND,
                  member_name.c_str(), it->second.is_array);
        return &it->second;
    }

    throw std::runtime_error("Struct member not found: " + var_name + "." +
                             member_name);
}

// ========================================================================
// Phase 3.5: 構造体同期メソッド群
// ========================================================================

void StructOperations::sync_individual_member_from_struct(
    Variable *struct_var, const std::string &member_name) {
    // struct_var->struct_members[member_name] の値を、
    // 対応する個別変数 (例: "p.x") に同期する

    if (!struct_var || member_name.empty()) {
        return;
    }

    // struct_var のアドレスを持つ変数名を全スコープから探す
    std::string found_var_name;

    // グローバルスコープを検索
    for (const auto &var_pair : interpreter_->global_scope.variables) {
        if (&var_pair.second == struct_var) {
            found_var_name = var_pair.first;
            break;
        }
    }

    // 見つからなければローカルスコープを検索
    if (found_var_name.empty()) {
        for (auto it = interpreter_->scope_stack.rbegin();
             it != interpreter_->scope_stack.rend(); ++it) {
            for (const auto &var_pair : it->variables) {
                if (&var_pair.second == struct_var) {
                    found_var_name = var_pair.first;
                    break;
                }
            }
            if (!found_var_name.empty()) {
                break;
            }
        }
    }

    // 見つからなければstatic変数を検索
    if (found_var_name.empty()) {
        for (const auto &var_pair :
             interpreter_->static_variable_manager_->get_static_variables()) {
            if (&var_pair.second == struct_var) {
                found_var_name = var_pair.first;
                break;
            }
        }
    }

    if (found_var_name.empty()) {
        // 変数名が見つからなかった（一時変数やポインタで参照されている構造体）
        // この場合は個別変数システムとは同期しない
        if (interpreter_->debug_mode) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "DEBUG: sync_individual_member_from_struct - ");
        }
        return;
    }

    // 個別変数のパスを構築
    std::string full_member_path = found_var_name + "." + member_name;

    // 個別変数を探して更新
    Variable *individual_var = interpreter_->find_variable(full_member_path);
    if (individual_var) {
        const Variable &member_value = struct_var->struct_members[member_name];
        individual_var->value = member_value.value;
        individual_var->type = member_value.type;
        individual_var->str_value = member_value.str_value;
        individual_var->float_value = member_value.float_value;
        individual_var->double_value = member_value.double_value;
        individual_var->quad_value = member_value.quad_value;
        individual_var->is_assigned = member_value.is_assigned;
        individual_var->is_const = member_value.is_const;
        individual_var->is_unsigned = member_value.is_unsigned;

        // 構造体メンバーの場合、struct_membersもコピー
        if (member_value.type == TYPE_STRUCT &&
            !member_value.struct_members.empty()) {
            individual_var->is_struct = true;
            individual_var->struct_type_name = member_value.struct_type_name;
            individual_var->struct_members = member_value.struct_members;

            if (interpreter_->debug_mode) {
                std::cerr
                    << "[SYNC_INDIVIDUAL_STRUCT] Copied struct_members for "
                    << full_member_path
                    << ", members count: " << member_value.struct_members.size()
                    << std::endl;
            }

            // ネストされた構造体のメンバーも再帰的に同期
            for (const auto &nested_member : member_value.struct_members) {
                std::string nested_member_path =
                    full_member_path + "." + nested_member.first;
                Variable *nested_var =
                    interpreter_->find_variable(nested_member_path);
                if (nested_var) {
                    const Variable &nested_value = nested_member.second;
                    nested_var->value = nested_value.value;
                    nested_var->type = nested_value.type;
                    nested_var->str_value = nested_value.str_value;
                    nested_var->float_value = nested_value.float_value;
                    nested_var->double_value = nested_value.double_value;
                    nested_var->quad_value = nested_value.quad_value;
                    nested_var->is_assigned = nested_value.is_assigned;
                    nested_var->is_const = nested_value.is_const;
                    nested_var->is_unsigned = nested_value.is_unsigned;

                    if (interpreter_->debug_mode) {
                        std::cerr << "[SYNC_NESTED_MEMBER] Updated "
                                  << nested_member_path << " = "
                                  << nested_value.value << std::endl;
                    }
                }
            }
        }

        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(
                    dbg_buf, sizeof(dbg_buf),
                    "DEBUG: sync_individual_member_from_struct - updated %s",
                    full_member_path.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }
}

// ========================================================================
// Phase 3.6: Struct Member Getter メソッド群
// ========================================================================

int64_t StructOperations::get_struct_member_array_element(
    const std::string &var_name, const std::string &member_name, int index) {
    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    return member_var->array_values[index];
}

// N次元配列アクセス対応版
int64_t StructOperations::get_struct_member_multidim_array_element(
    const std::string &var_name, const std::string &member_name,
    const std::vector<int64_t> &indices) {
    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    if (interpreter_->debug_mode) {
        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "get_struct_member_multidim_array_element: var=%s, member=%s",
                var_name.c_str(), member_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        debug_msg(DebugMsgId::GENERIC_DEBUG, "Indices: ");
        for (size_t i = 0; i < indices.size(); i++) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf), "[%lld]", indices[i]);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
        debug_msg(DebugMsgId::GENERIC_DEBUG, "");
        debug_msg(DebugMsgId::GENERIC_DEBUG, "Array dimensions: ");
        for (size_t i = 0; i < member_var->array_dimensions.size(); i++) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf), "[%d]",
                         member_var->array_dimensions[i]);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
        debug_msg(DebugMsgId::GENERIC_DEBUG, "");
    }

    // 多次元配列の場合、インデックスをフラットインデックスに変換
    if (member_var->is_multidimensional &&
        !member_var->array_dimensions.empty()) {
        // 次元数チェック
        if (indices.size() != member_var->array_dimensions.size()) {
            throw std::runtime_error(
                "Dimension mismatch: expected " +
                std::to_string(member_var->array_dimensions.size()) +
                " dimensions, got " + std::to_string(indices.size()));
        }

        // 各次元の境界チェックとフラットインデックス計算
        size_t flat_index = 0;
        size_t multiplier = 1;

        // 逆順（最後の次元から）でフラットインデックスを計算
        for (int d = static_cast<int>(indices.size()) - 1; d >= 0; d--) {
            if (indices[d] < 0 ||
                indices[d] >=
                    static_cast<int64_t>(member_var->array_dimensions[d])) {
                throw std::runtime_error(
                    "Array index out of bounds in dimension " +
                    std::to_string(d));
            }
            flat_index += static_cast<size_t>(indices[d]) * multiplier;
            multiplier *= member_var->array_dimensions[d];
        }

        if (interpreter_->debug_mode) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "Calculated flat_index: %zu, ");
        }

        if (flat_index >= member_var->multidim_array_values.size()) {
            throw std::runtime_error(
                "Calculated flat index out of bounds: " +
                std::to_string(flat_index) + " >= " +
                std::to_string(member_var->multidim_array_values.size()));
        }

        if (interpreter_->debug_mode) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "Reading from multidim_array_values[%zu] = %lld",
                         flat_index,
                         member_var->multidim_array_values[flat_index]);
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }

        return member_var->multidim_array_values[flat_index];
    } else {
        // 1次元配列の場合
        if (indices.size() != 1) {
            throw std::runtime_error(
                "Array is 1-dimensional but multiple indices provided");
        }
        return get_struct_member_array_element(var_name, member_name,
                                               static_cast<int>(indices[0]));
    }
}

std::string StructOperations::get_struct_member_array_string_element(
    const std::string &var_name, const std::string &member_name, int index) {
    if (interpreter_->debug_mode) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "get_struct_member_array_string_element: var=%s, ");
    }

    Variable *member_var = get_struct_member(var_name, member_name);
    if (!member_var->is_array) {
        throw std::runtime_error("Member is not an array: " + member_name);
    }

    // 配列インデックスの境界チェック
    if (index < 0 || index >= member_var->array_size) {
        throw std::runtime_error("Array index out of bounds");
    }

    auto is_string_array_type = [&](const Variable *var) {
        if (!var) {
            return false;
        }

        if (var->type == TYPE_STRING) {
            return true;
        }

        if (var->type == static_cast<TypeInfo>(TYPE_ARRAY_BASE + TYPE_STRING)) {
            return true;
        }

        if (var->array_type_info.base_type == TYPE_STRING) {
            return true;
        }

        // v0.13.2: Also check if array_strings has actual data
        if (!var->array_strings.empty()) {
            return true;
        }

        return false;
    };

    if (!is_string_array_type(member_var)) {
        throw std::runtime_error("Member is not a string array: " +
                                 member_name);
    }

    if (interpreter_->debug_mode) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "Returning string: array_strings[%d]=%s", index,
                     member_var->array_strings[index].c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    return member_var->array_strings[index];
}
