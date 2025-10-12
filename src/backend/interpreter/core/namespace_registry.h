#ifndef NAMESPACE_REGISTRY_H
#define NAMESPACE_REGISTRY_H

#include "../../../common/ast.h"
#include <map>
#include <string>
#include <vector>

/**
 * @brief 名前空間の情報を保持する構造体
 */
struct NamespaceInfo {
    std::string full_path;                    // 完全修飾名 "std::io"
    std::vector<std::string> path_components; // パス要素 ["std", "io"]
    const ASTNode *declaration_node; // 名前空間宣言ノード全体
    bool is_exported;                // export namespace かどうか
    std::map<std::string, ASTNode *>
        symbols; // シンボルテーブル (関数名 -> 宣言ノード)
};

/**
 * @brief 名前解決の結果を表す構造体
 */
struct ResolvedSymbol {
    std::string name;           // シンボル名 "add"
    std::string namespace_path; // 名前空間パス "math" または "" (global)
    std::string fully_qualified; // 完全修飾名 "math::add"
    ASTNode *declaration;        // 実際の宣言ノード
};

/**
 * @brief 名前空間の登録と名前解決を管理するクラス
 *
 * C++スタイルの名前空間機能を提供:
 * - namespace宣言の登録
 * - using namespaceの管理
 * - 名前衝突検出
 * - スコープベースの名前解決
 */
class NamespaceRegistry {
  private:
    // 名前空間階層マップ: "std::io" -> NamespaceInfo (ポインタで保持)
    std::map<std::string, NamespaceInfo *> namespaces_;

    // 現在の名前空間スコープスタック (ネスト対応)
    std::vector<std::string> current_namespace_stack_;

    // 現在のスコープで有効な using namespace リスト
    std::vector<std::string> active_using_namespaces_;

    /**
     * @brief パス要素を "::" で結合して完全修飾名を生成
     */
    std::string joinPath(const std::vector<std::string> &components) const;

    /**
     * @brief パスを "::" で分割してコンポーネントに分解
     */
    std::vector<std::string> splitPath(const std::string &path) const;

  public:
    NamespaceRegistry();  // 明示的なコンストラクタ
    ~NamespaceRegistry(); // デストラクタ (TODO: メモリリーク修正)

    /**
     * @brief 名前空間を登録する
     * @param ns_path 名前空間パス ("std::io" 形式)
     * @param decl_node 名前空間宣言ノード全体
     * @param is_exported export namespace かどうか
     */
    void registerNamespace(const std::string &ns_path, const ASTNode *decl_node,
                           bool is_exported);

    /**
     * @brief 名前空間スコープに入る
     * @param ns_name 名前空間名 (単一要素 "std" または "io")
     */
    void enterNamespace(const std::string &ns_name);

    /**
     * @brief 名前空間スコープから出る
     */
    void exitNamespace();

    /**
     * @brief 現在の名前空間の完全修飾パスを取得
     * @return 完全修飾パス (例: "std::io") または "" (グローバル)
     */
    std::string getCurrentNamespace() const;

    /**
     * @brief using namespace を追加
     * @param ns_path 名前空間パス ("std::io" 形式)
     */
    void addUsingNamespace(const std::string &ns_path);

    /**
     * @brief 現在のスコープの using namespace をクリア
     */
    void clearUsingNamespaces();

    /**
     * @brief シンボルを現在の名前空間に登録
     * @param name シンボル名
     * @param decl 宣言ノード
     */
    void registerSymbol(const std::string &name, ASTNode *decl);

    /**
     * @brief 名前を解決する (名前衝突検出付き)
     *
     * 優先順位:
     * 1. 現在の名前空間
     * 2. using namespace で有効化された名前空間
     * 3. グローバルスコープ
     *
     * @param name 解決する名前
     * @return 解決されたシンボルのリスト (複数の場合は衝突)
     */
    std::vector<ResolvedSymbol> resolveName(const std::string &name) const;

    /**
     * @brief 修飾名を解決する ("math::add" 形式)
     * @param qualified_name 修飾名
     * @return 解決されたシンボル (見つからない場合は nullptr)
     */
    ResolvedSymbol *resolveQualifiedName(const std::string &qualified_name);

    /**
     * @brief 名前空間がエクスポートされているか確認
     * @param ns_path 名前空間パス
     * @return エクスポートされている場合 true
     */
    bool isNamespaceExported(const std::string &ns_path) const;

    /**
     * @brief 名前空間が存在するか確認
     * @param ns_path 名前空間パス
     * @return 存在する場合 true
     */
    bool namespaceExists(const std::string &ns_path) const;

    /**
     * @brief デバッグ用: 登録されている名前空間をダンプ
     */
    void dump() const;
};

#endif // NAMESPACE_REGISTRY_H
