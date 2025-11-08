#pragma once

#include "../../../common/ast.h"
#include "../core/interpreter.h"
#include <memory>
#include <vector>

// v0.12.0: 非同期タスク構造体
// async関数の実行コンテキストを保持
struct AsyncTask {
    int task_id = -1;                       // タスクID
    std::string function_name;              // 関数名
    const ASTNode *function_node = nullptr; // 関数定義ノード
    std::vector<Variable> args;             // 引数リスト
    std::shared_ptr<Scope> task_scope;      // タスク専用スコープ

    // 実行状態
    bool is_started = false;  // 実行開始済みか
    bool is_executed = false; // 実行完了したか
    size_t current_statement_index =
        0; // 現在実行中のステートメントインデックス

    // 戻り値
    bool has_return_value = false;    // 戻り値があるか
    int64_t return_value = 0;         // 戻り値（int型の場合）
    std::string return_string_value;  // 戻り値（string型の場合）
    TypeInfo return_type = TYPE_VOID; // 戻り値の型

    // 協調的マルチタスク用
    bool auto_yield = true; // 各ステートメント後に自動yield

    // v0.12.0: 非同期sleep対応
    bool is_sleeping = false; // sleep中か
    int64_t wake_up_time_ms = 0; // 起床時刻（エポックからのミリ秒）
};
