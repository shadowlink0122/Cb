#include "namespace_registry.h"
#include <algorithm>
#include <iostream>
#include <sstream>

// ========================================================================
// コンストラクタ/デストラクタ
// ========================================================================

NamespaceRegistry::NamespaceRegistry() {
    // デフォルトコンストラクタ
}

NamespaceRegistry::~NamespaceRegistry() {
    // デフォルトデストラクタ (メモリリーク - TODOで修正)
}

// ========================================================================
// ユーティリティメソッド
// ========================================================================

std::string
NamespaceRegistry::joinPath(const std::vector<std::string> &components) const {
    if (components.empty()) {
        return "";
    }
    std::string result = components[0];
    for (size_t i = 1; i < components.size(); ++i) {
        result += "::" + components[i];
    }
    return result;
}

std::vector<std::string>
NamespaceRegistry::splitPath(const std::string &path) const {
    std::vector<std::string> components;
    std::string current;
    for (size_t i = 0; i < path.length(); ++i) {
        if (i + 1 < path.length() && path[i] == ':' && path[i + 1] == ':') {
            if (!current.empty()) {
                components.push_back(current);
                current.clear();
            }
            ++i; // Skip second ':'
        } else {
            current += path[i];
        }
    }
    if (!current.empty()) {
        components.push_back(current);
    }
    return components;
}

// ========================================================================
// 名前空間登録
// ========================================================================

void NamespaceRegistry::registerNamespace(const std::string &ns_path,
                                          const ASTNode *decl_node,
                                          bool is_exported) {
    // std::cerr << "[NamespaceRegistry] registerNamespace called with: " <<
    // ns_path << std::endl;

    // 既に存在する場合は更新
    auto it = namespaces_.find(ns_path);
    if (it != namespaces_.end()) {
        it->second->declaration_node = decl_node;
        it->second->is_exported = is_exported;
        return;
    }

    // 新しいNamespaceInfoをヒープに作成
    NamespaceInfo *info = new NamespaceInfo();
    info->full_path = ns_path;
    info->path_components = splitPath(ns_path);
    info->declaration_node = decl_node;
    info->is_exported = is_exported;
    // symbols mapは空のまま（デフォルト初期化）

    // ポインタを挿入
    namespaces_[ns_path] = info;
}

// ========================================================================
// スコープ管理
// ========================================================================

void NamespaceRegistry::enterNamespace(const std::string &ns_name) {
    current_namespace_stack_.push_back(ns_name);

    // デバッグ出力
    // std::cerr << "[NamespaceRegistry] Entered namespace: " <<
    // getCurrentNamespace()
    //           << std::endl;
}

void NamespaceRegistry::exitNamespace() {
    if (!current_namespace_stack_.empty()) {
        current_namespace_stack_.pop_back();

        // デバッグ出力
        // std::cerr << "[NamespaceRegistry] Exited namespace, now in: "
        //           << (current_namespace_stack_.empty() ? "(global)" :
        //           getCurrentNamespace())
        //           << std::endl;
    }
}

std::string NamespaceRegistry::getCurrentNamespace() const {
    return joinPath(current_namespace_stack_);
}

// ========================================================================
// using namespace 管理
// ========================================================================

void NamespaceRegistry::addUsingNamespace(const std::string &ns_path) {
    // 重複チェック
    if (std::find(active_using_namespaces_.begin(),
                  active_using_namespaces_.end(),
                  ns_path) != active_using_namespaces_.end()) {
        return; // 既に追加済み
    }

    active_using_namespaces_.push_back(ns_path);

    // デバッグ出力
    // std::cerr << "[NamespaceRegistry] Added using namespace: " << ns_path
    //           << std::endl;
}

void NamespaceRegistry::clearUsingNamespaces() {
    active_using_namespaces_.clear();

    // デバッグ出力
    // std::cerr << "[NamespaceRegistry] Cleared using namespaces" <<
    // std::endl;
}

// ========================================================================
// シンボル登録
// ========================================================================

void NamespaceRegistry::registerSymbol(const std::string &name, ASTNode *decl) {
    std::string current_ns = getCurrentNamespace();

    if (current_ns.empty()) {
        // グローバルスコープ (Interpreterの既存機能を使用)
        return;
    }

    // 現在の名前空間に登録
    auto it = namespaces_.find(current_ns);
    if (it != namespaces_.end()) {
        it->second->symbols[name] = decl;

        // デバッグ出力
        // std::cerr << "[NamespaceRegistry] Registered symbol: " << name
        //           << " in namespace: " << current_ns << std::endl;
    }
}

// ========================================================================
// 名前解決
// ========================================================================

std::vector<ResolvedSymbol>
NamespaceRegistry::resolveName(const std::string &name) const {
    std::vector<ResolvedSymbol> candidates;

    // ステップ1: 現在の名前空間で検索
    std::string current_ns = getCurrentNamespace();
    if (!current_ns.empty()) {
        auto it = namespaces_.find(current_ns);
        if (it != namespaces_.end()) {
            auto sym_it = it->second->symbols.find(name);
            if (sym_it != it->second->symbols.end()) {
                candidates.push_back({name, current_ns,
                                      current_ns + "::" + name,
                                      sym_it->second});
            }
        }
    }

    // ステップ2: using namespace で有効化された名前空間を検索
    for (const auto &using_ns : active_using_namespaces_) {
        auto it = namespaces_.find(using_ns);
        if (it != namespaces_.end()) {
            auto sym_it = it->second->symbols.find(name);
            if (sym_it != it->second->symbols.end()) {
                candidates.push_back(
                    {name, using_ns, using_ns + "::" + name, sym_it->second});
            }
        }
    }

    // ステップ3: グローバルスコープ
    // (Interpreterの既存の関数・変数検索機能を使用)
    // このメソッドは名前空間内のシンボルのみを返す

    return candidates;
}

ResolvedSymbol *
NamespaceRegistry::resolveQualifiedName(const std::string &qualified_name) {
    // "math::add" を ["math", "add"] に分割
    auto components = splitPath(qualified_name);
    if (components.size() < 2) {
        return nullptr; // 修飾名ではない
    }

    // 最後の要素がシンボル名、それ以外が名前空間パス
    std::string symbol_name = components.back();
    components.pop_back();
    std::string ns_path = joinPath(components);

    // 名前空間を検索
    auto it = namespaces_.find(ns_path);
    if (it == namespaces_.end()) {
        return nullptr; // 名前空間が存在しない
    }

    // シンボルを検索
    auto sym_it = it->second->symbols.find(symbol_name);
    if (sym_it == it->second->symbols.end()) {
        return nullptr; // シンボルが存在しない
    }

    // 静的に確保して返す (注意: メモリリークの可能性)
    static ResolvedSymbol result;
    result = {symbol_name, ns_path, qualified_name, sym_it->second};
    return &result;
}

// ========================================================================
// クエリメソッド
// ========================================================================

bool NamespaceRegistry::isNamespaceExported(const std::string &ns_path) const {
    auto it = namespaces_.find(ns_path);
    if (it != namespaces_.end()) {
        return it->second->is_exported;
    }
    return false;
}

bool NamespaceRegistry::namespaceExists(const std::string &ns_path) const {
    return namespaces_.find(ns_path) != namespaces_.end();
}

// ========================================================================
// デバッグ
// ========================================================================

void NamespaceRegistry::dump() const {
    std::cerr << "\n=== NamespaceRegistry Dump ===" << std::endl;
    std::cerr << "Current namespace: "
              << (current_namespace_stack_.empty() ? "(global)"
                                                   : getCurrentNamespace())
              << std::endl;

    std::cerr << "Active using namespaces: ";
    if (active_using_namespaces_.empty()) {
        std::cerr << "(none)";
    } else {
        for (size_t i = 0; i < active_using_namespaces_.size(); ++i) {
            if (i > 0)
                std::cerr << ", ";
            std::cerr << active_using_namespaces_[i];
        }
    }
    std::cerr << std::endl;

    std::cerr << "\nRegistered namespaces:" << std::endl;
    for (const auto &pair : namespaces_) {
        const NamespaceInfo *ns_info = pair.second;
        std::cerr << "  - " << ns_info->full_path
                  << (ns_info->is_exported ? " (exported)" : "") << std::endl;
        std::cerr << "    Symbols: ";
        if (ns_info->symbols.empty()) {
            std::cerr << "(none)";
        } else {
            bool first = true;
            for (const auto &sym_pair : ns_info->symbols) {
                if (!first)
                    std::cerr << ", ";
                std::cerr << sym_pair.first;
                first = false;
            }
        }
        std::cerr << std::endl;
    }
    std::cerr << "==============================\n" << std::endl;
}
