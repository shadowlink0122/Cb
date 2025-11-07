# async/await テストファイル

このディレクトリには、async/await機能のテストファイルが含まれています。

**注意**: これらのファイルの多くは、Phase 1実装前の実験的なコードです。
設計見直し後は、多くが動作しない可能性があります。

## ファイル分類

### 動作するファイル（Phase 1設計準拠）

Phase 1実装後に作成される新しいテストファイル：
- TBD

### 古い実験的ファイル（要見直し）

以下のファイルは設計見直し前のものです：

#### 基本テスト
- `async_await_test.cb` - 初期のasync/awaitテスト
- `async_keyword_test.cb` - asyncキーワードのパーステスト
- `async_minimal_test.cb` - 最小限のテスト
- `async_simple_test.cb` - シンプルなテスト

#### 並行実行テスト（Phase 1では未サポート）
- `async_builtin_concurrent.cb`
- `async_concurrent_demo.cb`
- `async_concurrent_test.cb`
- `async_interleave_demo.cb`
- `async_interleaved_manual.cb`
- `async_manual_interleave.cb`
- `async_true_concurrent.cb`

#### イベントループテスト（Phase 2以降）
- `async_event_loop_direct.cb`
- `async_event_loop_simple.cb`
- `async_event_loop_test.cb`

#### 遅延実行テスト（Phase 2以降）
- `async_deferred_execution_demo.cb`
- `async_deferred_execution_test.cb`
- `async_simple_deferred_test.cb`

#### yieldテスト（Phase 2以降）
- `yield_basic.cb`
- `yield_simple.cb`

#### デモファイル
- `async_complete_demo.cb`
- `async_comprehensive_demo.cb`
- `async_practical_demo.cb`
- `async_builtin_direct.cb`
- `async_multiple_functions_test.cb`

#### その他のテストファイル
- `test_future_basic.cb`
- `test_minimal_struct_array.cb`
- `test_simple_int_interpolation.cb`
- `test_struct_array_assign_fix.cb`
- `test_struct_array_assign.cb`
- `test_struct_array.cb`
- `test_task_queue_basic.cb`
- `test_task_queue_comprehensive.cb`
- `test_task_queue.cb`

## Phase 1実装後の作業

1. 新しいPhase 1準拠のテストファイルを作成
2. 古いファイルを見直し、Phase 1で動作するものを特定
3. Phase 2以降で必要なファイルをマーク
4. 不要なファイルを削除

## 参照

- [async/await 設計ドキュメント](../../../docs/features/async_await_design.md)
- [並行処理概要](../../../docs/features/concurrency_overview.md)
