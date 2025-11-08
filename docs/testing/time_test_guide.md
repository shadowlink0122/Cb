# Time測定テストの作成方法

## 概要

`stdlib/std/time.cb`を使用した時間測定テストの作成方法を説明します。

## 基本的なテストパターン

### 1. 単純な精度測定

```cb
import stdlib.std.time;

void test_sleep_accuracy() {
    Stopwatch sw;
    sw.start();
    sleep(1000);
    sw.stop();
    
    long actual = sw.elapsed_ms();
    long expected = 1000;
    long diff = actual - expected;
    
    println("実測: {actual}ms, 差分: {diff}ms");
}
```

### 2. 複数回実行の統計測定

```cb
void test_multiple_runs() {
    long[] times = [0, 0, 0, 0, 0];
    
    int i = 0;
    while (i < 5) {
        Stopwatch sw;
        sw.start();
        sleep(500);
        sw.stop();
        
        times[i] = sw.elapsed_ms();
        i = i + 1;
    }
    
    // 統計計算
    long total = 0;
    long min_time = times[0];
    long max_time = times[0];
    
    i = 0;
    while (i < 5) {
        total = total + times[i];
        if (times[i] < min_time) min_time = times[i];
        if (times[i] > max_time) max_time = times[i];
        i = i + 1;
    }
    
    long avg = total / 5;
    long range = max_time - min_time;
    
    println("平均: {avg}ms, 範囲: {range}ms");
}
```

### 3. 非同期関数のテスト

```cb
async void async_task(int id, int duration) {
    long start = now();
    println("[タスク{id}] 開始");
    
    sleep(duration);
    
    long end = now();
    println("[タスク{id}] 完了 ({end - start}ms)");
}

void test_async_concurrent() {
    println("10個のタスクを起動");
    
    int i = 0;
    while (i < 10) {
        async_task(i + 1, 500);
        i = i + 1;
    }
    
    // タスク完了を待つ
    sleep(3000);
}
```

### 4. Futureを使った結果待機

```cb
async int calculate(int value) {
    sleep(500);
    return value * 2;
}

async void test_with_future() {
    // 複数のFutureを起動
    Future<int> f1 = calculate(10);
    Future<int> f2 = calculate(20);
    Future<int> f3 = calculate(30);
    
    // 結果を待つ
    int r1 = await f1;
    int r2 = await f2;
    int r3 = await f3;
    
    println("結果: {r1}, {r2}, {r3}");
}
```

## テスト項目の例

### 精度測定テスト

- 単一sleep実行の精度
- 連続sleep実行の安定性
- 異なる時間のsleep比較
- 累積誤差の測定

### 非同期実行テスト

- ループ内でのasync関数起動
- 異なる時間のタスク並行実行
- Future結果の取得
- タスク完了時間の測定

## 推奨されるテスト構成

```cb
import stdlib.std.time;

void test_case_1() {
    println("=== テストケース1 ===");
    // テストロジック
}

void test_case_2() {
    println("=== テストケース2 ===");
    // テストロジック
}

void main() {
    Stopwatch suite_timer;
    suite_timer.start();
    
    test_case_1();
    test_case_2();
    
    suite_timer.stop();
    print("総実行時間: ");
    suite_timer.print_elapsed();
}
```

## 注意事項

- sleep()は実装の都合上、指定時間より数ms長くかかることがある
- 500ms以上のsleepでは誤差率1%以下が期待される
- 短時間(10ms以下)のsleepは精度が低い
- 非同期関数はfire-and-forgetで起動され、awaitで結果を待つ
