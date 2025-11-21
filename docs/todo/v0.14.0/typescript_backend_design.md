# TypeScript バックエンド詳細設計

**バージョン**: v0.14.0
**作成日**: 2025-11-13
**ステータス**: 設計中

---

## 目次

1. [TypeScriptバックエンドの概要](#1-typescriptバックエンドの概要)
2. [LIRからTypeScriptへの変換](#2-lirからtypescriptへの変換)
3. [型マッピング](#3-型マッピング)
4. [メモリモデルとポインタのエミュレーション](#4-メモリモデルとポインタのエミュレーション)
5. [ランタイムライブラリ](#5-ランタイムライブラリ)
6. [最適化とトランスパイル](#6-最適化とトランスパイル)
7. [実装の詳細](#7-実装の詳細)

---

## 1. TypeScriptバックエンドの概要

### 1.1 目標

Cb言語で書かれたコードをTypeScriptに変換し、Node.jsやブラウザで実行可能にします。

**ユースケース**:
- フロントエンド開発（React/Vue/Angularとの統合）
- Node.jsアプリケーション開発
- デバッグとプロトタイピング
- 既存のTypeScript/JavaScriptエコシステムとの統合

### 1.2 アプローチ

```
LIR → TypeScript (.ts) → (tsc) → JavaScript (.js)
```

**メリット**:
- 実装が比較的簡単
- デバッグが容易
- 型安全性の恩恵
- 既存のTypeScriptツールチェーンの活用

**デメリット**:
- パフォーマンスはWASMより劣る
- ファイルサイズが大きい

### 1.3 TypeScript出力の例

#### Cbコード
```cb
int  add(x: int, y: int) {
    return x + y;
}

fn main() {
    int result = add(10, 20);
    println(result);
}
```

#### 生成されるTypeScript
```typescript
// 生成されたTypeScript
export function add(x: number, y: number): number {
    return x + y;
}

export function main(): void {
    const result: number = add(10, 20);
    console.log(result);
}

// エントリーポイント
if (require.main === module) {
    main();
}
```

---

## 2. LIRからTypeScriptへの変換

### 2.1 TypeScriptコード生成器の設計

#### ファイル: `src/backend/codegen/typescript/typescript_codegen.h`

```cpp
#pragma once
#include "backend/ir/lir/lir_node.h"
#include "ts_emitter.h"
#include <string>
#include <unordered_map>

namespace cb {
namespace codegen {
namespace typescript {

// TypeScriptコード生成器
class TypeScriptCodegen {
public:
    TypeScriptCodegen();

    // LIRプログラムからTypeScriptを生成
    void generate(const ir::lir::LIRProgram& program, const std::string& output_file);

private:
    // 関数の変換
    void generate_function(const ir::lir::LIRFunction& func);

    // 基本ブロックの変換
    void generate_basic_block(const ir::lir::LIRBasicBlock& block);

    // 命令の変換
    std::string generate_instruction(const ir::lir::LIRInstruction& inst);

    // LIR命令からTypeScript式へのマッピング
    std::string emit_move(const ir::lir::LIRInstruction& inst);
    std::string emit_load(const ir::lir::LIRInstruction& inst);
    std::string emit_store(const ir::lir::LIRInstruction& inst);
    std::string emit_add(const ir::lir::LIRInstruction& inst);
    std::string emit_sub(const ir::lir::LIRInstruction& inst);
    std::string emit_mul(const ir::lir::LIRInstruction& inst);
    std::string emit_div(const ir::lir::LIRInstruction& inst);
    std::string emit_cmp(const ir::lir::LIRInstruction& inst);
    std::string emit_jump(const ir::lir::LIRInstruction& inst);
    std::string emit_jump_if(const ir::lir::LIRInstruction& inst);
    std::string emit_call(const ir::lir::LIRInstruction& inst);
    std::string emit_return(const ir::lir::LIRInstruction& inst);

    // レジスタから変数名へのマッピング
    std::string get_variable_name(uint32_t vreg);

    // 型変換
    std::string lir_type_to_ts_type(TypeInfo type);

    TSEmitter emitter;
    std::unordered_map<uint32_t, std::string> vreg_to_var;
    uint32_t next_var_index;
};

} // namespace typescript
} // namespace codegen
} // namespace cb
```

### 2.2 TypeScript Emitter

#### ファイル: `src/backend/codegen/typescript/ts_emitter.h`

```cpp
#pragma once
#include <string>
#include <sstream>
#include <vector>

namespace cb {
namespace codegen {
namespace typescript {

// TypeScript出力器
class TSEmitter {
public:
    TSEmitter();

    // インポート
    void emit_import(const std::string& module, const std::vector<std::string>& names);

    // 型定義
    void emit_type_alias(const std::string& name, const std::string& type);
    void emit_interface(const std::string& name, const std::vector<std::pair<std::string, std::string>>& fields);

    // 関数定義
    void begin_function(const std::string& name, const std::vector<std::pair<std::string, std::string>>& params, const std::string& return_type);
    void end_function();

    // 変数宣言
    void emit_variable_declaration(const std::string& name, const std::string& type, const std::string& init_value = "");

    // 文
    void emit_statement(const std::string& stmt);
    void emit_assignment(const std::string& lhs, const std::string& rhs);
    void emit_return(const std::string& value);

    // 制御フロー
    void begin_if(const std::string& condition);
    void begin_else();
    void end_if();

    void begin_while(const std::string& condition);
    void end_while();

    void begin_block();
    void end_block();

    // ラベル（gotoのエミュレーション）
    void emit_label(const std::string& label);
    void emit_goto(const std::string& label);

    // インデント管理
    void increase_indent();
    void decrease_indent();

    // 出力
    std::string get_output() const;
    void write_to_file(const std::string& filename);

private:
    std::ostringstream output;
    int indent_level;

    void write_indent();
};

} // namespace typescript
} // namespace codegen
} // namespace cb
```

### 2.3 命令マッピング

#### LIR → TypeScript マッピング表

| LIR命令 | TypeScript |
|---------|------------|
| `Move dst, src` | `dst = src;` |
| `Load dst, [addr]` | `dst = memory.load(addr);` |
| `Store [addr], src` | `memory.store(addr, src);` |
| `Add dst, src1, src2` | `dst = src1 + src2;` |
| `Sub dst, src1, src2` | `dst = src1 - src2;` |
| `Mul dst, src1, src2` | `dst = src1 * src2;` |
| `Div dst, src1, src2` | `dst = Math.floor(src1 / src2);` （整数除算） |
| `Div dst, src1, src2` | `dst = src1 / src2;` （浮動小数点除算） |
| `Cmp src1, src2` | `cmpResult = src1 - src2;` |
| `Jump label` | `goto label;` （エミュレーション） |
| `JumpIf cond, label` | `if (cond) goto label;` （エミュレーション） |
| `Call func, args...` | `func(args...);` |
| `Return val` | `return val;` |

---

## 3. 型マッピング

### 3.1 Cb型からTypeScript型へのマッピング

| Cb型 | TypeScript型 | 備考 |
|------|--------------|------|
| `tiny` (i8) | `number` | JavaScriptのnumberは64bit浮動小数点 |
| `short` (i16) | `number` |  |
| `int` (i32) | `number` |  |
| `long` (i64) | `bigint` | 64bit整数はBigInt |
| `float` (f32) | `number` |  |
| `double` (f64) | `number` |  |
| `char` | `string` | 長さ1の文字列 |
| `bool` | `boolean` | 直接マップ |
| `string` | `string` | 直接マップ |
| `T*` (ポインタ) | `Pointer<T>` | カスタム型（後述） |
| `struct Foo` | `interface Foo` | インターフェース |
| `array[N]` | `T[]` または `Array<T>` | 配列 |
| `void` | `void` | 直接マップ |

### 3.2 構造体の変換

#### Cbコード
```cb
struct Point {
    int x;
    int y;
}
```

#### 生成されるTypeScript
```typescript
export interface Point {
    x: number;
    y: number;
}
```

### 3.3 Enumの変換

#### Cbコード
```cb
enum Color {
    Red,
    Green,
    Blue,
}
```

#### 生成されるTypeScript
```typescript
export enum Color {
    Red = 0,
    Green = 1,
    Blue = 2,
}
```

---

## 4. メモリモデルとポインタのエミュレーション

### 4.1 メモリクラス

TypeScriptではポインタが存在しないため、メモリクラスでエミュレートします。

#### ファイル: `runtime/typescript/memory.ts`

```typescript
// メモリエミュレーション
export class Memory {
    private buffer: ArrayBuffer;
    private view: DataView;
    private heap: Map<number, any>;
    private nextAddress: number;

    constructor(size: number = 1024 * 1024) {  // 1MB
        this.buffer = new ArrayBuffer(size);
        this.view = new DataView(this.buffer);
        this.heap = new Map();
        this.nextAddress = 0;
    }

    // メモリの割り当て
    allocate(size: number): number {
        const address = this.nextAddress;
        this.nextAddress += size;
        return address;
    }

    // i8の読み書き
    loadI8(address: number): number {
        return this.view.getInt8(address);
    }

    storeI8(address: number, value: number): void {
        this.view.setInt8(address, value);
    }

    // i16の読み書き
    loadI16(address: number): number {
        return this.view.getInt16(address, true);  // リトルエンディアン
    }

    storeI16(address: number, value: number): void {
        this.view.setInt16(address, value, true);
    }

    // i32の読み書き
    loadI32(address: number): number {
        return this.view.getInt32(address, true);
    }

    storeI32(address: number, value: number): void {
        this.view.setInt32(address, value, true);
    }

    // i64の読み書き（BigInt）
    loadI64(address: number): bigint {
        return this.view.getBigInt64(address, true);
    }

    storeI64(address: number, value: bigint): void {
        this.view.setBigInt64(address, value, true);
    }

    // f32の読み書き
    loadF32(address: number): number {
        return this.view.getFloat32(address, true);
    }

    storeF32(address: number, value: number): void {
        this.view.setFloat32(address, value, true);
    }

    // f64の読み書き
    loadF64(address: number): number {
        return this.view.getFloat64(address, true);
    }

    storeF64(address: number, value: number): void {
        this.view.setFloat64(address, value, true);
    }

    // オブジェクトの保存（ヒープ）
    storeObject(address: number, obj: any): void {
        this.heap.set(address, obj);
    }

    loadObject(address: number): any {
        return this.heap.get(address);
    }
}

// グローバルメモリインスタンス
export const memory = new Memory();
```

### 4.2 ポインタ型

```typescript
// ポインタ型（型安全なラッパー）
export class Pointer<T> {
    constructor(public address: number) {}

    // デリファレンス
    deref(): T {
        return memory.loadObject(this.address) as T;
    }

    // 値の設定
    store(value: T): void {
        memory.storeObject(this.address, value);
    }

    // ポインタ演算
    offset(n: number): Pointer<T> {
        return new Pointer<T>(this.address + n);
    }

    // NULL チェック
    isNull(): boolean {
        return this.address === 0;
    }
}

// NULLポインタ
export const nullptr = new Pointer(0);

// ポインタの作成
export function allocPointer<T>(value?: T): Pointer<T> {
    const address = memory.allocate(8);  // ポインタサイズ
    const ptr = new Pointer<T>(address);
    if (value !== undefined) {
        ptr.store(value);
    }
    return ptr;
}
```

### 4.3 配列の実装

```typescript
// 配列型
export class CbArray<T> {
    private data: T[];

    constructor(size: number, initialValue?: T) {
        this.data = new Array(size);
        if (initialValue !== undefined) {
            this.data.fill(initialValue);
        }
    }

    get(index: number): T {
        if (index < 0 || index >= this.data.length) {
            throw new Error(`Array index out of bounds: ${index}`);
        }
        return this.data[index];
    }

    set(index: number, value: T): void {
        if (index < 0 || index >= this.data.length) {
            throw new Error(`Array index out of bounds: ${index}`);
        }
        this.data[index] = value;
    }

    get length(): number {
        return this.data.length;
    }
}
```

---

## 5. ランタイムライブラリ

### 5.1 標準ライブラリのTypeScript実装

#### ファイル: `runtime/typescript/stdlib.ts`

```typescript
import { memory, Pointer, CbArray } from './memory';

// 標準出力
export function print(value: any): void {
    process.stdout.write(String(value));
}

export function println(value: any): void {
    console.log(value);
}

// 文字列操作
export function strLen(str: string): number {
    return str.length;
}

export function strConcat(s1: string, s2: string): string {
    return s1 + s2;
}

export function strSubstr(str: string, start: number, length: number): string {
    return str.substr(start, length);
}

// 数学関数
export namespace Math {
    export function abs(x: number): number {
        return globalThis.Math.abs(x);
    }

    export function sqrt(x: number): number {
        return globalThis.Math.sqrt(x);
    }

    export function pow(x: number, y: number): number {
        return globalThis.Math.pow(x, y);
    }

    export function sin(x: number): number {
        return globalThis.Math.sin(x);
    }

    export function cos(x: number): number {
        return globalThis.Math.cos(x);
    }

    export function tan(x: number): number {
        return globalThis.Math.tan(x);
    }
}

// メモリ操作
export function memcpy(dest: number, src: number, size: number): void {
    for (let i = 0; i < size; i++) {
        const byte = memory.loadI8(src + i);
        memory.storeI8(dest + i, byte);
    }
}

export function memset(dest: number, value: number, size: number): void {
    for (let i = 0; i < size; i++) {
        memory.storeI8(dest + i, value);
    }
}

// 型変換
export function intToString(value: number): string {
    return value.toString();
}

export function stringToInt(str: string): number {
    return parseInt(str, 10);
}

export function floatToString(value: number): string {
    return value.toString();
}

export function stringToFloat(str: string): number {
    return parseFloat(str);
}
```

### 5.2 非同期関数のサポート

```typescript
// Futureの実装（非同期処理）
export class Future<T> {
    private promise: Promise<T>;

    constructor(executor: (resolve: (value: T) => void, reject: (reason?: any) => void) => void) {
        this.promise = new Promise(executor);
    }

    // await相当
    async wait(): Promise<T> {
        return await this.promise;
    }

    // then相当
    then<U>(onFulfilled: (value: T) => U): Future<U> {
        return new Future<U>((resolve, reject) => {
            this.promise.then(value => resolve(onFulfilled(value))).catch(reject);
        });
    }

    // catch相当
    catch(onRejected: (reason: any) => void): Future<T> {
        return new Future<T>((resolve, reject) => {
            this.promise.catch(onRejected).then(resolve).catch(reject);
        });
    }
}
```

---

## 6. 最適化とトランスパイル

### 6.1 TypeScriptコンパイラオプション

生成されるTypeScriptコードは、以下のオプションでコンパイルします：

#### `tsconfig.json`
```json
{
  "compilerOptions": {
    "target": "ES2020",
    "module": "commonjs",
    "lib": ["ES2020"],
    "outDir": "./dist",
    "rootDir": "./src",
    "strict": true,
    "esModuleInterop": true,
    "skipLibCheck": true,
    "forceConsistentCasingInFileNames": true,
    "declaration": true,
    "declarationMap": true,
    "sourceMap": true,
    "noUnusedLocals": true,
    "noUnusedParameters": true,
    "noImplicitReturns": true,
    "noFallthroughCasesInSwitch": true
  },
  "include": ["src/**/*"],
  "exclude": ["node_modules", "dist"]
}
```

### 6.2 ビルドスクリプト

```bash
#!/bin/bash
# build.sh

# TypeScriptコンパイル
tsc

# Node.jsで実行
node dist/main.js
```

---

## 7. 実装の詳細

### 7.1 TypeScriptコード生成の実装例

#### ファイル: `src/backend/codegen/typescript/typescript_codegen.cpp`

```cpp
#include "typescript_codegen.h"
#include <fstream>

namespace cb {
namespace codegen {
namespace typescript {

TypeScriptCodegen::TypeScriptCodegen() : next_var_index(0) {}

void TypeScriptCodegen::generate(const ir::lir::LIRProgram& program, const std::string& output_file) {
    // ランタイムライブラリのインポート
    emitter.emit_import("./runtime/stdlib", {"print", "println", "memory", "Pointer", "CbArray"});
    emitter.emit_statement("");

    // 関数の生成
    for (const auto& func : program.functions) {
        generate_function(func);
        emitter.emit_statement("");
    }

    // エントリーポイント
    emitter.emit_statement("// Entry point");
    emitter.emit_statement("if (require.main === module) {");
    emitter.increase_indent();
    emitter.emit_statement("main();");
    emitter.decrease_indent();
    emitter.emit_statement("}");

    // ファイルに書き込み
    emitter.write_to_file(output_file);
}

void TypeScriptCodegen::generate_function(const ir::lir::LIRFunction& func) {
    // 変数マッピングのリセット
    vreg_to_var.clear();
    next_var_index = 0;

    // パラメータ
    std::vector<std::pair<std::string, std::string>> params;
    for (size_t i = 0; i < func.arg_count; ++i) {
        std::string param_name = "arg" + std::to_string(i);
        std::string param_type = lir_type_to_ts_type(func.locals[i].type);
        params.push_back({param_name, param_type});
        vreg_to_var[i] = param_name;
    }

    // 戻り値の型
    std::string return_type = (func.return_type == TYPE_VOID) ? "void" : lir_type_to_ts_type(func.return_type);

    // 関数定義の開始
    emitter.begin_function(func.name, params, return_type);

    // ローカル変数の宣言
    for (size_t i = func.arg_count; i < func.locals.size(); ++i) {
        std::string var_name = "v" + std::to_string(next_var_index++);
        std::string var_type = lir_type_to_ts_type(func.locals[i].type);
        vreg_to_var[i] = var_name;
        emitter.emit_variable_declaration(var_name, var_type);
    }

    // 基本ブロックの生成
    for (const auto& block : func.blocks) {
        generate_basic_block(block);
    }

    // 関数定義の終了
    emitter.end_function();
}

void TypeScriptCodegen::generate_basic_block(const ir::lir::LIRBasicBlock& block) {
    // ラベル
    if (!block.label.empty()) {
        emitter.emit_label(block.label);
    }

    // 命令の生成
    for (const auto& inst : block.instructions) {
        std::string stmt = generate_instruction(inst);
        if (!stmt.empty()) {
            emitter.emit_statement(stmt);
        }
    }
}

std::string TypeScriptCodegen::generate_instruction(const ir::lir::LIRInstruction& inst) {
    switch (inst.opcode) {
    case ir::lir::LIRInstruction::Opcode::Move:
        return emit_move(inst);
    case ir::lir::LIRInstruction::Opcode::Add:
        return emit_add(inst);
    case ir::lir::LIRInstruction::Opcode::Sub:
        return emit_sub(inst);
    case ir::lir::LIRInstruction::Opcode::Mul:
        return emit_mul(inst);
    case ir::lir::LIRInstruction::Opcode::Call:
        return emit_call(inst);
    case ir::lir::LIRInstruction::Opcode::Return:
        return emit_return(inst);
    // 他の命令...
    default:
        return "";
    }
}

std::string TypeScriptCodegen::emit_add(const ir::lir::LIRInstruction& inst) {
    // dst = src1 + src2
    std::string dst = get_variable_name(inst.operands[0].vreg);
    std::string src1 = get_variable_name(inst.operands[1].vreg);
    std::string src2 = get_variable_name(inst.operands[2].vreg);

    return dst + " = " + src1 + " + " + src2 + ";";
}

std::string TypeScriptCodegen::emit_call(const ir::lir::LIRInstruction& inst) {
    // func(args...)
    std::string func_name = inst.func_name;
    std::string args;

    for (size_t i = 0; i < inst.operands.size(); ++i) {
        if (i > 0) args += ", ";
        args += get_variable_name(inst.operands[i].vreg);
    }

    return func_name + "(" + args + ");";
}

std::string TypeScriptCodegen::emit_return(const ir::lir::LIRInstruction& inst) {
    if (inst.operands.empty()) {
        return "return;";
    } else {
        std::string value = get_variable_name(inst.operands[0].vreg);
        return "return " + value + ";";
    }
}

std::string TypeScriptCodegen::lir_type_to_ts_type(TypeInfo type) {
    switch (type) {
    case TYPE_TINY:
    case TYPE_SHORT:
    case TYPE_INT:
    case TYPE_FLOAT:
    case TYPE_DOUBLE:
        return "number";
    case TYPE_LONG:
        return "bigint";
    case TYPE_CHAR:
    case TYPE_STRING:
        return "string";
    case TYPE_BOOL:
        return "boolean";
    case TYPE_VOID:
        return "void";
    default:
        return "any";
    }
}

std::string TypeScriptCodegen::get_variable_name(uint32_t vreg) {
    return vreg_to_var[vreg];
}

} // namespace typescript
} // namespace codegen
} // namespace cb
```

---

## 8. テストとデバッグ

### 8.1 TypeScriptテストフレームワーク

```typescript
// tests/typescript/test_add.ts
import { add } from '../../dist/generated';
import { expect } from 'chai';

describe('TypeScript Backend Tests', () => {
    it('should add two numbers', () => {
        const result = add(10, 20);
        expect(result).to.equal(30);
    });

    it('should multiply two numbers', () => {
        const result = mul(5, 7);
        expect(result).to.equal(35);
    });

    // 他のテスト...
});
```

### 8.2 デバッグ

TypeScriptのソースマップを生成することで、デバッグが容易になります：

```bash
# ソースマップ付きでコンパイル
tsc --sourceMap

# VS Codeでデバッグ
# launch.json
{
    "type": "node",
    "request": "launch",
    "name": "Debug TypeScript",
    "program": "${workspaceFolder}/dist/main.js",
    "outFiles": ["${workspaceFolder}/dist/**/*.js"],
    "sourceMaps": true
}
```

---

## 9. まとめ

このTypeScriptバックエンド設計により：

1. **TypeScript出力**: 型安全なコード生成
2. **デバッグ容易性**: ソースマップとVS Code統合
3. **エコシステム統合**: npm, Node.js, ブラウザで実行可能
4. **ポインタエミュレーション**: メモリクラスによる安全な実装
5. **ランタイムライブラリ**: 標準ライブラリのTypeScript実装

v0.14.0完了後、TypeScript対応により、Cb言語でフロントエンドとバックエンドの両方を開発可能になります。
