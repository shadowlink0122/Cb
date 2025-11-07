#include "sync.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/debug_messages.h"
#include "../../../../common/type_helpers.h"
#include "../../core/interpreter.h"
#include "../types/manager.h"
#include "../variables/static.h"
#include <functional>

StructSyncManager::StructSyncManager(Interpreter *interpreter)
    : interpreter_(interpreter) {}

void StructSyncManager::sync_direct_access_from_struct_value(
    const std::string &var_name, const Variable &struct_value) {
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START,
              var_name.c_str());

    std::map<std::string, Variable> *target_map = nullptr;
    for (auto it = interpreter_->scope_stack.rbegin();
         it != interpreter_->scope_stack.rend(); ++it) {
        if (it->variables.find(var_name) != it->variables.end()) {
            target_map = &it->variables;
            break;
        }
    }

    if (!target_map) {
        auto global_it = interpreter_->global_scope.variables.find(var_name);
        if (global_it != interpreter_->global_scope.variables.end()) {
            target_map = &interpreter_->global_scope.variables;
        }
    }

    if (!target_map) {
        auto *static_vars = interpreter_->static_variable_manager_
                                ->get_static_variables_mutable();
        auto static_it = static_vars->find(var_name);
        if (static_it != static_vars->end()) {
            target_map = static_vars;
        }
    }

    if (!target_map) {
        target_map = &interpreter_->current_scope().variables;
    }

    // ルートの構造体変数を最新の値で更新
    (*target_map)[var_name] = struct_value;
    Variable &root_var = (*target_map)[var_name];
    root_var.is_assigned = true;
    root_var.is_struct = true;

    if (interpreter_->debug_mode) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "DIRECT_SYNC: updating %s with %zu members",
                     var_name.c_str(), struct_value.struct_members.size());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    std::function<void(std::map<std::string, Variable> &, const std::string &,
                       const Variable &)>
        copy_members;
    copy_members = [&](std::map<std::string, Variable> &vars,
                       const std::string &base_name, const Variable &source) {
        for (const auto &member_pair : source.struct_members) {
            const std::string &member_name = member_pair.first;
            const Variable &member_value = member_pair.second;
            std::string qualified_name = base_name + "." + member_name;

            vars[qualified_name] = member_value;
            Variable &dest_member = vars[qualified_name];
            dest_member.is_assigned = true;

            if (interpreter_->debug_mode) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "DIRECT_SYNC_MEMBER: %s value=%lld str='%s' ");
            }

            if (member_value.is_array || member_value.type >= TYPE_ARRAY_BASE ||
                member_value.is_multidimensional) {
                dest_member.array_size = member_value.array_size;
                dest_member.array_dimensions = member_value.array_dimensions;
                dest_member.array_values = member_value.array_values;
                dest_member.array_strings = member_value.array_strings;
                dest_member.multidim_array_values =
                    member_value.multidim_array_values;
                dest_member.multidim_array_strings =
                    member_value.multidim_array_strings;
                dest_member.is_array = member_value.is_array;
                dest_member.is_multidimensional =
                    member_value.is_multidimensional;

                int total_size = 0;
                if (!member_value.array_dimensions.empty()) {
                    total_size = 1;
                    for (int dim : member_value.array_dimensions) {
                        total_size *= dim;
                    }
                } else if (member_value.is_multidimensional) {
                    total_size = static_cast<int>(
                        member_value.multidim_array_values.size());
                    if (total_size == 0) {
                        total_size = static_cast<int>(
                            member_value.multidim_array_strings.size());
                    }
                } else if (!member_value.array_values.empty() ||
                           !member_value.array_strings.empty()) {
                    total_size = static_cast<int>(
                        std::max(member_value.array_values.size(),
                                 member_value.array_strings.size()));
                }

                if (total_size == 0) {
                    total_size = member_value.array_size;
                }

                if (total_size < 0) {
                    total_size = 0;
                }

                for (int i = 0; i < total_size; ++i) {
                    std::string element_name =
                        qualified_name + "[" + std::to_string(i) + "]";
                    Variable element_var;
                    element_var.is_assigned = true;
                    element_var.is_const = dest_member.is_const;
                    element_var.is_unsigned = dest_member.is_unsigned;

                    bool treat_as_string =
                        (TypeHelpers::isString(member_value.type) ||
                         member_value.current_type == TYPE_STRING ||
                         !member_value.array_strings.empty() ||
                         !member_value.multidim_array_strings.empty());

                    if (treat_as_string) {
                        element_var.type = TYPE_STRING;
                        if (i < static_cast<int>(
                                    member_value.array_strings.size())) {
                            element_var.str_value =
                                member_value.array_strings[i];
                        } else if (i < static_cast<int>(
                                           member_value.multidim_array_strings
                                               .size())) {
                            element_var.str_value =
                                member_value.multidim_array_strings[i];
                        } else {
                            element_var.str_value = "";
                        }
                    } else {
                        element_var.type = member_value.type;
                        if (element_var.type >= TYPE_ARRAY_BASE) {
                            element_var.type = static_cast<TypeInfo>(
                                element_var.type - TYPE_ARRAY_BASE);
                        }
                        int64_t numeric_value = 0;
                        if (member_value.is_multidimensional &&
                            i < static_cast<int>(
                                    member_value.multidim_array_values
                                        .size())) {
                            numeric_value =
                                member_value.multidim_array_values[i];
                        } else if (i < static_cast<int>(
                                           member_value.array_values.size())) {
                            numeric_value = member_value.array_values[i];
                        }
                        element_var.value = numeric_value;
                    }

                    vars[element_name] = element_var;
                    if (interpreter_->debug_mode) {
                        if (TypeHelpers::isString(element_var.type)) {
                            {
                                char dbg_buf[512];
                                snprintf(dbg_buf, sizeof(dbg_buf),
                                         "DIRECT_SYNC_ARRAY_ELEM: %s str='%s'",
                                         element_name.c_str(),
                                         element_var.str_value.c_str());
                                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                            }
                        } else {
                            {
                                char dbg_buf[512];
                                snprintf(
                                    dbg_buf, sizeof(dbg_buf),
                                    "DIRECT_SYNC_ARRAY_ELEM: %s value=%lld",
                                    element_name.c_str(),
                                    static_cast<long long>(element_var.value));
                                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                            }
                        }
                    }
                }
            }

            if (member_value.is_struct &&
                !member_value.struct_members.empty()) {
                copy_members(vars, qualified_name, member_value);
            }
        }
    };

    copy_members(*target_map, var_name, struct_value);

    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_END,
              var_name.c_str());
}

void StructSyncManager::sync_struct_members_from_direct_access(
    const std::string &var_name) {
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START,
              var_name.c_str());
    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_START,
              var_name.c_str());

    // 空の変数名はスキップ
    if (var_name.empty()) {
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND, "empty variable name");
        return;
    }

    // デバッグ: sync開始前に特定の個別変数を確認
    if (interpreter_->debug_mode && var_name == "student1") {
        Variable *test_var = interpreter_->find_variable("student1.scores[0]");
        if (test_var) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "SYNC_DEBUG: Before sync, student1.scores[0] = %lld, ");
        }
    }

    // 変数を取得
    Variable *var = interpreter_->find_variable(var_name);
    if (!var) {
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_FOUND, var_name.c_str());
        return;
    }
    // v0.11.0: enum型の場合は同期不要（メンバーなし）
    if (!var->is_struct) {
        if (var->is_enum) {
            // enum型は同期不要
            return;
        }
        debug_msg(DebugMsgId::INTERPRETER_VAR_NOT_STRUCT, var_name.c_str());
        return;
    }

    // 構造体定義を取得（typedefを解決）
    std::string resolved_struct_name =
        interpreter_->type_manager_->resolve_typedef(var->struct_type_name);
    const StructDefinition *struct_def =
        interpreter_->find_struct_definition(resolved_struct_name);
    if (!struct_def) {
        debug_msg(DebugMsgId::INTERPRETER_STRUCT_DEFINITION_STORED,
                  var->struct_type_name.c_str());
        return;
    }

    debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBERS_FOUND,
              struct_def->members.size());

    // 各メンバについてダイレクトアクセス変数から struct_members に同期
    for (const auto &member : struct_def->members) {
        std::string direct_var_name = var_name + "." + member.name;
        Variable *direct_var = interpreter_->find_variable(direct_var_name);

        if (direct_var) {
            debug_msg(DebugMsgId::INTERPRETER_STRUCT_MEMBER_FOUND,
                      member.name.c_str());
            auto resolve_base_type = [](TypeInfo type) {
                if (type >= TYPE_ARRAY_BASE) {
                    return static_cast<TypeInfo>(type - TYPE_ARRAY_BASE);
                }
                return type;
            };
            TypeInfo member_base_type = resolve_base_type(member.type);
            TypeInfo direct_base_type = resolve_base_type(direct_var->type);

            // struct_membersに保存（配列チェックを先に実行）
            if (member.type >= TYPE_ARRAY_BASE ||
                member.array_info.base_type != TYPE_UNKNOWN ||
                direct_var->is_array) {
                // v0.11.1: 配列サイズの決定
                int array_size = -1;

                // 1. direct_varが有効な配列でサイズが設定されている場合
                if (direct_var->is_array && direct_var->array_size > 0) {
                    array_size = direct_var->array_size;
                }
                // 2. member定義にサイズがある場合
                else if (!member.array_info.dimensions.empty() &&
                         member.array_info.dimensions[0].size > 0) {
                    array_size = member.array_info.dimensions[0].size;
                }
                // 3. 実際の配列要素をカウント
                else {
                    int count = 0;
                    for (int i = 0; i < 1000; ++i) { // 上限1000
                        std::string element_name = var_name + "." +
                                                   member.name + "[" +
                                                   std::to_string(i) + "]";
                        if (!interpreter_->find_variable(element_name)) {
                            break;
                        }
                        count++;
                    }
                    if (count > 0) {
                        array_size = count;
                    } else {
                        array_size = 1; // デフォルト
                    }
                }

                if (interpreter_->debug_mode) {
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf),
                                 "[SYNC_DEBUG] member=%s, final array_size=%d",
                                 member.name.c_str(), array_size);
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                }

                debug_msg(DebugMsgId::INTERPRETER_STRUCT_ARRAY_MEMBER_ADDED,
                          member.name.c_str(), (int)member.type, array_size);

                // 既存のメンバーがあれば保持、なければ新規作成
                if (var->struct_members.find(member.name) ==
                    var->struct_members.end()) {
                    var->struct_members[member.name] = Variable();
                }

                var->struct_members[member.name].type = member.type;
                var->struct_members[member.name].is_array = true;
                var->struct_members[member.name].array_size = array_size;

                // 多次元配列情報をコピー
                if (direct_var->is_multidimensional) {
                    var->struct_members[member.name].is_multidimensional = true;
                    var->struct_members[member.name].array_dimensions =
                        direct_var->array_dimensions;
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "SYNC_STRUCT: Preserved multidimensional info ");
                }

                // 配列要素を個別にチェックして同期
                // 既存の値を保持するため、サイズが変わる場合のみresize
                if (var->struct_members[member.name].array_values.size() !=
                    static_cast<size_t>(array_size)) {
                    var->struct_members[member.name].array_values.resize(
                        array_size);
                }
                if (var->struct_members[member.name].array_strings.size() !=
                    static_cast<size_t>(array_size)) {
                    var->struct_members[member.name].array_strings.resize(
                        array_size);
                }

                // 多次元配列の場合は multidim_array_values
                // も初期化（元の値をコピー）
                if (var->struct_members[member.name].is_multidimensional) {
                    // 多次元配列のtotal sizeを計算
                    size_t total_size = 1;
                    for (int dim : direct_var->array_dimensions) {
                        if (dim > 0) {
                            total_size *= dim;
                        }
                    }

                    // 既存の multidim_array_values
                    // をバックアップしてからリサイズ
                    std::vector<int64_t> backup_values =
                        direct_var->multidim_array_values;
                    var->struct_members[member.name]
                        .multidim_array_values.resize(total_size);

                    // バックアップした値を復元
                    size_t copy_size =
                        std::min(backup_values.size(), total_size);
                    for (size_t i = 0; i < copy_size; i++) {
                        var->struct_members[member.name]
                            .multidim_array_values[i] = backup_values[i];
                    }

                    debug_msg(
                        DebugMsgId::GENERIC_DEBUG,
                        "SYNC_STRUCT: Initialized multidim_array_values for ");
                }

                // 個別要素変数からデータをコピー
                for (int i = 0; i < direct_var->array_size; i++) {
                    std::string element_name = var_name + "." + member.name +
                                               "[" + std::to_string(i) + "]";
                    std::string element_key =
                        member.name + "[" + std::to_string(i) + "]";
                    Variable *element_var =
                        interpreter_->find_variable(element_name);

                    // まず親構造体のstruct_membersから配列要素を探す（構造体配列の場合）
                    auto element_it = var->struct_members.find(element_key);
                    bool found_in_struct_members =
                        (element_it != var->struct_members.end());

                    if (element_var) {
                        if (interpreter_->debug_mode) {
                            debug_msg(
                                DebugMsgId::GENERIC_DEBUG,
                                "SYNC_STRUCT: Found element_var for %s: ");
                        }
                        // 構造体配列の場合、配列要素も構造体として同期する
                        if (element_var->is_struct &&
                            !element_var->struct_members.empty()) {
                            // 配列要素の構造体を再帰的に同期
                            sync_struct_members_from_direct_access(
                                element_name);

                            // 親構造体のstruct_membersに配列要素を追加
                            var->struct_members[element_key] = *element_var;

                            if (interpreter_->debug_mode) {
                                debug_msg(DebugMsgId::GENERIC_DEBUG,
                                          "SYNC_STRUCT: Synced struct array "
                                          "element ");
                            }
                        } else {
                            // プリミティブ型配列の場合
                            TypeInfo element_base_type =
                                resolve_base_type(element_var->type);
                            if (member_base_type == TYPE_STRING ||
                                direct_base_type == TYPE_STRING ||
                                element_base_type == TYPE_STRING) {
                                var->struct_members[member.name]
                                    .array_strings[i] = element_var->str_value;
                                if (interpreter_->debug_mode) {
                                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                                              "SYNC_STRUCT: Copied string ");
                                }
                            } else {
                                var->struct_members[member.name]
                                    .array_values[i] = element_var->value;
                                if (interpreter_->debug_mode) {
                                    debug_msg(
                                        DebugMsgId::GENERIC_DEBUG,
                                        "SYNC_STRUCT: Copied element[%d] = ");
                                }
                                // 多次元配列の場合は multidim_array_values
                                // にも設定
                                if (var->struct_members[member.name]
                                        .is_multidimensional) {
                                    var->struct_members[member.name]
                                        .multidim_array_values[i] =
                                        element_var->value;
                                    if (interpreter_->debug_mode) {
                                        debug_msg(DebugMsgId::GENERIC_DEBUG,
                                                  "SYNC_STRUCT: Copied "
                                                  "element[%d] = ");
                                    }
                                }
                            }
                        }
                    } else if (found_in_struct_members &&
                               element_it->second.is_struct) {
                        // 個別変数がないが、struct_membersに構造体配列要素がある場合
                        // (これは既に同期済みの可能性がある)
                        if (interpreter_->debug_mode) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "SYNC_STRUCT: Struct array element %s ");
                        }
                    } else {
                        // 要素変数が見つからない場合、direct_var自体の配列データを使用
                        if ((member_base_type == TYPE_STRING ||
                             direct_base_type == TYPE_STRING) &&
                            i < static_cast<int>(
                                    direct_var->array_strings.size())) {
                            var->struct_members[member.name].array_strings[i] =
                                direct_var->array_strings[i];
                        } else if (member_base_type != TYPE_STRING &&
                                   i < static_cast<int>(
                                           direct_var->array_values.size())) {
                            var->struct_members[member.name].array_values[i] =
                                direct_var->array_values[i];
                            if (var->struct_members[member.name]
                                    .is_multidimensional) {
                                var->struct_members[member.name]
                                    .multidim_array_values[i] =
                                    direct_var->array_values[i];
                            }
                        }
                    }
                }

                var->struct_members[member.name].is_assigned = true;
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED,
                          member.name.c_str(), direct_var->array_size);
            } else {
                std::string member_union_alias =
                    member.is_pointer ? member.pointer_base_type_name
                                      : member.type_alias;
                bool direct_is_union =
                    interpreter_->type_manager_->is_union_type(*direct_var);
                bool member_is_union =
                    (!member_union_alias.empty() &&
                     interpreter_->type_manager_->is_union_type(
                         member_union_alias));

                Variable member_value = *direct_var;

                // メンバー定義由来のメタ情報を優先度高く上書き
                member_value.is_pointer = member.is_pointer;
                member_value.pointer_depth = member.pointer_depth;
                member_value.pointer_base_type_name =
                    member.pointer_base_type_name;
                member_value.pointer_base_type = member.pointer_base_type;
                member_value.is_private_member = member.is_private;
                member_value.is_reference = member.is_reference;
                member_value.is_unsigned = member.is_unsigned;
                member_value.is_const = member.is_const;

                // unionメンバーの場合、型名とcurrent_typeを確実に保持
                if (direct_is_union || member_is_union) {
                    member_value.type = TYPE_UNION;
                    if (!direct_var->type_name.empty()) {
                        member_value.type_name = direct_var->type_name;
                    } else if (!member_union_alias.empty()) {
                        member_value.type_name = member_union_alias;
                    }
                    member_value.current_type = direct_var->current_type;
                } else {
                    // 通常のメンバー型を構造体定義に合わせる
                    member_value.type = member.type;
                }

                // 数値/文字列/浮動小数などの基本値を direct_var
                // に合わせて整合させる
                member_value.value = direct_var->value;
                member_value.str_value = direct_var->str_value;
                member_value.float_value = direct_var->float_value;
                member_value.double_value = direct_var->double_value;
                member_value.quad_value = direct_var->quad_value;
                member_value.big_value = direct_var->big_value;
                member_value.is_assigned = direct_var->is_assigned;

                // 構造体・配列などの複合型もコピー（=
                // *direct_varでコピー済みだが念のため整合）
                member_value.is_array = direct_var->is_array;
                member_value.array_size = direct_var->array_size;
                member_value.array_values = direct_var->array_values;
                member_value.array_strings = direct_var->array_strings;
                member_value.array_dimensions = direct_var->array_dimensions;
                member_value.is_multidimensional =
                    direct_var->is_multidimensional;
                member_value.multidim_array_values =
                    direct_var->multidim_array_values;
                member_value.multidim_array_strings =
                    direct_var->multidim_array_strings;
                member_value.is_struct = direct_var->is_struct;
                member_value.struct_type_name = direct_var->struct_type_name;
                member_value.struct_members = direct_var->struct_members;

                // ネストされた構造体の場合、再帰的に同期
                if (direct_var->is_struct &&
                    !direct_var->struct_members.empty()) {
                    sync_struct_members_from_direct_access(direct_var_name);
                    // 再度取得して最新の値を使用
                    Variable *updated_direct_var =
                        interpreter_->find_variable(direct_var_name);
                    if (updated_direct_var) {
                        member_value.struct_members =
                            updated_direct_var->struct_members;
                        if (interpreter_->debug_mode) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "SYNC_STRUCT: Recursively synced nested "
                                      "struct ");
                        }
                    }
                }

                var->struct_members[member.name] = member_value;
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED,
                          member.name.c_str(),
                          member_value.struct_members.size());
            }
        } else {
            // ダイレクトアクセス変数が見つからない場合、配列メンバーかチェック
            if (member.array_info.base_type != TYPE_UNKNOWN) {
                int arr_size = member.array_info.dimensions.empty()
                                   ? 0
                                   : member.array_info.dimensions[0].size;
                debug_msg(DebugMsgId::INTERPRETER_STRUCT_ARRAY_MEMBER_ADDED,
                          member.name.c_str(), (int)member.type, arr_size);

                var->struct_members[member.name] = Variable();
                var->struct_members[member.name].type = member.type;
                var->struct_members[member.name].is_array = true;
                auto resolve_base_type = [](TypeInfo type) {
                    if (type >= TYPE_ARRAY_BASE) {
                        return static_cast<TypeInfo>(type - TYPE_ARRAY_BASE);
                    }
                    return type;
                };
                TypeInfo member_base_type = resolve_base_type(member.type);

                int array_size = (!member.array_info.dimensions.empty())
                                     ? member.array_info.dimensions[0].size
                                     : 1;
                var->struct_members[member.name].array_size = array_size;

                // 多次元配列情報を構造体定義から設定
                if (member.array_info.dimensions.size() > 1) {
                    var->struct_members[member.name].is_multidimensional = true;
                    for (const auto &dim : member.array_info.dimensions) {
                        var->struct_members[member.name]
                            .array_dimensions.push_back(dim.size);
                    }
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "SYNC_STRUCT: Set multidimensional info for "
                              "%s.%s from ");
                }

                // 配列要素を個別にチェックして同期
                var->struct_members[member.name].array_values.resize(
                    array_size);
                var->struct_members[member.name].array_strings.resize(
                    array_size);

                // 多次元配列の場合は multidim_array_values
                // も初期化（元の値を保持）
                if (var->struct_members[member.name].is_multidimensional) {
                    // 既存のサイズを確認してリサイズが必要な場合のみ実行
                    if (var->struct_members[member.name]
                            .multidim_array_values.size() !=
                        static_cast<size_t>(array_size)) {
                        var->struct_members[member.name]
                            .multidim_array_values.resize(array_size);
                        debug_msg(
                            DebugMsgId::GENERIC_DEBUG,
                            "SYNC_STRUCT: Resized multidim_array_values for ");
                    }
                }

                bool found_elements = false;
                for (int i = 0; i < array_size; i++) {
                    std::string element_name = var_name + "." + member.name +
                                               "[" + std::to_string(i) + "]";
                    Variable *element_var =
                        interpreter_->find_variable(element_name);
                    if (element_var) {
                        found_elements = true;
                        if (member_base_type == TYPE_STRING ||
                            resolve_base_type(element_var->type) ==
                                TYPE_STRING) {
                            var->struct_members[member.name].array_strings[i] =
                                element_var->str_value;
                        } else {
                            var->struct_members[member.name].array_values[i] =
                                element_var->value;
                            // 多次元配列の場合は multidim_array_values にも設定
                            if (var->struct_members[member.name]
                                    .is_multidimensional) {
                                var->struct_members[member.name]
                                    .multidim_array_values[i] =
                                    element_var->value;
                                if (interpreter_->debug_mode) {
                                    debug_msg(
                                        DebugMsgId::GENERIC_DEBUG,
                                        "SYNC_STRUCT: Copied element[%d] = ");
                                }
                            }
                        }
                    }
                }

                if (found_elements) {
                    var->struct_members[member.name].is_assigned = true;
                    debug_msg(DebugMsgId::INTERPRETER_STRUCT_SYNCED,
                              member.name.c_str(), (size_t)array_size);
                }
            }
        }
    }

    debug_msg(DebugMsgId::INTERPRETER_SYNC_STRUCT_MEMBERS_END,
              var_name.c_str());
}
