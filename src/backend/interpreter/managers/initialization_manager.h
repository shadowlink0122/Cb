#pragma once
#include "../../../../common/ast.h"
#include "../../core/interpreter.h"
#include <string>

class RecursiveParser;

// 初期化とグローバル宣言を管理するクラス
// interpreter.cppから抽出した初期化関連の機能を提供
class InitializationManager {
  public:
    explicit InitializationManager(Interpreter *interpreter);

    // グローバル宣言の登録
    void register_global_declarations(const ASTNode *node);

    // グローバル変数の初期化
    void initialize_global_variables(const ASTNode *node);

    // パーサーからの定義同期
    void sync_enum_definitions_from_parser(RecursiveParser *parser);
    void sync_struct_definitions_from_parser(RecursiveParser *parser);

  private:
    Interpreter *interpreter_;
};
