# Cb言語 機能ドキュメント

このディレクトリには、Cb言語の各機能の設計仕様と実装状況が記載されています。

---

## 📋 v0.12.1 機能一覧

### ✅ 実装完了
1. **簡潔なasync構文** (`async T` vs `async Future<T>`)
2. **enum型の直接return対応** (async関数内)
3. **ジェネリックインターフェース + async** の統合

### 📝 設計・ドキュメント完了（実装予定）
4. **?オペレーター** - エラー伝播演算子
   - [question_operator_design.md](question_operator_design.md)
   - テストケース: `tests/cases/error_propagation/`
   
5. **タイムアウト機能** - async関数のタイムアウト
   - [timeout_design.md](timeout_design.md)
   - テストケース: `tests/cases/error_propagation/test_timeout_basic.cb`（予定）

---

## 📚 ドキュメント一覧

### 実装完了レポート
- [v0.12.1_implementation_complete.md](v0.12.1_implementation_complete.md) - v0.12.1全機能サマリー
- [v0.12.1_test_coverage_report.md](v0.12.1_test_coverage_report.md) - テストカバレッジレポート

### 設計仕様
- [question_operator_design.md](question_operator_design.md) - ?オペレーター設計
- [timeout_design.md](timeout_design.md) - タイムアウト機能設計
- [error_propagation_design.md](error_propagation_design.md) - エラー伝播全般
- [comprehensive_error_handling_design.md](comprehensive_error_handling_design.md) - 包括的エラーハンドリング

### ステータス文書
- [v0.12.1_IMPLEMENTATION_NOTE.md](v0.12.1_IMPLEMENTATION_NOTE.md) - 重要：実装状況の説明
- [v0.12.1_question_operator_timeout_status.md](v0.12.1_question_operator_timeout_status.md) - 詳細ステータス

### 将来バージョン
- [v0.14.0_implementation_progress.md](v0.14.0_implementation_progress.md) - v0.14.0進捗
- [v0.15.0_untested_behaviors.md](v0.15.0_untested_behaviors.md) - 未テスト挙動一覧

---

## 🎯 実装優先度

### 次の実装（優先度1）
1. ?オペレーターのパーサー実装
2. ?オペレーターのインタープリター実装
3. テストの実行と検証

### 中期実装（優先度2）
1. `create_timer()` builtin実装
2. `timeout()` stdlib実装
3. Event Loopのタイマー機能拡張

### 長期実装（v0.12.1以降）
1. `race()` 関数
2. `select!` マクロ
3. エラー型自動変換（FromError trait）
4. Try traitとカスタム型サポート

---

## 📖 読み方ガイド

### 新しい機能について知りたい
1. [v0.12.1リリースノート](../../release_notes/v0.12.1.md)を読む
2. 各機能の設計ドキュメントを参照

### 実装状況を確認したい
1. [v0.12.1_IMPLEMENTATION_NOTE.md](v0.12.1_IMPLEMENTATION_NOTE.md)を読む
2. [v0.12.1_question_operator_timeout_status.md](v0.12.1_question_operator_timeout_status.md)で詳細確認

### 実装に取り組みたい
1. 設計ドキュメント（`*_design.md`）を読む
2. テストケース（`tests/cases/`）を確認
3. 実装方針セクションを参照

---

**最終更新**: 2025年11月11日  
**バージョン**: v0.12.1
