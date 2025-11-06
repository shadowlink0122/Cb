# String - 文字列ライブラリ

## 概要

`String`は、Cbのプリミティブ`string`型をラップした拡張文字列ライブラリです。便利なメソッドを提供します。

## インポート

```cb
import stdlib.std.string;
```

## 基本的な使い方

```cb
String str;
str.data = "Hello, World";
str.length = 12;

int len = str.size();              // 12
bool empty = str.is_empty();       // false
string raw = str.get();            // "Hello, World"
```

## メソッド一覧

### 基本操作

| メソッド | 説明 | 戻り値 |
|---------|------|-------|
| `size()` | 文字列の長さ | `int` |
| `len()` | 文字列の長さ（エイリアス） | `int` |
| `is_empty()` | 空文字列か | `bool` |
| `get()` | 生の文字列を取得 | `string` |

### 比較

| メソッド | 説明 | 戻り値 |
|---------|------|-------|
| `equals(String other)` | 等価性チェック | `bool` |
| `equals_str(string other)` | 生文字列と比較 | `bool` |
| `compare(String other)` | 辞書順比較 | `int` |
| `compare_str(string other)` | 生文字列と辞書順比較 | `int` |

### 検索

| メソッド | 説明 | 戻り値 |
|---------|------|-------|
| `index_of(string substring)` | 最初の出現位置 | `int` (-1: 見つからない) |
| `last_index_of(string substring)` | 最後の出現位置 | `int` (-1: 見つからない) |
| `contains(string substring)` | 部分文字列を含むか | `bool` |
| `starts_with(string prefix)` | プレフィックスチェック | `bool` |
| `ends_with(string suffix)` | サフィックスチェック | `bool` |

### 変換

| メソッド | 説明 | 戻り値 | 状態 |
|---------|------|-------|-----|
| `to_upper()` | 大文字変換 | `String` | 🚧 制限あり |
| `to_lower()` | 小文字変換 | `String` | 🚧 制限あり |
| `trim()` | 前後の空白削除 | `String` | ✅ 実装済み |
| `substring(int start, int end)` | 部分文字列 | `String` | ✅ 実装済み |

### 分割・結合

| メソッド | 説明 | 戻り値 | 状態 |
|---------|------|-------|-----|
| `split(string delimiter)` | 文字列分割 | `Vector<String>` | 🚧 制限あり |
| `concat(String other)` | 連結 | `String` | ✅ 実装済み |
| `concat_str(string other)` | 生文字列連結 | `String` | ✅ 実装済み |

### その他

| メソッド | 説明 | 戻り値 |
|---------|------|-------|
| `char_at(int index)` | インデックスの文字 | `int` (ASCII) |
| `print()` | 出力 | `void` |
| `println()` | 出力（改行付き） | `void` |

## 使用例

### 基本的な操作

```cb
import stdlib.std.string;

void main() {
    String msg;
    msg.data = "Hello";
    msg.length = 5;
    
    println("Length: {msg.size()}");
    println("Empty: {msg.is_empty()}");
}
```

### 検索

```cb
import stdlib.std.string;

void main() {
    String text;
    text.data = "The quick brown fox";
    text.length = 19;
    
    int pos = text.index_of("quick");        // 4
    bool has = text.contains("fox");         // true
    bool starts = text.starts_with("The");   // true
}
```

### 比較

```cb
import stdlib.std.string;

void main() {
    String s1;
    s1.data = "apple";
    s1.length = 5;
    
    String s2;
    s2.data = "banana";
    s2.length = 6;
    
    int cmp = s1.compare(s2);  // 負の値（apple < banana）
    bool eq = s1.equals_str("apple");  // true
}
```

### 連結

```cb
import stdlib.std.string;

void main() {
    String hello;
    hello.data = "Hello";
    hello.length = 5;
    
    String world;
    world.data = " World";
    world.length = 6;
    
    String result = hello.concat(world);
    result.println();  // "Hello World"
}
```

## パフォーマンス

| 操作 | 時間計算量 |
|-----|-----------|
| `size`, `is_empty`, `get` | O(1) |
| `equals`, `compare` | O(n) |
| `index_of`, `contains` | O(n×m) |
| `substring` | O(n) |
| `concat` | O(n+m) |

## 制限事項

### インタプリタ制限

⚠️ 一部のメソッドはインタプリタの制約により動作が制限されています：

1. **`to_upper()` / `to_lower()`**: ASCII範囲外の文字は変換されません
2. **`split()`**: デリミタが1文字のみサポート
3. **動的メモリ**: 一部の操作で予期しない動作が発生する可能性があります

### 詳細情報

実装状況の詳細は以下を参照してください：
- [docs/features/string_library_status.md](/docs/features/string_library_status.md)
- [docs/features/string_interpolation.md](/docs/features/string_interpolation.md)

## ベストプラクティス

### ✅ 推奨

```cb
// 明示的な長さ設定
String s;
s.data = "test";
s.length = 4;

// 検索前のチェック
if (!s.is_empty() && s.contains("es")) {
    // ...
}
```

### ❌ 非推奨

```cb
// 長さ未設定（未定義動作）
String s;
s.data = "test";
// s.length未設定

// 範囲外アクセス
int ch = s.char_at(100);  // 範囲チェックなし
```

## ユーティリティ関数

### グローバル関数（stdlib.std.string）

```cb
// 文字列長取得
int strlen(string s);

// 文字列比較
int strcmp(string s1, string s2);

// 文字列コピー
void strcpy(string dest, string src);
```

## テスト

包括的なテストは以下にあります：
- `tests/cases/stdlib/string/`

## 関連項目

- [Vector](./vector.md)
- [文字列補間](../../features/string_interpolation.md)
