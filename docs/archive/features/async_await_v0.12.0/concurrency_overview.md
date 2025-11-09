# Cb言語 並行処理システム 概要

**作成日**: 2025年11月7日

---

## ドキュメント構成

Cb言語の並行処理システムは、2つの独立したサブシステムで構成されます：

1. **[async/await 設計](./async_await_design.md)** - 協調的マルチタスク
2. **[Coroutine 設計](./coroutine_design.md)** - OSスレッドベース並列実行

---

## 概要比較

| 特徴 | async/await | Coroutine |
|------|-------------|-----------|
| **参考言語** | TypeScript, Rust (Tokio) | Go (goroutine) |
| **実行モデル** | 協調的（明示的yield） | プリエンプティブ（OS管理） |
| **スレッド** | シングルスレッド | マルチスレッド |
| **並列性** | 並行（concurrent） | 並列（parallel） |
| **用途** | I/O待ち、UI応答性 | CPU集約的処理 |
| **オーバーヘッド** | 低い | 高い |
| **実装バージョン** | v0.12.0 (Phase 1) | v0.14.0 (予定) |

---

## async/await (協調的マルチタスク)

### 設計方針

- **TypeScript風のAPI**: シンプルで直感的
- **Rust風の内部実装**: Future/Poll機構（簡易版）
- **段階的実装**:
  - **Phase 1 (v0.12.0)**: 即座実行モデル（yieldなし、順次実行）
  - **Phase 2 (v0.13.0以降)**: yieldサポート、真の協調的マルチタスク

### Phase 1 使用例

```cb
async Future<int> fetch_data(int id) {
    println("Fetching {id}...");
    return id * 100;
}

void main() {
    Future<int> f1 = fetch_data(1);  // 即座に実行完了
    Future<int> f2 = fetch_data(2);  // 即座に実行完了
    
    int r1 = await f1;  // 値を取得（既に完了済み）
    int r2 = await f2;  // 値を取得
    
    println("Results: {r1}, {r2}");
}
```

### デバッグトレース

```bash
./main --debug test.cb
```

出力:
```
[ASYNC] Entering async function: fetch_data(id=1)
Fetching 1...
[ASYNC] Returning from async function: fetch_data -> Future{is_ready=true}
[AWAIT] Awaiting Future (already ready)
[AWAIT] Extracted value: 100
```

---

## Coroutine (OSスレッドベース並列実行)

### 設計方針

- **Go言語goroutine風のAPI**: `go func()`で起動
- **std::threadによる実装**: 真の並列実行
- **同期プリミティブ**: Channel, Mutex

### 使用例

```cb
void worker(int id, Channel<int> ch) {
    for (int i = 0; i < 5; i = i + 1) {
        ch.send(id * 10 + i);
    }
}

void main() {
    Channel<int> ch = Channel<int>(10);
    
    go worker(1, ch);  // Coroutine起動
    go worker(2, ch);
    
    for (int i = 0; i < 10; i = i + 1) {
        int value = ch.recv();
        println("Received: {value}");
    }
}
```

### デバッグトレース

```bash
./main --debug test.cb
```

出力:
```
[COROUTINE] Launching coroutine #1: worker(id=1)
[COROUTINE] Thread #1 started
[CHANNEL] Send: value=10 (buffer: 1/10)
[CHANNEL] Recv: value=10 (buffer: 0/10)
Received: 10
```

---

## 使い分けガイド

### async/await を使うべき場合

- ✅ I/O待ち（ファイル、ネットワーク）
- ✅ UI応答性の向上
- ✅ 軽量な並行処理
- ✅ 多数の小さなタスク
- ✅ シングルスレッドで十分な処理

### Coroutine を使うべき場合

- ✅ CPU集約的な計算
- ✅ 真の並列実行が必要
- ✅ マルチコアCPUを活用したい
- ✅ 長時間実行されるタスク
- ✅ OSレベルの並列性が必要

### 組み合わせの例

```cb
// CPU集約的な処理はCoroutineで
void heavy_computation(int id, Channel<int> results) {
    int sum = 0;
    for (int i = 0; i < 1000000; i = i + 1) {
        sum = sum + i;
    }
    results.send(sum);
}

// I/O処理はasync/awaitで
async Future<void> save_result(int result) {
    // ファイルに書き込み（将来のI/Oライブラリ）
    println("Saving result: {result}");
}

void main() {
    Channel<int> ch = Channel<int>(10);
    
    // 複数のCPU集約タスクを並列実行
    go heavy_computation(1, ch);
    go heavy_computation(2, ch);
    
    // 結果をasync/awaitで処理
    for (int i = 0; i < 2; i = i + 1) {
        int result = ch.recv();
        Future<void> f = save_result(result);
        await f;
    }
}
```

---

## 実装ロードマップ

### v0.12.0 (現在)
- [x] async/await Phase 1 設計完了
- [ ] async/await Phase 1 実装
  - [ ] async関数の即座実行
  - [ ] awaitの簡易評価
  - [ ] デバッグトレース

### v0.13.0
- [ ] async/await Phase 2
  - [ ] yieldサポート
  - [ ] バイトコードVMまたはCPS変換
  - [ ] イベントループ

### v0.14.0
- [ ] Coroutine Phase 1
  - [ ] go文の実装
  - [ ] スレッド管理
  - [ ] デバッグトレース

### v0.14.1
- [ ] Coroutine Phase 2
  - [ ] Channel実装
  - [ ] Mutex実装

### v0.15.0
- [ ] 高度な機能
  - [ ] select文
  - [ ] タイムアウト
  - [ ] キャンセル機能

---

## デバッグフラグ

全ての並行処理機能は `--debug` フラグで詳細なトレースが出力されます：

```bash
# async/awaitのトレース
./main --debug async_test.cb

# Coroutineのトレース
./main --debug coroutine_test.cb
```

### デバッグ出力プレフィックス

| プレフィックス | システム | 内容 |
|--------------|---------|------|
| `[ASYNC]` | async/await | 関数の開始・終了 |
| `[AWAIT]` | async/await | await式の評価 |
| `[YIELD]` | async/await (Phase 2) | 実行の中断 |
| `[RESUME]` | async/await (Phase 2) | 実行の再開 |
| `[COROUTINE]` | Coroutine | スレッドの管理 |
| `[CHANNEL]` | Coroutine | Channel操作 |
| `[MUTEX]` | Coroutine | Mutex操作 |

---

## まとめ

Cb言語の並行処理システムは、モダンな言語（TypeScript, Rust, Go）のベストプラクティスを取り入れつつ、段階的に実装される設計です。

- **Phase 1**: シンプルで確実な基礎実装
- **Phase 2以降**: 高度な機能の追加

詳細は各設計ドキュメントを参照してください：
- [async/await 設計](./async_await_design.md)
- [Coroutine 設計](./coroutine_design.md)
