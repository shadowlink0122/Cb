#include "generic_instantiation.h"
#include "../../../../common/ast.h"
#include "../../../../common/debug.h"
#include "../../../../common/type_alias.h"
#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace GenericInstantiation {

// v0.11.0: インスタンス化キャッシュ（グローバル）
// キー: "function_name<type1,type2,...>"
// 値: インスタンス化されたASTノード
static std::map<std::string, std::unique_ptr<ASTNode>> instantiation_cache;

// 正規化されたジェネリック型名を置換（例: "Box_T" + {"T"->"int"} -> "Box_int"）
static std::string substitute_normalized_generic_type(
    const std::string &type_name,
    const std::map<std::string, std::string> &type_map) {

    // '_'で分割して型パラメータを探す
    // 例: "Box_T" -> ["Box", "T"]
    //     "Pair_T1_T2" -> ["Pair", "T1", "T2"]
    std::vector<std::string> parts;
    std::string current_part;

    for (char c : type_name) {
        if (c == '_') {
            if (!current_part.empty()) {
                parts.push_back(current_part);
                current_part.clear();
            }
        } else {
            current_part += c;
        }
    }
    if (!current_part.empty()) {
        parts.push_back(current_part);
    }

    if (parts.empty()) {
        return type_name;
    }

    // 最初の部分は構造体名
    std::string result = parts[0];

    // 残りの部分は型パラメータ
    for (size_t i = 1; i < parts.size(); ++i) {
        result += "_";
        // 型パラメータを置換
        auto it = type_map.find(parts[i]);
        if (it != type_map.end()) {
            result += it->second;
        } else {
            result += parts[i];
        }
    }

    return result;
}

// ジェネリック型名を正規化（例: "Box<int>" -> "Box_int"）
static std::string normalize_generic_type_name(const std::string &type_name) {
    std::string result = type_name;

    // '<'を'_'に、'>'を削除、','と空白を'_'に置換
    for (char &c : result) {
        if (c == '<' || c == ',' || c == ' ') {
            c = '_';
        } else if (c == '>') {
            c = '\0'; // 削除マーク
        }
    }

    // '\0'（削除マーク）を削除
    result.erase(std::remove(result.begin(), result.end(), '\0'), result.end());

    // 連続するアンダースコアを1つに
    std::string normalized;
    bool last_was_underscore = false;
    for (char c : result) {
        if (c == '_') {
            if (!last_was_underscore) {
                normalized += c;
                last_was_underscore = true;
            }
        } else {
            normalized += c;
            last_was_underscore = false;
        }
    }

    // 末尾のアンダースコアを削除
    while (!normalized.empty() && normalized.back() == '_') {
        normalized.pop_back();
    }

    return normalized;
}

// ジェネリック型名の型パラメータを置換するヘルパー関数
// 例: "Box<T>" + {"T" -> "int"} => "Box<int>"
static std::string substitute_generic_type_name(
    const std::string &type_name,
    const std::map<std::string, std::string> &type_map) {

    // '<' がない場合は通常の型名
    size_t lt_pos = type_name.find('<');
    if (lt_pos == std::string::npos) {
        // 単純な型パラメータの置換
        auto it = type_map.find(type_name);
        if (it != type_map.end()) {
            return it->second;
        }
        return type_name;
    }

    // ジェネリック型名: "Box<T>" や "Pair<T1, T2>"
    std::string base_name = type_name.substr(0, lt_pos);
    size_t gt_pos = type_name.rfind('>');
    if (gt_pos == std::string::npos) {
        return type_name; // 不正な形式
    }

    std::string type_params_str =
        type_name.substr(lt_pos + 1, gt_pos - lt_pos - 1);

    // 型パラメータをカンマで分割
    std::vector<std::string> type_params;
    std::string current_param;
    int depth = 0;

    for (char c : type_params_str) {
        if (c == '<') {
            depth++;
            current_param += c;
        } else if (c == '>') {
            depth--;
            current_param += c;
        } else if (c == ',' && depth == 0) {
            // トリム
            size_t start = current_param.find_first_not_of(" \t");
            size_t end = current_param.find_last_not_of(" \t");
            if (start != std::string::npos) {
                type_params.push_back(
                    current_param.substr(start, end - start + 1));
            }
            current_param.clear();
        } else {
            current_param += c;
        }
    }

    // 最後のパラメータを追加
    if (!current_param.empty()) {
        size_t start = current_param.find_first_not_of(" \t");
        size_t end = current_param.find_last_not_of(" \t");
        if (start != std::string::npos) {
            type_params.push_back(current_param.substr(start, end - start + 1));
        }
    }

    // 各型パラメータを置換
    std::string result = base_name + "<";
    for (size_t i = 0; i < type_params.size(); ++i) {
        if (i > 0)
            result += ", ";

        // 再帰的に置換（ネストしたジェネリクスのため）
        result += substitute_generic_type_name(type_params[i], type_map);
    }
    result += ">";

    return result;
}

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

    // sizeof関連のフィールドをコピー
    cloned->sizeof_type_name = node->sizeof_type_name;
    if (node->sizeof_expr) {
        cloned->sizeof_expr = clone_ast_node(node->sizeof_expr.get());
    }

    // キャスト関連のフィールドをコピー (v0.11.0 Fix)
    cloned->cast_target_type = node->cast_target_type;
    cloned->cast_type_info = node->cast_type_info;
    if (node->cast_expr) {
        cloned->cast_expr = clone_ast_node(node->cast_expr.get());
    }

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
        std::string substituted;

        // 正規化済みのジェネリック型名 (例: Box_T) または通常のジェネリック型名
        // (例: Box<T>)
        if (node->type_name.find('<') != std::string::npos) {
            // Box<T> 形式
            substituted =
                substitute_generic_type_name(node->type_name, type_map);
            if (substituted != node->type_name) {
                node->type_name = substituted;
                // ジェネリック型名を正規化 (例: Box<int> -> Box_int)
                if (substituted.find('<') != std::string::npos) {
                    node->type_name = normalize_generic_type_name(substituted);
                }
            }
        } else if (node->type_name.find('_') != std::string::npos) {
            // Box_T 形式（既に正規化済み）
            substituted =
                substitute_normalized_generic_type(node->type_name, type_map);
            if (substituted != node->type_name) {
                node->type_name = substituted;
            }
        } else {
            // 単純な型パラメータ (例: T)
            substituted =
                substitute_generic_type_name(node->type_name, type_map);
            if (substituted != node->type_name) {
                node->type_name = substituted;
            }
        }

        // 基本型の場合のみtype_infoを更新
        if (!node->type_name.empty() &&
            node->type_name.find('<') == std::string::npos &&
            node->type_name.find('_') == std::string::npos) {
            node->type_info = parse_type_from_string(node->type_name);
        }
    }

    // 戻り値型の置換
    if (!node->return_type_name.empty()) {
        std::string substituted;
        if (node->return_type_name.find('<') != std::string::npos) {
            substituted =
                substitute_generic_type_name(node->return_type_name, type_map);
            if (substituted != node->return_type_name) {
                node->return_type_name = substituted;
                if (substituted.find('<') != std::string::npos) {
                    node->return_type_name =
                        normalize_generic_type_name(substituted);
                }
            }
        } else if (node->return_type_name.find('_') != std::string::npos) {
            substituted = substitute_normalized_generic_type(
                node->return_type_name, type_map);
            if (substituted != node->return_type_name) {
                node->return_type_name = substituted;
            }
        } else {
            substituted =
                substitute_generic_type_name(node->return_type_name, type_map);
            if (substituted != node->return_type_name) {
                node->return_type_name = substituted;
            }
        }
    }

    // ポインタベース型の置換
    if (!node->pointer_base_type_name.empty()) {
        std::string substituted;
        if (node->pointer_base_type_name.find('<') != std::string::npos) {
            substituted = substitute_generic_type_name(
                node->pointer_base_type_name, type_map);
            if (substituted != node->pointer_base_type_name) {
                node->pointer_base_type_name = substituted;
                if (substituted.find('<') != std::string::npos) {
                    node->pointer_base_type_name =
                        normalize_generic_type_name(substituted);
                }
            }
        } else if (node->pointer_base_type_name.find('_') !=
                   std::string::npos) {
            substituted = substitute_normalized_generic_type(
                node->pointer_base_type_name, type_map);
            if (substituted != node->pointer_base_type_name) {
                node->pointer_base_type_name = substituted;
            }
        } else {
            substituted = substitute_generic_type_name(
                node->pointer_base_type_name, type_map);
            if (substituted != node->pointer_base_type_name) {
                node->pointer_base_type_name = substituted;
            }
        }

        // 基本型の場合のみtype_infoを更新
        if (!node->pointer_base_type_name.empty() &&
            node->pointer_base_type_name.find('<') == std::string::npos &&
            node->pointer_base_type_name.find('_') == std::string::npos) {
            node->pointer_base_type =
                parse_type_from_string(node->pointer_base_type_name);
        }
    }

    // sizeof型名の置換
    if (!node->sizeof_type_name.empty()) {
        std::string substituted =
            substitute_generic_type_name(node->sizeof_type_name, type_map);
        if (substituted != node->sizeof_type_name) {
            node->sizeof_type_name = substituted;
        }
    }

    // sizeof式の処理
    if (node->sizeof_expr) {
        substitute_type_parameters(node->sizeof_expr.get(), type_map);
    }

    // キャスト式の処理 (v0.11.0 Fix: QueueNode<T>* キャストのサポート)
    if (node->cast_expr) {
        substitute_type_parameters(node->cast_expr.get(), type_map);
    }

    // キャストターゲット型の置換 (例: QueueNode<T>* -> QueueNode<int>*)
    if (!node->cast_target_type.empty()) {
        std::string substituted =
            substitute_generic_type_name(node->cast_target_type, type_map);
        if (substituted != node->cast_target_type) {
            node->cast_target_type = substituted;
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

// implブロックをインスタンス化（新機能: impl VectorOps<T> for Vector<T>対応）
// impl_node: implブロックのASTノード
// type_arguments: 型引数リスト ["int"] など
// interface_name: インターフェース名 "VectorOps<T>" など
// struct_name: 構造体名 "Vector<T>" など
// 戻り値: {インスタンス化されたinterfacename, 構造体名, ASTノード}
std::tuple<std::string, std::string, std::unique_ptr<ASTNode>>
instantiate_generic_impl(const ASTNode *impl_node,
                         const std::vector<std::string> &type_arguments,
                         const std::string &interface_name,
                         const std::string &struct_name) {
    if (!impl_node) {
        throw std::runtime_error("instantiate_generic_impl: null impl_node");
    }

    // 型パラメータの抽出（interface_nameまたはstruct_nameから）
    // "VectorOps<T>" -> ["T"] または "Box<T>" -> ["T"]
    std::vector<std::string> type_parameters;

    // まずinterface_nameから型パラメータを抽出を試みる
    std::string source_name =
        interface_name.empty() ? struct_name : interface_name;

    size_t lt_pos = source_name.find('<');
    if (lt_pos != std::string::npos) {
        size_t gt_pos = source_name.rfind('>');
        if (gt_pos != std::string::npos) {
            std::string params_str =
                source_name.substr(lt_pos + 1, gt_pos - lt_pos - 1);
            // カンマで分割（簡易版、ネストしたジェネリクスは未対応）
            std::stringstream ss(params_str);
            std::string param;
            while (std::getline(ss, param, ',')) {
                // トリム
                size_t start = param.find_first_not_of(" \t");
                size_t end = param.find_last_not_of(" \t");
                if (start != std::string::npos) {
                    type_parameters.push_back(
                        param.substr(start, end - start + 1));
                }
            }
        }
    }

    // 型パラメータと型引数の数が一致するかチェック
    if (type_parameters.size() != type_arguments.size()) {
        throw std::runtime_error(
            "Type argument count mismatch for impl: expected " +
            std::to_string(type_parameters.size()) + ", got " +
            std::to_string(type_arguments.size()));
    }

    // 型パラメータ → 型引数のマッピングを作成
    std::map<std::string, std::string> type_map;
    for (size_t i = 0; i < type_parameters.size(); ++i) {
        type_map[type_parameters[i]] = type_arguments[i];
    }

    // implノードをクローン
    debug_print("[INSTANTIATE_IMPL] Cloning impl node: arguments.size()=%zu\n",
                impl_node->arguments.size());
    for (size_t i = 0; i < impl_node->arguments.size(); ++i) {
        const auto &arg = impl_node->arguments[i];
        if (arg && arg->node_type == ASTNodeType::AST_FUNC_DECL) {
            debug_print("[INSTANTIATE_IMPL] Method[%zu]: name='%s', body=%p, "
                        "statements.size()=%zu\n",
                        i, arg->name.c_str(), (void *)arg->body.get(),
                        arg->body ? arg->body->statements.size() : 0);
        }
    }

    auto instantiated = clone_ast_node(impl_node);

    debug_print("[INSTANTIATE_IMPL] After clone: arguments.size()=%zu\n",
                instantiated->arguments.size());
    for (size_t i = 0; i < instantiated->arguments.size(); ++i) {
        const auto &arg = instantiated->arguments[i];
        if (arg && arg->node_type == ASTNodeType::AST_FUNC_DECL) {
            debug_print("[INSTANTIATE_IMPL] Cloned method[%zu]: name='%s', "
                        "body=%p, statements.size()=%zu\n",
                        i, arg->name.c_str(), (void *)arg->body.get(),
                        arg->body ? arg->body->statements.size() : 0);
        }
    }

    // 型パラメータを置換
    substitute_type_parameters(instantiated.get(), type_map);

    // interface_nameとstruct_nameも置換
    std::string instantiated_interface =
        substitute_generic_type_name(interface_name, type_map);
    std::string instantiated_struct =
        substitute_generic_type_name(struct_name, type_map);

    // v0.12.0: インスタンス化されたnodeのフィールドを更新
    instantiated->interface_name = instantiated_interface;
    instantiated->struct_name = instantiated_struct;
    instantiated->name = instantiated_interface + "_for_" + instantiated_struct;

    return {instantiated_interface, instantiated_struct,
            std::move(instantiated)};
}

} // namespace GenericInstantiation
