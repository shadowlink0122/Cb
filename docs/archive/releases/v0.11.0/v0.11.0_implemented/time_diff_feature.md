# Time差分表示機能

## 概要

stdlib/std/time に、2つの時刻の差分を人間が読みやすい形式で表示する機能を追加しました。

## 追加された機能

### 1. TimeDiff構造体

2つの時刻の差分を表現する構造体。

```cb
export struct TimeDiff {
    int days;           // 日数
    int hours;          // 時間
    int minutes;        // 分
    int seconds;        // 秒
    int milliseconds;   // ミリ秒
    bool is_negative;   // 負の差分かどうか
};
```

### 2. calculate_time_diff - 時刻差分の計算

```cb
export TimeDiff calculate_time_diff(Time time1, Time time2);
```

2つの時刻の差分を計算します（time1 - time2）。

**パラメータ:**
- `time1`: 時刻1
- `time2`: 時刻2

**戻り値:**
- `TimeDiff`: 時刻差分（絶対値と符号）

**使用例:**
```cb
Time t1 = get_current_time();
sleep(5000);
Time t2 = get_current_time();

TimeDiff diff = calculate_time_diff(t2, t1);
println("日数: {diff.days}");
println("時間: {diff.hours}");
println("分: {diff.minutes}");
println("秒: {diff.seconds}");
println("ミリ秒: {diff.milliseconds}");
```

### 3. format_time_diff - 詳細形式で表示

```cb
export void format_time_diff(TimeDiff diff);
```

TimeDiffを詳細形式で表示します。

**出力例:**
- `1日 2時間 30分 15秒 500ミリ秒`
- `-3秒 524ミリ秒`
- `2秒 351ミリ秒`

### 4. format_time_diff_compact - コンパクト形式で表示

```cb
export void format_time_diff_compact(TimeDiff diff);
```

TimeDiffをコンパクト形式で表示します。

**出力形式:** `±[DD日 ]HH:MM:SS[.mmm]`

**出力例:**
- `+1日 02:30:15.500`
- `-00:00:03.524`
- `+00:00:02.351`

### 5. print_time_diff - 2つの時刻の差分を詳細形式で表示

```cb
export void print_time_diff(Time time1, Time time2);
```

2つの時刻の差分を計算して詳細形式で表示します。

**使用例:**
```cb
Time start = get_current_time();
sleep(3500);
Time end = get_current_time();

print("差分: ");
print_time_diff(end, start);
// 出力: 差分: 3秒 524ミリ秒
```

### 6. print_time_diff_compact - 2つの時刻の差分をコンパクト形式で表示

```cb
export void print_time_diff_compact(Time time1, Time time2);
```

2つの時刻の差分を計算してコンパクト形式で表示します。

**使用例:**
```cb
Time start = get_current_time();
sleep(3500);
Time end = get_current_time();

print("差分: ");
print_time_diff_compact(end, start);
// 出力: 差分: +00:00:03.524
```

### 7. get_time_diff_string - 文章形式で表示

```cb
export void get_time_diff_string(Time time1, Time time2);
```

2つの時刻の差分を文章形式で表示します。

**使用例:**
```cb
Time start = get_current_time();
sleep(3500);
Time end = get_current_time();

get_time_diff_string(end, start);
// 出力: time1はtime2より 3秒524ミリ秒 後です

get_time_diff_string(start, end);
// 出力: time1はtime2より 3秒524ミリ秒 前です
```

## タイムゾーンオフセット

### マイナスのオフセットのサポート

`utc_offset_hours`パラメータは、プラス・マイナス両方のオフセットをサポートしています。

**使用例:**

```cb
import stdlib.std.time;

void main() {
    Time current = get_current_time();
    
    // プラスのオフセット
    print("JST (UTC+9): ");
    convert_time(current, 9);
    
    print("CET (UTC+1): ");
    convert_time(current, 1);
    
    // ゼロ（UTC）
    print("UTC (UTC+0): ");
    convert_time(current, 0);
    
    // マイナスのオフセット
    print("EST (UTC-5): ");
    convert_time(current, -5);
    
    print("PST (UTC-8): ");
    convert_time(current, -8);
    
    print("HST (UTC-10): ");
    convert_time(current, -10);
}
```

**出力例:**
```
JST (UTC+9): 2025/11/08 09:19:29
CET (UTC+1): 2025/11/08 01:19:29
UTC (UTC+0): 2025/11/08 00:19:29
EST (UTC-5): 2025/11/07 19:19:29
PST (UTC-8): 2025/11/07 16:19:29
HST (UTC-10): 2025/11/07 14:19:29
```

## 実用例

### 処理時間の測定

```cb
import stdlib.std.time;

void main() {
    Time start = get_current_time();
    
    // 何か処理
    for (int i = 0; i < 1000000; i = i + 1) {
        // 処理内容
    }
    
    Time end = get_current_time();
    
    print("処理時間: ");
    print_time_diff(end, start);
}
```

### タイムアウト判定

```cb
import stdlib.std.time;

void main() {
    Time start = get_current_time();
    int timeout_seconds = 10;
    
    while (true) {
        Time current = get_current_time();
        TimeDiff diff = calculate_time_diff(current, start);
        
        if (diff.seconds >= timeout_seconds) {
            println("タイムアウト!");
            break;
        }
        
        // 処理を続ける
        sleep(100);
    }
}
```

### 日時の比較

```cb
import stdlib.std.time;

void main() {
    Time meeting_time = create_time(1735660800000);  // 2025/1/1 0:00:00 UTC
    Time current = get_current_time();
    
    TimeDiff diff = calculate_time_diff(meeting_time, current);
    
    if (diff.is_negative) {
        print("会議は");
        print_time_diff(current, meeting_time);
        println("前に終わりました");
    } else {
        print("会議まであと");
        print_time_diff(meeting_time, current);
        println("です");
    }
}
```

### 長時間の差分表示

```cb
import stdlib.std.time;

void main() {
    // 2000年1月1日 00:00:00 UTC
    Time millennium = create_time(946684800000);
    
    // 2024年1月1日 00:00:00 UTC
    Time year_2024 = create_time(1704067200000);
    
    println("2000年から2024年までの期間:");
    print_time_diff(year_2024, millennium);
    // 出力: 8766日 0時間 0分 0秒 0ミリ秒
    
    print_time_diff_compact(year_2024, millennium);
    // 出力: +8766日 00:00:00
}
```

## テストプログラム

### sample/time_diff_demo.cb

包括的なTime差分機能のデモ。

```bash
./main sample/time_diff_demo.cb
```

**テスト内容:**
1. 基本的な時刻差分（3.5秒）
2. 負の差分（過去との比較）
3. 長時間の差分（1日2時間30分15秒500ミリ秒）
4. TimeDiff構造体による条件分岐
5. マイナスのタイムゾーンオフセット
6. 実用的な使用例（処理時間測定）

### sample/time_diff_simple_test.cb

シンプルなTime差分テスト。

```bash
./main sample/time_diff_simple_test.cb
```

**テスト内容:**
- 基本的な差分（1.5秒）
- add_millisecondsの動作確認（5秒）
- 大きな値での差分（1日2時間30分15秒500ミリ秒）

## まとめ

stdlib/std/timeに追加された機能:

### ✅ Time差分表示機能
1. **TimeDiff構造体** - 日/時間/分/秒/ミリ秒と符号を表現
2. **calculate_time_diff()** - 2つの時刻の差分を計算
3. **format_time_diff()** - 詳細形式で表示（"1日 2時間 30分 15秒 500ミリ秒"）
4. **format_time_diff_compact()** - コンパクト形式で表示（"+1日 02:30:15.500"）
5. **print_time_diff()** - 直接差分を表示（詳細形式）
6. **print_time_diff_compact()** - 直接差分を表示（コンパクト形式）
7. **get_time_diff_string()** - 文章形式で表示

### ✅ タイムゾーンオフセット
- **プラスのオフセット**: JST (+9), CET (+1)
- **ゼロ（UTC）**: UTC (0)
- **マイナスのオフセット**: EST (-5), PST (-8), HST (-10)

すべての機能が正常に動作し、テストで検証済みです。
