# Error Propagation Tests

このディレクトリには、?オペレーター（エラー伝播）とタイムアウト機能のテストが含まれています。

## v0.12.1 実装予定機能

### ?オペレーター
- `test_question_operator_result.cb` - Result<T, E>でのエラー伝播
- `test_question_operator_option.cb` - Option<T>でのNone伝播

### タイムアウト機能
- `test_timeout_basic.cb` - 基本的なタイムアウト機能（作成予定）

### async関数との組み合わせ
- `../async/test_async_question_operator.cb` - async関数内での?オペレーター

## ステータス

**設計**: ✅ 完了  
**テストケース**: ✅ 作成完了  
**実装**: ⏳ 予定

これらのテストは、実装完了後に実行されます。

## 参考ドキュメント

- [question_operator_design.md](../../docs/features/question_operator_design.md)
- [timeout_design.md](../../docs/features/timeout_design.md)
- [v0.12.1リリースノート](../../release_notes/v0.12.1.md)
