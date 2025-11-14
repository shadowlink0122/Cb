# v0.13.0 機能クイックリファレンス

**バージョン**: v0.13.0  
**更新日**: 2025-11-14

---

## 🌟 新機能

### 1. FFI (Foreign Function Interface)

外部C/C++ライブラリの関数を直接呼び出せます。

#### 基本構文
```cb
use foreign.module_name {
    return_type function_name(param_type param_name, ...);
}

void main() {
    return_type result = module_name.function_name(args);
}
```

#### 例：数学ライブラリの使用
```cb
use foreign.m {
    double sqrt(double x);
    double pow(double base, double exp);
    double sin(double x);
    double cos(double x);
}

void main() {
    double result1 = m.sqrt(16.0);      // 4.0
    double result2 = m.pow(2.0, 3.0);   // 8.0
    double result3 = m.sin(0.0);         // 0.0
    
    println("sqrt(16) =", result1);
    println("2^3 =", result2);
    println("sin(0) =", result3);
}
```

#### 例：標準Cライブラリの使用
```cb
use foreign.c {
    int abs(int x);
    void exit(int status);
}

void main() {
    int value = c.abs(-42);
    println(value);  // 42
}
```

#### 対応する型
- `int`, `long`, `short`, `tiny`
- `double`, `float`
- `bool`, `char`
- `string` (制限あり)
- `void` (戻り値のみ)

#### 複数モジュールの使用
```cb
use foreign.m {
    double sqrt(double x);
}

use foreign.c {
    int abs(int x);
}

void main() {
    double d = m.sqrt(9.0);   // math module
    int i = c.abs(-5);         // C module
    println(d, i);
}
```

---

### 2. プリプロセッサ

コンパイル前にソースコードを変換します。

#### マクロ定義
```cb
#define MACRO_NAME value
#define PI 3.14159
#define MAX_SIZE 1024
#define DEBUG
```

#### 条件付きコンパイル
```cb
#ifdef MACRO_NAME
    // マクロが定義されている場合のコード
#endif

#ifndef MACRO_NAME
    // マクロが定義されていない場合のコード
#endif

#ifdef CONDITION1
    // 条件1
#elseif CONDITION2
    // 条件2
#else
    // その他
#endif
```

#### 実用例
```cb
#define DEBUG
#define VERSION "v0.13.0"
#define MAX_BUFFER 1024

#ifdef DEBUG
    void log(string msg) {
        println("[DEBUG]", msg);
    }
#else
    void log(string msg) { }
#endif

void main() {
    println("Application", VERSION);
    log("Starting...");
    
    int buffer_size = MAX_BUFFER;
    println("Buffer:", buffer_size);
}
```

#### 組み込みマクロ
```cb
void main() {
    println("Version:", __VERSION__);  // "0.13.0"
    println("File:", __FILE__);        // ファイル名
    println("Line:", __LINE__);        // 行番号
    println("Date:", __DATE__);        // コンパイル日
    println("Time:", __TIME__);        // コンパイル時刻
}
```

#### マクロの削除
```cb
#define TEMP 100
#undef TEMP
// TEMPは未定義になる
```

#### ネスト
```cb
#define FEATURE_A
#define FEATURE_B

#ifdef FEATURE_A
    #ifdef FEATURE_B
        println("Both features enabled");
    #else
        println("Only A enabled");
    #endif
#endif
```

---

### 3. VSCodeシンタックスハイライト

#### ハイライトカラー

**ピンク色**（制御構文）:
- プリプロセッサディレクティブ: `#define`, `#ifdef`, `#ifndef`, `#elseif`, `#else`, `#endif`, `#undef`
- `use` キーワード

**青色**（型・ストレージ）:
- `foreign` キーワード
- `static`, `const` キーワード
- プリミティブ型: `int`, `double`, `void`, `string`, `bool`, etc.

**数値と同じ色**:
- 定数（大文字+アンダースコア）: `MAX_SIZE`, `PI`, `BUFFER_SIZE`
- 数値リテラル: `123`, `3.14`, `0x1A`

---

## 📝 使用例：総合

```cb
// プリプロセッサで設定
#define DEBUG
#define PI 3.14159
#define RADIUS 5.0

// FFI宣言
use foreign.m {
    double sqrt(double x);
    double pow(double base, double exp);
}

use foreign.c {
    int abs(int x);
}

// デバッグ関数（条件付きコンパイル）
#ifdef DEBUG
    void debug_log(string msg) {
        println("[DEBUG]", msg);
    }
#else
    void debug_log(string msg) { }
#endif

// メイン処理
void main() {
    println("=== Cb v0.13.0 Demo ===");
    
    // プリプロセッサマクロの使用
    double area = PI * RADIUS * RADIUS;
    println("Circle area:", area);
    
    // FFI関数の呼び出し
    double sqrt_result = m.sqrt(16.0);
    double pow_result = m.pow(2.0, 3.0);
    int abs_result = c.abs(-42);
    
    println("sqrt(16) =", sqrt_result);
    println("2^3 =", pow_result);
    println("abs(-42) =", abs_result);
    
    // デバッグログ
    debug_log("All calculations completed");
    
    // 組み込みマクロ
    println("Version:", __VERSION__);
    println("File:", __FILE__);
}
```

**出力**:
```
=== Cb v0.13.0 Demo ===
Circle area: 78.53975
sqrt(16) = 4.0
2^3 = 8.0
abs(-42) = 42
[DEBUG] All calculations completed
Version: 0.13.0
File: demo.cb
```

---

## ⚠️ 注意事項

### FFI
- 外部ライブラリは実行時に動的にロードされます
- ライブラリパスは環境変数`LD_LIBRARY_PATH`（Linux）、`DYLD_LIBRARY_PATH`（macOS）で設定できます
- 標準Cライブラリ（`libc`）と数学ライブラリ（`libm`）は自動的に検索されます

### プリプロセッサ
- マクロは文字列内では展開されません
- 識別子の境界が保護されます（`MAX`は`MAX_VALUE`内で展開されません）
- コメント内のマクロは展開されません

### シンタックスハイライト
- VSCode拡張機能をインストールする必要があります
- ファイル拡張子は `.cb` を使用してください

---

## 📚 詳細ドキュメント

- 設計詳細: `docs/todo/v0.13.0/modern_ffi_macro_design.md`
- 実装報告: `docs/todo/v0.13.0/FINAL_IMPLEMENTATION_COMPLETE.md`
- ロードマップ: `docs/todo/v0.13.0/version_roadmap.md`

---

**v0.13.0** | **2025-11-14**
