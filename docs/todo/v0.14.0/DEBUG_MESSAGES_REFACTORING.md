# Debug Messages Refactoring - Modular Architecture

## 実施日
2024-11-16

## 概要
デバッグメッセージを機能ごとにモジュール分割し、保守性と拡張性を向上させました。

## 問題点

### Before（リファクタリング前）
- すべてのデバッグメッセージが`debug_messages.cpp`（1481行）に集中
- パーサ、AST、インタープリタ、HIRのメッセージが混在
- 新しいIRレベル（MIR、LIR）の追加が困難
- 可読性と保守性が低い

## 解決策

### モジュール分割
デバッグメッセージを以下の4つのモジュールに分割：

1. **Parser** - パーサ関連メッセージ
2. **AST** - AST処理関連メッセージ  
3. **Interpreter** - インタープリタ実行メッセージ
4. **HIR** - High-level IR関連メッセージ

将来的には以下を追加可能：
- **MIR** - Mid-level IR
- **LIR** - Low-level IR
- **Optimizer** - 最適化パス
- **Codegen** - コード生成

## 新しいディレクトリ構造

```
src/common/
├── debug.h                              # DebugMsgId列挙型定義
├── debug_impl.cpp                       # デバッグ実装
├── debug_messages.h                     # メッセージテンプレート定義
├── debug_messages.cpp                   # メインメッセージ初期化
└── debug/                               # デバッグメッセージモジュール
    ├── debug_parser_messages.h          # パーサモジュール（ヘッダー）
    ├── debug_parser_messages.cpp        # パーサモジュール（実装）
    ├── debug_ast_messages.h             # ASTモジュール（ヘッダー）
    ├── debug_ast_messages.cpp           # ASTモジュール（実装）
    ├── debug_interpreter_messages.h     # インタープリタモジュール（ヘッダー）
    ├── debug_interpreter_messages.cpp   # インタープリタモジュール（実装）
    ├── debug_hir_messages.h             # HIRモジュール（ヘッダー）
    └── debug_hir_messages.cpp           # HIRモジュール（実装）
```

## 実装詳細

### 1. モジュールヘッダー例

`debug_parser_messages.h`:
```cpp
#ifndef DEBUG_PARSER_MESSAGES_H
#define DEBUG_PARSER_MESSAGES_H

#include "../debug.h"
#include "../debug_messages.h"

namespace DebugMessages {
namespace Parser {

// パーサ関連のデバッグメッセージを初期化
void init_parser_messages(std::vector<DebugMessageTemplate> &messages);

} // namespace Parser
} // namespace DebugMessages

#endif // DEBUG_PARSER_MESSAGES_H
```

### 2. モジュール実装例

`debug_parser_messages.cpp`:
```cpp
#include "debug_parser_messages.h"

namespace DebugMessages {
namespace Parser {

void init_parser_messages(std::vector<DebugMessageTemplate> &messages) {
    // パーサ関連メッセージ
    messages[static_cast<int>(DebugMsgId::PARSER_ERROR)] = {
        "[PARSE_ERROR] Parser error", 
        "[PARSE_ERROR] パーサーエラー"};
    
    messages[static_cast<int>(DebugMsgId::PARSING_START)] = {
        "[PARSE] Parsing started", 
        "[PARSE] パース開始"};
    
    // ... その他のパーサメッセージ
}

} // namespace Parser
} // namespace DebugMessages
```

### 3. メインファイルでの統合

`debug_messages.cpp`:
```cpp
#include "debug_messages.h"
#include "debug/debug_parser_messages.h"
#include "debug/debug_ast_messages.h"
#include "debug/debug_interpreter_messages.h"
#include "debug/debug_hir_messages.h"
#include <vector>

static std::vector<DebugMessageTemplate> init_debug_messages() {
    std::vector<DebugMessageTemplate> messages(
        static_cast<int>(DebugMsgId::MAX_DEBUG_MSG_ID));

    // 各モジュールのメッセージを初期化
    DebugMessages::Parser::init_parser_messages(messages);
    DebugMessages::AST::init_ast_messages(messages);
    DebugMessages::Interpreter::init_interpreter_messages(messages);
    DebugMessages::HIR::init_hir_messages(messages);

    return messages;
}
```

### 4. Makefileの更新

```makefile
# デバッグメッセージモジュール
DEBUG_DIR=$(COMMON_DIR)/debug
DEBUG_OBJS = \
$(DEBUG_DIR)/debug_parser_messages.o \
$(DEBUG_DIR)/debug_ast_messages.o \
$(DEBUG_DIR)/debug_interpreter_messages.o \
$(DEBUG_DIR)/debug_hir_messages.o

COMMON_OBJS=$(COMMON_DIR)/type_utils.o ... $(DEBUG_OBJS) ...

# デバッグメッセージモジュールのコンパイル
$(DEBUG_DIR)/%.o: $(DEBUG_DIR)/%.cpp
$(CC) $(CFLAGS) -c -o $@ $<
```

## メリット

### 1. 保守性の向上 ✅
- 各モジュールが独立したファイル
- 特定の機能のメッセージを簡単に編集
- ファイルサイズが管理しやすい（各モジュール100-200行程度）

### 2. 拡張性の向上 ✅
- 新しいIRレベル（MIR、LIR）を簡単に追加
- 各モジュールが独立しているため、並行開発が可能
- 新機能のデバッグメッセージを適切な場所に配置

### 3. 可読性の向上 ✅
- 機能ごとにファイルが分かれている
- 関連するメッセージをまとめて確認可能
- コードレビューが容易

### 4. コンパイル時間の最適化 ✅
- 特定モジュールの変更時、そのモジュールのみ再コンパイル
- メインのdebug_messages.cppを変更する頻度が減少

### 5. 名前空間による整理 ✅
- `DebugMessages::Parser`、`DebugMessages::Interpreter`など
- 関数名の衝突を防止
- コードの意図が明確

## モジュール別メッセージ分類

### Parser Module
- ノード作成メッセージ
- 関数定義パース
- パラメータリスト処理
- パースエラー

### AST Module  
- AST検証
- AST変換
- AST最適化（将来）

### Interpreter Module
- 変数宣言・代入
- 配列処理
- 式評価
- エラーメッセージ
- メイン関数実行

### HIR Module
- HIR生成（将来）
- HIR最適化（将来）
- HIR変換（将来）

## 移行戦略

### Phase 1: 基本構造の構築 ✅（完了）
- モジュールファイルの作成
- Makefileの更新
- 基本メッセージの移動

### Phase 2: メッセージの段階的移行（今後）
- 既存メッセージを適切なモジュールに移動
- 重複メッセージの削除
- メッセージIDの整理

### Phase 3: 新機能の追加（今後）
- MIR/LIRモジュールの追加
- 最適化パスのメッセージ
- コード生成のメッセージ

## 使用例

### 新しいモジュールの追加方法

#### 1. ヘッダーファイル作成
`src/common/debug/debug_mir_messages.h`:
```cpp
#ifndef DEBUG_MIR_MESSAGES_H
#define DEBUG_MIR_MESSAGES_H

#include "../debug.h"
#include "../debug_messages.h"

namespace DebugMessages {
namespace MIR {

void init_mir_messages(std::vector<DebugMessageTemplate> &messages);

} // namespace MIR
} // namespace DebugMessages

#endif
```

#### 2. 実装ファイル作成
`src/common/debug/debug_mir_messages.cpp`:
```cpp
#include "debug_mir_messages.h"

namespace DebugMessages {
namespace MIR {

void init_mir_messages(std::vector<DebugMessageTemplate> &messages) {
    messages[static_cast<int>(DebugMsgId::MIR_GENERATION_START)] = {
        "[MIR] MIR generation started",
        "[MIR] MIR生成開始"};
    
    messages[static_cast<int>(DebugMsgId::MIR_OPTIMIZATION_PASS)] = {
        "[MIR] Running optimization pass",
        "[MIR] 最適化パス実行中"};
}

} // namespace MIR
} // namespace DebugMessages
```

#### 3. Makefileに追加
```makefile
DEBUG_OBJS = \
$(DEBUG_DIR)/debug_parser_messages.o \
$(DEBUG_DIR)/debug_ast_messages.o \
$(DEBUG_DIR)/debug_interpreter_messages.o \
$(DEBUG_DIR)/debug_hir_messages.o \
$(DEBUG_DIR)/debug_mir_messages.o
```

#### 4. メインファイルで初期化
```cpp
#include "debug/debug_mir_messages.h"

static std::vector<DebugMessageTemplate> init_debug_messages() {
    // ...
    DebugMessages::MIR::init_mir_messages(messages);
    return messages;
}
```

## テスト結果

```bash
✅ コンパイル成功
✅ リンク成功
✅ 4373/4373 integration tests passed
✅ 既存の機能に影響なし
✅ デバッグメッセージが正常に表示
```

## 将来の拡張計画

### 短期（v0.14.x）
- [ ] 既存メッセージの完全な分類
- [ ] 重複メッセージの削除
- [ ] メッセージIDの整理

### 中期（v0.15.x）
- [ ] MIRモジュールの追加
- [ ] 最適化パスメッセージ
- [ ] コード生成メッセージ

### 長期（v0.16.x+）
- [ ] LIRモジュールの追加
- [ ] バックエンド別メッセージ
- [ ] プロファイリングメッセージ

## まとめ

このリファクタリングにより：

1. ✅ **モジュール化**: 機能ごとにファイル分割
2. ✅ **保守性向上**: 各モジュールが独立して管理可能
3. ✅ **拡張性向上**: 新しいIRレベルを簡単に追加
4. ✅ **可読性向上**: コードが整理され理解しやすい
5. ✅ **並行開発**: 複数人で同時開発が可能

Cbコンパイラのデバッグシステムがよりスケーラブルで保守しやすくなりました！
