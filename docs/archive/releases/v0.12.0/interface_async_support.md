# Interface Async Support (v0.13.0 Phase 2.0)

## 概要

v0.13.0 Phase 2.0で、interfaceメソッドに`async`修飾子を使用できるようになりました。これにより、interfaceで非同期APIを定義し、実装側でもasync関数を使用できます。

## 機能

### 1. interfaceでのasync宣言

```cb
interface DataProcessor {
    async int process(int data);
    async bool validate(int data);
}
```

### 2. implでのasync実装

```cb
impl DataProcessor for SimpleProcessor {
    async int process(int data) {
        await sleep(100);
        return data * 3;
    }
    
    async bool validate(int data) {
        await sleep(50);
        return data > 0;
    }
}
```

### 3. 使用例

```cb
void main() {
    SimpleProcessor processor;
    
    // asyncメソッドはFuture<T>を返す
    Future<int> result_fut = processor.process(42);
    Future<bool> valid_fut = processor.validate(10);
    
    // awaitで結果を取得
    int result = await result_fut;
    bool is_valid = await valid_fut;
    
    println("結果: {result}, 有効: {is_valid}");
}
```

## 実装の詳細

### パーサー (interface_parser.cpp)

1. **interfaceメソッド宣言**: `async`キーワードを検出し、`InterfaceMember::is_async`フラグを設定
2. **implメソッド実装**: `async`キーワードを検出し、`ASTNode::is_async_function`フラグを設定
3. **型チェック**: interfaceとimplの`async`修飾子が一致するかを検証

### インタプリタ (call_impl.cpp)

1. **レシーバー型の判定**: メソッド呼び出し時、レシーバーが`TYPE_INTERFACE`かをチェック
2. **interface定義の参照**: interface名から`InterfaceDefinition`を取得
3. **asyncフラグの取得**: メソッド名で`InterfaceMember`を検索し、`is_async`を確認
4. **Future生成**: `is_async == true`の場合、EventLoopにタスクを登録し、`Future<T>`を返す

## 型チェック

interface定義と実装の`async`修飾子が一致しない場合、コンパイル時エラーが発生します：

```
error: Method signature mismatch: async modifier mismatch for method 'process'. 
Interface declares async but implementation is non-async
```

または

```
error: Method signature mismatch: async modifier mismatch for method 'process'. 
Interface declares non-async but implementation is async
```

## 制限事項

1. **ジェネリックとの組み合わせ**: ジェネリックinterface + asyncメソッドは未テスト
2. **デフォルト実装**: interfaceでのデフォルト実装はサポートされていない
3. **EventLoop依存**: async関数は`SimpleEventLoop`に依存（シングルスレッド）

## 使用例

完全な例は `sample/async/async_interface_impl_demo.cb` を参照してください。

## 今後の拡張

- [ ] ジェネリックinterface + asyncメソッドの完全サポート
- [ ] interface内でのデフォルトasync実装
- [ ] マルチスレッド対応EventLoop
- [ ] async trait bounds (例: `interface Processor: async`)

## リリース情報

- **導入バージョン**: v0.13.0 Phase 2.0
- **関連機能**: async/await (v0.12.0), interface/impl (v0.11.0)
- **テストファイル**: `sample/async/async_interface_impl_demo.cb`
