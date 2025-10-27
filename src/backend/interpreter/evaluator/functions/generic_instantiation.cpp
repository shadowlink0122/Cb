#include "generic_instantiation.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/type_alias.h"
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace GenericInstantiation {

// v0.11.0: インスタンス化キャッシュ（グローバル）
// キー: "function_name<type1,type2,...>"
// 値: インスタンス化されたASTノード
static std::map<std::string, std::unique_ptr<ASTNode>> instantiation_cache;

// キャッシュキーを生成
std::string generate_cache_key(const std::string &function_name,
                               const std::vector<std::string> &type_arguments) {
    std::string key = function_name + "<";
    for (size_t i = 0; i < type_arguments.size(); ++i) {
        if (i > 0) {
            key += ",";
        }
        key += type_arguments[i];
    }
    key += ">";
    return key;
}

// キャッシュからインスタンスを取得
const ASTNode *get_cached_instance(const std::string &cache_key) {
    auto it = instantiation_cache.find(cache_key);
    if (it != instantiation_cache.end()) {
        return it->second.get();
    }
    return nullptr;
}

// インスタンスをキャッシュに保存
void cache_instance(const std::string &cache_key,
                    std::unique_ptr<ASTNode> instance) {
    instantiation_cache[cache_key] = std::move(instance);
}

// キャッシュをクリア
void clear_cache() { instantiation_cache.clear(); }

// ASTノードを深くコピー
std::unique_ptr<ASTNode> clone_ast_node(const ASTNode *node) {
    if (!node) {
        return nullptr;
    }

    auto cloned = std::make_unique<ASTNode>(node->node_type);

    // 基本フィールドをコピー
    cloned->name = node->name;
    cloned->op = node->op;
    cloned->int_value = node->int_value;
    cloned->double_value = node->double_value;
    cloned->str_value = node->str_value;
    cloned->type_name = node->type_name;
    cloned->type_info = node->type_info;
    cloned->return_type_name = node->return_type_name;
    cloned->is_unsigned = node->is_unsigned;
    cloned->is_const = node->is_const;
    cloned->is_static = node->is_static;
    cloned->is_pointee_const_qualifier = node->is_pointee_const_qualifier;
    cloned->is_pointer = node->is_pointer;
    cloned->pointer_depth = node->pointer_depth;
    cloned->pointer_base_type_name = node->pointer_base_type_name;
    cloned->is_array = node->is_array;
    cloned->is_reference = node->is_reference;
    cloned->is_generic = node->is_generic;
    cloned->type_parameters = node->type_parameters;
    cloned->type_arguments = node->type_arguments;

    // 子ノードを再帰的にコピー
    if (node->left) {
        cloned->left = clone_ast_node(node->left.get());
    }
    if (node->right) {
        cloned->right = clone_ast_node(node->right.get());
    }
    if (node->condition) {
        cloned->condition = clone_ast_node(node->condition.get());
    }
    if (node->init_expr) {
        cloned->init_expr = clone_ast_node(node->init_expr.get());
    }
    if (node->lambda_body) {
        cloned->lambda_body = clone_ast_node(node->lambda_body.get());
    }
    if (node->body) {
        cloned->body = clone_ast_node(node->body.get());
    }

    // ベクタをコピー
    for (const auto &stmt : node->statements) {
        cloned->statements.push_back(clone_ast_node(stmt.get()));
    }
    for (const auto &param : node->parameters) {
        cloned->parameters.push_back(clone_ast_node(param.get()));
    }
    for (const auto &arg : node->arguments) {
        cloned->arguments.push_back(clone_ast_node(arg.get()));
    }
    for (const auto &case_node : node->cases) {
        cloned->cases.push_back(clone_ast_node(case_node.get()));
    }

    // return_types配列をコピー
    cloned->return_types = node->return_types;

    return cloned;
}

// 型パラメータを実際の型に置換
void substitute_type_parameters(
    ASTNode *node, const std::map<std::string, std::string> &type_map) {
    if (!node) {
        return;
    }

    // 型名の置換
    if (!node->type_name.empty()) {
        auto it = type_map.find(node->type_name);
        if (it != type_map.end()) {
            node->type_name = it->second;
            // 型情報も更新
            node->type_info = parse_type_from_string(it->second);
        }
    }

    // 戻り値型の置換
    if (!node->return_type_name.empty()) {
        auto it = type_map.find(node->return_type_name);
        if (it != type_map.end()) {
            node->return_type_name = it->second;
            // 戻り値型情報はtype_infoで管理（関数宣言の場合）
        }
    }

    // ポインタベース型の置換
    if (!node->pointer_base_type_name.empty()) {
        auto it = type_map.find(node->pointer_base_type_name);
        if (it != type_map.end()) {
            node->pointer_base_type_name = it->second;
            // ポインタベース型情報も更新
            node->pointer_base_type = parse_type_from_string(it->second);
        }
    }

    // 子ノードを再帰的に処理
    if (node->left) {
        substitute_type_parameters(node->left.get(), type_map);
    }
    if (node->right) {
        substitute_type_parameters(node->right.get(), type_map);
    }
    if (node->condition) {
        substitute_type_parameters(node->condition.get(), type_map);
    }
    if (node->init_expr) {
        substitute_type_parameters(node->init_expr.get(), type_map);
    }
    if (node->lambda_body) {
        substitute_type_parameters(node->lambda_body.get(), type_map);
    }
    if (node->body) {
        substitute_type_parameters(node->body.get(), type_map);
    }

    // ベクタ内のノードを処理
    for (const auto &stmt : node->statements) {
        substitute_type_parameters(stmt.get(), type_map);
    }
    for (const auto &param : node->parameters) {
        substitute_type_parameters(param.get(), type_map);
    }
    for (const auto &arg : node->arguments) {
        substitute_type_parameters(arg.get(), type_map);
    }
    for (const auto &case_node : node->cases) {
        substitute_type_parameters(case_node.get(), type_map);
    }
}

// ジェネリック関数をインスタンス化
std::unique_ptr<ASTNode>
instantiate_generic_function(const ASTNode *func,
                             const std::vector<std::string> &type_arguments) {
    if (!func || !func->is_generic) {
        throw std::runtime_error(
            "instantiate_generic_function: not a generic function");
    }

    // 型パラメータと型引数の数が一致するかチェック
    if (func->type_parameters.size() != type_arguments.size()) {
        throw std::runtime_error("Type argument count mismatch: expected " +
                                 std::to_string(func->type_parameters.size()) +
                                 ", got " +
                                 std::to_string(type_arguments.size()));
    }

    // 型パラメータ → 型引数のマッピングを作成
    // {"T" -> "int", "E" -> "string"} のような形式
    std::map<std::string, std::string> type_map;
    for (size_t i = 0; i < func->type_parameters.size(); ++i) {
        type_map[func->type_parameters[i]] = type_arguments[i];
    }

    // 関数ASTをクローン
    auto instantiated = clone_ast_node(func);

    // 型パラメータを置換
    substitute_type_parameters(instantiated.get(), type_map);

    // ジェネリックフラグをクリア（インスタンス化済み）
    instantiated->is_generic = false;
    instantiated->type_parameters.clear();

    return instantiated;
}

} // namespace GenericInstantiation
