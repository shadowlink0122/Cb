# タイムアウト機能設計仕様

**バージョン**: v0.12.1  
**ステータス**: ✅ 実装完了（基本機能）

---

## 概要

async関数にタイムアウト機能を追加し、指定時間内に完了しない場合は`Result::Err`を返すようにします。

---

## 構文

### 方式1: timeout関数（stdlib実装）

```cb
use std::async::timeout;

async Result<int, string> compute(int x) {
    sleep(1000);  // 1秒かかる処理
    return Result::Ok(x * 2);
}

async Result<int, string> main() {
    // 500msのタイムアウトで実行
    Result<int, string> result = await timeout(compute(10), 500);
    
    if result.is_err() {
        println("Timeout!");
        return Result::Err("timeout");
    }
    
    println("Result:", result.unwrap());
    return Result::Ok(0);
}
```

### 方式2: with_timeout属性（構文拡張）

```cb
async Result<int, string> compute(int x) with_timeout(500) {
    sleep(1000);  // 1秒かかる処理
    return Result::Ok(x * 2);
}

int main() {
    Result<int, string> result = await compute(10);
    if result.is_err() {
        println("Timeout!");
    }
    return 0;
}
```

---

## 実装方針（方式1: timeout関数）

### Phase 1: stdlibにtimeout関数を追加

```cb
// stdlib/async.cb
namespace std {
namespace async {

// タイムアウト付きでFutureを実行
async Result<T, string> timeout<T>(Future<T> future, int timeout_ms) {
    // タイマーFutureを作成
    Future<void> timer = create_timer(timeout_ms);
    
    // どちらか先に完了した方を使う
    int completed = await race(future, timer);
    
    if completed == 1 {
        // タイマーが先に完了 = タイムアウト
        return Result::Err("Timeout");
    }
    
    // futureが先に完了
    T value = await future;
    return Result::Ok(value);
}

// 2つのFutureを競争させ、先に完了した方のインデックスを返す
async int race<T1, T2>(Future<T1> f1, Future<T2> f2) {
    // event loopで両方を登録し、先に完了した方を返す
    // 実装は内部的にevent_loop APIを使用
}

// タイマーFutureを作成
Future<void> create_timer(int ms) {
    // msミリ秒後に完了するFutureを作成
}

}}
```

### Phase 2: Event Loop拡張

```cpp
// src/backend/interpreter/event_loop/simple_event_loop.h
class SimpleEventLoop {
public:
    // タイマーを登録
    void register_timer(int timeout_ms, std::function<void()> callback);
    
    // 複数のFutureを競争
    int race(std::vector<FutureHandle> futures);
};
```

---

## 使用例

### 例1: 基本的なタイムアウト

```cb
use std::async::timeout;

async int slow_computation(int x) {
    sleep(2000);  // 2秒かかる
    return x * x;
}

async Result<int, string> main() {
    // 1秒でタイムアウト
    Result<int, string> result = await timeout(slow_computation(10), 1000);
    
    match result {
        Result::Ok(value) => {
            println("Success:", value);
        },
        Result::Err(msg) => {
            println("Error:", msg);  // "Timeout"
        }
    }
    
    return Result::Ok(0);
}
```

### 例2: 複数の非同期処理にタイムアウト

```cb
use std::async::timeout;

async int fetch_data(string url) {
    // ネットワークリクエスト（遅い場合がある）
    sleep(rand() % 3000);
    return 42;
}

async Result<int[], string> main() {
    int[] results = [];
    
    // 各リクエストに500msのタイムアウト
    for int i = 0; i < 5; i++ {
        Result<int, string> r = await timeout(fetch_data("url"), 500);
        if r.is_ok() {
            results.push(r.unwrap());
        }
    }
    
    return Result::Ok(results);
}
```

### 例3: ?オペレーターとの組み合わせ

```cb
use std::async::timeout;

async Result<int, string> fetch_with_timeout(string url, int timeout_ms) {
    int data = await timeout(fetch(url), timeout_ms)?;
    return Result::Ok(data);
}

async Result<string, string> main() {
    int data = await fetch_with_timeout("api/data", 1000)?;
    println("Fetched:", data);
    return Result::Ok("done");
}
```

---

## 実装優先度

### v0.12.1（✅ 実装完了）

- [x] 設計仕様の策定
- [x] `sleep()`関数の実装確認
- [x] `create_timer()` builtin関数の追加
- [x] `timeout()` stdlib関数の実装
- [x] テストケースの作成
- [x] ?オペレーターとの組み合わせ

### v0.13.0以降（🔜 予定）

- [ ] `race()` builtin関数の追加
- [ ] `select!`マクロ（複数Futureから選択）
- [ ] `with_timeout`属性構文
- [ ] タイムアウト時のキャンセル処理

---

## テストケース

### test_timeout_basic.cb

```cb
use std::async::timeout;

async int fast_task() {
    sleep(100);
    return 42;
}

async int slow_task() {
    sleep(1000);
    return 99;
}

async Result<int, string> main() {
    // 速いタスクは成功
    Result<int, string> r1 = await timeout(fast_task(), 500);
    assert(r1.is_ok());
    assert(r1.unwrap() == 42);
    
    // 遅いタスクはタイムアウト
    Result<int, string> r2 = await timeout(slow_task(), 500);
    assert(r2.is_err());
    
    println("Timeout test passed");
    return Result::Ok(0);
}
```

### test_timeout_question_operator.cb

```cb
use std::async::timeout;

async Result<int, string> fetch(int id) {
    sleep(100);
    if id < 0 {
        return Result::Err("Invalid ID");
    }
    return Result::Ok(id * 10);
}

async Result<int, string> fetch_with_timeout(int id) {
    int result = await timeout(fetch(id), 500)?;
    return Result::Ok(result);
}

async Result<int, string> main() {
    int data = await fetch_with_timeout(5)?;
    assert(data == 50);
    
    println("Timeout with ? operator test passed");
    return Result::Ok(0);
}
```

---

## 制限事項（v0.12.1）

1. **キャンセル未実装**: タイムアウト後も元の処理は継続
2. **race未実装**: 2つのFutureから先に完了した方を選ぶ機能は未実装
3. **カスタムエラー型**: タイムアウトエラーは常に`string`型

---

## 参考

- Rustの`tokio::time::timeout`
- JavaScriptの`Promise.race`
- Goの`context.WithTimeout`

