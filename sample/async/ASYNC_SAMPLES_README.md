# Cb言語 非同期プログラミング サンプル集

このディレクトリには、Cb言語のasync/await機能を使った実用的なサンプルプログラムが含まれています。

## サンプルファイル

### 1. async_practical.cb ⭐ おすすめ
**シンプルで実用的な非同期パターン集**

実装されているパターン:
- ✅ 基本的なawait式
- ✅ 文字列を返す非同期関数
- ✅ 複数の非同期処理を順次実行
- ✅ 式の中でawaitを使用
- ✅ 条件分岐と非同期処理
- ✅ エラーハンドリングパターン（Result型風）
- ✅ チェーン処理
- ✅ データ取得とフィルタリング
- ✅ フィボナッチ数列の非同期計算

実行方法:
```bash
./main sample/async/async_practical.cb
```

期待される出力:
```
==========================================
  Cb async/await 実用サンプル集
==========================================

=== 1. 基本的なawait ===
取得した値: 42

=== 2. 文字列のawait ===
こんにちは、非同期の世界!

=== 3. 順次実行 ===
10 + 20 = 30
5 * 6 = 30
合計: 60

...（省略）...

==========================================
  すべてのサンプル完了!
==========================================
```

### 2. async_examples.cb
**包括的な非同期機能デモ**

実装されている例:
- 基本的なFutureの使用
- 複数のFutureを組み合わせる
- 文字列を返す非同期処理
- 条件分岐を使った非同期処理
- awaitを式の中で使用
- 複数のFutureを順次処理
- フィボナッチ数列計算
- エラーシミュレーション
- チェーン化された非同期処理

実行方法:
```bash
./main sample/async/async_examples.cb
```

### 3. async_data_processing.cb
**データベース風の非同期クエリシミュレーション**

実装されている機能:
- ユーザー、商品、注文のデータモデル
- 非同期データベースクエリ（シミュレート）
- 複数のクエリを組み合わせたビジネスロジック
- データ分析（集計）
- キャッシングのシミュレーション

実行方法:
```bash
./main sample/async/async_data_processing.cb
```

### 4. async_error_handling.cb
**エラーハンドリングとパターン集**

実装されているパターン:
- Result型風のエラーハンドリング
- 成功/失敗のチェック
- 複数の非同期処理でのエラーチェック
- タイムアウトハンドリング
- APIリクエストのエラーハンドリング
- エラーからのリカバリー（フォールバック）
- チェーン処理でのエラーハンドリング

実行方法:
```bash
./main sample/async/async_error_handling.cb
```

## 使用例

### 基本的な使い方

```cb
import stdlib.std.future;

Future<int> get_value() {
    Future<int> result;
    result.value = 42;
    result.is_ready = true;
    return result;
}

int main() {
    int value = await get_value();
    print(value);  // 42
    return 0;
}
```

### エラーハンドリング

```cb
struct Result {
    bool success;
    int value;
    string error;
};

Future<Result> divide(int a, int b) {
    Future<Result> future;
    Result result;
    
    if (b == 0) {
        result.success = false;
        result.value = 0;
        result.error = "Division by zero";
    } else {
        result.success = true;
        result.value = a / b;
        result.error = "";
    }
    
    future.value = result;
    future.is_ready = true;
    return future;
}

int main() {
    Result res = await divide(10, 2);
    
    if (res.success) {
        print("Result: ");
        print(res.value);
    } else {
        print("Error: ");
        print(res.error);
    }
    
    return 0;
}
```

### チェーン処理

```cb
Future<int> step1(int x) {
    Future<int> result;
    result.value = x + 10;
    result.is_ready = true;
    return result;
}

Future<int> step2(int x) {
    Future<int> result;
    result.value = x * 2;
    result.is_ready = true;
    return result;
}

int main() {
    int initial = 5;
    int after_step1 = await step1(initial);  // 15
    int final_result = await step2(after_step1);  // 30
    
    print(final_result);
    return 0;
}
```

## 現在の制限事項

### サポートされている機能
- ✅ await式（Future<T>から値を取り出す）
- ✅ 全ての基本型（int, string, float, double, bool）
- ✅ 式の中でのawait使用
- ✅ 関数がFutureを返す
- ✅ チェーン処理

### 制限事項
- ⚠️ 配列要素のawaitは未サポート（回避策: 個別の変数を使用）
- ⚠️ 構造体を返すFutureは一部動作が不安定
- ⚠️ async関数の自動Futureラッピングは未実装
- ⚠️ 真の非同期実行（イベントループ統合）は未実装

### 回避策

**配列要素のawaitを避ける:**
```cb
// ❌ 動作しない
Future<int> futures[3];
for (int i = 0; i < 3; i++) {
    int value = await futures[i];
}

// ✅ 代わりにこう書く
Future<int> fut1;
Future<int> fut2;
Future<int> fut3;

int val1 = await fut1;
int val2 = await fut2;
int val3 = await fut3;
```

**async関数は手動でFutureを返す:**
```cb
// 現在の書き方
Future<int> compute() {
    Future<int> result;
    result.value = 123;
    result.is_ready = true;
    return result;
}

// 将来的には（未実装）
async int compute() {
    return 123;  // 自動的にFuture<int>でラップ
}
```

## 開発状況

- **v0.12.0**: async/await基本実装
  - Phase 1: EventLoopとTimer（完了）
  - Phase 2: Future<T>基礎（完了）
  - Phase 4: async/awaitキーワード（部分完了）
    - awaitパースと評価（完了）
    - async関数自動ラッピング（未実装）
  - Phase 5: 真の非同期実行（未実装）

詳細は `docs/features/async_await_implementation_status.md` を参照してください。

## 参考ドキュメント

- `docs/features/async_await_design.md` - 設計ドキュメント
- `docs/features/async_await_implementation_status.md` - 実装状況
- `stdlib/std/future.cb` - Future<T>定義

## ライセンス

これらのサンプルコードはCb言語プロジェクトの一部です。
