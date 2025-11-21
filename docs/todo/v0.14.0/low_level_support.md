# 低レイヤアプリケーション対応設計

**バージョン**: v0.14.0
**作成日**: 2025-11-13
**ステータス**: 追加設計

---

## 目次

1. [概要](#1-概要)
2. [ベアメタル実行サポート](#2-ベアメタル実行サポート)
3. [インラインアセンブラ](#3-インラインアセンブラ)
4. [メモリマップドIO](#4-メモリマップドio)
5. [割り込みハンドラ](#5-割り込みハンドラ)
6. [カスタムリンカースクリプト](#6-カスタムリンカースクリプト)
7. [OS開発用機能](#7-os開発用機能)

---

## 1. 概要

### 1.1 不足していた機能

現在のv0.14.0設計では、以下の低レイヤアプリケーション開発に必要な機能が不足しています：

**不足している機能**:
- ✗ ベアメタル実行（OSなし）
- ✗ インラインアセンブラ
- ✗ メモリマップドIO
- ✗ 割り込みハンドラ
- ✗ カスタムリンカースクリプト
- ✗ カーネルモードコード

### 1.2 追加する機能

v0.14.0に以下の機能を追加します：

**Phase 1（v0.14.0）: 基盤**
- ✓ インラインアセンブラの構文とAST
- ✓ ベアメタルターゲットの定義
- ✓ メモリマップドIOのサポート
- ✓ 揮発性アクセス（volatile）
- ✓ カスタムリンカースクリプト生成

**Phase 2（v0.15.0以降）: 拡張**
- 割り込みハンドラ
- システムコール実装
- カーネルモジュール開発

---

## 2. ベアメタル実行サポート

### 2.1 ベアメタルターゲットの定義

#### ターゲット情報の拡張

```cpp
// src/backend/codegen/common/target_info.h に追加

enum class ExecutionEnvironment {
    Hosted,           // 通常のOS環境
    Freestanding,     // ベアメタル環境（OSなし）
};

struct TargetInfo {
    // 既存のフィールド...
    ExecutionEnvironment environment;

    // ベアメタル用の追加情報
    uint64_t ram_start;         // RAMの開始アドレス
    uint64_t ram_size;          // RAMのサイズ
    uint64_t rom_start;         // ROMの開始アドレス（フラッシュメモリ等）
    uint64_t rom_size;          // ROMのサイズ
    std::string entry_point;    // エントリーポイント名（デフォルト: "_start"）

    // ベアメタルターゲットの作成
    static TargetInfo baremetal_arm_cortex_m();
    static TargetInfo baremetal_riscv();
    static TargetInfo baremetal_x86_64();
};
```

#### 例：ARM Cortex-M用ベアメタルターゲット

```cpp
TargetInfo TargetInfo::baremetal_arm_cortex_m() {
    TargetInfo target;
    target.arch = TargetArch::ARM_CORTEX_M4;
    target.os = TargetOS::None;
    target.environment = ExecutionEnvironment::Freestanding;
    target.endian = Endian::Little;
    target.calling_convention = CallingConvention::AAPCS;
    target.pointer_size = 4;
    target.pointer_alignment = 4;
    target.ram_start = 0x20000000;    // STM32典型的なRAM
    target.ram_size = 128 * 1024;     // 128KB
    target.rom_start = 0x08000000;    // STM32典型的なFlash
    target.rom_size = 512 * 1024;     // 512KB
    target.entry_point = "Reset_Handler";
    target.triple = "arm-none-eabi";
    return target;
}
```

### 2.2 ベアメタルランタイム

#### スタートアップコード

**Cbコード例**:
```cb
// startup.cb - ARM Cortex-M用スタートアップコード

// ベアメタル用属性
#[section(".text.reset")]
#[no_mangle]
!  Reset_Handler() {
    // データセクションの初期化
    extern "C" {
        static _sdata: u32;
        static _edata: u32;
        static _sidata: u32;
    }

    // .dataセクションをRAMにコピー
    volatile_copy(&_sdata, &_sidata, (&_edata - &_sdata) as usize);

    // .bssセクションをゼロクリア
    extern "C" {
        static _sbss: u32;
        static _ebss: u32;
    }
    volatile_memset(&_sbss, 0, (&_ebss - &_sbss) as usize);

    // メイン関数の呼び出し
    main();

    // メインから戻った場合は無限ループ
    loop {}
}

// ベクタテーブル
#[section(".vector_table")]
#[no_mangle]
static VECTOR_TABLE: [u32; 256] = [
    0x20020000,              // 初期スタックポインタ
    Reset_Handler as u32,    // リセットハンドラ
    // 他の割り込みベクタ...
];
```

### 2.3 コマンドラインオプション

```bash
# ベアメタル用コンパイル
./main firmware.cb \
    --backend=native \
    --target=arm-none-eabi \
    --environment=freestanding \
    --ram-start=0x20000000 \
    --ram-size=128K \
    --rom-start=0x08000000 \
    --rom-size=512K \
    --output=firmware.elf

# リンカースクリプト生成
./main firmware.cb \
    --backend=native \
    --target=arm-none-eabi \
    --environment=freestanding \
    --emit-linker-script=firmware.ld
```

---

## 3. インラインアセンブラ

### 3.1 構文設計

#### Cb言語でのインラインアセンブリ構文

```cb
// x86-64インラインアセンブリ
ulong  read_cr0() {
    long result;
    asm volatile (
        "mov %cr0, %rax"
        : "=r"(result)  // 出力オペランド
        :               // 入力オペランド
        : "rax"         // クロバーリスト
    );
    return result;
}

// ARMインラインアセンブリ
fn enable_interrupts() {
    asm volatile (
        "cpsie i"       // IRQ割り込み有効化
        :               // 出力オペランド
        :               // 入力オペランド
        :               // クロバーリスト
    );
}

// 入出力オペランド付き
int  atomic_add(ptr: int*, value: int) {
    int result;
    asm volatile (
        "lock xadd %0, %1"
        : "=r"(result), "+m"(*ptr)  // 出力
        : "0"(value)                // 入力
        : "memory"                  // メモリクロバー
    );
    return result;
}
```

#### 構文仕様

```
asm [volatile] (
    <assembly_code>
    [: <output_operands>]
    [: <input_operands>]
    [: <clobbers>]
);
```

### 3.2 ASTノードの追加

#### ファイル: `src/common/ast.h` に追加

```cpp
// AST_INLINE_ASM ノード
struct ASTInlineAsm {
    std::string assembly_code;              // アセンブリコード
    bool is_volatile;                       // volatile指定

    // オペランド
    struct Operand {
        std::string constraint;             // 制約文字列（"=r", "+m"等）
        std::unique_ptr<ASTNode> expr;      // 式
    };
    std::vector<Operand> outputs;           // 出力オペランド
    std::vector<Operand> inputs;            // 入力オペランド

    // クロバーリスト
    std::vector<std::string> clobbers;      // クロバーされるレジスタ/メモリ
};

// ASTNodeに追加
struct ASTNode {
    // 既存のフィールド...

    // インラインアセンブリ用
    std::unique_ptr<ASTInlineAsm> inline_asm;
};
```

### 3.3 HIR/MIR/LIRでの表現

#### HIRレベル

```cpp
// src/backend/ir/hir/hir_node.h に追加

struct HIRInlineAsm {
    std::string assembly_code;
    bool is_volatile;
    std::vector<HIRInlineAsmOperand> outputs;
    std::vector<HIRInlineAsmOperand> inputs;
    std::vector<std::string> clobbers;
    TargetArch target_arch;                 // ターゲットアーキテクチャ
};
```

#### MIRレベル

```cpp
// src/backend/ir/mir/mir_node.h に追加

// MIR Statement
enum class MIRStatementKind {
    // 既存...
    InlineAsm,        // インラインアセンブリ
};

struct MIRInlineAsm {
    std::string assembly_code;
    bool is_volatile;
    std::vector<MIRPlace> outputs;          // 出力先
    std::vector<MIROperand> inputs;         // 入力値
    std::vector<std::string> clobbers;
};
```

#### LIRレベル

```cpp
// src/backend/ir/lir/lir_node.h に追加

struct LIRInstruction {
    enum class Opcode {
        // 既存...
        InlineAsm,    // インラインアセンブリ
    };

    // インラインアセンブリ用
    std::string asm_code;
    std::vector<LIROperand> asm_operands;
};
```

### 3.4 コード生成

#### ネイティブバックエンド

```cpp
// src/backend/codegen/native/native_codegen.cpp

void NativeCodegen::emit_inline_asm(const LIRInstruction& inst) {
    // アセンブリコードをそのまま出力
    output << "    # Inline assembly\n";
    output << inst.asm_code << "\n";
}
```

#### WASMバックエンド

WASMではインラインアセンブリをサポートできないため、エラーを出力：

```cpp
void WASMCodegen::emit_inline_asm(const LIRInstruction& inst) {
    throw std::runtime_error("Inline assembly is not supported for WASM target");
}
```

---

## 4. メモリマップドIO

### 4.1 Volatile アクセス

#### 構文

```cb
// volatileポインタ
int  read_register(addr: volatile int*) {
    return *addr;  // volatile読み取り
}

fn write_register(addr: volatile int*, value: int) {
    *addr = value;  // volatile書き込み
}

// メモリマップドレジスタ
struct UART {
    volatile data: int;      // データレジスタ
    volatile status: int;    // ステータスレジスタ
    volatile control: int;   // コントロールレジスタ
}

// UARTレジスタへのアクセス
fn uart_send(c: char) {
    let uart = 0x40000000 as UART*;  // UART基底アドレス

    // ステータスレジスタをポーリング
    while (uart->status & 0x80) == 0 {
        // 送信可能になるまで待機
    }

    // データ送信
    uart->data = c as int;
}
```

### 4.2 ASTとIRでのサポート

```cpp
// ASTNode に volatile フラグを追加
struct ASTNode {
    // 既存のフィールド...
    bool is_volatile_access = false;    // volatile アクセスか
};

// MIR Statement に volatile フラグ
struct MIRStatement {
    bool is_volatile = false;           // volatile アクセス
};
```

### 4.3 コード生成

#### x86-64

```asm
; volatile 読み取り
mov eax, dword ptr [rdi]
mfence                      ; メモリフェンス

; volatile 書き込み
mov dword ptr [rdi], esi
mfence
```

#### ARM

```asm
; volatile 読み取り
ldr r0, [r1]
dmb                         ; データメモリバリア

; volatile 書き込み
str r0, [r1]
dmb
```

---

## 5. 割り込みハンドラ

### 5.1 構文

```cb
// 割り込みハンドラの宣言
#[interrupt]
fn timer_interrupt_handler() {
    // タイマー割り込み処理
    // ...
}

// 特定の割り込み番号を指定
#[interrupt(irq = 16)]
fn usart1_handler() {
    // USART1割り込み処理
    // ...
}

// ARM Cortex-M用例外ハンドラ
#[exception]
fn HardFault_Handler() {
    // ハードフォルト例外処理
    loop {}
}
```

### 5.2 属性の実装

```cpp
// ASTNode に属性を追加
struct FunctionAttribute {
    enum class Kind {
        None,
        Interrupt,      // 割り込みハンドラ
        Exception,      // 例外ハンドラ
        NoMangle,       // 名前マングリングなし
        Section,        // カスタムセクション
        Naked,          // プロローグ/エピローグなし
    };

    Kind kind;
    std::unordered_map<std::string, std::string> params;  // 属性パラメータ
};

struct ASTNode {
    // 既存...
    std::vector<FunctionAttribute> attributes;
};
```

### 5.3 コード生成

#### ARM Cortex-M

```asm
    .section .text.timer_handler
    .global timer_interrupt_handler
    .type timer_interrupt_handler, %function
timer_interrupt_handler:
    push {lr}           ; リンクレジスタ保存
    ; 割り込み処理本体
    pop {pc}            ; 割り込みから復帰
```

---

## 6. カスタムリンカースクリプト

### 6.1 リンカースクリプト生成

#### ファイル: `src/backend/codegen/native/linker_script_generator.h`

```cpp
#pragma once
#include "backend/codegen/common/target_info.h"
#include <string>

namespace cb {
namespace codegen {

class LinkerScriptGenerator {
public:
    explicit LinkerScriptGenerator(const TargetInfo& target);

    // リンカースクリプト生成
    std::string generate();

    // カスタムセクションの追加
    void add_section(const std::string& name, uint64_t address, uint64_t size);

private:
    TargetInfo target;
    std::vector<Section> custom_sections;
};

} // namespace codegen
} // namespace cb
```

### 6.2 生成例（ARM Cortex-M用）

```ld
/* リンカースクリプト: ARM Cortex-M4 */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 512K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 128K
}

ENTRY(Reset_Handler)

SECTIONS
{
    .vector_table :
    {
        KEEP(*(.vector_table))
    } > FLASH

    .text :
    {
        *(.text.reset)
        *(.text*)
        *(.rodata*)
    } > FLASH

    .data :
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM

    /* スタック */
    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        _sstack = .;
        . = . + 0x2000;  /* 8KB stack */
        _estack = .;
    } > RAM
}
```

---

## 7. OS開発用機能

### 7.1 必要な機能

```cb
// ページテーブル操作
fn setup_page_table() {
    let pml4 = allocate_page_table();

    // ページテーブルエントリの設定
    pml4[0] = 0x1000 | PAGE_PRESENT | PAGE_WRITE;

    // CR3レジスタに設定
    asm volatile (
        "mov %0, %%cr3"
        :
        : "r"(pml4 as ulong)
        : "memory"
    );
}

// GDT（グローバルディスクリプタテーブル）設定
struct GDTEntry {
    limit_low: short;
    base_low: short;
    base_middle: byte;
    access: byte;
    granularity: byte;
    base_high: byte;
}

#[repr(C, packed)]
struct GDTPointer {
    limit: short;
    base: GDTEntry*;
}

fn load_gdt(gdt: GDTEntry*, size: int) {
    let gdtp = GDTPointer {
        limit: (size * sizeof(GDTEntry) - 1) as short,
        base: gdt
    };

    asm volatile (
        "lgdt (%0)"
        :
        : "r"(&gdtp)
        : "memory"
    );
}

// システムコール実装
#[naked]
fn syscall_handler() {
    asm volatile (
        "push %rbp"
        "push %rbx"
        "push %r12"
        "push %r13"
        "push %r14"
        "push %r15"

        // システムコール番号に応じて分岐
        "call dispatch_syscall"

        "pop %r15"
        "pop %r14"
        "pop %r13"
        "pop %r12"
        "pop %rbx"
        "pop %rbp"
        "iretq"
    );
}
```

### 7.2 必要な組み込み型と関数

```cb
// ポート入出力（x86）
fn outb(port: short, value: byte) {
    asm volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

byte  inb(port: short) {
    let result: byte;
    asm volatile (
        "inb %1, %0"
        : "=a"(result)
        : "Nd"(port)
    );
    return result;
}

// MSR読み書き（x86）
ulong  rdmsr(msr: int) {
    let low: int, high: int;
    asm volatile (
        "rdmsr"
        : "=a"(low), "=d"(high)
        : "c"(msr)
    );
    return (high as ulong << 32) | (low as ulong);
}

fn wrmsr(msr: int, value: ulong) {
    asm volatile (
        "wrmsr"
        :
        : "c"(msr), "a"(value as int), "d"((value >> 32) as int)
    );
}

// メモリバリア
fn memory_barrier() {
    asm volatile ("mfence" ::: "memory");
}

// アトミック操作
int  atomic_load(ptr: volatile int*) {
    asm volatile (
        "mov (%1), %0"
        : "=r"(result)
        : "r"(ptr)
        : "memory"
    );
    return result;
}
```

---

## 8. コンパイラオプションの追加

### 8.1 新しいオプション

```bash
# ベアメタル用オプション
--environment=freestanding     # ベアメタル環境
--no-stdlib                   # 標準ライブラリなし
--no-runtime                  # ランタイムなし

# メモリレイアウト
--ram-start=<address>         # RAMの開始アドレス
--ram-size=<size>             # RAMのサイズ
--rom-start=<address>         # ROMの開始アドレス
--rom-size=<size>             # ROMのサイズ

# リンカースクリプト
--emit-linker-script=<file>   # リンカースクリプト生成
--linker-script=<file>        # カスタムリンカースクリプト使用

# デバッグ
--emit-asm                    # アセンブリコード出力
--emit-obj                    # オブジェクトファイル出力
```

---

## 9. 実装スケジュール

### Phase 1: v0.14.0（基盤）

**Week 11-12に追加**:
- [ ] インラインアセンブラのAST/HIR/MIR/LIR実装
- [ ] volatile アクセスのサポート
- [ ] ベアメタルターゲットの定義
- [ ] リンカースクリプト生成基盤

### Phase 2: v0.15.0以降（拡張）

- [ ] 割り込みハンドラ属性
- [ ] カスタムセクション配置
- [ ] OS開発用組み込み関数

### Phase 3: v0.18.0（最適化）

- [ ] インラインアセンブリの最適化
- [ ] レジスタ割り当てとの統合
- [ ] ベアメタル用最適化

---

## 10. まとめ

これらの機能追加により、v0.14.0は以下をサポートします：

**低レイヤアプリケーション開発**:
- ✓ ベアメタル実行
- ✓ インラインアセンブラ
- ✓ メモリマップドIO
- ✓ カスタムリンカースクリプト

**OS開発**:
- ✓ 割り込みハンドラ
- ✓ ページテーブル操作
- ✓ GDT/IDT設定
- ✓ システムコール実装

**組み込みシステム**:
- ✓ ARM Cortex-M対応
- ✓ RISC-V対応
- ✓ ハードウェア直接制御
- ✓ リアルタイム処理
