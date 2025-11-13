# v0.13.0 インラインアセンブリ・インラインC++ 実現可能性調査

**バージョン**: v0.13.0
**作成日**: 2025-11-13
**ステータス**: 設計フェーズ

## 概要

v0.13.0で、`asm("")`によるインラインアセンブリと`cpp("")`によるインラインC++の実装を検討します。本ドキュメントでは、技術的な実現可能性を調査し、実装アプローチを提案します。

## 目標機能

### 1. インラインアセンブリ (`asm("")`)

```cb
void main() {
    int x = 42;
    int result;

    // インラインアセンブリで値を操作
    asm("mov eax, {x}");
    asm("add eax, 10");
    asm("mov {result}, eax");

    println(result);  // 52
}
```

### 2. インラインC++ (`cpp("")`)

```cb
void main() {
    int x = 10;
    int y = 20;

    // インラインC++コード
    int result = cpp("x + y * 2");  // 50

    println(result);
}
```

### 3. インラインC++（生の構文版）

```cb
void main() {
    int x = 10;

    // 文字列ではなく、生のC++構文で記述
    // CbとC++の構文互換性を利用
    cpp {
        std::cout << "Hello from C++!" << std::endl;
        x = x * 2;
    }

    println(x);  // 20
}
```

## 現状のCbアーキテクチャ

### インタプリタベースの実装

Cbは**インタプリタ型言語**であり、以下の構成です：

```
ソースコード (.cb)
    ↓
再帰下降パーサー (recursive_parser)
    ↓
抽象構文木 (AST)
    ↓
インタプリタ (interpreter)
    ↓
実行
```

### 組み込み関数の実装方法

現在、Cbには2つの組み込み関数実装方法があります：

#### 方法1: builtin_function_namesリスト

```cpp
// src/backend/interpreter/evaluator/functions/call_impl.cpp:3090-3112
static const std::vector<std::string> builtin_function_names = {
    "malloc", "free", "sizeof",
    "println", "print", "sleep", "now", ...
};
```

実装例（malloc）:
```cpp
if (node->name == "malloc") {
    int64_t size = interpreter_.eval_expression(node->arguments[0].get());
    void *ptr = std::malloc(static_cast<size_t>(size));
    return reinterpret_cast<int64_t>(ptr);
}
```

#### 方法2: 特別なASTノード型

```cpp
// src/common/ast.h:1003-1007
enum ASTNodeType {
    AST_PRINT_STMT,
    AST_PRINTLN_STMT,
    AST_PRINTLN_EMPTY,
    AST_PRINTF_STMT,
    ...
};
```

## 技術的実現可能性の評価

### 1. インラインアセンブリ (`asm("")`)

#### 課題

1. **インタプリタの制限**
   - Cbはインタプリタであり、コンパイラではない
   - アセンブリコードを実行するには、機械語を生成して実行する必要がある
   - インタプリタで直接アセンブリを実行することは極めて困難

2. **プラットフォーム依存性**
   - x86-64, ARM, RISC-Vなど、CPUアーキテクチャごとに異なるアセンブリが必要
   - 移植性が大幅に低下

3. **セキュリティリスク**
   - 任意のアセンブリコードを実行できるため、システムを破壊する可能性がある

#### 実装アプローチ

##### アプローチA: JITコンパイル（LLVM）

**概要**: LLVM JITを使用してアセンブリコードをその場でコンパイルし、実行

```cpp
#include <llvm/ExecutionEngine/JIT.h>

int64_t execute_inline_asm(const std::string& asm_code) {
    // LLVM JITでアセンブリをコンパイル
    llvm::Module* module = compile_assembly(asm_code);

    // 実行
    llvm::Function* func = module->getFunction("inline_asm");
    return execute_jit_function(func);
}
```

**長所**:
- ✅ 高性能な実行
- ✅ LLVMの最適化を利用可能
- ✅ 複数のアーキテクチャに対応

**短所**:
- ❌ **LLVM依存**: 巨大なライブラリ（数百MB）
- ❌ **複雑性**: JITエンジンの実装が非常に複雑
- ❌ **コンパイル時間**: 初回実行時のオーバーヘッドが大きい
- ❌ **ビルド時間**: LLVMのビルドに数時間かかる

##### アプローチB: 外部アセンブラ（nasm/gas）

**概要**: 外部アセンブラでオブジェクトファイルを生成し、動的にロード

```cpp
int64_t execute_inline_asm(const std::string& asm_code) {
    // 一時ファイルにアセンブリコードを保存
    write_temp_file("temp.asm", asm_code);

    // nasmでアセンブル
    system("nasm -f elf64 temp.asm -o temp.o");

    // 共有ライブラリを作成
    system("ld -shared temp.o -o temp.so");

    // 動的にロード
    void* handle = dlopen("temp.so", RTLD_NOW);
    auto func = (int64_t(*)())dlsym(handle, "inline_asm");

    return func();
}
```

**長所**:
- ✅ LLVMなしで実装可能
- ✅ 既存のツールチェーンを利用

**短所**:
- ❌ **パフォーマンス**: ファイルI/O、外部プロセス呼び出しのオーバーヘッド
- ❌ **プラットフォーム依存**: macOS(darwin), Linux, Windowsで異なる実装が必要
- ❌ **依存関係**: nasm/gasが必要
- ❌ **一時ファイル管理**: クリーンアップが複雑

##### アプローチC: 制限付きインタプリタ

**概要**: 基本的なアセンブリ命令のみをシミュレート

```cpp
int64_t execute_inline_asm(const std::string& asm_code) {
    AsmInterpreter interpreter;
    interpreter.parse(asm_code);
    return interpreter.execute();
}
```

**長所**:
- ✅ 外部依存なし
- ✅ シンプルな実装
- ✅ ポータブル

**短所**:
- ❌ **機能制限**: 限られた命令のみサポート
- ❌ **パフォーマンス**: ネイティブアセンブリより遅い
- ❌ **互換性**: 実際のアセンブリとは異なる

#### なぜインタプリタでアセンブリは難しいのか？

**根本的な問題**:

1. **実行モデルの違い**
   ```
   インタプリタ: ASTノードを解釈 → 値を計算 → 次のノードへ
   アセンブリ:   機械語命令 → CPUレジスタ操作 → メモリ操作
   ```
   この2つの世界は全く異なるモデルで動作します。

2. **変数とレジスタの対応**
   ```cb
   int x = 42;
   asm("mov eax, {x}");  // ← どうやってxをレジスタに渡す？
   asm("add eax, 10");
   asm("mov {x}, eax");  // ← どうやって結果をxに戻す？
   ```

   Cbの変数（メモリ上の値）とCPUレジスタの間に橋渡しが必要

3. **メモリ管理の複雑さ**
   - アセンブリは生のメモリアドレスを扱う
   - インタプリタは抽象化されたメモリ空間で動作
   - この2つをどう統合するか？

#### 技術的には可能だが、3つの大きな障壁がある

##### 障壁1: アセンブリを機械語に変換する仕組みが必要

**問題**: インタプリタはテキストを解釈するが、CPUはバイナリしか実行できない

**解決策**:
- ✅ LLVM JIT: アセンブリ → LLVM IR → 機械語
- ✅ AsmJit: アセンブリを直接機械語に変換
- ✅ 外部アセンブラ: nasm/gasを呼び出す
- ❌ すべて追加の依存関係やオーバーヘッドが発生

##### 障壁2: 生成した機械語を実行する仕組みが必要

**問題**: 通常のメモリは実行権限がない（セキュリティ保護）

**解決策**:
```cpp
// 実行可能メモリを確保
void* exec_mem = mmap(nullptr, size,
                      PROT_READ | PROT_WRITE | PROT_EXEC,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

// 機械語をコピー
memcpy(exec_mem, machine_code, code_size);

// 関数ポインタとして実行
auto func = (int64_t(*)())exec_mem;
int64_t result = func();

// メモリ解放
munmap(exec_mem, size);
```

これは可能ですが：
- ❌ プラットフォーム依存（macOS/Linux/Windows で異なる）
- ❌ セキュリティリスク（任意のコードを実行可能）
- ❌ メモリ管理が複雑

##### 障壁3: Cbの変数とアセンブリの橋渡し

**問題**: Cbの変数をどうやってアセンブリコードに渡すか？

**アプローチ1: レジスタ渡し**
```cb
int x = 42;
asm("mov eax, {x}");  // xをeaxに
```

実装：
```cpp
// {x}を実際の値に置換
std::string asm_code = "mov eax, 42";  // x=42を埋め込む

// しかし、これでは実行時の値が使えない！
```

**アプローチ2: メモリアドレス渡し**
```cpp
// Cbの変数のアドレスを取得
int64_t* x_addr = &interpreter.current_scope().variables["x"].value;

// アセンブリコード生成時にアドレスを埋め込む
std::stringstream asm_code;
asm_code << "mov rax, qword ptr [" << (uint64_t)x_addr << "]";
asm_code << "add rax, 10";
asm_code << "mov qword ptr [" << (uint64_t)x_addr << "], rax";

// これは動作するが、生成されたコードは再利用不可能
```

**アプローチ3: 関数呼び出し規約を使う（最も現実的）**
```cpp
// Cbの変数を引数として渡す
extern "C" int64_t inline_asm_wrapper(int64_t x) {
    int64_t result;
    __asm__ volatile(
        "mov %1, %%rax\n"
        "add $10, %%rax\n"
        "mov %%rax, %0\n"
        : "=r"(result)
        : "r"(x)
        : "rax"
    );
    return result;
}
```

しかし、これは**コンパイル時**にしか書けない！

#### 実装可能な軽量アプローチ：AsmJit

**AsmJit**は比較的軽量なJITライブラリです（LLVM より小さい）：

```cpp
#include <asmjit/asmjit.h>

int64_t execute_inline_asm(const std::string& asm_code,
                           const std::map<std::string, int64_t>& vars) {
    using namespace asmjit;

    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());

    x86::Assembler a(&code);

    // {x}を実際の値に置換してアセンブリを生成
    // 例: "mov eax, {x}" → "mov eax, 42"
    std::string resolved = resolve_variables(asm_code, vars);

    // アセンブリをパース（これが難しい！）
    parse_and_emit(a, resolved);

    // 関数として実行
    typedef int64_t (*Func)();
    Func func;
    rt.add(&func, &code);

    int64_t result = func();
    rt.release(func);

    return result;
}
```

**問題点**:
- ❌ アセンブリのパーサーを自分で書く必要がある
- ❌ x86-64専用（ARMなど他のアーキテクチャは別実装）
- ❌ デバッグが非常に困難
- ❌ 依存関係の追加（AsmJit）

#### より現実的な代替案：「疑似アセンブリ」

**アイデア**: 本物のアセンブリではなく、アセンブリ風の命令セットを定義

```cb
// 疑似アセンブリ（実際のx86アセンブリではない）
void main() {
    int x = 42;
    int result;

    // Cb専用の「アセンブリ風」構文
    asm {
        mov(result, x);      // result = x
        add(result, 10);     // result += 10
        mul(result, 2);      // result *= 2
    }

    println(result);  // 104
}
```

**実装**:
```cpp
// 疑似アセンブリインタプリタ
struct AsmInterpreter {
    std::map<std::string, int64_t>& variables;

    void execute(const ASTNode* asm_block) {
        for (const auto& instruction : asm_block->statements) {
            if (instruction->name == "mov") {
                int64_t src = get_value(instruction->args[1]);
                set_value(instruction->args[0], src);
            } else if (instruction->name == "add") {
                int64_t val = get_value(instruction->args[0]);
                int64_t operand = get_value(instruction->args[1]);
                set_value(instruction->args[0], val + operand);
            }
            // ...
        }
    }
};
```

**メリット**:
- ✅ 実装が簡単
- ✅ ポータブル（すべてのプラットフォームで動作）
- ✅ デバッグしやすい
- ✅ Cbの変数と自然に統合

**デメリット**:
- ❌ 本物のアセンブリではない
- ❌ パフォーマンスは通常のCbコードと同じ
- ❌ 「アセンブリを使う」という目的を達成できない

#### 推奨度: ⚠️ **実装困難**

**結論**: インラインアセンブリは、以下の理由から**v0.13.0での実装は推奨しません**:

1. **インタプリタベースのアーキテクチャとの不整合**
   - 変数とレジスタの橋渡しが複雑
   - 実行可能メモリの管理が困難

2. **巨大な依存関係（LLVMなど）またはプラットフォーム依存（AsmJit）**
   - LLVM: 数百MB、ビルドに数時間
   - AsmJit: 軽量だがx86-64専用

3. **プラットフォーム依存性が高い**
   - x86-64, ARM, RISC-Vで完全に異なる実装

4. **実装コストとメリットのバランスが悪い**
   - 数週間〜数ヶ月の開発時間
   - 実際の用途は限定的（ほとんどのユーザーは使わない）

5. **デバッグの困難さ**
   - アセンブリのバグは極めて見つけにくい
   - セグメンテーション違反を起こしやすい

**より現実的な選択肢**:
- ✅ FFI（外部関数呼び出し）の方が100倍実用的
- ✅ 必要なら、アセンブリをC++で書いて共有ライブラリにし、FFI経由で呼び出す

### 2. インラインC++ (`cpp("")`)

#### 課題

1. **動的コンパイルの必要性**
   - C++コードをその場でコンパイルして実行する必要がある
   - コンパイルのオーバーヘッドが大きい

2. **変数スコープの共有**
   - Cbの変数をC++コードから参照できるようにする必要がある
   - 型情報の変換が必要

3. **コンパイラ依存性**
   - C++コンパイラ（g++, clang++）が必要

#### 実装アプローチ

##### アプローチA: 動的コンパイル（共有ライブラリ）

**概要**: C++コードをコンパイルして共有ライブラリを生成し、動的にロード

```cpp
int64_t execute_inline_cpp(const std::string& cpp_code,
                           const std::map<std::string, int64_t>& variables) {
    // C++コードを生成
    std::string full_code = generate_wrapper(cpp_code, variables);

    // 一時ファイルに保存
    write_temp_file("temp.cpp", full_code);

    // コンパイル
    system("g++ -shared -fPIC temp.cpp -o temp.so");

    // ロード
    void* handle = dlopen("temp.so", RTLD_NOW);
    auto func = (int64_t(*)(int64_t*))dlsym(handle, "inline_cpp");

    // 実行
    int64_t args[] = {variables["x"], variables["y"], ...};
    return func(args);
}
```

ラッパーコードの生成例:
```cpp
extern "C" int64_t inline_cpp(int64_t* args) {
    int64_t x = args[0];
    int64_t y = args[1];

    // ユーザーが書いたコード
    {cpp_code}

    return result;
}
```

**長所**:
- ✅ 完全なC++機能を利用可能
- ✅ 標準ライブラリが使える
- ✅ 最適化されたコードを実行

**短所**:
- ❌ **コンパイル時間**: 数秒のオーバーヘッド
- ❌ **一時ファイル**: 管理が複雑
- ❌ **コンパイラ依存**: g++/clang++が必要

##### アプローチB: Cling (C++ JIT)

**概要**: ClingというC++ REPL/JITを使用

```cpp
#include <cling/Interpreter/Interpreter.h>

int64_t execute_inline_cpp(const std::string& cpp_code) {
    cling::Interpreter interp;

    // 変数を登録
    interp.declare("int x = 10;");

    // コードを実行
    cling::Value result;
    interp.process(cpp_code, &result);

    return result.getAs<int64_t>();
}
```

**長所**:
- ✅ インタラクティブなC++実行
- ✅ 高速なJITコンパイル
- ✅ LLVM技術を活用

**短所**:
- ❌ **巨大な依存**: Cling + LLVM（数百MB）
- ❌ **ビルド時間**: LLVMのビルドに数時間
- ❌ **メモリ消費**: JITエンジンのオーバーヘッド

##### アプローチC: プリコンパイル済みC++関数の呼び出し（FFI）

**概要**: あらかじめC++関数をコンパイルしておき、FFI経由で呼び出す

Cbコード:
```cb
// C++関数を宣言（FFI）
extern "C++" int my_cpp_function(int x, int y);

void main() {
    int result = my_cpp_function(10, 20);
    println(result);
}
```

C++側:
```cpp
// my_lib.cpp
extern "C" int my_cpp_function(int x, int y) {
    return x + y * 2;
}
```

コンパイル:
```bash
g++ -shared -fPIC my_lib.cpp -o libmy_lib.so
```

**長所**:
- ✅ **実行時オーバーヘッドなし**: コンパイル済み
- ✅ **シンプル**: 既存のFFI技術を使用
- ✅ **型安全**: C++の型システムを利用

**短所**:
- ❌ 事前コンパイルが必要（動的ではない）
- ❌ 別途C++ファイルを管理

#### 推奨度: ⚠️ **条件付きで実装可能**

インラインC++は、以下の条件下で実装可能です:

1. **動的コンパイル方式**: 実装可能だが、コンパイル時間のオーバーヘッドが大きい
2. **FFI方式**: より実用的だが、"インライン"ではない

### 3. インラインC++（生の構文版）

#### 課題

1. **構文の曖昧性**
   - `cpp { ... }` ブロック内の構文をどう解釈するか
   - C++とCbの構文が似ているが完全には互換性がない

2. **パーサーの複雑化**
   - C++の完全なパーサーを実装する必要がある
   - C++の構文は非常に複雑（テンプレート、ラムダなど）

#### 実装アプローチ

##### アプローチA: C++パーサーを組み込む

**概要**: Clangのパーサーを使用してC++ブロックを解析

```cpp
#include <clang/AST/AST.h>

void parse_cpp_block(const std::string& cpp_code) {
    clang::Parser parser;
    clang::ASTContext* ast = parser.parse(cpp_code);

    // C++のASTを処理
    ...
}
```

**長所**:
- ✅ 完全なC++構文をサポート

**短所**:
- ❌ **巨大な依存**: Clang全体（数百MB）
- ❌ **極めて複雑**: C++の完全なパースは非常に難しい

##### アプローチB: 簡易C++サブセット

**概要**: C++の簡単なサブセットのみをサポート

```cb
cpp {
    // 単純な式のみサポート
    x = x * 2;
    y = y + 10;
}
```

**長所**:
- ✅ 実装が比較的簡単

**短所**:
- ❌ **機能制限**: 限られたC++機能のみ
- ❌ **混乱**: "C++"と名乗っているが実際は違う

#### 推奨度: ❌ **実装非推奨**

生の構文版は、以下の理由から**実装を推奨しません**:

1. C++パーサーの実装が極めて複雑
2. サブセット版では"C++"としての価値が低い
3. 文字列版 `cpp("")` で十分

## 代替案：より実用的なアプローチ

### 提案1: FFI (Foreign Function Interface)

**より実用的な機能**: C/C++の外部関数を呼び出す機能

#### モダンな構文案

##### 案A: `@foreign` アノテーション（Rust/Swift風）

```cb
// @foreignアノテーションで外部関数を宣言
@foreign("libmath.so")
int add(int a, int b);

@foreign("libmath.so")
double sqrt(double x);

void main() {
    int result = add(10, 20);
    println(result);  // 30

    double s = sqrt(16.0);
    println(s);  // 4.0
}
```

**特徴**:
- ✅ モダンで読みやすい
- ✅ ライブラリ名が明示的
- ✅ 複数の関数を同じライブラリから読み込める

##### 案B: `foreign` キーワード（Zig風）

```cb
// foreignブロックでライブラリ全体をインポート
foreign "libmath.so" {
    int add(int a, int b);
    double sqrt(double x);
    void* malloc(int size);
}

void main() {
    int result = add(10, 20);
    println(result);
}
```

**特徴**:
- ✅ ブロック構文で整理しやすい
- ✅ 同じライブラリの関数をグループ化
- ✅ Cbらしいブロック構文

##### 案C: `use` 構文（Rust風、推奨）⭐

```cb
// useで外部ライブラリをインポート
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
    void* malloc(int size);
}

void main() {
    int result = add(10, 20);
    println(result);
}
```

**特徴**:
- ✅ Rustライクで型安全
- ✅ 明示的な戻り値型
- ✅ 個別インポートも可能
- ✅ Cbの既存のuse構文と統一感

##### 案D: `import` 拡張（Python/JavaScript風）

```cb
// importでネイティブライブラリを読み込み
import native "libmath.so" as math;

void main() {
    int result = math.add(10, 20);
    println(result);

    double s = math.sqrt(16.0);
    println(s);
}
```

**特徴**:
- ✅ 名前空間が明確
- ✅ 衝突を避けられる
- ✅ JavaScript/Pythonライク

##### 案E: 型安全な自動FFI（最もモダン）⭐⭐

```cb
// 型情報付きで外部ライブラリを宣言
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
    void* malloc(int size);
}

void main() {
    int result = add(10, 20);  // 型チェック済み
    println(result);

    // コンパイル時エラー：型が一致しない
    // int wrong = add("hello", 20);  // エラー！
}
```

**特徴**:
- ✅ 完全な型安全性
- ✅ コンパイル時の型チェック
- ✅ TypeScriptライクな型注釈
- ✅ 最もモダンで安全

#### 実装方法（全案共通）

```cpp
// src/backend/interpreter/evaluator/functions/ffi.cpp
int64_t call_external_function(const std::string& lib_path,
                                const std::string& func_name,
                                const std::vector<int64_t>& args) {
    // ライブラリをキャッシュ
    static std::map<std::string, void*> lib_cache;

    void* handle = lib_cache[lib_path];
    if (!handle) {
        handle = dlopen(lib_path.c_str(), RTLD_LAZY);
        if (!handle) {
            throw std::runtime_error("Cannot load library: " +
                                     std::string(dlerror()));
        }
        lib_cache[lib_path] = handle;
    }

    // 関数を取得
    auto func = (int64_t(*)(int64_t, int64_t))dlsym(handle, func_name.c_str());
    if (!func) {
        throw std::runtime_error("Cannot find symbol: " + func_name);
    }

    return func(args[0], args[1]);
}
```

#### 推奨構文：案C (`use foreign`) + 案E (型安全)

**最終推奨構文**:
```cb
// v0.13.0推奨：use foreignでモジュールを宣言
use foreign.m {
    int add(int a, int b);
    double sqrt(double x);
    void* malloc(int size);
}

void main() {
    int result = add(10, 20);
    println(result);
}
```

または、個別に宣言：
```cb
// 関数シグネチャを明示
use foreign.m {
    int add(int a, int b);
}

use foreign.m {
    double sqrt(double x);
}

void main() {
    int result = add(10, 20);
    double s = sqrt(16.0);
    println(result, s);
}
```

**メリット**:
- ✅ モダンで読みやすい
- ✅ 型安全性が高い
- ✅ 既存のC/C++ライブラリを活用できる
- ✅ 実装が比較的簡単
- ✅ 実行時オーバーヘッドが小さい
- ✅ Cbの既存構文（use, import）と統一感がある

### 提案2: プラグインシステム

**概要**: C++で書かれたプラグインを動的にロード

```cb
// プラグインをロード
plugin MyPlugin from "libmyplugin.so";

void main() {
    MyPlugin.initialize();
    int result = MyPlugin.compute(42);
    println(result);
}
```

C++プラグイン:
```cpp
// myplugin.cpp
extern "C" {
    void initialize() {
        std::cout << "Plugin initialized" << std::endl;
    }

    int64_t compute(int64_t x) {
        return x * 2;
    }
}
```

**メリット**:
- ✅ 拡張性が高い
- ✅ モジュール化されたアーキテクチャ
- ✅ C++の完全な機能を利用可能

### 提案3: システムコマンド実行

**概要**: 外部コマンドを実行する機能（シェルスクリプトのように）

```cb
void main() {
    string output = exec("ls -la");
    println(output);

    int status = system("gcc -o myprogram myprogram.c");
    println("Exit status:", status);
}
```

**メリット**:
- ✅ 実装が非常に簡単
- ✅ 外部ツールを活用できる
- ✅ 実用性が高い

## 推奨実装プラン (v0.13.0)

### フェーズ1: FFI実装（優先度: 高）

1. **extern "C" 関数宣言**
   ```cb
   extern "C" int my_function(int x);
   ```

2. **dlopen/dlsymによる動的ロード**
   ```cpp
   void* handle = dlopen("libmylib.so", RTLD_LAZY);
   auto func = dlsym(handle, "my_function");
   ```

3. **基本的な型変換**
   - int, long, double, string の引数/戻り値

### フェーズ2: システムコマンド実行（優先度: 中）

1. **system() 関数**
   ```cb
   int status = system("ls -la");
   ```

2. **exec() 関数（出力キャプチャ）**
   ```cb
   string output = exec("git status");
   ```

### フェーズ3: プラグインシステム（優先度: 低）

1. **plugin構文**
   ```cb
   plugin MyPlugin from "libmyplugin.so";
   ```

2. **プラグインレジストリ**
3. **型安全なFFI**

### インラインasm/cpp: 将来的な検討事項

**結論**: `asm("")` と `cpp("")` は、**v0.13.0では実装しない**

理由:
1. 技術的複雑性が高すぎる
2. 依存関係が巨大（LLVM, Cling）
3. インタプリタとの相性が悪い
4. より実用的な代替案（FFI）がある

**将来的な実装の可能性**:
- **v1.0.0（ネイティブコンパイラ）**: インラインアセンブリの実装を検討
  - C++のように実行ファイルを生成
  - `asm("")` 構文がコンパイラなら比較的容易に実装可能
  - GCCやClangと同様の実装パターンが使える

**注**: JITコンパイラは実装しない
- JITは複雑で巨大な依存関係（LLVM等）が必要
- インタプリタ → ネイティブコンパイラへ直接移行

## 技術的な実装詳細（参考資料）

### dlopen/dlsym の使用方法

```cpp
#include <dlfcn.h>

// 共有ライブラリをロード
void* handle = dlopen("libexample.so", RTLD_LAZY);
if (!handle) {
    std::cerr << "Cannot load library: " << dlerror() << std::endl;
    return;
}

// 関数を取得
typedef int (*func_t)(int, int);
func_t func = (func_t)dlsym(handle, "my_function");
if (!func) {
    std::cerr << "Cannot load symbol: " << dlerror() << std::endl;
    dlclose(handle);
    return;
}

// 関数を呼び出し
int result = func(10, 20);

// ライブラリをアンロード
dlclose(handle);
```

### プラットフォーム別の対応

| プラットフォーム | 動的ロード API | 共有ライブラリ拡張子 |
|-----------------|---------------|-------------------|
| Linux           | dlopen/dlsym  | .so              |
| macOS           | dlopen/dlsym  | .dylib           |
| Windows         | LoadLibrary/GetProcAddress | .dll |

### セキュリティ考慮事項

1. **任意コード実行のリスク**
   - FFIで任意のライブラリをロードできるため、セキュリティリスクがある
   - サンドボックス化を検討

2. **パス検証**
   - ライブラリパスの検証を行う
   - 絶対パス、相対パスのチェック

3. **シンボル検証**
   - 関数シグネチャの検証
   - 型安全性の確保

## 実装タイムライン (v0.13.0)

### Week 1-2: FFI基盤実装
- [ ] dlopen/dlsym ラッパーの実装
- [ ] extern "C" 構文のパース
- [ ] 基本的な型変換（int, long, double）

### Week 3-4: FFI拡張機能
- [ ] 文字列型のサポート
- [ ] 構造体の受け渡し
- [ ] ポインタ型のサポート

### Week 5-6: システムコマンド実行
- [ ] system() 関数の実装
- [ ] exec() 関数の実装（出力キャプチャ）

### Week 7-8: テストとドキュメント
- [ ] 統合テストの作成
- [ ] サンプルコードの作成
- [ ] ドキュメントの整備

## まとめ

### 実装推奨度

| 機能 | 推奨度 | 理由 |
|-----|-------|------|
| FFI (Foreign Function Interface) | ⭐⭐⭐⭐⭐ | 実用的、実装容易、高い拡張性 |
| システムコマンド実行 | ⭐⭐⭐⭐ | 実用的、実装容易 |
| プラグインシステム | ⭐⭐⭐ | 拡張性高いが、実装複雑 |
| インラインC++ (`cpp("")`) | ⭐⭐ | 実装可能だが、オーバーヘッド大 |
| インラインアセンブリ (`asm("")`) | ⭐ | 実装困難、メリット少ない |

### v0.13.0 での実装内容

**実装する**:
- ✅ FFI (Foreign Function Interface)
- ✅ システムコマンド実行 (system, exec)

**実装しない**:
- ❌ インラインアセンブリ (`asm("")`)
- ❌ インラインC++ (`cpp("")`)

**理由**:
- FFIの方が実用的で、実装も容易
- インラインasm/cppは技術的複雑性が高すぎる
- 将来的にJITコンパイラを導入する際に再検討

### 次のステップ

1. **FFI実装の詳細設計**
   - 構文設計
   - 型システム設計
   - エラーハンドリング

2. **プロトタイプ実装**
   - dlopen/dlsym の基本実装
   - シンプルなテストケース

3. **ユーザーフィードバック**
   - FFI機能で十分か確認
   - インラインasm/cppの実需要を調査

## 参考資料

- [LLVM JIT Documentation](https://llvm.org/docs/tutorial/BuildingAJIT1.html)
- [Cling - C++ Interpreter](https://github.com/root-project/cling)
- [dlopen man page](https://man7.org/linux/man-pages/man3/dlopen.3.html)
- [FFI in Programming Languages](https://en.wikipedia.org/wiki/Foreign_function_interface)
