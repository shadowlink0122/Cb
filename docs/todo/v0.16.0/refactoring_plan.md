# v0.16.0 リファクタリング計画

**バージョン**: v0.16.0
**作成日**: 2025-11-13
**ステータス**: 計画中

---

## 目次

1. [リファクタリングの概要](#1-リファクタリングの概要)
2. [変更が必要なファイル一覧](#2-変更が必要なファイル一覧)
3. [既存コードの影響範囲](#3-既存コードの影響範囲)
4. [段階的リファクタリング計画](#4-段階的リファクタリング計画)
5. [互換性の維持](#5-互換性の維持)
6. [マイグレーション手順](#6-マイグレーション手順)

---

## 1. リファクタリングの概要

### 1.1 主要な変更点

v0.16.0では、以下の大規模なリファクタリングを実施します：

1. **IR層の導入**: AST → HIR → MIR → LIR → コード生成
2. **複数バックエンド対応**: インタプリタ / ネイティブ / WASM / TypeScript
3. **共通型システムの強化**: 全バックエンドで共有
4. **コンパイラフラグの追加**: バックエンド選択、IR出力等

### 1.2 アーキテクチャの変更

```
【旧アーキテクチャ】
AST → Interpreter → 実行

【新アーキテクチャ】
                    ┌→ Interpreter → 実行
                    │
AST → HIR → MIR → LIR ┼→ Native Codegen → 実行ファイル
                    │
                    ├→ WASM Codegen → .wasm
                    │
                    └→ TypeScript Codegen → .ts
```

---

## 2. 変更が必要なファイル一覧

### 2.1 新規作成ファイル

#### IRレイヤー（新規）

**HIR (High-level IR)**
```
src/backend/ir/hir/
├── hir_node.h                    # HIRノード定義
├── hir_node.cpp                  # HIRノード実装
├── hir_generator.h               # ASTからHIRへの変換
├── hir_generator.cpp             # HIRジェネレーター実装
├── hir_visitor.h                 # HIRビジターインターフェース
├── hir_visitor.cpp               # HIRビジター実装
├── hir_dumper.h                  # HIRダンプ機能
├── hir_dumper.cpp                # HIRダンパー実装
├── hir_validator.h               # HIR検証
└── hir_validator.cpp             # HIRバリデーター実装
```

**MIR (Mid-level IR)**
```
src/backend/ir/mir/
├── mir_node.h                    # MIRノード定義
├── mir_node.cpp                  # MIRノード実装
├── mir_generator.h               # HIRからMIRへの変換
├── mir_generator.cpp             # MIRジェネレーター実装
├── cfg.h                         # 制御フローグラフ
├── cfg.cpp                       # CFG実装
├── ssa_builder.h                 # SSA形式構築
├── ssa_builder.cpp               # SSAビルダー実装
├── dominator_tree.h              # 支配木
├── dominator_tree.cpp            # 支配木実装
├── mir_dumper.h                  # MIRダンプ機能
├── mir_dumper.cpp                # MIRダンパー実装
├── graphviz_gen.h                # GraphViz可視化
├── graphviz_gen.cpp              # GraphViz生成実装
└── mir_validator.h               # MIR検証
```

**LIR (Low-level IR)**
```
src/backend/ir/lir/
├── lir_node.h                    # LIRノード定義
├── lir_node.cpp                  # LIRノード実装
├── lir_generator.h               # MIRからLIRへの変換
├── lir_generator.cpp             # LIRジェネレーター実装
├── lir_dumper.h                  # LIRダンプ機能
└── lir_dumper.cpp                # LIRダンパー実装
```

**データフロー解析**
```
src/backend/ir/analysis/
├── liveness.h                    # 生存変数解析
├── liveness.cpp                  # 生存変数解析実装
├── reaching_defs.h               # 到達定義解析
├── reaching_defs.cpp             # 到達定義解析実装
├── use_def_chain.h               # 使用-定義チェーン
└── use_def_chain.cpp             # 使用-定義チェーン実装
```

**共通IR機能**
```
src/backend/ir/common/
├── ir_error.h                    # IRエラー定義
├── ir_error.cpp                  # IRエラー実装
├── ir_context.h                  # IRコンテキスト
└── ir_context.cpp                # IRコンテキスト実装
```

#### コード生成レイヤー（新規）

**ネイティブコード生成**
```
src/backend/codegen/native/
├── native_codegen.h              # ネイティブコード生成
├── native_codegen.cpp            # ネイティブコード生成実装
├── x86_64_codegen.h              # x86-64コード生成
├── x86_64_codegen.cpp            # x86-64実装
├── arm64_codegen.h               # ARM64コード生成
├── arm64_codegen.cpp             # ARM64実装
├── register_allocator.h          # レジスタ割り当て
├── register_allocator.cpp        # レジスタ割り当て実装
├── instruction_selector.h        # 命令選択
└── instruction_selector.cpp      # 命令選択実装
```

**WASMコード生成**
```
src/backend/codegen/wasm/
├── wasm_codegen.h                # WASMコード生成
├── wasm_codegen.cpp              # WASMコード生成実装
├── wasm_module_builder.h         # WASMモジュールビルダー
├── wasm_module_builder.cpp       # WASMモジュールビルダー実装
├── wasm_function_builder.h       # WASM関数ビルダー
└── wasm_function_builder.cpp     # WASM関数ビルダー実装
```

**TypeScriptコード生成**
```
src/backend/codegen/typescript/
├── typescript_codegen.h          # TypeScriptコード生成
├── typescript_codegen.cpp        # TypeScriptコード生成実装
├── ts_emitter.h                  # TypeScript出力
└── ts_emitter.cpp                # TypeScript出力実装
```

**コード生成共通**
```
src/backend/codegen/common/
├── codegen_context.h             # コード生成コンテキスト
├── codegen_context.cpp           # コード生成コンテキスト実装
├── target_info.h                 # ターゲット情報
└── target_info.cpp               # ターゲット情報実装
```

### 2.2 変更が必要な既存ファイル

#### フロントエンド（パーサー）

**メインファイル**
```
src/frontend/main.cpp
- コマンドラインオプションの追加
  * --backend=<interpreter|native|wasm|typescript>
  * --dump-hir / --dump-mir / --dump-lir
  * --emit-cfg-dot
  * --stop-at=<hir|mir|lir|asm>
  * --target=<x86_64|arm64|wasm32|wasm64>
  * --output=<file>
- IR生成パイプラインの統合
- バックエンド選択ロジック
```

**ヘルプメッセージ**
```
src/frontend/help_messages.h
src/frontend/help_messages.cpp
- 新しいコマンドラインオプションのヘルプ追加
- IR関連オプションの説明
- バックエンド選択オプションの説明
```

#### 共通コード（型システム）

**型システムの強化**
```
src/common/type_system.h (新規または既存の拡張)
- 全バックエンドで共有する型定義
- 型情報の完全な表現
- 型変換ルール
- 型互換性チェック
```

**ASTノード**
```
src/common/ast.h
src/common/ast.cpp
- IR生成に必要な情報の追加
- ソースコード位置情報の強化
- 型情報の完全性向上
```

**型ヘルパー**
```
src/common/type_utils.h
src/common/type_utils.cpp
- IR生成用の型変換関数
- 型サイズ・アライメント計算
- 型互換性チェック関数
```

#### バックエンド（インタプリタ）

**インタプリタのリファクタリング**
```
src/backend/interpreter/core/interpreter.h
src/backend/interpreter/core/interpreter.cpp
- IRからの実行もサポート（オプション）
- 既存のAST実行パスは維持
```

**インタプリタのモード切り替え**
```
src/backend/interpreter/core/execution_mode.h (新規)
- AST直接実行モード
- HIR実行モード（オプション）
```

### 2.3 ビルドシステムの変更

**Makefile**
```
Makefile
- IR関連のオブジェクトファイル定義
- コード生成関連のオブジェクトファイル定義
- 新しいターゲット追加
  * ir-test
  * codegen-test
  * wasm-build
  * typescript-build
- ディレクトリ作成ルールの更新
```

---

## 3. 既存コードの影響範囲

### 3.1 破壊的変更なし（後方互換性維持）

以下の機能は**変更不要**で、既存の動作を維持します：

#### インタプリタ
- AST直接実行モード（デフォルト）
- すべての既存のインタプリタ機能
- 既存のテストケースは全てパス

#### パーサー
- 構文解析ロジック
- ASTノード構築
- エラー報告

#### 型システム（既存部分）
- 基本型（int, float, string等）
- 構造体、enum、interface
- ジェネリクス

### 3.2 拡張が必要な部分（非破壊的）

以下の部分は**拡張**しますが、既存の動作は維持します：

#### コマンドラインインターフェース
```cpp
// 既存の動作（変更なし）
./main example.cb                    # インタプリタで実行

// 新しいオプション（追加）
./main example.cb --backend=native   # ネイティブコンパイル
./main example.cb --backend=wasm     # WASMにコンパイル
./main example.cb --dump-hir         # HIRダンプ
```

#### 型情報
```cpp
// 既存のTypeInfo（維持）
enum TypeInfo { TYPE_INT, TYPE_FLOAT, ... };

// 拡張（新しい型情報を追加）
struct ExtendedTypeInfo {
    TypeInfo base_type;
    size_t size;            // バイトサイズ
    size_t alignment;       // アライメント
    bool is_signed;         // 符号付きか
    // ...
};
```

### 3.3 移行が必要な部分（オプション）

将来的に以下の部分を移行することを検討します（v0.16.0では不要）：

- インタプリタのHIR実行モード（パフォーマンス向上のため）
- 型システムの完全な統一（現在は共存可能）

---

## 4. 段階的リファクタリング計画

### Phase 1: IR層の基盤構築（Week 1-4）

#### Week 1: プロジェクト構造の準備
```bash
# ディレクトリ作成
mkdir -p src/backend/ir/{hir,mir,lir,analysis,common}
mkdir -p src/backend/codegen/{native,wasm,typescript,common}
mkdir -p tests/unit/ir
mkdir -p tests/integration/ir
```

**変更ファイル**:
- `Makefile`: 新しいディレクトリのビルドルール追加
- `src/frontend/main.cpp`: IRビルドオプションの枠組み追加（実装は後で）

**影響範囲**: なし（既存機能は影響を受けない）

#### Week 2-3: HIR実装
```cpp
// 新規ファイル（既存コードに影響なし）
src/backend/ir/hir/hir_node.h
src/backend/ir/hir/hir_generator.h
src/backend/ir/hir/hir_generator.cpp
```

**変更ファイル**:
- `src/common/ast.h`: HIR生成に必要な情報を追加（既存フィールドは維持）
  ```cpp
  // 追加（既存コードに影響なし）
  struct ASTNode {
      // 既存フィールド（変更なし）
      // ...

      // 新規フィールド（追加のみ）
      bool ir_generation_enabled = false;  // IR生成フラグ
  };
  ```

**影響範囲**: なし（追加のみ）

#### Week 4: HIRテストと統合
```cpp
// 新規テストファイル
tests/unit/ir/test_hir_generation.cpp
tests/integration/ir/test_hir_roundtrip.cpp
```

**変更ファイル**:
- `src/frontend/main.cpp`: `--dump-hir`オプションの実装
  ```cpp
  // main.cpp に追加
  if (dump_hir_flag) {
      HIRGenerator generator;
      HIRProgram hir = generator.generate(ast.get());
      HIRDumper dumper;
      std::cout << dumper.dump_program(&hir) << std::endl;
      return 0;
  }

  // 既存のインタプリタ実行（変更なし）
  Interpreter interpreter;
  interpreter.execute(ast.get());
  ```

**影響範囲**: なし（新しいコードパスのみ）

### Phase 2: MIR実装（Week 5-8）

#### Week 5-6: MIRとCFG実装
```cpp
// 新規ファイル
src/backend/ir/mir/mir_node.h
src/backend/ir/mir/mir_generator.h
src/backend/ir/mir/cfg.h
```

**変更ファイル**: なし

**影響範囲**: なし（新規コードのみ）

#### Week 7: SSA形式実装
```cpp
// 新規ファイル
src/backend/ir/mir/ssa_builder.h
src/backend/ir/mir/dominator_tree.h
```

**変更ファイル**: なし

**影響範囲**: なし

#### Week 8: MIRテストと統合
```cpp
// 新規テストファイル
tests/unit/ir/test_mir_generation.cpp
tests/unit/ir/test_cfg_construction.cpp
tests/unit/ir/test_ssa_construction.cpp
```

**変更ファイル**:
- `src/frontend/main.cpp`: `--dump-mir`、`--dump-cfg`オプションの実装

**影響範囲**: なし

### Phase 3: LIR実装（Week 9-10）

#### Week 9: LIR実装
```cpp
// 新規ファイル
src/backend/ir/lir/lir_node.h
src/backend/ir/lir/lir_generator.h
```

**変更ファイル**: なし

**影響範囲**: なし

#### Week 10: LIRテストと統合
```cpp
// 新規テストファイル
tests/unit/ir/test_lir_generation.cpp
```

**変更ファイル**:
- `src/frontend/main.cpp`: `--dump-lir`オプションの実装

**影響範囲**: なし

### Phase 4: コード生成基盤（Week 11-12）

#### Week 11: コード生成インターフェース
```cpp
// 新規ファイル
src/backend/codegen/common/codegen_interface.h
src/backend/codegen/common/target_info.h
```

**変更ファイル**:
- `src/frontend/main.cpp`: バックエンド選択ロジック
  ```cpp
  // バックエンド選択
  if (backend == "interpreter") {
      // 既存のインタプリタパス（変更なし）
      Interpreter interpreter;
      interpreter.execute(ast.get());
  } else if (backend == "native" || backend == "wasm" || backend == "typescript") {
      // 新しいコンパイラパス
      HIRProgram hir = generate_hir(ast.get());
      MIRProgram mir = generate_mir(hir);
      LIRProgram lir = generate_lir(mir);

      if (backend == "native") {
          NativeCodegen codegen;
          codegen.generate(lir);
      } else if (backend == "wasm") {
          WASMCodegen codegen;
          codegen.generate(lir);
      } else if (backend == "typescript") {
          TypeScriptCodegen codegen;
          codegen.generate(lir);
      }
  }
  ```

**影響範囲**: なし（既存のインタプリタパスは維持）

---

## 5. 互換性の維持

### 5.1 後方互換性の保証

v0.16.0では以下の互換性を保証します：

#### コマンドラインインターフェース
```bash
# 既存のコマンド（変更なし）
./main example.cb              # インタプリタで実行（デフォルト）
./main example.cb --debug      # デバッグモード

# 新しいコマンド（追加）
./main example.cb --backend=native   # ネイティブコンパイル
./main example.cb --dump-hir         # HIRダンプ
```

#### 既存のテストスイート
```bash
# 全ての既存テストがパスすることを保証
make test                      # 全テストスイート
make integration-test          # 統合テスト
make unit-test                 # ユニットテスト
```

#### インタプリタの動作
- AST直接実行モードは完全に維持
- パフォーマンス特性は変更なし
- エラーメッセージは変更なし

### 5.2 移行期間の共存

v0.16.0からv0.17.0への移行期間中は、以下の両方をサポートします：

```
┌─────────────────────────┐
│   AST                   │
│                         │
├─────────────────────────┤
│   既存パス（維持）      │
│   AST → Interpreter     │
│                         │
│   新規パス（追加）      │
│   AST → HIR → MIR → ... │
└─────────────────────────┘
```

---

## 6. マイグレーション手順

### 6.1 開発者向け手順

#### Step 1: ブランチの作成
```bash
git checkout -b feature/ir-implementation
```

#### Step 2: 段階的なコミット
```bash
# Phase 1: IR基盤
git commit -m "Add IR directory structure"
git commit -m "Implement HIR nodes"
git commit -m "Implement HIR generator"

# Phase 2: MIR
git commit -m "Implement MIR nodes"
git commit -m "Implement CFG builder"
git commit -m "Implement SSA builder"

# Phase 3: LIR
git commit -m "Implement LIR nodes"
git commit -m "Implement LIR generator"

# Phase 4: 統合
git commit -m "Integrate IR pipeline into main"
```

#### Step 3: テストの実行
```bash
# 既存テストの確認（パスすることを確認）
make test

# 新しいIRテストの実行
make ir-test
```

#### Step 4: マージ
```bash
# メインブランチへのマージ
git checkout main
git merge feature/ir-implementation
```

### 6.2 ユーザー向け手順

#### 既存ユーザー
- **変更不要**: 既存のコマンドは全て動作します
- **新機能**: 新しい`--backend`オプションを使用してコンパイラ機能を試せます

#### 新規ユーザー
- インタプリタモードとコンパイラモードの両方を選択可能

---

## 7. リスク管理

### 7.1 リスクと対策

#### リスク1: 既存機能の破壊
**対策**:
- 全ての変更を段階的に実施
- 各フェーズで既存テストスイートを実行
- CI/CDで自動テスト

#### リスク2: コードベースの肥大化
**対策**:
- モジュール化された設計
- 共通コードの抽出
- 不要なコードの削除

#### リスク3: パフォーマンスの低下
**対策**:
- ベンチマークの実施
- パフォーマンス測定
- 最適化の実施

### 7.2 ロールバック計画

各フェーズで問題が発生した場合のロールバック手順：

```bash
# 特定のコミットに戻る
git revert <commit-hash>

# 既存テストが全てパスすることを確認
make test
```

---

## 8. チェックリスト

### Phase 1完了条件
- [ ] IRディレクトリ構造が作成されている
- [ ] HIRノード定義が完了
- [ ] HIRジェネレーターが実装されている
- [ ] HIRダンプ機能が動作する
- [ ] 既存のテストスイートが全てパス

### Phase 2完了条件
- [ ] MIRノード定義が完了
- [ ] CFGビルダーが実装されている
- [ ] SSA形式への変換が動作する
- [ ] MIRダンプ機能が動作する
- [ ] 既存のテストスイートが全てパス

### Phase 3完了条件
- [ ] LIRノード定義が完了
- [ ] LIRジェネレーターが実装されている
- [ ] LIRダンプ機能が動作する
- [ ] 既存のテストスイートが全てパス

### Phase 4完了条件
- [ ] コード生成インターフェースが定義されている
- [ ] バックエンド選択ロジックが実装されている
- [ ] 全てのバックエンドで基本的なコード生成が動作する
- [ ] 既存のテストスイートが全てパス

---

## 9. まとめ

このリファクタリング計画により：

1. **後方互換性を維持**: 既存のインタプリタ機能は完全に保持
2. **段階的な移行**: 各フェーズで既存機能を破壊しない
3. **新機能の追加**: IR層とコード生成機能を追加
4. **テストの充実**: 各フェーズでテストを実施

v0.16.0完了後、v0.17.0以降で最適化とコード生成の実装を進めます。
