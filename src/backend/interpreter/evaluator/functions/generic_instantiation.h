#ifndef GENERIC_INSTANTIATION_H
#define GENERIC_INSTANTIATION_H

#include "../../../../common/ast.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace GenericInstantiation {

// v0.11.0: ジェネリック関数のインスタンス化
// 型パラメータを実際の型に置換したASTノードを生成
//
// 機能:
// - ASTノードの深いコピー（すべてのフィールドを保持）
// - 型パラメータの置換（type_name, type_info, pointer_base_type等）
// - インスタンス化のキャッシュ（パフォーマンス最適化）
//
// 依存関係:
// - common/ast.h: ASTNode構造体、TypeInfo列挙型
// - common/type_alias.h: parse_type_from_string() 型名→TypeInfo変換

// ASTノードを深くコピー
std::unique_ptr<ASTNode> clone_ast_node(const ASTNode *node);

// 型パラメータを実際の型に置換
// type_map: {"T" -> "int", "E" -> "string"} のようなマッピング
//
// この関数は以下のフィールドを更新します:
// - type_name: 型パラメータ名を実際の型名に置換
// - type_info: parse_type_from_string()を使用してTypeInfo値を更新
// - return_type_name: 戻り値の型パラメータを置換
// - pointer_base_type_name: ポインタの基底型パラメータを置換
// - pointer_base_type: ポインタ基底型のTypeInfo値を更新
//
// 注意: この関数はASTツリーを再帰的に走査し、すべてのノードを更新します
void substitute_type_parameters(
    ASTNode *node, const std::map<std::string, std::string> &type_map);

// ジェネリック関数をインスタンス化
// func: ジェネリック関数のASTノード
// type_arguments: 型引数リスト ["int", "string"]
// 戻り値: インスタンス化された関数のASTノード
//
// この関数は以下の処理を行います:
// 1. 関数のASTツリーを深くコピー
// 2. 型パラメータから型引数へのマッピングを作成
// 3. substitute_type_parameters()で型名とtype_infoを置換
// 4. 結果を返す（キャッシュへの保存は呼び出し側が行う）
//
// 例: identity<T>(T x) に type_arguments=["int"] を適用
//     -> identity<int>(int x) にtype_nameとtype_infoが更新される
std::unique_ptr<ASTNode>
instantiate_generic_function(const ASTNode *func,
                             const std::vector<std::string> &type_arguments);

// v0.11.0: インスタンス化キャッシュ（パフォーマンス最適化）
// キャッシュキーを生成: "function_name<type1,type2,...>"
std::string generate_cache_key(const std::string &function_name,
                               const std::vector<std::string> &type_arguments);

// キャッシュからインスタンスを取得（存在しない場合はnullptr）
const ASTNode *get_cached_instance(const std::string &cache_key);

// インスタンスをキャッシュに保存
void cache_instance(const std::string &cache_key,
                    std::unique_ptr<ASTNode> instance);

// キャッシュをクリア（テスト用）
void clear_cache();

// v0.12.0: implブロックのインスタンス化
// impl VectorOps<T> for Vector<T>のような汎用implブロックを
// impl VectorOps<int> for Vector<int>のように具体的な型でインスタンス化
// 戻り値: {インスタンス化されたinterface名, struct名, ASTノード}
std::tuple<std::string, std::string, std::unique_ptr<ASTNode>>
instantiate_generic_impl(const ASTNode *impl_node,
                         const std::vector<std::string> &type_arguments,
                         const std::string &interface_name,
                         const std::string &struct_name);

} // namespace GenericInstantiation

#endif // GENERIC_INSTANTIATION_H
