# WASM バックエンド詳細設計

**バージョン**: v0.14.0
**作成日**: 2025-11-13
**ステータス**: 設計中

---

## 目次

1. [WASMバックエンドの概要](#1-wasmバックエンドの概要)
2. [WASM出力戦略](#2-wasm出力戦略)
3. [LIRからWASMへの変換](#3-lirからwasmへの変換)
4. [WASMモジュール構造](#4-wasmモジュール構造)
5. [メモリ管理](#5-メモリ管理)
6. [型マッピング](#6-型マッピング)
7. [JavaScript/TypeScript統合](#7-javascripttypescript統合)
8. [実装の詳細](#8-実装の詳細)

---

## 1. WASMバックエンドの概要

### 1.1 目標

Cb言語で書かれたコードをWebAssembly（WASM）にコンパイルし、ブラウザやNode.jsで実行可能にします。

**ターゲット**:
- **WASM 1.0**: 基本機能（MVP）
- **WASM 2.0**: 将来的な拡張（ガベージコレクション、スレッド等）

**ユースケース**:
- Webフロントエンド開発
- ブラウザゲーム開発
- Node.jsでの高速計算
- エッジコンピューティング

### 1.2 アプローチの選択

**2つのアプローチを検討**:

#### アプローチ1: 直接WASM出力（採用）
```
LIR → WASM Binary Format (.wasm)
```
**メリット**:
- パフォーマンスが最高
- ファイルサイズが小さい
- ロード時間が短い

**デメリット**:
- 実装が複雑
- デバッグが困難

#### アプローチ2: TypeScript経由（補助的に採用）
```
LIR → TypeScript (.ts) → (tsc) → JavaScript (.js)
```
**メリット**:
- 実装が簡単
- デバッグが容易
- 型安全性

**デメリット**:
- パフォーマンスが低い
- ファイルサイズが大きい

### 1.3 ハイブリッド戦略

両方のアプローチを実装し、ユーザーが選択できるようにします：

```bash
# 直接WASM出力（パフォーマンス重視）
./main example.cb --backend=wasm --output=example.wasm

# TypeScript出力（デバッグ重視）
./main example.cb --backend=typescript --output=example.ts
```

---

## 2. WASM出力戦略

### 2.1 WASMバイナリフォーマット

#### WASM モジュールの構造
```
WASM Module
├── Type Section         # 関数シグネチャ
├── Import Section       # 外部関数のインポート
├── Function Section     # 関数定義
├── Table Section        # 関数テーブル（間接呼び出し用）
├── Memory Section       # メモリ定義
├── Global Section       # グローバル変数
├── Export Section       # エクスポート定義
├── Start Section        # 開始関数
├── Element Section      # テーブル初期化
├── Code Section         # 関数の実装
└── Data Section         # データセグメント
```

### 2.2 WASM命令セット

#### 基本命令
```wasm
;; 制御フロー
block
loop
if/else/end
br/br_if/br_table
return
call/call_indirect

;; 変数アクセス
local.get
local.set
local.tee
global.get
global.set

;; メモリアクセス
i32.load
i32.store
i64.load
i64.store
f32.load
f32.store
f64.load
f64.store

;; 数値演算
i32.add/sub/mul/div_s/div_u
i64.add/sub/mul/div_s/div_u
f32.add/sub/mul/div
f64.add/sub/mul/div

;; 比較演算
i32.eq/ne/lt_s/lt_u/le_s/le_u/gt_s/gt_u/ge_s/ge_u
i64.eq/ne/lt_s/lt_u/le_s/le_u/gt_s/gt_u/ge_s/ge_u
f32.eq/ne/lt/le/gt/ge
f64.eq/ne/lt/le/gt/ge

;; 定数
i32.const
i64.const
f32.const
f64.const
```

---

## 3. LIRからWASMへの変換

### 3.1 WASMコード生成器の設計

#### ファイル: `src/backend/codegen/wasm/wasm_codegen.h`

```cpp
#pragma once
#include "backend/ir/lir/lir_node.h"
#include "wasm_module_builder.h"
#include <vector>
#include <unordered_map>

namespace cb {
namespace codegen {
namespace wasm {

// WASMコード生成器
class WASMCodegen {
public:
    WASMCodegen();

    // LIRプログラムからWASMモジュールを生成
    void generate(const ir::lir::LIRProgram& program, const std::string& output_file);

private:
    // 関数の変換
    void generate_function(const ir::lir::LIRFunction& func);

    // 基本ブロックの変換
    void generate_basic_block(const ir::lir::LIRBasicBlock& block);

    // 命令の変換
    void generate_instruction(const ir::lir::LIRInstruction& inst);

    // LIR命令からWASM命令へのマッピング
    void emit_move(const ir::lir::LIRInstruction& inst);
    void emit_load(const ir::lir::LIRInstruction& inst);
    void emit_store(const ir::lir::LIRInstruction& inst);
    void emit_add(const ir::lir::LIRInstruction& inst);
    void emit_sub(const ir::lir::LIRInstruction& inst);
    void emit_mul(const ir::lir::LIRInstruction& inst);
    void emit_div(const ir::lir::LIRInstruction& inst);
    void emit_cmp(const ir::lir::LIRInstruction& inst);
    void emit_jump(const ir::lir::LIRInstruction& inst);
    void emit_jump_if(const ir::lir::LIRInstruction& inst);
    void emit_call(const ir::lir::LIRInstruction& inst);
    void emit_return(const ir::lir::LIRInstruction& inst);

    // レジスタからローカル変数へのマッピング
    uint32_t get_local_index(uint32_t vreg);

    // 型変換
    wasm::ValueType lir_type_to_wasm_type(TypeInfo type);

    WASMModuleBuilder module_builder;
    std::unordered_map<uint32_t, uint32_t> vreg_to_local;
    uint32_t next_local_index;
};

} // namespace wasm
} // namespace codegen
} // namespace cb
```

### 3.2 命令マッピング

#### LIR → WASM 命令マッピング表

| LIR命令 | WASM命令 | 備考 |
|---------|----------|------|
| `Move dst, src` | `local.get src`<br>`local.set dst` | ローカル変数間の移動 |
| `Load dst, [addr]` | `local.get addr`<br>`i32.load` または `i64.load`<br>`local.set dst` | メモリからのロード |
| `Store [addr], src` | `local.get addr`<br>`local.get src`<br>`i32.store` または `i64.store` | メモリへのストア |
| `Add dst, src1, src2` | `local.get src1`<br>`local.get src2`<br>`i32.add` または `i64.add`<br>`local.set dst` | 加算 |
| `Sub dst, src1, src2` | `local.get src1`<br>`local.get src2`<br>`i32.sub` または `i64.sub`<br>`local.set dst` | 減算 |
| `Mul dst, src1, src2` | `local.get src1`<br>`local.get src2`<br>`i32.mul` または `i64.mul`<br>`local.set dst` | 乗算 |
| `Div dst, src1, src2` | `local.get src1`<br>`local.get src2`<br>`i32.div_s` または `i64.div_s`<br>`local.set dst` | 除算（符号付き） |
| `Cmp src1, src2` | `local.get src1`<br>`local.get src2`<br>`i32.eq` 等<br>（フラグ設定） | 比較 |
| `Jump label` | `br $label` | 無条件ジャンプ |
| `JumpIf cond, label` | `local.get cond`<br>`br_if $label` | 条件付きジャンプ |
| `Call func, args...` | `local.get arg0`<br>`local.get arg1`<br>...<br>`call $func` | 関数呼び出し |
| `Return val` | `local.get val`<br>`return` | 関数からの返却 |

### 3.3 制御フローの変換

#### LIRの制御フロー → WASMの構造化制御フロー

WASMは構造化制御フローを要求するため、LIRのラベル/ジャンプを変換する必要があります。

**変換例**:
```cpp
// LIR (ラベルとジャンプ)
bb0:
    cmp %0, %1
    jump_if_eq bb2
    jump bb1
bb1:
    // ...
    jump bb3
bb2:
    // ...
    jump bb3
bb3:
    return

// WASM (構造化制御フロー)
(block $bb3
    (block $bb2
        (block $bb1
            (local.get $0)
            (local.get $1)
            (i32.eq)
            (br_if $bb2)
            (br $bb1)
        )
        ;; bb1
        ;; ...
        (br $bb3)
    )
    ;; bb2
    ;; ...
)
;; bb3
(return)
```

**変換アルゴリズム**:
1. CFGを解析して構造化制御フロー（if/block/loop）を特定
2. ジャンプを`br`/`br_if`/`br_table`に変換
3. ネストしたブロックを生成

---

## 4. WASMモジュール構造

### 4.1 WASMモジュールビルダー

#### ファイル: `src/backend/codegen/wasm/wasm_module_builder.h`

```cpp
#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace cb {
namespace codegen {
namespace wasm {

// WASM値型
enum class ValueType : uint8_t {
    I32 = 0x7F,
    I64 = 0x7E,
    F32 = 0x7D,
    F64 = 0x7C,
};

// WASM関数シグネチャ
struct FunctionType {
    std::vector<ValueType> params;
    std::vector<ValueType> results;
};

// WASM命令
enum class Opcode : uint8_t {
    // 制御フロー
    Block = 0x02,
    Loop = 0x03,
    If = 0x04,
    Else = 0x05,
    End = 0x0B,
    Br = 0x0C,
    BrIf = 0x0D,
    BrTable = 0x0E,
    Return = 0x0F,
    Call = 0x10,
    CallIndirect = 0x11,

    // ローカル変数
    LocalGet = 0x20,
    LocalSet = 0x21,
    LocalTee = 0x22,
    GlobalGet = 0x23,
    GlobalSet = 0x24,

    // メモリアクセス
    I32Load = 0x28,
    I64Load = 0x29,
    F32Load = 0x2A,
    F64Load = 0x2B,
    I32Store = 0x36,
    I64Store = 0x37,
    F32Store = 0x38,
    F64Store = 0x39,

    // 定数
    I32Const = 0x41,
    I64Const = 0x42,
    F32Const = 0x43,
    F64Const = 0x44,

    // i32演算
    I32Add = 0x6A,
    I32Sub = 0x6B,
    I32Mul = 0x6C,
    I32DivS = 0x6D,
    I32DivU = 0x6E,
    I32RemS = 0x6F,
    I32RemU = 0x70,
    I32And = 0x71,
    I32Or = 0x72,
    I32Xor = 0x73,
    I32Shl = 0x74,
    I32ShrS = 0x75,
    I32ShrU = 0x76,

    // i32比較
    I32Eq = 0x46,
    I32Ne = 0x47,
    I32LtS = 0x48,
    I32LtU = 0x49,
    I32LeS = 0x4C,
    I32LeU = 0x4D,
    I32GtS = 0x4A,
    I32GtU = 0x4B,
    I32GeS = 0x4E,
    I32GeU = 0x4F,

    // i64演算（同様に定義）
    I64Add = 0x7C,
    // ...

    // f32/f64演算（同様に定義）
    F32Add = 0x92,
    // ...
};

// WASM命令
struct Instruction {
    Opcode opcode;
    std::vector<uint8_t> immediate;  // 即値データ
};

// WASMモジュールビルダー
class WASMModuleBuilder {
public:
    WASMModuleBuilder();

    // 関数の追加
    uint32_t add_function_type(const FunctionType& type);
    uint32_t add_function(uint32_t type_index);
    void add_local(uint32_t func_index, ValueType type);

    // 命令の追加
    void emit_instruction(Opcode opcode);
    void emit_i32_const(int32_t value);
    void emit_i64_const(int64_t value);
    void emit_f32_const(float value);
    void emit_f64_const(double value);
    void emit_local_get(uint32_t local_index);
    void emit_local_set(uint32_t local_index);
    void emit_call(uint32_t func_index);
    void emit_block(ValueType result_type);
    void emit_loop(ValueType result_type);
    void emit_if(ValueType result_type);
    void emit_br(uint32_t depth);
    void emit_br_if(uint32_t depth);
    void emit_end();

    // メモリの追加
    void add_memory(uint32_t min_pages, uint32_t max_pages);

    // エクスポートの追加
    void add_export(const std::string& name, uint32_t func_index);

    // インポートの追加
    void add_import(const std::string& module, const std::string& name, uint32_t type_index);

    // バイナリ出力
    std::vector<uint8_t> build();
    void write_to_file(const std::string& filename);

private:
    // セクションの書き込み
    void write_type_section(std::vector<uint8_t>& buffer);
    void write_function_section(std::vector<uint8_t>& buffer);
    void write_memory_section(std::vector<uint8_t>& buffer);
    void write_export_section(std::vector<uint8_t>& buffer);
    void write_code_section(std::vector<uint8_t>& buffer);

    // エンコーディングヘルパー
    void encode_u32(std::vector<uint8_t>& buffer, uint32_t value);
    void encode_s32(std::vector<uint8_t>& buffer, int32_t value);
    void encode_u64(std::vector<uint8_t>& buffer, uint64_t value);
    void encode_s64(std::vector<uint8_t>& buffer, int64_t value);
    void encode_f32(std::vector<uint8_t>& buffer, float value);
    void encode_f64(std::vector<uint8_t>& buffer, double value);
    void encode_string(std::vector<uint8_t>& buffer, const std::string& str);

    // モジュールデータ
    std::vector<FunctionType> types;
    std::vector<uint32_t> functions;  // type_index
    std::vector<std::vector<ValueType>> locals;
    std::vector<std::vector<Instruction>> function_bodies;
    std::vector<std::pair<std::string, uint32_t>> exports;
    uint32_t memory_min_pages = 0;
    uint32_t memory_max_pages = 0;
};

} // namespace wasm
} // namespace codegen
} // namespace cb
```

### 4.2 WASMバイナリエンコーディング

#### WASM Module Header
```
Magic Number: 0x00 0x61 0x73 0x6D  (\0asm)
Version:      0x01 0x00 0x00 0x00  (1)
```

#### セクションの構造
```
[Section ID: 1 byte]
[Section Size: varuint32]
[Section Content: ...]
```

#### セクションID一覧
```
0  = Custom Section
1  = Type Section
2  = Import Section
3  = Function Section
4  = Table Section
5  = Memory Section
6  = Global Section
7  = Export Section
8  = Start Section
9  = Element Section
10 = Code Section
11 = Data Section
```

---

## 5. メモリ管理

### 5.1 WASMリニアメモリ

WASMは単一のリニアメモリ空間を使用します：

```
┌─────────────────────────────────────┐
│  WASM Linear Memory                 │
│                                     │
│  0x0000: [Stack]                    │  ← スタック領域
│  ...                                │
│  0x1000: [Heap]                     │  ← ヒープ領域
│  ...                                │
│  0xFFFF: [End]                      │
└─────────────────────────────────────┘
```

### 5.2 メモリレイアウト

```cpp
// メモリレイアウト定義
namespace memory_layout {
    constexpr uint32_t STACK_BASE = 0x0000;
    constexpr uint32_t STACK_SIZE = 0x1000;    // 4KB
    constexpr uint32_t HEAP_BASE = 0x1000;
    constexpr uint32_t HEAP_SIZE = 0xF000;     // 60KB
    constexpr uint32_t TOTAL_PAGES = 1;        // 64KB (1 page)
}
```

### 5.3 メモリアロケータ

#### ファイル: `src/backend/codegen/wasm/wasm_allocator.h`

```cpp
#pragma once
#include <cstdint>

namespace cb {
namespace codegen {
namespace wasm {

// WASMメモリアロケータ
class WASMAllocator {
public:
    // メモリの初期化
    void initialize(uint32_t heap_base, uint32_t heap_size);

    // メモリの割り当て
    uint32_t allocate(uint32_t size);

    // メモリの解放
    void deallocate(uint32_t ptr);

    // ガベージコレクション（将来）
    void collect();

private:
    uint32_t heap_base;
    uint32_t heap_size;
    uint32_t heap_pointer;

    // フリーリスト（簡易実装）
    struct FreeBlock {
        uint32_t size;
        uint32_t next;
    };
};

} // namespace wasm
} // namespace codegen
} // namespace cb
```

---

## 6. 型マッピング

### 6.1 Cb型からWASM型へのマッピング

| Cb型 | WASM型 | サイズ | 備考 |
|------|--------|--------|------|
| `tiny` (i8) | `i32` | 4 bytes | WASMはi32として扱う |
| `short` (i16) | `i32` | 4 bytes | WASMはi32として扱う |
| `int` (i32) | `i32` | 4 bytes | 直接マップ |
| `long` (i64) | `i64` | 8 bytes | 直接マップ |
| `float` (f32) | `f32` | 4 bytes | 直接マップ |
| `double` (f64) | `f64` | 8 bytes | 直接マップ |
| `char` | `i32` | 4 bytes | Unicode codepoint |
| `bool` | `i32` | 4 bytes | 0=false, 1=true |
| `string` | `i32` (ptr) | 4 bytes | ヒープ上の文字列へのポインタ |
| `T*` (ポインタ) | `i32` (ptr) | 4 bytes | リニアメモリ上のオフセット |
| `struct` | - | varies | メモリ上のレイアウト |
| `array[N]` | `i32` (ptr) | 4 bytes | ヒープ上の配列へのポインタ |

### 6.2 構造体のメモリレイアウト

```cpp
// Cbコード
struct Point {
    int x;
    int y;
}

// WASMメモリレイアウト
// [x: 4 bytes][y: 4 bytes] = 8 bytes total
// オフセット 0: x
// オフセット 4: y
```

### 6.3 文字列の表現

```cpp
// 文字列のメモリレイアウト
// [length: 4 bytes][data: N bytes][null: 1 byte]

// 例: "Hello"
// [5][H][e][l][l][o][\0]
```

---

## 7. JavaScript/TypeScript統合

### 7.1 WASMモジュールのロード

#### JavaScript/TypeScriptラッパー

**ファイル**: `runtime/wasm/cb_runtime.ts`

```typescript
// WASMモジュールのロード
export async function loadCbModule(wasmPath: string): Promise<CbModule> {
    const response = await fetch(wasmPath);
    const buffer = await response.arrayBuffer();

    const importObject = {
        env: {
            // 外部関数のインポート
            print: (ptr: number) => {
                const str = readString(ptr);
                console.log(str);
            },
            println: (ptr: number) => {
                const str = readString(ptr);
                console.log(str);
            },
            // 他の組み込み関数...
        }
    };

    const result = await WebAssembly.instantiate(buffer, importObject);
    return new CbModule(result.instance);
}

// Cbモジュールクラス
export class CbModule {
    private instance: WebAssembly.Instance;
    private memory: WebAssembly.Memory;

    constructor(instance: WebAssembly.Instance) {
        this.instance = instance;
        this.memory = instance.exports.memory as WebAssembly.Memory;
    }

    // エクスポートされた関数の呼び出し
    call(funcName: string, ...args: any[]): any {
        const func = this.instance.exports[funcName] as Function;
        if (!func) {
            throw new Error(`Function '${funcName}' not found`);
        }
        return func(...args);
    }

    // メモリからの文字列読み取り
    readString(ptr: number): string {
        const view = new Uint8Array(this.memory.buffer);
        const length = new DataView(this.memory.buffer).getUint32(ptr, true);
        const decoder = new TextDecoder();
        return decoder.decode(view.slice(ptr + 4, ptr + 4 + length));
    }

    // メモリへの文字列書き込み
    writeString(str: string): number {
        const encoder = new TextEncoder();
        const bytes = encoder.encode(str);
        const ptr = this.allocate(4 + bytes.length + 1);
        const view = new Uint8Array(this.memory.buffer);
        new DataView(this.memory.buffer).setUint32(ptr, bytes.length, true);
        view.set(bytes, ptr + 4);
        view[ptr + 4 + bytes.length] = 0;  // null terminator
        return ptr;
    }

    // メモリの割り当て
    allocate(size: number): number {
        const allocFunc = this.instance.exports.allocate as Function;
        return allocFunc(size);
    }

    // メモリの解放
    deallocate(ptr: number): void {
        const deallocFunc = this.instance.exports.deallocate as Function;
        deallocFunc(ptr);
    }
}
```

### 7.2 使用例

```typescript
// Cbモジュールの使用
async function main() {
    // WASMモジュールのロード
    const module = await loadCbModule('example.wasm');

    // エクスポートされた関数の呼び出し
    const result = module.call('add', 10, 20);
    console.log('Result:', result);  // 30

    // 文字列を渡す
    const strPtr = module.writeString('Hello, WASM!');
    module.call('processString', strPtr);
    module.deallocate(strPtr);
}

main().catch(console.error);
```

---

## 8. 実装の詳細

### 8.1 WASMコード生成の実装例

#### ファイル: `src/backend/codegen/wasm/wasm_codegen.cpp`

```cpp
#include "wasm_codegen.h"
#include <fstream>

namespace cb {
namespace codegen {
namespace wasm {

WASMCodegen::WASMCodegen() : next_local_index(0) {}

void WASMCodegen::generate(const ir::lir::LIRProgram& program, const std::string& output_file) {
    // 関数型の登録
    for (const auto& func : program.functions) {
        FunctionType func_type;

        // パラメータの型
        for (size_t i = 0; i < func.arg_count; ++i) {
            func_type.params.push_back(lir_type_to_wasm_type(func.locals[i].type));
        }

        // 戻り値の型
        if (func.return_type != TYPE_VOID) {
            func_type.results.push_back(lir_type_to_wasm_type(func.return_type));
        }

        uint32_t type_index = module_builder.add_function_type(func_type);
        uint32_t func_index = module_builder.add_function(type_index);

        // 関数の実装
        generate_function(func);

        // エクスポート
        module_builder.add_export(func.name, func_index);
    }

    // メモリの追加（1ページ = 64KB）
    module_builder.add_memory(1, 1);

    // バイナリ出力
    module_builder.write_to_file(output_file);
}

void WASMCodegen::generate_function(const ir::lir::LIRFunction& func) {
    // ローカル変数のマッピング
    vreg_to_local.clear();
    next_local_index = 0;

    // パラメータはローカル変数0から
    for (size_t i = 0; i < func.arg_count; ++i) {
        vreg_to_local[i] = next_local_index++;
    }

    // その他のローカル変数
    for (size_t i = func.arg_count; i < func.locals.size(); ++i) {
        vreg_to_local[i] = next_local_index++;
        ValueType vtype = lir_type_to_wasm_type(func.locals[i].type);
        module_builder.add_local(func_index, vtype);
    }

    // 基本ブロックの生成
    for (const auto& block : func.blocks) {
        generate_basic_block(block);
    }
}

void WASMCodegen::generate_basic_block(const ir::lir::LIRBasicBlock& block) {
    // ブロックラベル
    module_builder.emit_block(ValueType::VOID);

    // 命令の生成
    for (const auto& inst : block.instructions) {
        generate_instruction(inst);
    }

    module_builder.emit_end();
}

void WASMCodegen::generate_instruction(const ir::lir::LIRInstruction& inst) {
    switch (inst.opcode) {
    case ir::lir::LIRInstruction::Opcode::Move:
        emit_move(inst);
        break;
    case ir::lir::LIRInstruction::Opcode::Load:
        emit_load(inst);
        break;
    case ir::lir::LIRInstruction::Opcode::Store:
        emit_store(inst);
        break;
    case ir::lir::LIRInstruction::Opcode::Add:
        emit_add(inst);
        break;
    // 他の命令...
    }
}

void WASMCodegen::emit_add(const ir::lir::LIRInstruction& inst) {
    // dst = src1 + src2
    uint32_t dst = get_local_index(inst.operands[0].vreg);
    uint32_t src1 = get_local_index(inst.operands[1].vreg);
    uint32_t src2 = get_local_index(inst.operands[2].vreg);

    module_builder.emit_local_get(src1);
    module_builder.emit_local_get(src2);

    // 型に応じて命令を選択
    if (inst.operands[0].type == TYPE_INT || inst.operands[0].type == TYPE_SHORT) {
        module_builder.emit_instruction(Opcode::I32Add);
    } else if (inst.operands[0].type == TYPE_LONG) {
        module_builder.emit_instruction(Opcode::I64Add);
    } else if (inst.operands[0].type == TYPE_FLOAT) {
        module_builder.emit_instruction(Opcode::F32Add);
    } else if (inst.operands[0].type == TYPE_DOUBLE) {
        module_builder.emit_instruction(Opcode::F64Add);
    }

    module_builder.emit_local_set(dst);
}

ValueType WASMCodegen::lir_type_to_wasm_type(TypeInfo type) {
    switch (type) {
    case TYPE_TINY:
    case TYPE_SHORT:
    case TYPE_INT:
    case TYPE_CHAR:
    case TYPE_BOOL:
        return ValueType::I32;
    case TYPE_LONG:
        return ValueType::I64;
    case TYPE_FLOAT:
        return ValueType::F32;
    case TYPE_DOUBLE:
        return ValueType::F64;
    default:
        return ValueType::I32;  // ポインタ等
    }
}

uint32_t WASMCodegen::get_local_index(uint32_t vreg) {
    return vreg_to_local[vreg];
}

} // namespace wasm
} // namespace codegen
} // namespace cb
```

---

## 9. テストとデバッグ

### 9.1 WASMテストフレームワーク

```typescript
// tests/wasm/test_framework.ts
import { loadCbModule, CbModule } from '../../runtime/wasm/cb_runtime';
import { expect } from 'chai';

describe('Cb WASM Tests', () => {
    let module: CbModule;

    before(async () => {
        module = await loadCbModule('tests/wasm/test.wasm');
    });

    it('should add two numbers', () => {
        const result = module.call('add', 10, 20);
        expect(result).to.equal(30);
    });

    it('should multiply two numbers', () => {
        const result = module.call('mul', 5, 7);
        expect(result).to.equal(35);
    });

    // 他のテスト...
});
```

### 9.2 WASMデバッグ

#### WASM Text Format (WAT) への変換

デバッグのため、WASM バイナリを WAT（テキスト形式）に変換できます：

```bash
# wasm2wat ツールを使用
wasm2wat example.wasm -o example.wat

# 結果の確認
cat example.wat
```

**WAT例**:
```wasm
(module
  (type $t0 (func (param i32 i32) (result i32)))
  (func $add (type $t0) (param $p0 i32) (param $p1 i32) (result i32)
    local.get $p0
    local.get $p1
    i32.add
  )
  (memory $memory 1)
  (export "add" (func $add))
  (export "memory" (memory $memory))
)
```

---

## 10. まとめ

このWASMバックエンド設計により：

1. **直接WASM出力**: 高パフォーマンス
2. **JavaScript/TypeScript統合**: ブラウザとNode.jsで実行可能
3. **メモリ管理**: リニアメモリの効率的な使用
4. **型マッピング**: Cb型とWASM型の完全なマッピング
5. **デバッグサポート**: WAT形式での検証

v0.14.0完了後、WASM対応により、Cb言語でWebアプリケーションの開発が可能になります。
