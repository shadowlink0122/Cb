# Time構造体 - 時刻の比較と操作

## 概要

stdlib/std/time.cbに**Time構造体**と**TimeOpsインターフェース**を追加しました。

## 主な機能

### 1. Time構造体

```cb
struct Time {
    long timestamp_ms;  // エポックからのミリ秒
};
```

### 2. TimeOpsインターフェース

Time構造体に以下のメソッドを提供:

#### 比較メソッド
- `bool is_before(Time other)` - この時刻が他の時刻より前か
- `bool is_after(Time other)` - この時刻が他の時刻より後か
- `bool equals(Time other)` - この時刻が他の時刻と等しいか

#### 差分計算
- `long diff(Time other)` - 差分をミリ秒で取得（負の値もあり得る）
- `Duration diff_duration(Time other)` - 差分をDuration構造体で取得（絶対値）

#### 時刻の加算
- `Time add_milliseconds(long ms)` - ミリ秒を加算
- `Time add_seconds(int seconds)` - 秒を加算
- `Time add_minutes(int minutes)` - 分を加算
- `Time add_hours(int hours)` - 時を加算

#### 表示
- `void format()` - 時刻をミリ秒として表示

### 3. ユーティリティ関数

- `Time create_time(long timestamp_ms)` - Time構造体を作成
- `int compare_times(Time t1, Time t2)` - 2つの時刻を比較 (-1, 0, 1)
- `Duration elapsed_time(Time start, Time end)` - 経過時間を取得

## 使用例

```cb
import stdlib.std.time;

void main() {
    // 時刻の作成
    Time t1 = create_time(1000);   // 1秒
    Time t2 = create_time(5000);   // 5秒
    
    // 比較
    if (t1.is_before(t2)) {
        println("t1はt2より前です");
    }
    
    // 差分計算
    long diff_ms = t2.diff(t1);  // 4000ms
    Duration elapsed = t2.diff_duration(t1);
    format_duration(elapsed);  // "4s 0ms"
    
    // 時刻の加算
    Time future = t1.add_seconds(10);  // t1 + 10秒
    
    // ストップウォッチ風の使い方
    Time start = create_time(0);
    sleep_seconds(2);
    Time checkpoint = create_time(2000);
    
    Duration lap = elapsed_time(start, checkpoint);
    format_duration(lap);  // "2s 0ms"
}
```

## 設計の特徴

### なぜインターフェースを使うのか？

Cbでは `impl Struct {}` にはコンストラクタ、デストラクタ、静的変数しか含められません。通常のメソッドを追加するには、以下のいずれかが必要です:

1. **インターフェースを定義して実装** (採用)
   ```cb
   interface TimeOps {
       bool is_before(Time other);
       // ...
   }
   
   impl TimeOps for Time {
       bool is_before(Time other) {
           return self.timestamp_ms < other.timestamp_ms;
       }
   }
   ```

2. **通常の関数として定義**
   ```cb
   bool time_is_before(Time t1, Time t2) {
       return t1.timestamp_ms < t2.timestamp_ms;
   }
   ```

インターフェース方式を採用した理由:
- メソッドチェーンが使える: `t1.is_before(t2).and_then(...)`
- オブジェクト指向的なAPI
- 他のstdlib (Vector, Map, String)と一貫性がある

### 演算子オーバーロードについて

ご提案の演算子オーバーロード構文:
```cb
impl for Time {
    + operator (Time t) { ... }
    - operator (Time t) { ... }
}
```

これは現在のCbではサポートされていません。将来的な機能として検討する価値はありますが、現時点では:

**代替案1: メソッド名で明示**
```cb
Time result = t1.add(t2);     // t1 + t2
Time result = t1.subtract(t2); // t1 - t2
```

**代替案2: グローバル関数**
```cb
Time result = time_add(t1, t2);
Time result = time_subtract(t1, t2);
```

現在の実装では、`add_seconds()`, `add_minutes()` などの明示的なメソッド名を使用しています。

## テスト結果

`sample/time_comparison_demo.cb` で以下をテスト:

✅ Time構造体の作成
✅ 時刻の比較 (is_before, is_after, equals)
✅ 時刻の差分 (diff, diff_duration)
✅ 時刻の加算 (add_milliseconds, add_seconds, add_minutes, add_hours)
✅ ユーティリティ関数 (compare_times, elapsed_time)
✅ ストップウォッチ風の実用例

## 将来の拡張

1. **現在時刻の取得**
   ```cb
   Time now = get_current_time();  // ビルトイン関数として実装予定
   ```

2. **タイムゾーン対応**
   ```cb
   Time utc = create_time_utc(timestamp_ms);
   Time jst = create_time_jst(timestamp_ms);
   ```

3. **日時フォーマット**
   ```cb
   string formatted = t.format("YYYY-MM-DD HH:mm:ss");
   ```

4. **演算子オーバーロード** (言語機能として)
   ```cb
   Time sum = t1 + t2;
   Time diff = t2 - t1;
   bool cmp = t1 < t2;
   ```
