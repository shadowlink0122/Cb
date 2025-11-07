#include "interfaces.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug_messages.h"
#include "../../core/interpreter.h"
#include "../../evaluator/functions/generic_instantiation.h"
#include <algorithm>
#include <sstream>

InterfaceOperations::InterfaceOperations(Interpreter *interpreter)
    : interpreter_(interpreter) {
    // v0.11.0: deque使用によりreserve不要（要素追加時に既存要素が移動しない）
}

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
    stored_def.constructors =
        impl_def.constructors; // v0.11.0: constructorsもコピー
    stored_def.destructor =
        impl_def.destructor; // v0.10.0: デストラクタもコピー
    // v0.13.1: 生ポインタなのでそのままコピー（所有権はパーサーが持つ）
    stored_def.impl_node = impl_def.impl_node;

    // v0.11.0: type_parameter_mapとis_generic_instanceもコピー
    stored_def.type_parameter_map = impl_def.type_parameter_map;
    stored_def.is_generic_instance = impl_def.is_generic_instance;

    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "[REGISTER_IMPL] Copying ImplDefinition: methods.size()=%zu, ");
    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "[REGISTER_IMPL]   methods.data()=%p (from %p)",
                 (void *)stored_def.methods.data(),
                 (void *)impl_def.methods.data());
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }
    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "[REGISTER_IMPL]   constructors.data()=%p (from %p)",
                 (void *)stored_def.constructors.data(),
                 (void *)impl_def.constructors.data());
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }
    for (size_t i = 0; i < stored_def.constructors.size(); ++i) {
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[REGISTER_IMPL]   constructors[%zu]=%p", i,
                     (void *)stored_def.constructors[i]);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    auto existing = std::find_if(
        impl_definitions_.begin(), impl_definitions_.end(),
        [&](const ImplDefinition &candidate) {
            return candidate.interface_name == stored_def.interface_name &&
                   candidate.struct_name == stored_def.struct_name;
        });

    if (existing != impl_definitions_.end()) {
        *existing = stored_def;
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "IMPL_DEF_STORAGE: Updated existing impl '%s' for '%s' "
                     "(addr=%p)",
                     stored_def.interface_name.c_str(),
                     stored_def.struct_name.c_str(),
                     (void *)&impl_definitions_);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
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

        impl_definitions_.push_back(stored_def);
        existing = std::prev(impl_definitions_.end());
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "IMPL_DEF_STORAGE: Added new impl '%s' for '%s' (total: ");
    }

    auto register_function = [&](const std::string &key,
                                 const ASTNode *method) {
        if (key.empty() || !method) {
            return;
        }
        interpreter_->register_function_to_global(key, method);
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "IMPL_REGISTER: Registered method key '%s'", key.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
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

    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "IMPL_DEF_END: Finishing register_impl_definition, ");
}

const std::deque<ImplDefinition> &
InterfaceOperations::get_impl_definitions() const {
    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "GET_IMPL_DEFS: Called! size=%zu, addr=%p",
                 impl_definitions_.size(), (void *)&impl_definitions_);
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }
    return impl_definitions_;
}

const ImplDefinition *
InterfaceOperations::find_impl_for_struct(const std::string &struct_name,
                                          const std::string &interface_name) {
    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "[FIND_IMPL] Searching for: struct='%s', interface='%s'",
                 struct_name.c_str(), interface_name.c_str());
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }

    // 1. 完全一致で検索（既存の動作）
    for (const auto &impl_def : impl_definitions_) {
        if (impl_def.struct_name == struct_name &&
            impl_def.interface_name == interface_name) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[FIND_IMPL] Found exact match");
            return &impl_def;
        }
    }

    // 2. ジェネリックimplのインスタンス化を試みる
    // 例: Vector<int>が要求された場合、impl VectorOps<T> for
    // Vector<T>を探してインスタンス化

    // struct_nameから型引数を抽出（例: "Vector<int>" -> ["int"]）
    std::vector<std::string> type_arguments;
    std::string base_struct_name;

    size_t lt_pos = struct_name.find('<');
    if (lt_pos != std::string::npos) {
        base_struct_name = struct_name.substr(0, lt_pos);
        size_t gt_pos = struct_name.rfind('>');
        if (gt_pos != std::string::npos) {
            std::string args_str =
                struct_name.substr(lt_pos + 1, gt_pos - lt_pos - 1);
            std::stringstream ss(args_str);
            std::string arg;
            while (std::getline(ss, arg, ',')) {
                // トリム
                size_t start = arg.find_first_not_of(" \t");
                size_t end = arg.find_last_not_of(" \t");
                if (start != std::string::npos) {
                    type_arguments.push_back(
                        arg.substr(start, end - start + 1));
                }
            }
        }
    }

    // interface_nameからも同様に抽出
    std::string base_interface_name;
    if (!interface_name.empty()) {
        size_t if_lt_pos = interface_name.find('<');
        if (if_lt_pos != std::string::npos) {
            base_interface_name = interface_name.substr(0, if_lt_pos);
        } else {
            base_interface_name = interface_name;
        }
    }

    // ジェネリックimplを探す（例: impl VectorOps<T> for Vector<T>）
    if (!type_arguments.empty()) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[FIND_IMPL] Looking for generic impl: base_struct='%s', ");

        std::string generic_struct_pattern = base_struct_name + "<T>";
        std::string generic_interface_pattern =
            base_interface_name.empty() ? "" : base_interface_name + "<T>";

        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "[FIND_IMPL] Patterns: struct='%s', interface='%s'",
                     generic_struct_pattern.c_str(),
                     generic_interface_pattern.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }

        for (const auto &impl_def : impl_definitions_) {
            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[FIND_IMPL] Checking impl: struct='%s', ");

            // ジェネリックパターンにマッチするか確認
            // 重要: 型パラメータを含むimplのみをマッチ対象にする
            // これにより、インスタンス化済みのVector<int>がVector<T>として再処理されるのを防ぐ
            // v0.14.0: 複数の型パラメータにも対応（例: Map<K, V>）
            bool struct_match = false;

            // impl_def.struct_nameから型パラメータ数を取得
            size_t impl_type_param_count = 0;
            if (impl_def.impl_node &&
                !impl_def.impl_node->type_parameters.empty()) {
                impl_type_param_count =
                    impl_def.impl_node->type_parameters.size();
            }

            // base_struct_nameが一致し、型パラメータ数も一致する場合にマッチ
            size_t impl_lt_pos = impl_def.struct_name.find('<');
            if (impl_lt_pos != std::string::npos) {
                std::string impl_base =
                    impl_def.struct_name.substr(0, impl_lt_pos);
                if (impl_base == base_struct_name &&
                    impl_type_param_count == type_arguments.size()) {
                    struct_match = true;
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "[FIND_IMPL] Struct match: impl_base='%s', ");
                }
            }

            bool interface_match = false;

            if (interface_name.empty()) {
                // interface_nameが指定されていない場合（メソッド検索時）、
                // 非空のinterface_nameを持つimplをマッチング
                interface_match = !impl_def.interface_name.empty();
            } else {
                // interface_nameが指定されている場合
                // v0.14.0: 複数の型パラメータにも対応
                size_t if_impl_lt_pos = impl_def.interface_name.find('<');
                if (if_impl_lt_pos != std::string::npos) {
                    std::string impl_if_base =
                        impl_def.interface_name.substr(0, if_impl_lt_pos);
                    if (impl_if_base == base_interface_name &&
                        impl_type_param_count == type_arguments.size()) {
                        interface_match = true;
                        {
                            char dbg_buf[512];
                            snprintf(dbg_buf, sizeof(dbg_buf),
                                     "[FIND_IMPL] Interface match: "
                                     "impl_if_base='%s'",
                                     impl_if_base.c_str());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                    }
                } else if (impl_def.interface_name == interface_name) {
                    // 型パラメータなしの完全一致
                    interface_match = true;
                }
            }

            debug_msg(DebugMsgId::GENERIC_DEBUG,
                      "[FIND_IMPL] Match result: struct_match=%d, ");

            // v0.13.1: interface impl と constructor-only impl の両方を処理
            // 1. interface implの場合: struct_match && interface_match
            // 2. constructor-only implの場合: struct_match &&
            // interface_name.empty()
            bool is_interface_impl = struct_match && interface_match;
            bool is_constructor_impl =
                struct_match && impl_def.interface_name.empty();

            if ((is_interface_impl || is_constructor_impl) &&
                impl_def.impl_node) {
                // v0.13.1: 実行時型解決アプローチ
                // ノードをクローンせず、型マッピング情報を含む新しいImplDefinitionを作成
                try {
                    auto result =
                        GenericInstantiation::instantiate_generic_impl(
                            impl_def.impl_node, type_arguments,
                            impl_def.interface_name, impl_def.struct_name);

                    auto &inst_interface = std::get<0>(result);
                    auto &inst_struct = std::get<1>(result);
                    // inst_node は nullptr（もう使わない）

                    {
                        char dbg_buf[512];
                        snprintf(
                            dbg_buf, sizeof(dbg_buf),
                            "[GENERIC_IMPL] Instantiated (runtime): %s for %s",
                            inst_interface.c_str(), inst_struct.c_str());
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }

                    // 型パラメータマッピングを作成
                    std::map<std::string, std::string> type_map;
                    auto type_params = impl_def.impl_node->type_parameters;
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "[GENERIC_IMPL] Building type_map: ");
                    for (size_t i = 0;
                         i < type_params.size() && i < type_arguments.size();
                         ++i) {
                        {
                            char dbg_buf[512];
                            snprintf(dbg_buf, sizeof(dbg_buf),
                                     "[GENERIC_IMPL]   Mapping: %s -> %s",
                                     type_params[i].c_str(),
                                     type_arguments[i].c_str());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                        type_map[type_params[i]] = type_arguments[i];
                    }

                    // 既にこのインスタンス化が存在するかチェック
                    for (size_t cached_idx = 0;
                         cached_idx < impl_definitions_.size(); ++cached_idx) {
                        const auto &impl = impl_definitions_[cached_idx];
                        if (impl.struct_name == inst_struct &&
                            impl.interface_name == inst_interface) {
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "[GENERIC_IMPL] Found cached instance: ");
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "[GENERIC_IMPL]   impl_node=%p, ");
                            debug_msg(DebugMsgId::GENERIC_DEBUG,
                                      "[GENERIC_IMPL]   ");
                            if (!impl.type_parameter_map.empty()) {
                                for (const auto &pair :
                                     impl.type_parameter_map) {
                                    {
                                        char dbg_buf[512];
                                        snprintf(dbg_buf, sizeof(dbg_buf),
                                                 "[GENERIC_IMPL]     %s -> %s",
                                                 pair.first.c_str(),
                                                 pair.second.c_str());
                                        debug_msg(DebugMsgId::GENERIC_DEBUG,
                                                  dbg_buf);
                                    }
                                }
                            }
                            if (!impl.methods.empty()) {
                                {
                                    char dbg_buf[512];
                                    snprintf(dbg_buf, sizeof(dbg_buf),
                                             "[GENERIC_IMPL]   First method=%p",
                                             (void *)impl.methods[0]);
                                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                                              dbg_buf);
                                }
                            }

                            // v0.13.1: constructor-only
                            // implからconstructorを追加 interface
                            // implにconstructorがない場合、同じstructのconstructor-only
                            // implを探す
                            if (impl.constructors.empty()) {
                                debug_msg(DebugMsgId::GENERIC_DEBUG,
                                          "[GENERIC_IMPL] No constructors in ");
                                for (const auto &ctor_impl :
                                     impl_definitions_) {
                                    if (ctor_impl.struct_name ==
                                            generic_struct_pattern &&
                                        ctor_impl.interface_name.empty() &&
                                        !ctor_impl.constructors.empty()) {
                                        debug_msg(DebugMsgId::GENERIC_DEBUG,
                                                  "[GENERIC_IMPL] Found ");
                                        // constructorを追加（非const参照経由）
                                        auto &mutable_impl =
                                            impl_definitions_[cached_idx];
                                        for (const auto *ctor :
                                             ctor_impl.constructors) {
                                            mutable_impl.constructors.push_back(
                                                ctor);
                                            debug_msg(
                                                DebugMsgId::GENERIC_DEBUG,
                                                "[GENERIC_IMPL]   Added ");
                                        }
                                        break;
                                    }
                                }
                            }

                            // インデックス経由でポインタを返す（参照の安全性向上）
                            return &impl_definitions_[cached_idx];
                        }
                    }

                    // 新しいImplDefinitionを作成（元のノードを参照、型マッピングを保存）
                    ImplDefinition new_impl;
                    new_impl.interface_name = inst_interface;
                    new_impl.struct_name = inst_struct;
                    new_impl.impl_node =
                        impl_def.impl_node; // 元のジェネリックノードを参照
                    new_impl.type_parameter_map = type_map;
                    new_impl.is_generic_instance = true;

                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "[GENERIC_IMPL] Creating new instance: ");
                    for (const auto &pair : type_map) {
                        {
                            char dbg_buf[512];
                            snprintf(dbg_buf, sizeof(dbg_buf),
                                     "[GENERIC_IMPL]   Setting map: %s -> %s",
                                     pair.first.c_str(), pair.second.c_str());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                    }

                    // implノードからメソッド、コンストラクタ、デストラクタを抽出
                    if (impl_def.impl_node &&
                        !impl_def.impl_node->arguments.empty()) {
                        // v0.11.0: ベクター再配置を防ぐため、事前にreserve
                        new_impl.methods.reserve(
                            impl_def.impl_node->arguments.size());
                        new_impl.constructors.reserve(
                            impl_def.impl_node->arguments.size());

                        debug_msg(DebugMsgId::GENERIC_DEBUG,
                                  "[GENERIC_IMPL] Extracting methods from ");

                        for (const auto &arg : impl_def.impl_node->arguments) {
                            if (!arg)
                                continue;

                            debug_msg(
                                DebugMsgId::GENERIC_DEBUG,
                                "[GENERIC_IMPL]   arg=%p, arg.get()=%p, ");

                            // AST_FUNC_DECL (methods) と AST_CONSTRUCTOR_DECL
                            // (constructors) と AST_DESTRUCTOR_DECL
                            // (destructor) を処理
                            if (arg->node_type == ASTNodeType::AST_FUNC_DECL) {
                                if (arg->name == "new") {
                                    new_impl.constructors.push_back(arg.get());
                                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                                              "[GENERIC_IMPL]     Added ");
                                } else if (arg->name.find('~') == 0) {
                                    new_impl.destructor = arg.get();
                                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                                              "[GENERIC_IMPL]     Added ");
                                } else {
                                    new_impl.methods.push_back(arg.get());
                                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                                              "[GENERIC_IMPL]     Added ");
                                }
                            } else if (arg->node_type ==
                                       ASTNodeType::AST_CONSTRUCTOR_DECL) {
                                new_impl.constructors.push_back(arg.get());
                                debug_msg(
                                    DebugMsgId::GENERIC_DEBUG,
                                    "[GENERIC_IMPL]     Added constructor ");
                            } else if (arg->node_type ==
                                       ASTNodeType::AST_DESTRUCTOR_DECL) {
                                new_impl.destructor = arg.get();
                                debug_msg(DebugMsgId::GENERIC_DEBUG,
                                          "[GENERIC_IMPL]     Added ");
                            }
                        }
                    }

                    impl_definitions_.push_back(new_impl);

                    size_t pushed_idx = impl_definitions_.size() - 1;

                    // v0.13.1: constructor-only implからconstructorを追加
                    // interface
                    // implにconstructorがない場合、同じstructのconstructor-only
                    // implを探す
                    if (impl_definitions_[pushed_idx].constructors.empty()) {
                        debug_msg(
                            DebugMsgId::GENERIC_DEBUG,
                            "[GENERIC_IMPL] New impl has no constructors, ");
                        for (const auto &ctor_impl : impl_definitions_) {
                            if (ctor_impl.struct_name ==
                                    generic_struct_pattern &&
                                ctor_impl.interface_name.empty() &&
                                !ctor_impl.constructors.empty()) {
                                debug_msg(
                                    DebugMsgId::GENERIC_DEBUG,
                                    "[GENERIC_IMPL] Found constructor-only ");
                                debug_msg(DebugMsgId::GENERIC_DEBUG,
                                          "[GENERIC_IMPL]   "
                                          "ctor_impl.impl_node=%p, ");

                                // constructorを追加（ctor_implのimpl_nodeからではなく、直接constructorsから）
                                for (const auto *ctor :
                                     ctor_impl.constructors) {
                                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                                              "[GENERIC_IMPL]   Constructor ");
                                    impl_definitions_[pushed_idx]
                                        .constructors.push_back(ctor);
                                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                                              "[GENERIC_IMPL]   Added "
                                              "constructor at ");
                                }
                                break;
                            }
                        }
                    }

                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "[GENERIC_IMPL] Created runtime instance: %zu ");

                    // push_back後の確認
                    debug_msg(DebugMsgId::GENERIC_DEBUG,
                              "[GENERIC_IMPL] After push_back: ");

                    // v0.11.0: メソッドをグローバル関数として登録
                    // Queue<int>::enqueue 形式で登録（マングリングしない）
                    // 注意: impl_definitions_が再配置される可能性があるため、
                    // 参照ではなくインデックスを使用
                    size_t impl_index = impl_definitions_.size() - 1;

                    for (const auto *method :
                         impl_definitions_[impl_index].methods) {
                        if (!method)
                            continue;

                        // Queue<int>::enqueue 形式
                        std::string method_key =
                            inst_struct + "::" + method->name;
                        interpreter_->register_function_to_global(method_key,
                                                                  method);

                        {
                            char dbg_buf[512];
                            snprintf(dbg_buf, sizeof(dbg_buf),
                                     "[GENERIC_IMPL] Registered method: %s",
                                     method_key.c_str());
                            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                        }
                    }

                    return &impl_definitions_[impl_index];

                    debug_msg(
                        DebugMsgId::GENERIC_DEBUG,
                        "[GENERIC_IMPL] Warning: processed impl not found");
                    return nullptr;
                } catch (const std::exception &e) {
                    {
                        char dbg_buf[512];
                        snprintf(dbg_buf, sizeof(dbg_buf),
                                 "[GENERIC_IMPL] Failed to instantiate: %s",
                                 e.what());
                        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                    }
                }
            }
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

    if (interpreter_->is_debug_mode()) {
        debug_msg(DebugMsgId::GENERIC_DEBUG,
                  "[HANDLE_IMPL] Processing impl: struct='%s', ");
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

    // v0.12.0:
    // node->struct_nameを優先的に使用（ジェネリックimplでは正しい名前が設定されている）
    if (!node->struct_name.empty()) {
        struct_name = node->struct_name;
    }

    // v0.12.0: node->interface_nameを優先的に使用
    if (!node->interface_name.empty()) {
        interface_name = node->interface_name;
    } else {
        // 古い形式: 名前から抽出
        size_t delim_pos = combined_name.find(delimiter);
        if (delim_pos != std::string::npos) {
            interface_name = combined_name.substr(0, delim_pos);
            if (struct_name.empty()) {
                struct_name =
                    combined_name.substr(delim_pos + delimiter.size());
            }
        }
    }

    interface_name = trim(interface_name);
    struct_name = trim(struct_name);

    if (interpreter_->is_debug_mode()) {
        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "[HANDLE_IMPL] After extraction: struct='%s', interface='%s'",
                struct_name.c_str(), interface_name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }

    // v0.10.0: impl Struct（インターフェースなし）の場合もimpl定義を登録する
    // interface_nameが空の場合は、デストラクタやコンストラクタのためのimpl定義
    // 以前はここで早期リターンしていたが、デストラクタのために登録を続ける

    ImplDefinition impl_def(interface_name, struct_name);
    // v0.13.1: 生ポインタに戻したので、そのまま保存（所有権はパーサーが持つ）
    impl_def.impl_node = node;

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
            // v0.13.0: コンストラクタをstruct_constructors_に登録
            interpreter_->register_constructor(struct_name, method_node.get());

            if (interpreter_->is_debug_mode()) {
                debug_msg(DebugMsgId::GENERIC_DEBUG,
                          "[IMPL_REGISTER] Registered constructor for %s ");
            }
            continue;
        } else if (method_node->node_type == ASTNodeType::AST_DESTRUCTOR_DECL) {
            // v0.10.0: デストラクタをImplDefinitionに追加
            impl_def.destructor = method_node.get();

            // v0.13.0: デストラクタをstruct_destructors_にも登録
            interpreter_->register_destructor(struct_name, method_node.get());

            if (interpreter_->is_debug_mode()) {
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "[IMPL_REGISTER] Registered destructor for %s",
                             struct_name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
            continue;
        } else if (method_node->node_type == ASTNodeType::AST_FUNC_DECL) {
            // 通常のメソッド
            if (method_node->type_name.empty()) {
                method_node->type_name = struct_name;
            }
            method_node->qualified_name =
                interface_name + "::" + struct_name + "::" + method_node->name;

            impl_def.add_method(method_node.get());

            // v0.12.0: メソッドをグローバル関数として登録
            // メソッド名のマングリング: struct_name::method_name
            std::string method_key = struct_name + "::" + method_node->name;
            interpreter_->get_global_scope().functions[method_key] =
                method_node.get();

            if (interpreter_->is_debug_mode()) {
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "[IMPL_REGISTER] Registered method: %s",
                             method_key.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
            }
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
                {
                    char dbg_buf[512];
                    snprintf(dbg_buf, sizeof(dbg_buf),
                             "SELF_RECEIVER_DEBUG: Found receiver path: %s",
                             name.c_str());
                    debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
                }
                return name;
            }
        }
    }

    // グローバルスコープもチェック
    auto &global_scope = interpreter_->get_global_scope();
    for (auto &[name, var] : global_scope.variables) {
        if (name != "self" && var.is_struct && var.is_assigned) {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "SELF_RECEIVER_DEBUG: Found global receiver path: %s",
                         name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
            return name;
        }
    }

    debug_msg(DebugMsgId::GENERIC_DEBUG,
              "SELF_RECEIVER_DEBUG: No receiver path found");
    return "";
}

void InterfaceOperations::sync_self_to_receiver(
    const std::string &receiver_path) {
    Variable *self_var = interpreter_->find_variable("self");
    Variable *receiver_var = interpreter_->find_variable(receiver_path);

    if (!self_var || !receiver_var) {
        {
            char dbg_buf[512];
            snprintf(
                dbg_buf, sizeof(dbg_buf),
                "SYNC_SELF_DEBUG: Variables not found: self=%p, receiver=%p",
                (void *)self_var, (void *)receiver_var);
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
        return;
    }

    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "SYNC_SELF_DEBUG: Syncing self to %s", receiver_path.c_str());
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }

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

            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "SYNC_SELF_DEBUG: Synced %s to %s",
                         ("self." + member_name).c_str(),
                         receiver_member_name.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
        }
    }
}

// ========================================================================
// 一時変数管理（メソッドチェーン用）
// ========================================================================

void InterfaceOperations::add_temp_variable(const std::string &name,
                                            const Variable &var) {
    interpreter_->add_variable_to_current_scope(name, var);
    {
        char dbg_buf[512];
        snprintf(dbg_buf, sizeof(dbg_buf),
                 "TEMP_VAR: Added temporary variable %s", name.c_str());
        debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
    }
}

void InterfaceOperations::remove_temp_variable(const std::string &name) {
    auto &current_scope = interpreter_->current_scope();
    auto &vars = current_scope.variables;
    auto it = vars.find(name);
    if (it != vars.end()) {
        vars.erase(it);
        {
            char dbg_buf[512];
            snprintf(dbg_buf, sizeof(dbg_buf),
                     "TEMP_VAR: Removed temporary variable %s", name.c_str());
            debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
        }
    }
}

void InterfaceOperations::clear_temp_variables() {
    auto &current_scope = interpreter_->current_scope();
    auto &vars = current_scope.variables;
    for (auto it = vars.begin(); it != vars.end();) {
        if (it->first.substr(0, 12) == "__temp_chain" ||
            it->first.substr(0, 12) == "__chain_self") {
            {
                char dbg_buf[512];
                snprintf(dbg_buf, sizeof(dbg_buf),
                         "TEMP_VAR: Clearing temporary variable %s",
                         it->first.c_str());
                debug_msg(DebugMsgId::GENERIC_DEBUG, dbg_buf);
            }
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
