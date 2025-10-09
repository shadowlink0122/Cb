#ifndef CB_INTERPRETER_GLOBAL_INITIALIZATION_MANAGER_H
#define CB_INTERPRETER_GLOBAL_INITIALIZATION_MANAGER_H

// 前方宣言
struct ASTNode;
class Interpreter;
class RecursiveParser;

/**
 * @brief グローバル変数とEnum定義の初期化を管理するマネージャークラス
 *
 * このクラスは以下の責務を持ちます：
 * - グローバル変数の初期化（2パス処理）
 * - Enum定義のパーサーからの同期
 *
 * Interpreterクラスの初期化関連メソッドをこのクラスに移動し、
 * Interpreterクラスのサイズを削減します。
 */
class GlobalInitializationManager {
  public:
    explicit GlobalInitializationManager(Interpreter *interpreter);

    /**
     * @brief グローバル変数を初期化する
     *
     * 2パス処理で初期化：
     * 1. const変数を先に初期化（配列サイズで使用される可能性があるため）
     * 2. その他の変数を初期化
     *
     * @param node 初期化するASTノード
     */
    void initialize_global_variables(const ASTNode *node);

    /**
     * @brief パーサーからEnum定義を同期する
     *
     * @param parser パーサーインスタンス
     */
    void sync_enum_definitions_from_parser(RecursiveParser *parser);

  private:
    Interpreter *interpreter_;
};

#endif // CB_INTERPRETER_GLOBAL_INITIALIZATION_MANAGER_H
