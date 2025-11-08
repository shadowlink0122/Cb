# UTC時間変換機能

## 概要

stdlib/std/time に、UTCタイムゾーンオフセットを使用した日時変換機能を追加しました。

## 追加された機能

### 1. DateTime構造体

人間が読める形式の日時を表現する構造体。

```cb
export struct DateTime {
    int year;    // 西暦年
    int month;   // 月 (1-12)
    int day;     // 日 (1-31)
    int hour;    // 時 (0-23)
    int minute;  // 分 (0-59)
    int second;  // 秒 (0-59)
};
```

### 2. タイムスタンプ変換関数

```cb
export DateTime timestamp_to_datetime(long timestamp_ms, int utc_offset_hours);
```

エポックタイムスタンプ（1970年1月1日 00:00:00 UTCからのミリ秒）を、
指定したタイムゾーンオフセットを適用したDateTime構造体に変換します。

**パラメータ:**
- `timestamp_ms`: エポックからのミリ秒
- `utc_offset_hours`: UTCからのオフセット時間（例: 日本は+9）

**戻り値:**
- `DateTime`: 変換された日時

**使用例:**
```cb
import stdlib.std.time;

void main() {
    long current = now();
    
    // JST (日本標準時: UTC+9)
    DateTime jst = timestamp_to_datetime(current, 9);
    println("年: {jst.year}, 月: {jst.month}, 日: {jst.day}");
    println("時: {jst.hour}, 分: {jst.minute}, 秒: {jst.second}");
}
```

### 3. convert関数（簡易表示）

```cb
export void convert(int utc_offset_hours, long timestamp_ms);
```

指定したタイムスタンプを日時形式に変換して表示します。
`timestamp_ms`が0の場合は現在時刻を使用します。

**パラメータ:**
- `utc_offset_hours`: UTCからのオフセット時間
- `timestamp_ms`: エポックからのミリ秒（0の場合は現在時刻）

**出力形式:**
- `YYYY/MM/DD HH:MM:SS` （ゼロ埋めあり）

**使用例:**
```cb
import stdlib.std.time;

void main() {
    long n = now();
    
    // 日本標準時 (JST: UTC+9)
    print("JST: ");
    convert(9, n);
    // 出力例: JST: 2025/11/08 08:52:03
    
    // 協定世界時 (UTC)
    print("UTC: ");
    convert(0, n);
    // 出力例: UTC: 2025/11/07 23:52:03
}
```

### 4. 日時フォーマット関数

#### format_datetime_padded（ゼロ埋めあり）

```cb
export void format_datetime_padded(DateTime dt);
```

DateTimeを `YYYY/MM/DD HH:MM:SS` 形式で表示（1桁の数値は0埋め）。

**使用例:**
```cb
DateTime dt = timestamp_to_datetime(now(), 9);
format_datetime_padded(dt);
// 出力: 2025/11/08 08:52:03
```

#### format_datetime（ゼロ埋めなし）

```cb
export void format_datetime(DateTime dt);
```

DateTimeを `YYYY/M/D H:M:S` 形式で表示（ゼロ埋めなし）。

**使用例:**
```cb
DateTime dt = timestamp_to_datetime(now(), 9);
format_datetime(dt);
// 出力: 2025/11/8 8:52:3
```

### 5. ヘルパー関数

#### is_leap_year

```cb
export bool is_leap_year(int year);
```

閏年判定を行います。

**戻り値:**
- `true`: 閏年
- `false`: 平年

**使用例:**
```cb
if (is_leap_year(2024)) {
    println("2024年は閏年です");
}
```

#### days_in_month

```cb
export int days_in_month(int year, int month);
```

指定した年月の日数を取得します（閏年を考慮）。

**戻り値:**
- その月の日数（28-31）

**使用例:**
```cb
int days = days_in_month(2024, 2);
println("2024年2月の日数: {days}");  // 29日
```

## タイムゾーンオフセット例

| タイムゾーン | オフセット | 使用例 |
|-------------|-----------|--------|
| JST（日本標準時） | +9 | `convert(9, now())` |
| UTC（協定世界時） | 0 | `convert(0, now())` |
| CET（中央ヨーロッパ時間） | +1 | `convert(1, now())` |
| EST（米国東部標準時） | -5 | `convert(-5, now())` |
| PST（米国太平洋標準時） | -8 | `convert(-8, now())` |
| IST（インド標準時）* | +5 | `convert(5, now())` |

*注: 現在は整数時間のオフセットのみサポート。インド標準時（UTC+5:30）は+5として扱われます。

## 実装の詳細

### 日時変換アルゴリズム

1. **タイムゾーンオフセット適用**: `timestamp_ms + (utc_offset_hours * 3600000)`
2. **日数計算**: タイムスタンプをミリ秒から日数に変換
3. **年の計算**: 1970年から、閏年を考慮しながら年を進める
4. **月の計算**: 各月の日数（閏年を考慮）を使用して月を計算
5. **日の計算**: 残りの日数を日として設定
6. **時刻の計算**: 残りのミリ秒から時、分、秒を計算

### 閏年判定

- 400で割り切れる年 → 閏年
- 100で割り切れる年 → 平年
- 4で割り切れる年 → 閏年
- それ以外 → 平年

### 月の日数

- 1, 3, 5, 7, 8, 10, 12月: 31日
- 4, 6, 9, 11月: 30日
- 2月: 平年は28日、閏年は29日

## テストプログラム

### sample/time_convert_demo.cb

基本的な使用例を示すデモプログラム。

```bash
./main sample/time_convert_demo.cb
```

### sample/time_timezone_conversion.cb

包括的なテストスイート。以下をテストします:
- 現在時刻の複数タイムゾーン表示
- 特定のタイムスタンプの変換
- DateTime構造体の個別フィールド
- 閏年の検証（2000年、2024年）
- エッジケース（エポック時刻、日付の遷移）
- フォーマットのバリエーション

```bash
./main sample/time_timezone_conversion.cb
```

## 使用例

### 基本的な使用

```cb
import stdlib.std.time;

void main() {
    long n = now();
    
    // 日本標準時で現在時刻を表示
    print("現在時刻 (JST): ");
    convert(9, n);
}
```

### DateTime構造体を使った詳細な操作

```cb
import stdlib.std.time;

void main() {
    long current = now();
    DateTime dt = timestamp_to_datetime(current, 9);
    
    // 個別のフィールドにアクセス
    println("年: {dt.year}");
    println("月: {dt.month}");
    println("日: {dt.day}");
    println("時: {dt.hour}");
    println("分: {dt.minute}");
    println("秒: {dt.second}");
    
    // 条件分岐
    if (dt.hour < 12) {
        println("午前です");
    } else {
        println("午後です");
    }
}
```

### 複数のタイムゾーンで表示

```cb
import stdlib.std.time;

void main() {
    long n = now();
    
    println("世界の現在時刻:");
    
    print("東京 (JST): ");
    convert(9, n);
    
    print("ロンドン (GMT): ");
    convert(0, n);
    
    print("ニューヨーク (EST): ");
    convert(-5, n);
    
    print("ロサンゼルス (PST): ");
    convert(-8, n);
}
```

### 特定の日時を変換

```cb
import stdlib.std.time;

void main() {
    // 2000年1月1日 00:00:00 UTC
    long millennium = 946684800000;
    
    println("ミレニアム:");
    print("  UTC: ");
    convert(0, millennium);
    print("  JST: ");
    convert(9, millennium);
}
```

## まとめ

stdlib/std/timeに追加された日時変換機能により、以下が可能になりました:

1. ✅ エポックタイムスタンプを人間が読める日時形式に変換
2. ✅ 任意のタイムゾーンオフセットの適用
3. ✅ 閏年を含む正確な日付計算
4. ✅ "西暦/月/日 時間:分:秒" 形式での表示
5. ✅ 個別の日時フィールドへのアクセス

これにより、ユーザーのリクエスト「time.convert(9, n)で、+9時間の時刻を"西暦/月/日 時間:分:秒"の形式で知りたい」が実現されました。
