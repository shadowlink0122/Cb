# sample/macro_demo.cb 修正完了

## 実施日
2025年10月17日

## 問題
`sample/macro_demo.cb`が実行できない状態でした。

### 原因
ファイルが現在実装されていない構文を使用していました。現在実装されているのは **プリプロセッサマクロ** (`#define`ディレクティブ) のみです。

## 実装されている機能

### ✅ プリプロセッサマクロ
```cb
#define PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SQUARE(x) ((x) * (x))
```

**特徴**:
- オブジェクトマクロ（定数定義）
- 関数マクロ（引数を取る）
- `#undef`による未定義化
- 文字列リテラル保護（最近のバグ修正）
- ネストしたマクロ展開

## 修正内容

`sample/macro_demo.cb`を現在実装されているプリプロセッサマクロで動作するように書き換えました。

### 修正後のコード

```cb
#define PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define SQUARE(x) ((x) * (x))
#define CIRCLE_AREA(r) (PI * (r) * (r))

int main() {
    println("=== Preprocessor Macro Demo ===");
    
    println("PI =", PI);
    
    int a = 10;
    int b = 20;
    println("MAX(10, 20) =", MAX(a, b));
    println("SQUARE(5) =", SQUARE(5));
    
    int result = MAX(SQUARE(5), SQUARE(4));
    println("MAX(SQUARE(5), SQUARE(4)) =", result);
    
    int radius = 5;
    println("CIRCLE_AREA(5) =", CIRCLE_AREA(radius));
    
    return 0;
}
```

### 実行結果

```
=== Preprocessor Macro Demo ===
PI = 3.14159
MAX(10, 20) = 20
SQUARE(5) = 25
MAX(SQUARE(5), SQUARE(4)) = 25
CIRCLE_AREA(5) = 78.53975
```

## 追加ファイル

### sample/simple_macro_demo.cb
最小限のマクロデモファイルも作成しました。これはプリプロセッサマクロの基本的な使い方を示しています。

## プリプロセッサマクロの使い方

### 定数定義
```cb
#define PI 3.14159
#define MAX_SIZE 100
```

### 関数マクロ
```cb
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
```

### ネストしたマクロ
```cb
#define SQUARE(x) ((x) * (x))
#define QUAD(x) SQUARE(SQUARE(x))
```

## 注意点

1. **括弧を必ず使用**: `(x)` not `x`
2. **式全体も括弧で囲む**: `((x) * (x))` not `(x * x)`
3. **副作用に注意**: `SQUARE(x++)` は危険（`x`が2回評価される）
4. **単純なテキスト置換**: コンパイル時に文字列として展開される

## 今後の拡張可能性

将来的に以下の機能を追加可能:
- `#if`, `#ifdef`, `#ifndef`, `#else`, `#endif`条件付きコンパイル
- `#include`ファイルインクルード（既存の`import`と統合）
- プリ定義マクロ: `__FILE__`, `__LINE__`, `__DATE__`, `__TIME__`

## まとめ

✅ `sample/macro_demo.cb`を現在実装されているプリプロセッサマクロで動作するように修正
✅ シンプルなデモファイル`simple_macro_demo.cb`も作成
✅ プリプロセッサマクロの基本的な使い方を実証

現在のCbインタプリタでは`#define`によるプリプロセッサマクロが利用可能です。
