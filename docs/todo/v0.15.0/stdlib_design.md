# 標準ライブラリ設計書

**バージョン**: v0.17.0
**作成日**: 2025-11-14
**ステータス**: 設計中

---

## 目次

1. [概要](#1-概要)
2. [FFI実装](#2-ffi実装)
3. [条件付きコンパイル](#3-条件付きコンパイル)
4. [モジュールシステム](#4-モジュールシステム)
5. [標準ライブラリ構造](#5-標準ライブラリ構造)
6. [プラットフォーム固有実装](#6-プラットフォーム固有実装)

---

## 1. 概要

### 1.1 v0.16.0で不足している機能

v0.17.0の標準ライブラリ化を実現するために、v0.16.0に以下の機能を追加する必要があります：

**不足している機能**:
- ✗ FFI (Foreign Function Interface)
- ✗ 条件付きコンパイル (#[cfg(...)])
- ✗ モジュールシステム (mod/use)
- ✗ extern "C" 宣言
- ✗ リンク制御属性 (#[link_name], #[link])
- ✗ 弱いシンボル (#[weak])

これらの機能をv0.16.0の実装計画に統合します。

---

## 2. FFI実装

### 2.1 FFI構文

#### extern "C" ブロック

```cb
// C関数の宣言
extern "C" {
    void* malloc(long size);
    void free(void* ptr);
    long write(int fd, char* buf, long count);
    long read(int fd, char* buf, long count);
}

// 単一の関数宣言
extern "C" int printf(char* format, ...);
```

#### Cb関数をCから呼び出し可能にする

```cb
// C互換の関数
#[no_mangle]
extern "C" int cb_add(int a, int b) {
    return a + b;
}
```

### 2.2 ASTノードの追加

#### ファイル: `src/common/ast.h` に追加

```cpp
// AST_EXTERN_BLOCK ノード
struct ASTExternBlock {
    std::string abi;                        // "C", "system", "stdcall"等
    std::vector<std::unique_ptr<ASTNode>> declarations;
};

// ASTNode に追加
struct ASTNode {
    // 既存のフィールド...

    // FFI用
    std::unique_ptr<ASTExternBlock> extern_block;
    std::string abi_spec;                   // 呼び出し規約
    bool is_extern = false;                 // extern関数かどうか
};
```

### 2.3 呼び出し規約

```cpp
// src/backend/codegen/common/calling_convention.h

enum class CallingConvention {
    // 既存...
    C,              // C calling convention (cdecl)
    System,         // システムデフォルト
    Stdcall,        // Windows stdcall
    Fastcall,       // x86 fastcall
    Vectorcall,     // x86 vectorcall
    Win64,          // Windows x64
    SysV64,         // System V AMD64 ABI
    AAPCS,          // ARM AAPCS
};

struct FunctionSignature {
    std::string name;
    CallingConvention calling_convention;
    std::vector<TypeInfo> param_types;
    TypeInfo return_type;
    bool is_variadic;                       // 可変長引数
};
```

### 2.4 C型とCb型のマッピング

```cpp
// src/backend/ir/common/c_abi.h

class CABIMapper {
public:
    // Cb型からC型へのマッピング
    static CType map_to_c_type(TypeInfo cb_type);

    // C型からCb型へのマッピング
    static TypeInfo map_from_c_type(CType c_type);

    // サイズとアライメントの計算
    static size_t get_c_type_size(CType type);
    static size_t get_c_type_alignment(CType type);
};

// 型マッピング例
// Cb型 -> C型
// int   -> int32_t
// long  -> int64_t
// usize -> size_t
// char* -> char*
// void* -> void*
```

### 2.5 HIR/MIR/LIRでの表現

#### HIRレベル

```cpp
// src/backend/ir/hir/hir_node.h に追加

struct HIRExternFunction {
    std::string name;
    std::string abi;                        // "C", "system"等
    std::vector<TypeInfo> param_types;
    TypeInfo return_type;
    bool is_variadic;
    std::string link_name;                  // #[link_name]で指定された名前
};

struct HIRFunctionCall {
    // 既存のフィールド...
    bool is_extern_call;                    // extern関数呼び出しか
    std::string abi;                        // 呼び出し規約
};
```

#### MIRレベル

```cpp
// src/backend/ir/mir/mir_node.h に追加

struct MIRTerminator {
    // 既存のフィールド...

    // Call用の追加情報
    std::optional<CallingConvention> calling_convention;
    bool is_extern_call;
};
```

### 2.6 コード生成

#### x86-64 (System V ABI)

```cpp
// src/backend/codegen/native/x86_64_codegen.cpp

void X86_64Codegen::emit_extern_call(
    const std::string& func_name,
    const std::vector<MIROperand>& args,
    CallingConvention cc
) {
    // 引数をレジスタ/スタックに配置
    // System V AMD64: rdi, rsi, rdx, rcx, r8, r9
    const std::vector<std::string> arg_regs = {
        "rdi", "rsi", "rdx", "rcx", "r8", "r9"
    };

    for (size_t i = 0; i < args.size(); ++i) {
        if (i < arg_regs.size()) {
            // レジスタ渡し
            emit_move(arg_regs[i], args[i]);
        } else {
            // スタック渡し
            emit_push(args[i]);
        }
    }

    // 関数呼び出し
    output << "    call " << func_name << "@PLT\n";

    // スタッククリーンアップ（必要に応じて）
    if (args.size() > arg_regs.size()) {
        size_t stack_size = (args.size() - arg_regs.size()) * 8;
        output << "    add rsp, " << stack_size << "\n";
    }
}
```

#### ARM (AAPCS)

```cpp
void ARMCodegen::emit_extern_call(
    const std::string& func_name,
    const std::vector<MIROperand>& args,
    CallingConvention cc
) {
    // AAPCS: r0-r3が引数レジスタ
    const std::vector<std::string> arg_regs = {
        "r0", "r1", "r2", "r3"
    };

    for (size_t i = 0; i < args.size() && i < 4; ++i) {
        emit_move(arg_regs[i], args[i]);
    }

    // 5つ目以降の引数はスタックへ
    for (size_t i = 4; i < args.size(); ++i) {
        emit_push(args[i]);
    }

    // 関数呼び出し
    output << "    bl " << func_name << "\n";
}
```

---

## 3. 条件付きコンパイル

### 3.1 #[cfg(...)] 属性

#### 構文

```cb
// OSによる条件分岐
#[cfg(target_os = "linux")]
void platform_specific() {
    // Linux固有の実装
}

#[cfg(target_os = "windows")]
void platform_specific() {
    // Windows固有の実装
}

// アーキテクチャによる条件分岐
#[cfg(target_arch = "x86_64")]
void arch_specific() {
    // x86-64固有の実装
}

#[cfg(target_arch = "arm")]
void arch_specific() {
    // ARM固有の実装
}

// 複数条件の組み合わせ
#[cfg(all(target_os = "linux", target_arch = "x86_64"))]
void linux_x64_specific() {
    // Linux x86-64固有の実装
}

#[cfg(any(target_os = "linux", target_os = "macos"))]
void unix_like() {
    // Unix系OS共通の実装
}

#[cfg(not(target_os = "windows"))]
void not_windows() {
    // Windows以外の実装
}
```

### 3.2 条件付きコンパイルの種類

```cb
// ターゲットOS
#[cfg(target_os = "linux")]
#[cfg(target_os = "macos")]
#[cfg(target_os = "windows")]
#[cfg(target_os = "freebsd")]
#[cfg(target_os = "none")]        // ベアメタル

// ターゲットアーキテクチャ
#[cfg(target_arch = "x86_64")]
#[cfg(target_arch = "x86")]
#[cfg(target_arch = "arm")]
#[cfg(target_arch = "aarch64")]
#[cfg(target_arch = "riscv64")]

// ポインタサイズ
#[cfg(target_pointer_width = "32")]
#[cfg(target_pointer_width = "64")]

// エンディアン
#[cfg(target_endian = "little")]
#[cfg(target_endian = "big")]

// 実行環境
#[cfg(target_env = "gnu")]
#[cfg(target_env = "msvc")]
#[cfg(target_env = "musl")]

// カスタムフィーチャー
#[cfg(feature = "std")]
#[cfg(feature = "alloc")]
#[cfg(feature = "no_std")]
```

### 3.3 ASTとIRでの表現

```cpp
// ASTNode に条件属性を追加
struct ConfigAttribute {
    enum class Kind {
        TargetOS,
        TargetArch,
        TargetPointerWidth,
        TargetEndian,
        TargetEnv,
        Feature,
        All,        // all(...)
        Any,        // any(...)
        Not,        // not(...)
    };

    Kind kind;
    std::string value;
    std::vector<ConfigAttribute> children;  // all/any/not用
};

struct ASTNode {
    // 既存...
    std::vector<ConfigAttribute> cfg_attributes;
};
```

### 3.4 条件評価

```cpp
// src/frontend/cfg_evaluator.h

class CfgEvaluator {
public:
    explicit CfgEvaluator(const TargetInfo& target);

    // 条件属性が現在のターゲットで有効かチェック
    bool evaluate(const ConfigAttribute& cfg);

private:
    TargetInfo target;
    std::unordered_set<std::string> enabled_features;

    bool check_target_os(const std::string& os);
    bool check_target_arch(const std::string& arch);
    bool check_pointer_width(const std::string& width);
    bool check_endian(const std::string& endian);
    bool check_feature(const std::string& feature);
};
```

### 3.5 AST段階での条件分岐処理

```cpp
// src/frontend/ast_conditional_filter.h

class ASTConditionalFilter {
public:
    explicit ASTConditionalFilter(const CfgEvaluator& evaluator);

    // 条件に応じてASTノードをフィルタリング
    std::unique_ptr<ASTNode> filter(std::unique_ptr<ASTNode> node);

private:
    const CfgEvaluator& evaluator;

    bool should_include(const ASTNode* node);
};
```

---

## 4. モジュールシステム

### 4.1 モジュール構文

#### モジュールの定義

```cb
// math.cb
mod math {
    export int add(int a, int b) {
        return a + b;
    }

    int internal_helper() {
        // プライベート関数（外部から見えない）
        return 42;
    }
}
```

#### モジュールのインポート

```cb
// main.cb
use math::add;
use math::*;  // 全てをインポート

void main() {
    int result = add(1, 2);
    println("Result: %d", result);
}
```

#### ネストしたモジュール

```cb
mod std {
    export mod io {
        export void println(string s) {
            // 実装
        }
    }

    export mod mem {
        export void* alloc(long size) {
            // 実装
        }
    }
}

// 使用例
use std::io::println;
use std::mem::alloc;
```

#### ファイル分割

```
src/
├── main.cb
├── math.cb
└── io.cb
```

```cb
// main.cb
mod math;  // math.cbを読み込む
mod io;    // io.cbを読み込む

use math::add;
use io::println;

void main() {
    println("Hello");
}
```

### 4.2 ASTノードの追加

```cpp
// AST_MODULE_DECL ノード
struct ASTModuleDecl {
    std::string name;
    std::vector<std::unique_ptr<ASTNode>> items;
    bool is_public;
    std::string file_path;                  // ファイルから読み込む場合
};

// AST_USE_DECL ノード
struct ASTUseDecl {
    std::vector<std::string> path;          // モジュールパス
    std::string alias;                      // エイリアス（オプション）
    bool is_glob;                           // use math::*; のような全インポート
};

// ASTNode に追加
struct ASTNode {
    // 既存...
    std::unique_ptr<ASTModuleDecl> module_decl;
    std::unique_ptr<ASTUseDecl> use_decl;
};
```

### 4.3 モジュール解決

```cpp
// src/frontend/module_resolver.h

class ModuleResolver {
public:
    explicit ModuleResolver(const std::string& project_root);

    // モジュールパスからファイルパスを解決
    std::optional<std::string> resolve_module_path(
        const std::vector<std::string>& module_path
    );

    // useパスからシンボルを解決
    Symbol* resolve_use_path(
        const std::vector<std::string>& use_path,
        const std::string& symbol_name
    );

    // モジュールをロード
    std::unique_ptr<ASTNode> load_module(const std::string& file_path);

private:
    std::string project_root;
    std::unordered_map<std::string, std::unique_ptr<ASTNode>> loaded_modules;
};
```

### 4.4 可視性制御

```cb
mod foo {
    // パブリック（外部から使用可能）
    export void public_func() {}

    // プライベート（モジュール内のみ）
    void private_func() {}

    export struct PublicStruct {
        export int public_field;      // パブリックフィールド
        int private_field;             // プライベートフィールド
    };
}

// main.cb
use foo::public_func;     // OK
use foo::private_func;    // エラー：プライベート
```

### 4.5 標準ライブラリのprelude

```cb
// 自動的にインポートされるモジュール
mod std {
    export mod prelude {
        export use std::io::{print, println};
        export use std::mem::{alloc, free};
        export use std::string::String;
        export use std::vec::Vec;
    }
}

// 全てのファイルに自動的に追加される
// use std::prelude::*;
```

---

## 5. 標準ライブラリ構造

### 5.1 ディレクトリ構造

```
std/
├── lib.cb                    # 標準ライブラリのルート
├── prelude.cb                # 自動インポートされるシンボル
├── io/
│   ├── mod.cb
│   ├── stdio.cb              # 標準入出力
│   ├── file.cb               # ファイルIO
│   └── buffered.cb           # バッファ付きIO
├── mem/
│   ├── mod.cb
│   ├── alloc.cb              # メモリアロケーター
│   └── ptr.cb                # ポインタ操作
├── string/
│   ├── mod.cb
│   └── string.cb             # 文字列型
├── collections/
│   ├── mod.cb
│   ├── vec.cb                # 動的配列
│   ├── map.cb                # ハッシュマップ
│   └── set.cb                # セット
├── sys/
│   ├── mod.cb
│   ├── unix/                 # Unix系OS
│   │   ├── mod.cb
│   │   ├── linux.cb
│   │   └── macos.cb
│   ├── windows/              # Windows
│   │   └── mod.cb
│   └── none/                 # ベアメタル
│       └── mod.cb
└── runtime/
    ├── mod.cb
    ├── panic.cb              # パニックハンドラ
    └── start.cb              # エントリーポイント
```

### 5.2 std::io::stdio.cb

```cb
// std/io/stdio.cb

mod std::io {
    use std::sys;

    // 標準出力への書き込み
    export void print(string s) {
        sys::write(1, s, strlen(s));
    }

    // 標準出力への書き込み（改行付き）
    export void println(string s) {
        print(s);
        print("\n");
    }

    // 標準エラー出力への書き込み
    export void eprint(string s) {
        sys::write(2, s, strlen(s));
    }

    export void eprintln(string s) {
        eprint(s);
        eprint("\n");
    }

    // 標準入力から読み取り
    export string read_line() {
        char buf[256];
        long n = sys::read(0, buf, 256);
        return string_from_bytes(buf, n);
    }
}
```

### 5.3 std::mem::alloc.cb

```cb
// std/mem/alloc.cb

mod std::mem {
    use std::sys;

    // メモリ割り当て
    export void* alloc(long size) {
        #[cfg(target_os = "linux")]
        return sys::linux::malloc(size);

        #[cfg(target_os = "macos")]
        return sys::macos::malloc(size);

        #[cfg(target_os = "windows")]
        return sys::windows::HeapAlloc(size);

        #[cfg(target_os = "none")]
        return sys::none::simple_alloc(size);
    }

    // メモリ解放
    export void free(void* ptr) {
        #[cfg(target_os = "linux")]
        sys::linux::free(ptr);

        #[cfg(target_os = "macos")]
        sys::macos::free(ptr);

        #[cfg(target_os = "windows")]
        sys::windows::HeapFree(ptr);

        #[cfg(target_os = "none")]
        sys::none::simple_free(ptr);
    }

    // 再割り当て
    export void* realloc(void* ptr, long new_size) {
        #[cfg(any(target_os = "linux", target_os = "macos"))]
        return sys::unix::realloc(ptr, new_size);

        #[cfg(target_os = "windows")]
        return sys::windows::HeapReAlloc(ptr, new_size);

        #[cfg(target_os = "none")]
        return sys::none::simple_realloc(ptr, new_size);
    }
}
```

---

## 6. プラットフォーム固有実装

### 6.1 Linux実装（std/sys/unix/linux.cb）

```cb
// std/sys/unix/linux.cb

mod std::sys::linux {
    // システムコール番号
    const SYS_READ: long = 0;
    const SYS_WRITE: long = 1;
    const SYS_OPEN: long = 2;
    const SYS_CLOSE: long = 3;
    const SYS_BRK: long = 12;

    // システムコールラッパー（インラインアセンブラ使用）
    export long write(int fd, char* buf, long count) {
        long result;
        asm volatile (
            "mov $1, %rax\n"       // SYS_WRITE
            "mov %0, %rdi\n"       // fd
            "mov %1, %rsi\n"       // buf
            "mov %2, %rdx\n"       // count
            "syscall"
            : "=r"(result)
            : "r"(fd), "r"(buf), "r"(count)
            : "rax", "rdi", "rsi", "rdx", "r11", "rcx", "memory"
        );
        return result;
    }

    export long read(int fd, char* buf, long count) {
        long result;
        asm volatile (
            "mov $0, %rax\n"       // SYS_READ
            "mov %0, %rdi\n"
            "mov %1, %rsi\n"
            "mov %2, %rdx\n"
            "syscall"
            : "=r"(result)
            : "r"(fd), "r"(buf), "r"(count)
            : "rax", "rdi", "rsi", "rdx", "r11", "rcx", "memory"
        );
        return result;
    }

    // C標準ライブラリのラッパー（FFI使用）
    extern "C" {
        export void* malloc(long size);
        export void free(void* ptr);
        export void* realloc(void* ptr, long size);
    }
}
```

### 6.2 macOS実装（std/sys/unix/macos.cb）

```cb
// std/sys/unix/macos.cb

mod std::sys::macos {
    // macOSのシステムコール番号（BSDスタイル）
    const SYS_READ: long = 3;
    const SYS_WRITE: long = 4;
    const SYS_OPEN: long = 5;
    const SYS_CLOSE: long = 6;

    export long write(int fd, char* buf, long count) {
        long result;
        asm volatile (
            "mov $0x2000004, %rax\n"  // SYS_WRITE (BSD)
            "mov %0, %rdi\n"
            "mov %1, %rsi\n"
            "mov %2, %rdx\n"
            "syscall"
            : "=r"(result)
            : "r"(fd), "r"(buf), "r"(count)
            : "rax", "rdi", "rsi", "rdx", "r11", "rcx", "memory"
        );
        return result;
    }

    // C標準ライブラリ（macOS）
    extern "C" {
        export void* malloc(long size);
        export void free(void* ptr);
        export void* realloc(void* ptr, long size);
    }
}
```

### 6.3 Windows実装（std/sys/windows/mod.cb）

```cb
// std/sys/windows/mod.cb

mod std::sys::windows {
    // Windows APIの定義
    extern "C" {
        #[link_name = "GetStdHandle"]
        export void* GetStdHandle(int nStdHandle);

        #[link_name = "WriteFile"]
        export int WriteFile(
            void* hFile,
            void* lpBuffer,
            int nNumberOfBytesToWrite,
            int* lpNumberOfBytesWritten,
            void* lpOverlapped
        );

        #[link_name = "ReadFile"]
        export int ReadFile(
            void* hFile,
            void* lpBuffer,
            int nNumberOfBytesToRead,
            int* lpNumberOfBytesRead,
            void* lpOverlapped
        );

        // ヒープメモリ管理
        #[link_name = "HeapAlloc"]
        export void* HeapAlloc(void* hHeap, int dwFlags, long dwBytes);

        #[link_name = "HeapFree"]
        export int HeapFree(void* hHeap, int dwFlags, void* lpMem);

        #[link_name = "GetProcessHeap"]
        export void* GetProcessHeap();
    }

    const STD_OUTPUT_HANDLE: int = -11;
    const STD_ERROR_HANDLE: int = -12;
    const STD_INPUT_HANDLE: int = -10;

    // 標準出力への書き込み
    export long write(int fd, char* buf, long count) {
        void* handle = GetStdHandle(
            fd == 1 ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE
        );
        int written;
        WriteFile(handle, buf as void*, count as int, &written, null);
        return written as long;
    }

    // メモリ割り当て
    export void* malloc(long size) {
        void* heap = GetProcessHeap();
        return HeapAlloc(heap, 0, size);
    }

    export void free(void* ptr) {
        void* heap = GetProcessHeap();
        HeapFree(heap, 0, ptr);
    }
}
```

### 6.4 ベアメタル実装（std/sys/none/mod.cb）

```cb
// std/sys/none/mod.cb

mod std::sys::none {
    // シンプルなバンプアロケーター
    static long HEAP_START = 0;
    static long HEAP_CURRENT = 0;
    static long HEAP_END = 0;

    export void init_allocator(long start, long size) {
        HEAP_START = start;
        HEAP_CURRENT = start;
        HEAP_END = start + size;
    }

    export void* simple_alloc(long size) {
        // アライメント調整
        long aligned = (HEAP_CURRENT + 7) & ~7;

        if (aligned + size > HEAP_END) {
            // メモリ不足
            return null;
        }

        void* ptr = aligned as void*;
        HEAP_CURRENT = aligned + size;
        return ptr;
    }

    export void simple_free(void* ptr) {
        // バンプアロケーターは個別の解放をサポートしない
        // （全体のリセットのみ）
    }

    export void* simple_realloc(void* ptr, long new_size) {
        // 新しいメモリを割り当ててコピー
        void* new_ptr = simple_alloc(new_size);
        if (new_ptr != null) {
            // 旧データをコピー（簡略化）
            memcpy(new_ptr, ptr, new_size);
        }
        return new_ptr;
    }

    // UARTへの出力（ベアメタル用）
    export long write(int fd, char* buf, long count) {
        // プラットフォーム固有のUART実装
        #[cfg(target_arch = "arm")]
        {
            // ARM用UART実装
            volatile int* uart_base = 0x40000000 as volatile int*;
            for (int i = 0; i < count; i++) {
                *uart_base = buf[i] as int;
            }
        }

        #[cfg(target_arch = "x86_64")]
        {
            // x86-64シリアルポート実装
            for (int i = 0; i < count; i++) {
                asm volatile (
                    "out %al, $0x3F8"
                    :
                    : "r"(buf[i] as char)
                    :
                );
            }
        }

        return count as long;
    }
}
```

---

## 7. v0.16.0への統合

### 7.1 追加が必要なタスク

v0.16.0の実装ロードマップに以下のタスクを追加します：

#### Month 3, Week 3: FFIとモジュールシステム（追加）

**実装タスク**:

1. **FFI基盤の実装**（Week 3）
   - [ ] extern "C" 構文のパース
   - [ ] 呼び出し規約の実装
   - [ ] C型マッピング
   - [ ] HIR/MIR/LIRでのFFIサポート
   - [ ] ネイティブバックエンドでのextern呼び出し
   - [ ] ユニットテスト（20テスト）

2. **条件付きコンパイルの実装**（Week 3）
   - [ ] #[cfg(...)] 属性のパース
   - [ ] 条件評価エンジン
   - [ ] AST段階でのフィルタリング
   - [ ] ユニットテスト（15テスト）

3. **モジュールシステムの実装**（Week 4）
   - [ ] mod/use構文のパース
   - [ ] モジュール解決
   - [ ] ファイル分割サポート
   - [ ] 可視性制御（pub）
   - [ ] ユニットテスト（20テスト）

### 7.2 更新されたスケジュール

```
Month 3: LIR実装とツール + FFI/モジュール

Week 1-2: LIR実装（変更なし）
- LIRノード定義
- 命令セット定義
- MIRからLIRへの変換
- ユニットテスト（30テスト）

Week 3: FFIと条件付きコンパイル（追加）
- FFI基盤の実装
- 条件付きコンパイルの実装
- ユニットテスト（35テスト）

Week 4: モジュールシステムと統合（変更）
- モジュールシステムの実装
- IRビューワーとツールの完成
- 統合テスト（20テスト）
- ドキュメント完成
- リリース準備
```

---

## 8. テスト計画

### 8.1 FFIテスト

```cb
// tests/cases/ffi/basic_c_call.cb

extern "C" {
    int abs(x: int);
}

fn test_c_abs() {
    assert(abs(-5) == 5);
    assert(abs(10) == 10);
}
```

### 8.2 条件付きコンパイルテスト

```cb
// tests/cases/cfg/platform_specific.cb

#[cfg(target_os = "linux")]
string  get_platform() {
    return "Linux";
}

#[cfg(target_os = "windows")]
string  get_platform() {
    return "Windows";
}

fn test_platform() {
    let platform = get_platform();
    #[cfg(target_os = "linux")]
    assert(platform == "Linux");

    #[cfg(target_os = "windows")]
    assert(platform == "Windows");
}
```

### 8.3 モジュールテスト

```cb
// tests/cases/module/basic_module.cb

mod math {
    pub int  add(a: int, b: int) {
        return a + b;
    }
}

use math::add;

fn test_module_import() {
    assert(add(1, 2) == 3);
}
```

---

## 9. まとめ

v0.17.0の標準ライブラリ化を実現するために、v0.16.0に以下の3つの主要機能を追加します：

1. **FFI (Foreign Function Interface)** - C関数の呼び出しとC互換性
2. **条件付きコンパイル** - プラットフォーム固有のコード分岐
3. **モジュールシステム** - コードの整理とインポート機能

これらの機能により、v0.17.0で完全な標準ライブラリを実装できます。
