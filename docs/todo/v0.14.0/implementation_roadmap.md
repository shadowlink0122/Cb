# v0.14.0 実装ロードマップ

**バージョン**: v0.14.0
**目標**: IR（中間表現）の設計と実装
**期間**: 3-4ヶ月
**作成日**: 2025-11-13

---

## 概要

v0.14.0では、Cb言語のネイティブコンパイラ実装の基盤となる中間表現（IR）を設計・実装します。このバージョンは、インタプリタ中心のアーキテクチャからコンパイラベースのアーキテクチャへの大規模な移行を開始する重要なマイルストーンです。

**主要な変更**:
- 3層IR構造（HIR/MIR/LIR）の導入
- SSA形式の実装
- 制御フローグラフ（CFG）の構築
- データフロー解析基盤の整備
- IRビューワーとデバッグツールの実装

---

## v0.14.0でやらなければならないこと

### 1. HIR (High-level IR) の実装

**目的**: ASTに型情報とスコープ情報を付加した高レベル中間表現

#### 実装タスク

**1.1 HIR基本構造の設計**
- [ ] HIRノード定義（`src/backend/ir/hir/hir_node.h`）
  - HIRExpr（式ノード）
  - HIRStmt（文ノード）
  - HIRFunction（関数定義）
  - HIRProgram（プログラム全体）
- [ ] 型情報の統合
- [ ] ソースコード位置情報の保持

**1.2 ASTからHIRへの変換器**
- [ ] HIRGenerator クラスの実装（`src/backend/ir/hir/hir_generator.h/cpp`）
- [ ] 式の変換
  - リテラル
  - 変数参照
  - 二項演算・単項演算
  - 関数呼び出し
  - メンバーアクセス
  - 配列アクセス
- [ ] 文の変換
  - 変数宣言
  - 代入文
  - if文
  - while文/for文
  - return文/break文/continue文
- [ ] 関数定義の変換
- [ ] 構造体・enum・interfaceの変換

**1.3 型解決とスコープ管理**
- [ ] シンボルテーブルとの統合
- [ ] 型推論の実行
- [ ] ジェネリクスの単相化（Monomorphization）

**1.4 HIRビジター**
- [ ] HIRVisitorインターフェース（`src/backend/ir/hir/hir_visitor.h`）
- [ ] HIRダンプ機能（`src/backend/ir/hir/hir_dumper.h/cpp`）
- [ ] HIR検証機能

**1.5 テスト**
- [ ] HIRユニットテスト（30テスト）
  - 式変換のテスト
  - 文変換のテスト
  - 型解決のテスト
  - ジェネリクス単相化のテスト

---

### 2. MIR (Mid-level IR) の実装

**目的**: 最適化に適したSSA形式の中レベル中間表現

#### 実装タスク

**2.1 MIR基本構造の設計**
- [ ] MIRノード定義（`src/backend/ir/mir/mir_node.h`）
  - MIRPlace（左辺値）
  - MIRRvalue（右辺値）
  - MIROperand（オペランド）
  - MIRStatement（文）
  - MIRTerminator（終端命令）
  - MIRLocal（ローカル変数）
- [ ] MIRFunction構造

**2.2 制御フローグラフ（CFG）の実装**
- [ ] BasicBlock構造（`src/backend/ir/mir/cfg.h/cpp`）
  - 命令列
  - 終端命令
  - predecessor/successorリンク
- [ ] ControlFlowGraph クラス
  - エントリーブロック・出口ブロックの管理
  - ブロック間のエッジ管理
- [ ] CFG構築アルゴリズム
  - HIRの制御フロー構造の解析
  - 基本ブロック境界の決定
  - エッジの構築

**2.3 SSA形式への変換**
- [ ] SSABuilder クラス（`src/backend/ir/mir/ssa_builder.h/cpp`）
- [ ] 支配木の構築（`src/backend/ir/mir/dominator_tree.h/cpp`）
  - Lengauer-Tarjanアルゴリズムの実装
  - 即座支配者（Immediate Dominator）の計算
  - 支配境界（Dominance Frontier）の計算
- [ ] PHIノードの挿入
  - 支配境界を使ったPHIノード配置
  - 最小限のPHIノード挿入
- [ ] 変数のリネーミング
  - スタックベースのリネーミング
  - SSA形式の完成

**2.4 データフロー解析**
- [ ] 生存変数解析（`src/backend/ir/analysis/liveness.h/cpp`）
  - live_in/live_outセットの計算
  - 反復データフロー解析
- [ ] 到達定義解析（`src/backend/ir/analysis/reaching_defs.h/cpp`）
  - gen/killセットの計算
  - 到達定義の伝播
- [ ] 使用-定義チェーン（`src/backend/ir/analysis/use_def_chain.h/cpp`）
  - 各変数の定義点の追跡
  - 各変数の使用点の追跡

**2.5 MIRジェネレーター**
- [ ] MIRGenerator クラス（`src/backend/ir/mir/mir_generator.h/cpp`）
- [ ] HIRからMIRへの変換
  - HIR式のMIR文への分解
  - 一時変数の導入
  - 制御フローの基本ブロックへの分割

**2.6 MIRダンプとビジュアライゼーション**
- [ ] MIRダンプ機能（`src/backend/ir/mir/mir_dumper.h/cpp`）
  - テキスト形式でのMIR出力
  - SSA形式の可読性の高い表示
- [ ] GraphViz可視化（`src/backend/ir/mir/graphviz_gen.h/cpp`）
  - CFGのDOT形式出力
  - 支配木のDOT形式出力
  - PNG/SVG生成

**2.7 テスト**
- [ ] MIRユニットテスト（40テスト）
  - CFG構築のテスト
  - SSA変換のテスト
  - 支配木構築のテスト
  - データフロー解析のテスト

---

### 3. LIR (Low-level IR) の実装

**目的**: ターゲット非依存の低レベル命令表現

#### 実装タスク

**3.1 LIR基本構造の設計**
- [ ] LIRノード定義（`src/backend/ir/lir/lir_node.h`）
  - LIRInstruction（命令）
  - LIROperand（オペランド）
  - LIRBasicBlock（基本ブロック）
  - LIRFunction（関数）
- [ ] 3アドレスコード形式
- [ ] 仮想レジスタの管理

**3.2 命令セットの定義**
- [ ] データ移動命令（Move, Load, Store）
- [ ] 算術演算命令（Add, Sub, Mul, Div, Mod）
- [ ] 論理演算命令（And, Or, Xor, Not）
- [ ] 比較命令（Cmp）
- [ ] 制御フロー命令（Jump, JumpIf, Call, Return）

**3.3 LIRジェネレーター**
- [ ] LIRGenerator クラス（`src/backend/ir/lir/lir_generator.h/cpp`）
- [ ] MIRからLIRへの変換
  - MIR文のLIR命令への変換
  - MIR terminatorの変換
  - オペランドの変換
- [ ] 仮想レジスタの割り当て
  - MIRローカル変数から仮想レジスタへのマッピング

**3.4 LIRダンプ**
- [ ] LIRダンプ機能（`src/backend/ir/lir/lir_dumper.h/cpp`）
  - テキスト形式でのLIR出力
  - 仮想レジスタの表示

**3.5 テスト**
- [ ] LIRユニットテスト（30テスト）
  - 命令生成のテスト
  - MIRからLIRへの変換テスト
  - 仮想レジスタ割り当てのテスト

---

### 4. IRビューワーとデバッグツール

#### 実装タスク

**4.1 IRダンプ機能**
- [ ] 統一されたダンプインターフェース
- [ ] HIRダンプのフォーマット
- [ ] MIRダンプのフォーマット
- [ ] LIRダンプのフォーマット
- [ ] ファイル出力機能

**4.2 GraphViz可視化**
- [ ] CFG可視化
- [ ] 支配木可視化
- [ ] データフロー情報の可視化
- [ ] PNG/SVG出力

**4.3 コマンドラインインターフェース**
- [ ] `--dump-hir` オプション
- [ ] `--dump-mir` オプション
- [ ] `--dump-lir` オプション
- [ ] `--dump-cfg` オプション
- [ ] `--emit-cfg-dot` オプション
- [ ] `--dump-all-ir` オプション
- [ ] `--stop-at=<level>` オプション

**4.4 IR検証ツール**
- [ ] HIR検証器
  - 型整合性のチェック
  - スコープ情報の検証
- [ ] MIR検証器
  - SSA形式の検証
  - CFG整合性のチェック
  - 到達可能性の検証
- [ ] LIR検証器
  - 命令の妥当性チェック
  - オペランドの検証

---

### 5. プロジェクト構造のリファクタリング

#### 実装タスク

**5.1 ディレクトリ構造の整備**
- [ ] `src/backend/ir/hir/` ディレクトリの作成
- [ ] `src/backend/ir/mir/` ディレクトリの作成
- [ ] `src/backend/ir/lir/` ディレクトリの作成
- [ ] `src/backend/ir/analysis/` ディレクトリの作成
- [ ] `tests/unit/ir/` ディレクトリの作成
- [ ] `tests/integration/ir/` ディレクトリの作成
- [ ] `tests/cases/ir/` ディレクトリの作成

**5.2 Makefileの更新**
- [ ] IR関連のオブジェクトファイルの定義
- [ ] IR関連のビルドルールの追加
- [ ] ir-test ターゲットの追加
- [ ] ir-benchmark ターゲットの追加

**5.3 ビルドシステムの拡張**
- [ ] 段階的ビルドのサポート（HIR/MIR/LIR個別ビルド）
- [ ] 並列ビルドの最適化
- [ ] 依存関係の整理

---

### 6. テストとベンチマーク

#### 実装タスク

**6.1 ユニットテスト**
- [ ] HIRユニットテスト（30テスト）
- [ ] MIRユニットテスト（40テスト）
- [ ] LIRユニットテスト（30テスト）
- [ ] テストフレームワークの整備（Google Test）

**6.2 統合テスト**
- [ ] IR生成のラウンドトリップテスト（20テスト）
  - AST → HIR → MIR → LIR
- [ ] IRダンプの検証テスト
- [ ] 期待される出力との比較テスト

**6.3 ベンチマーク**
- [ ] HIR生成のベンチマーク
- [ ] MIR生成のベンチマーク
- [ ] LIR生成のベンチマーク
- [ ] メモリ使用量のベンチマーク
- [ ] ベンチマーク結果のレポート

**6.4 テストケースの作成**
- [ ] 基本的な式・文（`tests/cases/ir/basic.cb`）
- [ ] 制御フロー（`tests/cases/ir/control_flow.cb`）
- [ ] ネストしたループ（`tests/cases/ir/nested_loops.cb`）
- [ ] 関数呼び出し（`tests/cases/ir/function_calls.cb`）
- [ ] ジェネリクス（`tests/cases/ir/generics.cb`）
- [ ] 構造体とメソッド（`tests/cases/ir/structs.cb`）

**6.5 期待される出力の作成**
- [ ] HIRダンプの期待される出力
- [ ] MIRダンプの期待される出力
- [ ] LIRダンプの期待される出力

---

### 7. ドキュメント

#### 実装タスク

**7.1 IR設計ドキュメント**
- [x] IR実装計画と技術選定（`docs/todo/v0.14.0/ir_implementation_plan.md`）
- [ ] HIR仕様書（`docs/ir/hir_specification.md`）
- [ ] MIR仕様書（`docs/ir/mir_specification.md`）
- [ ] LIR仕様書（`docs/ir/lir_specification.md`）
- [ ] SSA形式の説明（`docs/ir/ssa_format.md`）

**7.2 APIリファレンス**
- [ ] HIR APIドキュメント
- [ ] MIR APIドキュメント
- [ ] LIR APIドキュメント
- [ ] データフロー解析APIドキュメント

**7.3 使用方法ガイド**
- [ ] IRダンプの使い方
- [ ] GraphViz可視化の使い方
- [ ] IR検証ツールの使い方
- [ ] コンパイラフラグの説明

**7.4 サンプルコードと出力例**
- [ ] シンプルな関数のIR例
- [ ] 制御フローのIR例
- [ ] ジェネリクスのIR例
- [ ] CFG可視化の例

---

### 8. エラーハンドリングと診断

#### 実装タスク

**8.1 IRエラーの定義**
- [ ] IRError クラス（`src/backend/ir/common/ir_error.h`）
- [ ] エラー種別の定義
  - TypeError
  - UndefinedSymbol
  - InvalidConversion
  - SSAViolation
  - CFGError

**8.2 エラーレポーター**
- [ ] IRErrorReporter クラス
- [ ] エラーメッセージのフォーマット
- [ ] ソースコード位置の表示
- [ ] エラー復帰戦略

**8.3 警告システム**
- [ ] 警告の定義
- [ ] 警告レベルの設定
- [ ] 警告の抑制機能

---

## 実装スケジュール

### Month 1: HIR実装

**Week 1-2: HIR基本構造**
- HIRノード定義
- ASTからHIRへの基本的な変換
- 型情報の統合

**Week 3: HIR高度な機能**
- 制御フロー変換
- 関数定義・呼び出し
- 構造体・enum

**Week 4: HIRジェネリクスとテスト**
- ジェネリクス単相化
- HIRダンプ機能
- ユニットテスト（30テスト）

### Month 2: MIR実装

**Week 1: MIR基本構造とCFG**
- MIRノード定義
- 基本ブロック構造
- CFG構築

**Week 2: SSA形式の実装**
- 支配木の構築
- PHIノード挿入
- 変数リネーミング

**Week 3: データフロー解析**
- 生存変数解析
- 到達定義解析
- 使用-定義チェーン

**Week 4: MIR完成とテスト**
- MIRダンプ機能
- GraphViz可視化
- ユニットテスト（40テスト）

### Month 3: LIR実装、FFI、モジュールシステム

**Week 1-2: LIR実装**
- LIRノード定義
- 命令セット定義
- MIRからLIRへの変換
- ユニットテスト（30テスト）

**Week 3: FFIと条件付きコンパイル（次バージョン（v0.15.0以降）への準備）**
- FFI (Foreign Function Interface) 実装
  - extern "C" 構文のパース
  - 呼び出し規約の実装
  - C型マッピング
  - HIR/MIR/LIRでのFFIサポート
- 条件付きコンパイル実装
  - #[cfg(...)] 属性のパース
  - 条件評価エンジン
  - AST段階でのフィルタリング
- ユニットテスト（35テスト）

**Week 4: モジュールシステムと統合**
- モジュールシステム実装
  - mod/use構文のパース
  - モジュール解決
  - ファイル分割サポート
  - 可視性制御（pub）
- IRビューワーとツールの完成
  - ダンプ機能の完成
  - GraphViz可視化の完成
  - コマンドラインオプション
  - IR検証ツール
- 統合テスト（20テスト）
- ベンチマーク
- ドキュメント完成
- リリース準備

---

## チェックリスト

### コード実装
- [ ] HIR実装（10ファイル）
- [ ] MIR実装（15ファイル）
- [ ] LIR実装（8ファイル）
- [ ] データフロー解析（6ファイル）
- [ ] IRダンプ機能（3ファイル）
- [ ] GraphViz可視化（2ファイル）
- [ ] FFI実装（4ファイル）
- [ ] 条件付きコンパイル実装（3ファイル）
- [ ] モジュールシステム実装（5ファイル）

### テスト
- [ ] HIRユニットテスト（30テスト）
- [ ] MIRユニットテスト（40テスト）
- [ ] LIRユニットテスト（30テスト）
- [ ] FFIユニットテスト（20テスト）
- [ ] 条件付きコンパイルテスト（15テスト）
- [ ] モジュールシステムテスト（20テスト）
- [ ] 統合テスト（20テスト）
- [ ] ベンチマークテスト（5ベンチマーク）

### ドキュメント
- [ ] IR実装計画（完了）
- [ ] HIR仕様書
- [ ] MIR仕様書
- [ ] LIR仕様書
- [ ] FFI仕様書
- [ ] 条件付きコンパイル仕様書
- [ ] モジュールシステム仕様書
- [ ] APIリファレンス
- [ ] 使用方法ガイド

### ツール
- [ ] IRダンプコマンド
- [ ] CFG可視化ツール
- [ ] IR検証ツール
- [ ] ベンチマークツール

---

## 完了条件

v0.14.0は以下の条件を満たしたときに完了とします：

1. **機能完全性**
   - [ ] HIR/MIR/LIRの全ての実装が完了
   - [ ] ASTからLIRまでの完全な変換パイプライン
   - [ ] SSA形式の正しい実装
   - [ ] データフロー解析が動作
   - [ ] FFI (Foreign Function Interface) が動作
   - [ ] 条件付きコンパイルが動作
   - [ ] モジュールシステムが動作

2. **品質保証**
   - [ ] 全てのユニットテストがパス（155テスト以上）
   - [ ] 全ての統合テストがパス（20テスト以上）
   - [ ] コードカバレッジ > 85%
   - [ ] メモリリークゼロ（Valgrindで確認）

3. **パフォーマンス**
   - [ ] 1000行のコードをHIRまで100ms以内で処理
   - [ ] HIRからMIRへ50ms以内で変換
   - [ ] MIRからLIRへ30ms以内で変換
   - [ ] メモリ使用量 < 50MB（1000行のコード）

4. **ツール**
   - [ ] IRダンプ機能が動作
   - [ ] GraphViz可視化が動作
   - [ ] IR検証ツールが動作

5. **ドキュメント**
   - [ ] 全ての仕様書が完成
   - [ ] APIドキュメントが完成
   - [ ] 使用方法ガイドが完成
   - [ ] サンプルコードと出力例が完成

---

## 次のバージョンへの準備（v0.15.0以降）

v0.14.0完了後、v0.15.0以降で段階的に以下の機能を実装します：

**v0.15.0以降で実装する機能**:
1. 標準ライブラリの実装
   - std::io (println, print等)
   - std::mem (malloc, free等)
   - std::string (String型)
   - std::collections (Vec, Map等)
2. OS固有実装
   - Linux/macOS/Windows向け実装
   - ベアメタル向け実装
3. FFIを使用したシステムコールラッパー
4. 条件付きコンパイルによるプラットフォーム分岐

**v0.14.0で準備したこと**:
- [x] FFI基盤（extern "C", 呼び出し規約）
- [x] 条件付きコンパイル（#[cfg(...)]）
- [x] モジュールシステム（mod/use）
- [ ] インラインアセンブラ（low_level_support.mdで計画済み）

**v0.18.0の予定**:
- パッケージマネージャー（cbpkg）
- 依存関係管理
- パッケージレジストリ

---

## まとめ

v0.14.0では、Cb言語のネイティブコンパイラ実装の基盤となる3層IR構造（HIR/MIR/LIR）を設計・実装し、さらに将来のバージョン（v0.15.0: 複数バックエンド対応、v0.16.0、v0.17.0: 標準ライブラリ化）に向けた基盤機能を実装します。

**実装期間**: 3-4ヶ月
**主要成果物**:
- HIR/MIR/LIR実装
- IRビューワー
- データフロー解析基盤
- FFI (Foreign Function Interface)
- 条件付きコンパイル (#[cfg(...)])
- モジュールシステム (mod/use)

**目標**:
- 将来のバージョン（v0.15.0: 複数バックエンド、v0.16.0、v0.17.0: 標準ライブラリ）への準備完了
- v0.18.0でのパッケージエコシステム構築への準備完了
