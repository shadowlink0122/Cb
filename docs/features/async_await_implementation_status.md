# Cb言語 v0.12.0 async/await実装状況

## 実装済み機能

### Phase 1: イベントループとタイマー (完了)
- ✅ EventLoop C++実装
- ✅ Timer機能
- ✅ インタプリタとの統合

### Phase 2: Future<T>の基礎 (完了)
- ✅ Future<T>ジェネリック構造体 (stdlib/std/future.cb)
- ✅ C++ Future実装 (src/backend/interpreter/types/future.cpp)

### Phase 2.0: Cooperative Multitasking (完了)
- ✅ run_background_tasks_one_cycle()の統合
  - forループ、whileループでのバックグラウンドタスク実行
  - 再帰関数、ネストした関数呼び出しでの協調的実行
  - if-else、switch-case、matchステートメント対応
- ✅ async interface/impl support
  - インターフェースメソッドにasyncキーワード対応
  - 実装側でのasync override検証

### Phase 4: async/awaitキーワード (部分完了)
- ✅ asyncキーワードのパース
- ✅ awaitキーワードのパース
- ✅ await式の評価ロジック
  - `int x = await future_expr;`が動作
  - 式の中でawaitを使用可能: `(await fut) * 2`
- ✅ 手動Future返却のasync関数
  ```cb
  async Future<int> compute() {
      Future<int> result;
      result.value = 123;
      result.is_ready = true;
      return result;
  }
  ```

## 使用例

### 基本的なawait式

```cb
import stdlib.std.future;

int main() {
    // Futureを手動で作成
    Future<int> fut;
    fut.value = 42;
    fut.is_ready = true;
    
    // awaitで値を取得
    int result = await fut;
    print(result);  // 42
    
    return 0;
}
```

### 関数がFutureを返す場合

```cb
import stdlib.std.future;

Future<int> compute_async() {
    Future<int> result;
    result.value = 123;
    result.is_ready = true;
    return result;
}

int main() {
    int value = await compute_async();
    print(value);  // 123
    return 0;
}
```

### 式の中でawaitを使用

```cb
int main() {
    Future<int> fut;
    fut.value = 10;
    fut.is_ready = true;
    
    // 式の中でawait
    int doubled = (await fut) * 2;
    print(doubled);  // 20
    
    return 0;
}
```

## 今後の実装予定

### sleep_ms() - 真の非同期待機 (未実装)

現在のv0.12.0では`sleep_ms()`は除外されています。将来的には真の非同期待機機能として実装予定:

```cb
async void process() {
    println("開始");
    await sleep_ms(1000);  // 1秒待機（非同期）
    println("1秒後");
}
```

実装方針:
- タイマーイベントベースの非同期待機
- Future<void>を返す形式
- イベントループとの統合

### async関数の自動Futureラッピング (未実装)

現在は手動でFutureを構築して返す必要がありますが、将来的には自動ラッピングを実装予定:

```cb
// 目標: こう書けるようにする
async int compute() {
    return 123;  // 自動的にFuture<int>でラップ
}

int main() {
    int result = await compute();  // awaitで取り出し
    return 0;
}
```

実装方法:
- return文ハンドラでasync関数かどうかをチェック
- return値をFuture<T>構造体でラップ
- ReturnExceptionでFutureを返す

### 真の非同期実行 (未実装)

現在のasync関数は同期的に実行されます。将来的には:
- async関数をイベントループで非同期実行
- Promise/Futureの状態管理
- タスクスケジューリング

## テスト結果

### await式のテスト
- ✅ Future<int>からの値取得
- ✅ Future<string>からの値取得
- ✅ 関数が返すFutureのawait
- ✅ 式の中でのawait使用

### 統合テスト
- ✅ 全統合テスト (3570個) PASS
- ✅ 全ユニットテスト (30個) PASS
- ✅ 全標準ライブラリテスト (53個) PASS

## コミット履歴

v0.12.0として以下の機能を統合実装:

1. Phase 1: Event Loop foundation
   - EventLoop/Timer基盤実装
   
2. Phase 2: Future<T> implementation
   - ジェネリックFuture<T>型
   
3. Phase 2.0: Cooperative multitasking
   - 全言語構造での協調的マルチタスク対応
   - async interface/impl support
   
4. Phase 4: async/await syntax
   - async/awaitキーワード実装
   - await式評価ロジック

## ファイル構成

### 標準ライブラリ
- `stdlib/std/future.cb` - Future<T>ジェネリック構造体定義
- `stdlib/concurrency/` - 将来の非同期ユーティリティ用ディレクトリ

### C++実装
- `src/backend/interpreter/event_loop/` - イベントループ実装
- `src/backend/interpreter/types/future.{h,cpp}` - Future C++実装
- `src/frontend/recursive_parser/parsers/` - async/await構文パース
- `src/backend/interpreter/evaluator/` - await式評価、協調的マルチタスク
- `src/common/ast.h` - is_async_function, is_await_expression フラグ

### ドキュメント
- `docs/features/async_await_implementation_status.md` - 実装状況（本ファイル）
- `docs/todo/v0.12.0_async_await_design.md` - 設計ドキュメント

### サンプル
- `sample/async/` - v0.12.0で動作するサンプル
- `docs/async_samples_future/` - 将来実装予定機能を使うサンプル（sleep_msなど）
