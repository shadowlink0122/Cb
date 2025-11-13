# v0.13.0 モダンFFI・マクロ・プリプロセッサ設計

**バージョン**: v0.13.0
**作成日**: 2025-11-13
**ステータス**: 設計フェーズ

## 概要

v0.13.0では、以下の3つの機能を統合的に設計します：

1. **FFI (Foreign Function Interface)** - 外部ライブラリとの連携
2. **マクロシステム** - コンパイル時のコード生成
3. **プリプロセッサ** - 条件付きコンパイル、定数定義

これらを**モダンで型安全**な設計にすることを目指します。

## 設計原則

### 1. 型安全性 (Type Safety)

```cb
// ❌ 悪い例：型情報がない
use "libmath.so"::add;
int result = add(10, 20);  // 戻り値の型は？引数の型は？

// ✅ 良い例：完全な型情報
use "libmath.so"::add(int, int) -> int;
int result = add(10, 20);  // 型チェック済み
```

### 2. 明示的 > 暗黙的 (Explicit over Implicit)

```cb
// ❌ 悪い例：暗黙的な変換
use "libmath.so"::sqrt;
int x = sqrt(16);  // double → int の暗黙的変換

// ✅ 良い例：明示的な型
use "libmath.so"::sqrt(double) -> double;
double result = sqrt(16.0);
int x = (int)result;  // 明示的なキャスト
```

### 3. 最小驚き原則 (Principle of Least Astonishment)

```cb
// ✅ 直感的：既存のuse構文と統一
use "libmath.so"::add(int, int) -> int;

// ✅ 直感的：既存のマクロと統一
#define MAX(a, b) ((a) > (b) ? (a) : (b))
```

### 4. DRY (Don't Repeat Yourself)

```cb
// ❌ 悪い例：同じライブラリを何度も指定
use "libmath.so"::add(int, int) -> int;
use "libmath.so"::sub(int, int) -> int;
use "libmath.so"::mul(int, int) -> int;

// ✅ 良い例：ブロックでまとめる
use lib "libmath.so" {
    add: (int, int) -> int;
    sub: (int, int) -> int;
    mul: (int, int) -> int;
}
```

## Part 1: FFI (Foreign Function Interface)

### 基本構文（推奨）

#### 推奨構文：既存の関数定義形式に統一 ⭐⭐⭐⭐⭐

```cb
// 既存のCbの関数定義と同じ形式
use foreign.math {
    int add(int a, int b);
    int sub(int a, int b);
    double sqrt(double x);
    void* malloc(int size);
}

void main() {
    int x = add(10, 20);
    double s = sqrt(16.0);
    println(x, s);  // 30 4.0
}
```

**特徴**:
- ✅ 既存のCb関数定義と完全に一貫性がある
- ✅ 引数名も書ける（ドキュメントとして有用）
- ✅ C/C++の関数宣言と同じ形式
- ✅ 学習コストゼロ

**比較**（既存のCb関数）:
```cb
// Cbの通常の関数定義
int add(int a, int b) {
    return a + b;
}

// use でも同じ形式（セミコロンで終わる）
use foreign.math {
    int add(int a, int b);
}
```

#### 名前空間について

デフォルトでは、モジュール名が名前空間になります：

```cb
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
}

void main() {
    int x = math.add(10, 20);      // モジュール名.関数名
    double s = math.sqrt(16.0);
    println(x, s);
}
```

**理由**: importと同じ動作（一貫性）

```cb
// importの場合
import stdlib.vector;
Vector<int> vec;  // モジュール名.型名

// useの場合
use foreign.math;
int x = math.add(10, 20);  // モジュール名.関数名
```

### 高度な機能

#### 可変長引数のサポート

```cb
use foreign.libc {
    int printf(string format, ...);  // 可変長引数
    int sprintf(string dest, string format, ...);
}

void main() {
    libc.printf("Hello, %s! You are %d years old.\n", "Alice", 25);
}
```

#### 構造体の受け渡し

```cb
// Cb側の構造体定義
struct Point {
    int x;
    int y;
}

use foreign.graphics {
    void draw_point(Point p);
    double get_distance(Point p1, Point p2);
}

void main() {
    Point p1 = {x: 10, y: 20};
    Point p2 = {x: 30, y: 40};

    graphics.draw_point(p1);
    double dist = graphics.get_distance(p1, p2);
    println(dist);
}
```

#### コールバック関数

```cb
// コールバック型を定義（既存のCbの型定義）
typedef Callback = int(int);

use foreign.ui {
    void register_callback(Callback cb);
    void trigger_event(int value);
}

void my_callback(int value) {
    println("Callback called with:", value);
}

void main() {
    ui.register_callback(&my_callback);
    ui.trigger_event(42);
}
```

### プラットフォーム別のモジュール

```cb
// プラットフォームに応じてモジュールを切り替え
#ifdef OS_LINUX
    use foreign.linux_specific {
        int get_pid();
        int fork();
    }
#elseif OS_MACOS
    use foreign.macos_specific {
        int get_pid();
        // macOS固有の関数
    }
#elseif OS_WINDOWS
    use foreign.windows_specific {
        int GetCurrentProcessId();
        // Windows固有の関数
    }
#endif

// すべてのプラットフォームで共通
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
}
```

**利点**: モジュール名で抽象化されるため、拡張子を気にする必要がない

## Part 2: マクロシステム

### シンプルなマクロ設計（C風）

Rust風のマクロは複雑すぎるため、**C風のシンプルなマクロ**のみを提供します。

**設計方針**:
- ✅ シンプルで理解しやすい
- ✅ C/C++開発者にとって馴染み深い
- ❌ Rust風の複雑な構文は採用しない

### 2.1 関数マクロ（C風）

#### 基本的な定数定義

```cb
#define PI 3.14159265359
#define MAX_BUFFER_SIZE 1024
#define VERSION "0.13.0"

void main() {
    double area = PI * r * r;
    int buffer[MAX_BUFFER_SIZE];
    println("Version:", VERSION);
}
```

#### 関数風マクロ

```cb
// 基本的なマクロ
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// 型安全なマクロ（型を明示）
#define SQUARE(x: int) -> int  ((x) * (x))
#define ABS(x: double) -> double  ((x) < 0 ? -(x) : (x))

void main() {
    int max_val = MAX(10, 20);  // 20
    int sq = SQUARE(5);         // 25

    println(max_val, sq);
}
```

**型安全マクロの利点**:
- ✅ 型チェックがある
- ✅ 誤った型を渡すとコンパイルエラー
- ❌ 実装が複雑（将来の機能）

#### 複数行マクロ

```cb
#define DEBUG_PRINT(msg) \
    do { \
        println("[DEBUG]", __FILE__, ":", __LINE__, ":", msg); \
    } while(0)

void main() {
    DEBUG_PRINT("Hello, world!");
    // 出力: [DEBUG] main.cb : 10 : Hello, world!
}
```

#### 可変長引数マクロ

```cb
// C99風の可変長引数
#define LOG(level, ...) \
    println("[" + level + "]", __VA_ARGS__)

void main() {
    LOG("INFO", "Server started on port", 8080);
    LOG("ERROR", "Connection failed:", "timeout");
}

// 出力:
// [INFO] Server started on port 8080
// [ERROR] Connection failed: timeout
```

#### 条件付きマクロ

```cb
#ifdef DEBUG
    #define LOG(msg) println("[DEBUG]", msg)
#else
    #define LOG(msg)  // デバッグ無効時は何もしない
#endif

void main() {
    LOG("This is a debug message");
    // DEBUGが定義されている場合のみ出力
}
```

### 2.2 マクロの代替手段

複雑なマクロの代わりに、以下の機能を活用します：

#### 代替案1: ジェネリクス（すでに実装済み）

```cb
// マクロの代わりにジェネリック関数を使う
T max<T>(T a, T b) {
    return a > b ? a : b;
}

void main() {
    int x = max<int>(10, 20);      // 20
    double y = max<double>(3.14, 2.71);  // 3.14
}
```

**利点**:
- ✅ 型安全
- ✅ デバッグしやすい
- ✅ すでにv0.11.0で実装済み

#### 代替案2: インライン関数（将来）

```cb
// inline関数でマクロを置き換え
inline int square(int x) {
    return x * x;
}

void main() {
    int result = square(5);  // コンパイル時に展開
}
```

**利点**:
- ✅ マクロと同等のパフォーマンス
- ✅ 型チェックがある
- ✅ スコープが明確

#### 代替案3: constexpr（将来）

```cb
// コンパイル時定数
constexpr int BUFFER_SIZE = 1024;
constexpr double PI = 3.14159265359;

constexpr int factorial(int n) {
    return n <= 1 ? 1 : n * factorial(n - 1);
}

void main() {
    int buffer[factorial(5)];  // コンパイル時に計算
}
```

**結論**: 複雑なマクロは不要。既存のジェネリクスと将来のinline/constexprで十分。

## Part 3: プリプロセッサ

### 3.1 条件付きコンパイル（シンプル化）

#### 基本的な条件分岐

**採用する構文**: `ifdef`, `ifndef`, `elseif`, `else`, `endif`

```cb
#define DEBUG

#ifdef DEBUG
    void log(string msg) {
        println("[DEBUG]", msg);
    }
#else
    void log(string msg) {
        // リリースビルドでは何もしない
    }
#endif

void main() {
    log("Application started");
}
```

#### #ifndef (if not defined)

```cb
#ifndef MAX_BUFFER_SIZE
    #define MAX_BUFFER_SIZE 1024
#endif

// 重複定義の防止
#ifndef _MY_HEADER_H_
#define _MY_HEADER_H_

// ヘッダーの内容

#endif
```

#### #elseif（複数条件分岐）

```cb
#define PLATFORM_LINUX

#ifdef PLATFORM_LINUX
    #define OS_NAME "Linux"
    #define PATH_SEPARATOR '/'
#elseif PLATFORM_WINDOWS
    #define OS_NAME "Windows"
    #define PATH_SEPARATOR '\\'
#elseif PLATFORM_MACOS
    #define OS_NAME "macOS"
    #define PATH_SEPARATOR '/'
#else
    #error "Unknown platform"
#endif

void main() {
    println("OS:", OS_NAME);
}
```

**採用しない構文**:
- ❌ `#if defined(FLAG)` - 冗長でわかりづらい
- ❌ `#elif` - `#elseif` の方が明確

**理由**:
- `ifdef FLAG` の方がシンプルで読みやすい
- `elseif` の方が自然な英語（else if）

### 3.2 インクルード（import拡張は不要）

**現状**: Cbはすでに `import files.mod;` 形式でインポート可能

```cb
// すでに実装済みのimport構文
import stdlib.vector;
import stdlib.queue;
import utils.math;

void main() {
    Vector<int, SystemAllocator> vec;
    vec.init(10);
    vec.push(42);
}
```

**結論**: import拡張は不要。既存のimport構文で十分。

### 3.3 プリプロセッサの組み込みマクロ

```cb
#define __FILE__     // 現在のファイル名
#define __LINE__     // 現在の行番号
#define __DATE__     // コンパイル日付 "Nov 13 2025"
#define __TIME__     // コンパイル時刻 "14:30:00"
#define __VERSION__  // Cbのバージョン "0.13.0"

void main() {
    println("File:", __FILE__);
    println("Line:", __LINE__);
    println("Compiled:", __DATE__, __TIME__);
    println("Cb version:", __VERSION__);
}

// 出力例:
// File: main.cb
// Line: 9
// Compiled: Nov 13 2025 14:30:00
// Cb version: 0.13.0
```

### 3.4 プリプロセッサのエラー・警告

```cb
// コンパイルエラーを発生させる
#ifndef REQUIRED_FEATURE
    #error "REQUIRED_FEATURE must be defined"
#endif

// 警告を出す
#if VERSION_MAJOR < 1
    #warning "This is a beta version"
#endif

// 静的アサーション（コンパイル時チェック）
#if MAX_BUFFER_SIZE < 256
    #error "MAX_BUFFER_SIZE must be at least 256"
#endif
```

## 統合例：FFI + マクロ + プリプロセッサ

### 例1: クロスプラットフォームなFFI

```cb
// プラットフォーム検出マクロ
#if defined(__linux__)
    #define OS_LINUX
#elif defined(__APPLE__)
    #define OS_MACOS
#elif defined(_WIN32)
    #define OS_WINDOWS
#endif

// プラットフォーム別のライブラリパス
#ifdef OS_LINUX
    #define MATH_LIB "libmath.so"
    #define C_LIB "libc.so.6"
#elif defined(OS_MACOS)
    #define MATH_LIB "libmath.dylib"
    #define C_LIB "libSystem.dylib"
#elif defined(OS_WINDOWS)
    #define MATH_LIB "math.dll"
    #define C_LIB "msvcrt.dll"
#endif

// FFIライブラリの読み込み
use lib MATH_LIB {
    add: (int, int) -> int;
    sqrt: (double) -> double;
}

use lib C_LIB {
    malloc: (int) -> void*;
    free: (void*) -> void;
}

void main() {
    int x = add(10, 20);
    double s = sqrt(16.0);

    println("OS:",
        #ifdef OS_LINUX
            "Linux"
        #elif defined(OS_MACOS)
            "macOS"
        #else
            "Windows"
        #endif
    );

    println("Result:", x, s);
}
```

### 例2: デバッグビルドとリリースビルド

```cb
// デバッグフラグ（コマンドラインから指定: ./main -DDEBUG main.cb）
#ifdef DEBUG
    #define LOG(msg) println("[DEBUG]", __FILE__, ":", __LINE__, ":", msg)
    #define ASSERT(cond, msg) \
        if (!(cond)) { \
            println("[ASSERT FAILED]", __FILE__, ":", __LINE__, ":", msg); \
            exit(1); \
        }
#else
    #define LOG(msg)  // リリースビルドでは何もしない
    #define ASSERT(cond, msg)  // リリースビルドではアサート無効
#endif

use lib "libmath.so" {
    divide: (int, int) -> int;
}

void main() {
    LOG("Application started");

    int a = 10;
    int b = 0;

    ASSERT(b != 0, "Division by zero");

    int result = divide(a, b);
    LOG("Result: " + result);
}
```

### 例3: 機能フラグによる条件付きコンパイル

```cb
// 機能フラグ
#define FEATURE_NETWORKING
#define FEATURE_DATABASE
// #define FEATURE_GPU  // 無効

#ifdef FEATURE_NETWORKING
    use lib "libcurl.so" {
        http_get: (string) -> string;
        http_post: (string, string) -> string;
    }
#endif

#ifdef FEATURE_DATABASE
    use lib "libsqlite3.so" {
        db_open: (string) -> void*;
        db_query: (void*, string) -> string;
        db_close: (void*) -> void;
    }
#endif

#ifdef FEATURE_GPU
    use lib "libcuda.so" {
        gpu_malloc: (int) -> void*;
        gpu_compute: (void*, int) -> void;
    }
#endif

void main() {
    #ifdef FEATURE_NETWORKING
        string data = http_get("https://api.example.com/data");
        println("Fetched:", data);
    #endif

    #ifdef FEATURE_DATABASE
        void* db = db_open("mydb.sqlite");
        string result = db_query(db, "SELECT * FROM users");
        println("Query result:", result);
        db_close(db);
    #endif

    #ifdef FEATURE_GPU
        // GPU機能は無効なので、このコードはコンパイルされない
        void* gpu_mem = gpu_malloc(1024);
        gpu_compute(gpu_mem, 1024);
    #else
        println("GPU support is disabled");
    #endif
}
```

## 実装計画

### Phase 1: プリプロセッサ基盤（Week 1-2）

**目標**: 基本的なプリプロセッサを実装

- [ ] Lexerの拡張（#で始まるトークンを認識）
- [ ] プリプロセッサディレクティブのパース
  - [ ] #define
  - [ ] #ifdef / #ifndef / #endif
  - [ ] #if / #elif / #else
  - [ ] #include (既存のimportと統合)
- [ ] マクロ展開エンジン
- [ ] 組み込みマクロ (__FILE__, __LINE__, __DATE__, __TIME__)

**成果物**:
```cb
#define PI 3.14159
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void main() {
    double area = PI * r * r;
    int max_val = MAX(10, 20);
}
```

### Phase 2: FFI基盤（Week 3-4）

**目標**: 基本的なFFIを実装

- [ ] `use lib` 構文のパース
- [ ] dlopen/dlsym ラッパーの実装
- [ ] ライブラリキャッシュ
- [ ] 基本的な型変換（int, long, double, string）
- [ ] 関数シグネチャの検証

**成果物**:
```cb
use lib "libmath.so" {
    add: (int, int) -> int;
    sqrt: (double) -> double;
}

void main() {
    int x = add(10, 20);
    println(x);
}
```

### Phase 3: FFI拡張機能（Week 5-6）

**目標**: 高度なFFI機能

- [ ] 構造体の受け渡し
- [ ] ポインタ型のサポート
- [ ] 可変長引数のサポート
- [ ] コールバック関数
- [ ] エラーハンドリング（Result型との統合）

**成果物**:
```cb
struct Point { int x; int y; }

use lib "libgraphics.so" {
    draw_point: (Point) -> void;
    printf: (string, ...) -> int;
}
```

### Phase 4: プリプロセッサ高度機能（Week 7）

**目標**: 高度なプリプロセッサ機能

- [ ] #undef（マクロの削除）
- [ ] #error / #warning
- [ ] 可変長引数マクロ (__VA_ARGS__)
- [ ] 複数行マクロ（バックスラッシュ継続）
- [ ] プリプロセッサの式評価（#if defined(X) && VERSION > 1）

**成果物**:
```cb
#define LOG(level, ...) \
    println("[" + level + "]", __VA_ARGS__)

#if defined(DEBUG) && VERSION >= 13
    #warning "Debug mode enabled"
#endif
```

### Phase 5: 統合とテスト（Week 8）

**目標**: すべての機能を統合してテスト

- [ ] クロスプラットフォームテスト（Linux, macOS）
- [ ] FFI + プリプロセッサの統合テスト
- [ ] エッジケースのテスト
- [ ] ドキュメント作成
- [ ] サンプルコード作成

**成果物**:
- 100個以上の統合テスト
- FFI/マクロ/プリプロセッサのドキュメント
- 実用的なサンプルコード

## 技術的な実装詳細

### プリプロセッサの実装

```cpp
// src/frontend/preprocessor/preprocessor.h
class Preprocessor {
public:
    std::string process(const std::string& source_code);

private:
    std::map<std::string, std::string> defines_;  // マクロ定義
    std::set<std::string> conditionals_;          // 条件付きコンパイル

    std::string expand_macros(const std::string& line);
    bool evaluate_condition(const std::string& expr);
    std::string process_directive(const std::string& directive);
};
```

### FFIの実装

```cpp
// src/backend/interpreter/ffi/ffi_manager.h
class FFIManager {
public:
    void load_library(const std::string& lib_path,
                     const std::string& func_name,
                     const FunctionSignature& signature);

    TypedValue call_foreign_function(const std::string& func_name,
                                     const std::vector<TypedValue>& args);

private:
    struct LibraryHandle {
        void* handle;
        std::map<std::string, void*> functions;
    };

    std::map<std::string, LibraryHandle> libraries_;
    std::map<std::string, FunctionSignature> signatures_;

    void* get_function_pointer(const std::string& lib_path,
                               const std::string& func_name);
};
```

## セキュリティ考慮事項

### 1. ライブラリパスの検証

```cpp
bool is_safe_library_path(const std::string& path) {
    // 絶対パスのみ許可
    if (path[0] != '/') {
        return false;
    }

    // ディレクトリトラバーサル攻撃を防止
    if (path.find("..") != std::string::npos) {
        return false;
    }

    // ホワイトリスト方式
    const std::vector<std::string> allowed_dirs = {
        "/usr/lib",
        "/usr/local/lib",
        "/lib"
    };

    for (const auto& dir : allowed_dirs) {
        if (path.find(dir) == 0) {
            return true;
        }
    }

    return false;
}
```

### 2. サンドボックス化（将来）

```cb
// 安全なFFI呼び出し（将来の機能）
@sandbox
use lib "untrusted.so" {
    potentially_dangerous: (int) -> int;
}

// サンドボックス内で実行され、システムリソースへのアクセスが制限される
```

### 3. 型安全性の強制

```cb
use lib "libmath.so" {
    add: (int, int) -> int;
}

void main() {
    // ✅ OK: 型が一致
    int result = add(10, 20);

    // ❌ コンパイルエラー: 型が一致しない
    // int result = add("hello", 20);

    // ❌ コンパイルエラー: 引数の数が一致しない
    // int result = add(10);
}
```

## ベストプラクティス

### 1. FFIライブラリの整理

```cb
// 推奨：関連する関数をまとめる
use lib "libmath.so" {
    // 算術演算
    add: (int, int) -> int;
    sub: (int, int) -> int;
    mul: (int, int) -> int;
    div: (int, int) -> int;

    // 数学関数
    sqrt: (double) -> double;
    pow: (double, double) -> double;
    sin: (double) -> double;
    cos: (double) -> double;
}
```

### 2. マクロの適切な使用

```cb
// ✅ 良い例：定数にマクロを使う
#define MAX_BUFFER_SIZE 1024
#define PI 3.14159265359

// ❌ 悪い例：複雑なロジックをマクロにしない
// #define COMPLEX_LOGIC(x) ... // 代わりに関数を使う

// ✅ 良い例：デバッグ用のマクロ
#ifdef DEBUG
    #define LOG(msg) println("[DEBUG]", msg)
#else
    #define LOG(msg)
#endif
```

### 3. プリプロセッサの適切な使用

```cb
// ✅ 良い例：機能フラグで条件分岐
#ifdef FEATURE_NETWORKING
    use lib "libcurl.so" { ... }
#endif

// ✅ 良い例：プラットフォーム別の実装
#ifdef OS_LINUX
    // Linux専用のコード
#elif defined(OS_MACOS)
    // macOS専用のコード
#endif

// ❌ 悪い例：プリプロセッサで複雑なロジック
// 代わりにif文を使う
```

## まとめ

### v0.13.0で実装する機能

| 機能 | 優先度 | 実装時期 | 推奨度 |
|-----|-------|---------|--------|
| FFI基盤 (`use lib`) | 高 | Week 3-4 | ⭐⭐⭐⭐⭐ |
| プリプロセッサ基盤 (#define, #ifdef) | 高 | Week 1-2 | ⭐⭐⭐⭐⭐ |
| FFI拡張（構造体、ポインタ） | 中 | Week 5-6 | ⭐⭐⭐⭐ |
| プリプロセッサ拡張（#if, #error） | 中 | Week 7 | ⭐⭐⭐⭐ |
| 手続き型マクロ | 低 | v1.0.0 | ⭐⭐⭐ |

### 設計の強み

1. **型安全性**: 完全な型チェックにより、実行時エラーを防止
2. **モダンな構文**: Rust/TypeScript風の直感的な構文
3. **クロスプラットフォーム**: プリプロセッサで容易に対応
4. **既存構文との統一**: Cbの既存の`use`構文と統一感
5. **拡張性**: 将来の機能追加が容易

### 次のステップ

1. **プロトタイプ実装**（Week 1-2）
   - プリプロセッサの基本機能
   - FFIの基本機能

2. **ユーザーフィードバック**（Week 3）
   - 構文の使いやすさ
   - 実用性の確認

3. **本実装**（Week 4-8）
   - 完全な機能実装
   - テストとドキュメント
