#include "interfaces.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug_messages.h"
#include "../../core/interpreter.h"
#include <algorithm>

InterfaceOperations::InterfaceOperations(Interpreter *interpreter)
    : interpreter_(interpreter) {}

// ========================================================================
// Interface定義管理
// ========================================================================

void InterfaceOperations::register_interface_definition(
    const std::string &interface_name, const InterfaceDefinition &definition) {
    interface_definitions_[interface_name] = definition;
    debug_msg(DebugMsgId::PARSE_STRUCT_DEF, interface_name.c_str());
}

const InterfaceDefinition *InterfaceOperations::find_interface_definition(
    const std::string &interface_name) {
    auto it = interface_definitions_.find(interface_name);
    if (it != interface_definitions_.end()) {
        return &it->second;
    }
    return nullptr;
}

// ========================================================================
// Impl定義管理
// ========================================================================

void InterfaceOperations::register_impl_definition(
    const ImplDefinition &impl_def) {
    auto trim = [](const std::string &text) {
        const char *whitespace = " \t\r\n";
        size_t start = text.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return std::string();
        }
        size_t end = text.find_last_not_of(whitespace);
        return text.substr(start, end - start + 1);
    };

    auto normalize_struct = [](const std::string &name) {
        const std::string prefix = "struct ";
        if (name.rfind(prefix, 0) == 0) {
            return name.substr(prefix.size());
        }
        return name;
    };

    ImplDefinition stored_def(trim(impl_def.interface_name),
                              trim(impl_def.struct_name));
    stored_def.methods = impl_def.methods;
    stored_def.destructor =
        impl_def.destructor; // v0.10.0: デストラクタもコピー

    auto existing = std::find_if(
        impl_definitions_.begin(), impl_definitions_.end(),
        [&](const ImplDefinition &candidate) {
            return candidate.interface_name == stored_def.interface_name &&
                   candidate.struct_name == stored_def.struct_name;
        });

    if (existing != impl_definitions_.end()) {
        *existing = stored_def;
        debug_print(
            "IMPL_DEF_STORAGE: Updated existing impl '%s' for '%s' (addr=%p)\n",
            stored_def.interface_name.c_str(), stored_def.struct_name.c_str(),
            (void *)&impl_definitions_);
    } else {
        // 新規implブロックを追加する前に、同じ構造体に対する既存のimplブロックと
        // メソッド名の衝突がないかチェック
        std::string normalized_new_struct =
            normalize_struct(stored_def.struct_name);

        // この構造体の既存のimplブロックを全て収集
        std::map<std::string, std::string> method_to_interface;

        for (const auto &existing_impl : impl_definitions_) {
            std::string normalized_existing =
                normalize_struct(existing_impl.struct_name);

            // 同じ構造体に対するimplブロックの場合
            if (normalized_existing == normalized_new_struct) {
                // 既存のimplブロックの全メソッドを記録
                for (const auto *method : existing_impl.methods) {
                    if (method) {
                        method_to_interface[method->name] =
                            existing_impl.interface_name;
                    }
                }
            }
        }

        // 新しいimplブロックのメソッドが既存のものと衝突しないかチェック
        for (const auto *new_method : stored_def.methods) {
            if (new_method) {
                auto it = method_to_interface.find(new_method->name);
                if (it != method_to_interface.end()) {
                    // メソッド名が衝突している
                    throw std::runtime_error(
                        "Method name conflict: method '" + new_method->name +
                        "' is already defined in impl '" + it->second +
                        "' for type '" + normalized_new_struct +
                        "'. Cannot redefine in impl '" +
                        stored_def.interface_name + "'.");
                }
            }
        }

        impl_definitions_.emplace_back(stored_def);
        existing = std::prev(impl_definitions_.end());
        debug_print("IMPL_DEF_STORAGE: Added new impl '%s' for '%s' (total: "
                    "%zu, addr=%p)\n",
                    stored_def.interface_name.c_str(),
                    stored_def.struct_name.c_str(), impl_definitions_.size(),
                    (void *)&impl_definitions_);
    }

    auto register_function = [&](const std::string &key,
                                 const ASTNode *method) {
        if (key.empty() || !method) {
            return;
        }
        interpreter_->register_function_to_global(key, method);
        debug_print("IMPL_REGISTER: Registered method key '%s'\n", key.c_str());
    };

    // 型名のマングル変換（Vector<int, SystemAllocator> →
    // Vector_int_SystemAllocator）
    auto mangle_type_name = [](const std::string &type_name) -> std::string {
        std::string mangled = type_name;
        // '<', '>', ' ', ',' を '_' に置換
        for (char &c : mangled) {
            if (c == '<' || c == '>' || c == ' ' || c == ',') {
                c = '_';
            }
        }
        // 連続するアンダースコアを1つに
        std::string result;
        char prev = '\0';
        for (char c : mangled) {
            if (c != '_' || prev != '_') {
                result += c;
            }
            prev = c;
        }
        // 末尾のアンダースコアを削除
        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }
        return result;
    };

    std::string normalized_struct_name =
        normalize_struct(existing->struct_name);
    std::string original_struct_name = existing->struct_name;
    std::string interface_name = existing->interface_name;

    // マングル名も生成
    std::string mangled_struct_name = mangle_type_name(normalized_struct_name);

    for (const auto *method : existing->methods) {
        if (!method) {
            continue;
        }

        std::string method_name = method->name;

        // 元の型名で登録（Vector<int, SystemAllocator>::init）
        if (!normalized_struct_name.empty()) {
            register_function(normalized_struct_name + "::" + method_name,
                              method);
        }

        // マングル名でも登録（Vector_int_SystemAllocator::init）
        if (!mangled_struct_name.empty() &&
            mangled_struct_name != normalized_struct_name) {
            register_function(mangled_struct_name + "::" + method_name, method);
        }

        if (!original_struct_name.empty() &&
            original_struct_name != normalized_struct_name) {
            register_function(original_struct_name + "::" + method_name,
                              method);
        }

        if (!interface_name.empty()) {
            std::string interface_key = interface_name + "_" +
                                        normalized_struct_name + "_" +
                                        method_name;
            register_function(interface_key, method);

            if (!original_struct_name.empty() &&
                original_struct_name != normalized_struct_name) {
                register_function(interface_name + "_" + original_struct_name +
                                      "_" + method_name,
                                  method);
            }
        }
    }

    debug_msg(
        DebugMsgId::PARSE_STRUCT_DEF,
        (existing->interface_name + "_for_" + existing->struct_name).c_str());

    debug_print("IMPL_DEF_END: Finishing register_impl_definition, "
                "impl_definitions_.size()=%zu\n",
                impl_definitions_.size());
}

const std::vector<ImplDefinition> &
InterfaceOperations::get_impl_definitions() const {
    debug_print("GET_IMPL_DEFS: Called! size=%zu, addr=%p\n",
                impl_definitions_.size(), (void *)&impl_definitions_);
    return impl_definitions_;
}

const ImplDefinition *
InterfaceOperations::find_impl_for_struct(const std::string &struct_name,
                                          const std::string &interface_name) {
    for (const auto &impl_def : impl_definitions_) {
        if (impl_def.struct_name == struct_name &&
            impl_def.interface_name == interface_name) {
            return &impl_def;
        }
    }
    return nullptr;
}

// ========================================================================
// Interface型変数管理
// ========================================================================

void InterfaceOperations::create_interface_variable(
    const std::string &var_name, const std::string &interface_name) {
    Variable var(interface_name, true); // interface用コンストラクタを使用
    var.is_assigned = false;

    interpreter_->add_variable_to_current_scope(var_name, var);
    debug_msg(DebugMsgId::PARSE_VAR_DECL, var_name.c_str(),
              interface_name.c_str());
}

Variable *
InterfaceOperations::get_interface_variable(const std::string &var_name) {
    Variable *var = interpreter_->find_variable(var_name);
    if (var && var->type == TYPE_INTERFACE) {
        return var;
    }
    return nullptr;
}

// ========================================================================
// Impl宣言処理
// ========================================================================

void InterfaceOperations::handle_impl_declaration(const ASTNode *node) {
    if (!node) {
        return;
    }

    auto trim = [](const std::string &text) {
        const char *whitespace = " \t\r\n";
        size_t start = text.find_first_not_of(whitespace);
        if (start == std::string::npos) {
            return std::string();
        }
        size_t end = text.find_last_not_of(whitespace);
        return text.substr(start, end - start + 1);
    };

    const std::string delimiter = "_for_";
    std::string combined_name = node->name;
    std::string interface_name = combined_name;
    std::string struct_name = node->type_name;

    size_t delim_pos = combined_name.find(delimiter);
    if (delim_pos != std::string::npos) {
        interface_name = combined_name.substr(0, delim_pos);
        if (struct_name.empty()) {
            struct_name = combined_name.substr(delim_pos + delimiter.size());
        }
    }

    interface_name = trim(interface_name);
    struct_name = trim(struct_name);

    // v0.10.0: impl Struct（インターフェースなし）の場合もimpl定義を登録する
    // interface_nameが空の場合は、デストラクタやコンストラクタのためのimpl定義
    // 以前はここで早期リターンしていたが、デストラクタのために登録を続ける

    ImplDefinition impl_def(interface_name, struct_name);

    // impl static変数の登録（implコンテキストは一時的に設定）
    for (const auto &static_var_node : node->impl_static_variables) {
        if (!static_var_node ||
            static_var_node->node_type != ASTNodeType::AST_VAR_DECL) {
            continue;
        }
        // 各static変数登録時に一時的にコンテキストを設定
        interpreter_->enter_impl_context(interface_name, struct_name);
        interpreter_->create_impl_static_variable(static_var_node->name,
                                                  static_var_node.get());
        interpreter_->exit_impl_context();
    }

    // v0.10.0: コンストラクタ、デストラクタ、メソッドの登録
    for (const auto &method_node : node->arguments) {
        if (!method_node) {
            continue;
        }

        if (method_node->node_type == ASTNodeType::AST_CONSTRUCTOR_DECL) {
            // コンストラクタはargumentsに含まれるが、ImplDefinitionには追加しない
            // (struct_constructors_に直接格納される)
            continue;
        } else if (method_node->node_type == ASTNodeType::AST_DESTRUCTOR_DECL) {
            // v0.10.0: デストラクタをImplDefinitionに追加
            impl_def.destructor = method_node.get();
            continue;
        } else if (method_node->node_type == ASTNodeType::AST_FUNC_DECL) {
            // 通常のメソッド
            if (method_node->type_name.empty()) {
                method_node->type_name = struct_name;
            }
            method_node->qualified_name =
                interface_name + "::" + struct_name + "::" + method_node->name;

            impl_def.add_method(method_node.get());
        }
    }

    register_impl_definition(impl_def);
}

// ========================================================================
// Self処理用ヘルパー
// ========================================================================

std::string InterfaceOperations::get_self_receiver_path() {
    // デバッグモードの場合、self_receiver_pathを取得
    // 現在は簡単な実装として、最初に見つかったself以外の構造体変数を返す

    // ローカルスコープから検索
    auto &scope_stack = interpreter_->get_scope_stack();
    for (auto &scope : scope_stack) {
        for (auto &[name, var] : scope.variables) {
            if (name != "self" && var.is_struct && var.is_assigned) {
                debug_print("SELF_RECEIVER_DEBUG: Found receiver path: %s\n",
                            name.c_str());
                return name;
            }
        }
    }

    // グローバルスコープもチェック
    auto &global_scope = interpreter_->get_global_scope();
    for (auto &[name, var] : global_scope.variables) {
        if (name != "self" && var.is_struct && var.is_assigned) {
            debug_print("SELF_RECEIVER_DEBUG: Found global receiver path: %s\n",
                        name.c_str());
            return name;
        }
    }

    debug_print("SELF_RECEIVER_DEBUG: No receiver path found\n");
    return "";
}

void InterfaceOperations::sync_self_to_receiver(
    const std::string &receiver_path) {
    Variable *self_var = interpreter_->find_variable("self");
    Variable *receiver_var = interpreter_->find_variable(receiver_path);

    if (!self_var || !receiver_var) {
        debug_print(
            "SYNC_SELF_DEBUG: Variables not found: self=%p, receiver=%p\n",
            (void *)self_var, (void *)receiver_var);
        return;
    }

    debug_print("SYNC_SELF_DEBUG: Syncing self to %s\n", receiver_path.c_str());

    // self.memberからreceiver.memberに値をコピー
    for (auto &[member_name, self_member] : self_var->struct_members) {
        std::string receiver_member_name = receiver_path + "." + member_name;
        Variable *receiver_member =
            interpreter_->find_variable(receiver_member_name);

        if (receiver_member) {
            if (self_member.type == TYPE_STRING) {
                receiver_member->str_value = self_member.str_value;
            } else {
                receiver_member->value = self_member.value;
            }
            receiver_member->is_assigned = self_member.is_assigned;

            // receiver構造体のstruct_membersも更新
            if (receiver_var->struct_members.find(member_name) !=
                receiver_var->struct_members.end()) {
                receiver_var->struct_members[member_name] = self_member;
            }

            debug_print("SYNC_SELF_DEBUG: Synced %s to %s\n",
                        ("self." + member_name).c_str(),
                        receiver_member_name.c_str());
        }
    }
}

// ========================================================================
// 一時変数管理（メソッドチェーン用）
// ========================================================================

void InterfaceOperations::add_temp_variable(const std::string &name,
                                            const Variable &var) {
    interpreter_->add_variable_to_current_scope(name, var);
    debug_print("TEMP_VAR: Added temporary variable %s\n", name.c_str());
}

void InterfaceOperations::remove_temp_variable(const std::string &name) {
    auto &current_scope = interpreter_->current_scope();
    auto &vars = current_scope.variables;
    auto it = vars.find(name);
    if (it != vars.end()) {
        vars.erase(it);
        debug_print("TEMP_VAR: Removed temporary variable %s\n", name.c_str());
    }
}

void InterfaceOperations::clear_temp_variables() {
    auto &current_scope = interpreter_->current_scope();
    auto &vars = current_scope.variables;
    for (auto it = vars.begin(); it != vars.end();) {
        if (it->first.substr(0, 12) == "__temp_chain" ||
            it->first.substr(0, 12) == "__chain_self") {
            debug_print("TEMP_VAR: Clearing temporary variable %s\n",
                        it->first.c_str());
            it = vars.erase(it);
        } else {
            ++it;
        }
    }
}

// ========================================================================
// v0.11.0 Phase 1a: インターフェース境界チェック
// ========================================================================

/**
 * @brief 型がインターフェースを実装しているかチェック
 * @param type_name 型名（例: "SystemAllocator"）
 * @param interface_name インターフェース名（例: "Allocator"）
 * @return 実装している場合true
 */
bool InterfaceOperations::check_interface_bound(
    const std::string &type_name, const std::string &interface_name) {

    if (interpreter_->is_debug_mode()) {
        std::cerr << "[TYPE_CHECK] Checking if '" << type_name
                  << "' implements '" << interface_name << "'" << std::endl;
        std::cerr << "[TYPE_CHECK] Available impls:" << std::endl;
        for (const auto &impl_def : impl_definitions_) {
            std::cerr << "  - " << impl_def.interface_name << " for "
                      << impl_def.struct_name << std::endl;
        }
    }

    // インターフェース定義が存在するか確認
    const InterfaceDefinition *interface_def =
        find_interface_definition(interface_name);
    if (!interface_def) {
        if (interpreter_->is_debug_mode()) {
            std::cerr << "[TYPE_CHECK] Interface '" << interface_name
                      << "' not found" << std::endl;
        }
        return false; // インターフェース自体が未定義
    }

    // 型がインターフェースを実装しているか確認
    const ImplDefinition *impl_def =
        find_impl_for_struct(type_name, interface_name);

    if (interpreter_->is_debug_mode()) {
        std::cerr << "[TYPE_CHECK] Result: "
                  << (impl_def ? "FOUND" : "NOT FOUND") << std::endl;
    }

    return impl_def != nullptr;
}

/**
 * @brief ジェネリック型インスタンス化時にインターフェース境界を検証
 * @param struct_name 構造体名（例: "Vector"）
 * @param type_parameters 型パラメータリスト（例: ["T", "A"]）
 * @param type_arguments 型引数リスト（例: ["int", "SystemAllocator"]）
 * @param interface_bounds 型パラメータのインターフェース境界（例: {"A":
 * "Allocator", "Clone"}）
 * @throws std::runtime_error 型引数が必要なインターフェースを実装していない場合
 */
void InterfaceOperations::validate_interface_bounds(
    const std::string &struct_name,
    const std::vector<std::string> &type_parameters,
    const std::vector<std::string> &type_arguments,
    const std::unordered_map<std::string, std::vector<std::string>>
        &interface_bounds) {

    // 型パラメータと型引数の数が一致しているか確認（この段階では既にチェック済みのはず）
    if (type_parameters.size() != type_arguments.size()) {
        throw std::runtime_error("Type parameter count mismatch in " +
                                 struct_name);
    }

    // 各型パラメータについて、複数のインターフェース境界がある場合、
    // メソッド名の衝突をチェック
    for (const auto &bound_entry : interface_bounds) {
        const std::string &param_name = bound_entry.first;
        const std::vector<std::string> &interfaces = bound_entry.second;

        if (interfaces.size() > 1) {
            // 複数のインターフェースがある場合、メソッド名の衝突をチェック
            std::map<std::string, std::vector<std::string>>
                method_to_interfaces;

            for (const auto &interface_name : interfaces) {
                auto it = interface_definitions_.find(interface_name);
                if (it != interface_definitions_.end()) {
                    const InterfaceDefinition &interface_def = it->second;

                    // このインターフェースの全メソッドをチェック
                    for (const auto &method : interface_def.methods) {
                        method_to_interfaces[method.name].push_back(
                            interface_name);
                    }
                }
            }

            // 同じメソッド名が複数のインターフェースに存在するかチェック
            for (const auto &entry : method_to_interfaces) {
                const std::string &method_name = entry.first;
                const std::vector<std::string> &defining_interfaces =
                    entry.second;

                if (defining_interfaces.size() > 1) {
                    std::string error_msg =
                        "Method name conflict: method '" + method_name +
                        "' is defined in multiple interfaces (";
                    for (size_t i = 0; i < defining_interfaces.size(); ++i) {
                        if (i > 0)
                            error_msg += ", ";
                        error_msg += defining_interfaces[i];
                    }
                    error_msg += ") required by type parameter '" + param_name +
                                 "' in '" + struct_name + "<";

                    // 型パラメータリストを構築
                    for (size_t j = 0; j < type_parameters.size(); ++j) {
                        if (j > 0)
                            error_msg += ", ";
                        error_msg += type_parameters[j];

                        auto param_bound =
                            interface_bounds.find(type_parameters[j]);
                        if (param_bound != interface_bounds.end()) {
                            error_msg += ": ";
                            for (size_t k = 0; k < param_bound->second.size();
                                 ++k) {
                                if (k > 0)
                                    error_msg += " + ";
                                error_msg += param_bound->second[k];
                            }
                        }
                    }
                    error_msg += ">'";

                    throw std::runtime_error(error_msg);
                }
            }
        }
    }

    // 各型パラメータについて、インターフェース境界があればチェック
    for (size_t i = 0; i < type_parameters.size(); ++i) {
        const std::string &param_name = type_parameters[i];
        const std::string &arg_type = type_arguments[i];

        // この型パラメータに複数のインターフェース境界があるか確認
        auto bound_it = interface_bounds.find(param_name);
        if (bound_it != interface_bounds.end()) {
            const std::vector<std::string> &required_interfaces =
                bound_it->second;

            // すべてのインターフェースが実装されているかチェック
            for (const auto &required_interface : required_interfaces) {
                if (!check_interface_bound(arg_type, required_interface)) {
                    std::string error_msg =
                        "Type '" + arg_type +
                        "' does not implement interface '" +
                        required_interface + "' required by type parameter '" +
                        param_name + "' in '" + struct_name + "<";

                    // 型パラメータリストを構築
                    for (size_t j = 0; j < type_parameters.size(); ++j) {
                        if (j > 0)
                            error_msg += ", ";
                        error_msg += type_parameters[j];

                        auto param_bound =
                            interface_bounds.find(type_parameters[j]);
                        if (param_bound != interface_bounds.end()) {
                            error_msg += ": ";
                            for (size_t k = 0; k < param_bound->second.size();
                                 ++k) {
                                if (k > 0)
                                    error_msg += " + ";
                                error_msg += param_bound->second[k];
                            }
                        }
                    }
                    error_msg += ">'";

                    throw std::runtime_error(error_msg);
                }
            }
        }
    }
}
