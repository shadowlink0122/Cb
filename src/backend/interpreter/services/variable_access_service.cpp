#include "services/variable_access_service.h"
#include "../../../common/debug.h"
#include "core/interpreter.h"
#include "managers/variables/manager.h"
#include <stdexcept>

VariableAccessService::VariableAccessService(Interpreter *interpreter)
    : interpreter_(interpreter), stats_{}, cache_enabled_(true) {}

Variable *VariableAccessService::find_variable_safe(const std::string &name,
                                                    const std::string &context,
                                                    bool allow_null) {
    stats_.total_accesses++;

    // キャッシュチェック
    if (cache_enabled_) {
        auto cache_it = variable_cache_.find(name);
        if (cache_it != variable_cache_.end()) {
            stats_.cache_hits++;
            increment_stats("general", true);
            return cache_it->second;
        }
        stats_.cache_misses++;
    }

    // 実際の変数検索
    Variable *var = interpreter_->find_variable(name);

    if (!var && !allow_null) {
        std::string error_msg = "Variable '" + name + "' not found";
        if (!context.empty()) {
            error_msg += " (context: " + context + ")";
        }
        handle_access_error(error_msg, context);
        increment_stats("general", false);
        return nullptr;
    }

    // キャッシュ更新
    if (var && cache_enabled_) {
        update_cache(name, var);
    }

    increment_stats("general", var != nullptr);
    return var;
}

Variable *
VariableAccessService::find_struct_member_safe(const std::string &struct_name,
                                               const std::string &member_name,
                                               const std::string &context) {
    stats_.struct_member_accesses++;

    std::string full_member_name = struct_name + "." + member_name;

    // キャッシュチェック
    if (cache_enabled_) {
        auto cache_it = variable_cache_.find(full_member_name);
        if (cache_it != variable_cache_.end()) {
            stats_.cache_hits++;
            increment_stats("struct_member", true);
            return cache_it->second;
        }
        stats_.cache_misses++;
    }

    // 構造体の存在確認
    Variable *struct_var = interpreter_->find_variable(struct_name);
    if (!struct_var) {
        std::string error_msg = "Struct '" + struct_name + "' not found";
        if (!context.empty()) {
            error_msg += " (context: " + context + ")";
        }
        handle_access_error(error_msg, context);
        increment_stats("struct_member", false);
        return nullptr;
    }

    // メンバーの検索
    Variable *member_var = interpreter_->find_variable(full_member_name);
    if (!member_var) {
        std::string error_msg =
            "Struct member '" + struct_name + "." + member_name + "' not found";
        if (!context.empty()) {
            error_msg += " (context: " + context + ")";
        }
        handle_access_error(error_msg, context);
        increment_stats("struct_member", false);
        return nullptr;
    }

    // キャッシュ更新
    if (cache_enabled_) {
        update_cache(full_member_name, member_var);
    }

    increment_stats("struct_member", true);
    return member_var;
}

Variable *VariableAccessService::find_array_element_safe(
    const std::string &array_name, int64_t index, const std::string &context) {
    stats_.array_element_accesses++;

    // 配列の存在確認
    Variable *array_var = find_variable_safe(array_name, context, false);
    if (!array_var) {
        increment_stats("array_element", false);
        return nullptr;
    }

    // 配列境界チェック
    if (!array_var->is_array || index < 0 ||
        static_cast<size_t>(index) >= array_var->array_values.size()) {
        std::string error_msg =
            "Array index " + std::to_string(index) +
            " out of bounds for array '" + array_name +
            "' (size: " + std::to_string(array_var->array_values.size()) + ")";
        if (!context.empty()) {
            error_msg += " (context: " + context + ")";
        }
        handle_access_error(error_msg, context);
        increment_stats("array_element", false);
        return nullptr;
    }

    increment_stats("array_element", true);
    return array_var; // 配列要素へのアクセスは配列変数自体を返す
}

Variable *VariableAccessService::find_variable_cached(const std::string &name,
                                                      bool use_cache) {
    if (!use_cache || !cache_enabled_) {
        return interpreter_->find_variable(name);
    }

    return find_variable_safe(name, "",
                              true); // allow_null = true でキャッシュ機能を活用
}

bool VariableAccessService::variable_exists(const std::string &name) {
    Variable *var = find_variable_safe(name, "", true); // allow_null = true
    return var != nullptr;
}

void VariableAccessService::clear_cache() {
    variable_cache_.clear();
    stats_.cache_hits = 0;
    stats_.cache_misses = 0;
}

void VariableAccessService::handle_access_error(const std::string &error_msg,
                                                const std::string &context) {
    std::string formatted_error = "[VariableAccessService] " + error_msg;
    if (!context.empty()) {
        formatted_error += " (Context: " + context + ")";
    }

    debug_print("Variable access error: %s\n", formatted_error.c_str());
    throw std::runtime_error(formatted_error);
}

void VariableAccessService::increment_stats(const std::string &access_type,
                                            bool success) {
    if (!success) {
        stats_.failed_accesses++;
    }

    // 特定のアクセスタイプの統計は既に各メソッドで更新済み
}

void VariableAccessService::update_cache(const std::string &name,
                                         Variable *var) {
    if (!cache_enabled_)
        return;

    // キャッシュサイズ制限（メモリ使用量制御）
    const size_t MAX_CACHE_SIZE = 1000;
    if (variable_cache_.size() >= MAX_CACHE_SIZE) {
        // 古いエントリを削除（簡単な実装としてクリア）
        variable_cache_.clear();
    }

    variable_cache_[name] = var;
}
