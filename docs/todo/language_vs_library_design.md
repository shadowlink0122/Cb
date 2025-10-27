# 言語機能 vs ライブラリの設計方針

**作成日**: 2025年10月27日  
**対象**: Cb言語のコア設計  
**優先度**: 🔴 最重要（アーキテクチャ決定）

---

## 📋 概要

動的配列やメモリ管理を「言語機能」として実装するか、「標準ライブラリパッケージ」として実装するかの設計判断です。

**結論: ほとんどをライブラリパッケージとして実装すべき**

---

## 🎯 設計原則: "言語機能は最小限に"

### Rust/Zigの哲学を参考に

```rust
// Rust: Vec<T>は標準ライブラリ（言語機能ではない）
use std::vec::Vec;

let mut v = Vec::new();
v.push(10);
```

```zig
// Zig: ArrayListは標準ライブラリ
const std = @import("std");
var list = std.ArrayList(i32).init(allocator);
try list.append(10);
```

### Cb言語の方針

```cb
// ❌ 悪い例: 言語機能として組み込み
int[] arr;  // コンパイラが特殊扱い
arr.push(10);

// ✅ 良い例: ライブラリとして提供
import std.vector;

Vector<int> arr = Vector.new();
arr.push(10);
```

---

## 🔧 言語機能とライブラリの境界線

### 言語機能として実装すべきもの（最小限）

| 機能 | 理由 | 例 |
|------|------|-----|
| **基本型** | コンパイラが型チェック必要 | `int`, `double`, `bool`, `char` |
| **ポインタ** | メモリ安全性の基盤 | `T*`, `&T`, `nullptr` |
| **配列（固定サイズ）** | スタック確保、型の一部 | `int[10]` |
| **構造体** | 型システムの基盤 | `struct Point { ... }` |
| **ジェネリクス** | 型パラメータ | `<T>` |
| **演算子** | 構文の一部 | `+`, `-`, `*`, `/`, `[]`, `.` |
| **制御構文** | 構文の一部 | `if`, `for`, `while`, `defer` |

### ライブラリとして実装すべきもの

| 機能 | 理由 | パッケージ名 |
|------|------|-------------|
| **動的配列** | メモリアロケータに依存 | `std.vector` |
| **Queue/Stack** | データ構造は差し替え可能 | `std.collections` |
| **メモリアロケータ** | 環境依存 | `std.allocator` |
| **I/O** | プラットフォーム依存 | `std.io` |
| **文字列** | 動的メモリが必要 | `std.string` |
| **ファイル** | OS依存 | `std.fs` |
| **ネットワーク** | OS依存 | `std.net` |
| **スレッド** | OS依存 | `std.thread` |
| **イベントループ** | ランタイム機能 | `std.async` |

---

## 📦 標準ライブラリの構成

### パッケージ構造

```
std/
├── core/
│   ├── allocator.cb        # メモリアロケータのインターフェース
│   ├── option.cb           # Option<T>
│   ├── result.cb           # Result<T,E>
│   └── panic.cb            # パニック処理
│
├── collections/
│   ├── vector.cb           # Vector<T> 動的配列
│   ├── queue.cb            # Queue<T>
│   ├── stack.cb            # Stack<T>
│   ├── hashmap.cb          # HashMap<K,V>
│   └── linkedlist.cb       # LinkedList<T>
│
├── allocator/
│   ├── bump.cb             # バンプアロケータ
│   ├── freelist.cb         # フリーリストアロケータ
│   ├── slab.cb             # スラブアロケータ
│   └── system.cb           # システムアロケータ（malloc/free）
│
├── io/
│   ├── stdio.cb            # println, print_int
│   ├── uart.cb             # UARTベースI/O
│   └── file.cb             # ファイルI/O（OS環境のみ）
│
├── string/
│   ├── string.cb           # String型
│   └── format.cb           # 文字列フォーマット
│
├── async/
│   ├── event_loop.cb       # イベントループ
│   ├── timer.cb            # タイマー
│   └── task.cb             # 非同期タスク
│
└── platform/
    ├── bare_metal/
    │   ├── stm32.cb
    │   ├── esp32.cb
    │   └── avr.cb
    └── os/
        ├── linux.cb
        ├── macos.cb
        └── windows.cb
```

---

## 🎨 ユーザーコードの例

### 例1: 動的配列（アロケータを型パラメータで）

```cb
// std/collections/vector.cb

// Vectorはアロケータを型パラメータとして受け取る
struct Vector<T, A: Allocator> {
    T* data;
    int length;
    int capacity;
}

impl Vector<T, A> {
    // デフォルトコンストラクタ
    Vector<T, A> new() {
        Vector<T, A> v;
        v.data = nullptr;
        v.length = 0;
        v.capacity = 0;
        return v;
    }
    
    void push(T value) {
        if (self.length >= self.capacity) {
            self.resize();
        }
        self.data[self.length] = value;
        self.length = self.length + 1;
    }
    
    void resize() {
        int new_capacity = self.capacity == 0 ? 4 : self.capacity * 2;
        
        // Aはアロケータ型なので、静的メソッドとして呼べる
        T* new_data = A.allocate(sizeof(T) * new_capacity);
        
        // 既存データをコピー
        for (int i = 0; i < self.length; i = i + 1) {
            new_data[i] = self.data[i];
        }
        
        if (self.data != nullptr) {
            A.deallocate(self.data);
        }
        
        self.data = new_data;
        self.capacity = new_capacity;
    }
}
```

### 例2: ユーザーコード

```cb
// ユーザーコード

import std.collections.vector;
import std.allocator.system;  // OS環境
// import std.allocator.bump;  // ベアメタル環境

int main() {
    // アロケータを型パラメータで指定
    Vector<int, SystemAllocator> arr = Vector.new();
    arr.push(10);
    arr.push(20);
    arr.push(30);
    
    // または型エイリアスで簡潔に
    using Vec<T> = Vector<T, SystemAllocator>;
    Vec<int> arr2 = Vec.new();
    arr2.push(100);
    
    return 0;
}
```

```cb
// ベアメタル環境の例

import std.collections.vector;
import std.allocator.bump;

// ベアメタル用のVector型エイリアス
using Vec<T> = Vector<T, BumpAllocator>;

int main() {
    // グローバルヒープ（リンカスクリプトで定義）
    BumpAllocator.init(__heap_start, __heap_size);
    
    // 使い方はOS環境と同じ
    Vec<int> tasks = Vec.new();
    tasks.push(1);
    tasks.push(2);
    
    return 0;
}
```

---

## 🔌 アロケータの抽象化

### インターフェース定義（トレイト方式）

```cb
// std/core/allocator.cb

// アロケータトレイト（interface的なもの）
trait Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}

// 使用例：
// struct Vector<T, A: Allocator> { ... }
// ↑ Aは Allocator トレイトを実装している型でなければならない
```

### システムアロケータ（OS環境）

```cb
// std/allocator/system.cb

import std.core.allocator;

struct SystemAllocator {}

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        return malloc(size);
    }
    
    void deallocate(void* ptr) {
        free(ptr);
    }
}

// 使用例:
// Vector<int, SystemAllocator> v = Vector.new();
```

### バンプアロケータ（ベアメタル環境）

```cb
// std/allocator/bump.cb

import std.core.allocator;

struct BumpAllocator {
    // グローバル状態（静的変数）
    static char* heap_start;
    static char* heap_end;
    static char* current;
}

impl BumpAllocator {
    // 初期化（main前に呼ばれる）
    void init(char* heap, int size) {
        BumpAllocator.heap_start = heap;
        BumpAllocator.heap_end = heap + size;
        BumpAllocator.current = heap;
    }
}

impl Allocator for BumpAllocator {
    void* allocate(int size) {
        int aligned_size = (size + 7) & ~7;
        char* ptr = BumpAllocator.current;
        char* new_current = ptr + aligned_size;
        
        if (new_current > BumpAllocator.heap_end) {
            return nullptr;
        }
        
        BumpAllocator.current = new_current;
        return ptr;
    }
    
    void deallocate(void* ptr) {
        // バンプアロケータは個別解放しない
    }
}

// 使用例:
// Vector<int, BumpAllocator> v = Vector.new();
```

---

## 🎯 メリット

### 1. **型安全性とゼロコスト抽象化**
```cb
// アロケータは型パラメータなので、コンパイル時に解決
Vector<int, SystemAllocator> v1 = Vector.new();  // 静的ディスパッチ
Vector<int, BumpAllocator> v2 = Vector.new();    // 静的ディスパッチ

// 仮想関数テーブル不要！
// A.allocate() は直接 SystemAllocator.allocate() にコンパイルされる
```

### 2. **ベアメタル対応が自然**
```cb
// ベアメタル環境
using Vec<T> = Vector<T, BumpAllocator>;

BumpAllocator.init(__heap_start, __heap_size);

Vec<int> tasks = Vec.new();      // 自動的にBumpAllocatorを使用
Vec<Timer> timers = Vec.new();   // 同じく
```

### 3. **テストが容易**
```cb
// テスト用のモックアロケータ
struct MockAllocator {
    static int alloc_count;
    static int dealloc_count;
}

impl Allocator for MockAllocator {
    void* allocate(int size) {
        MockAllocator.alloc_count = MockAllocator.alloc_count + 1;
        return malloc(size);
    }
    
    void deallocate(void* ptr) {
        MockAllocator.dealloc_count = MockAllocator.dealloc_count + 1;
        free(ptr);
    }
}

// 使用
Vector<int, MockAllocator> v = Vector.new();
for (int i = 0; i < 100; i = i + 1) {
    v.push(i);
}
assert(MockAllocator.alloc_count == MockAllocator.dealloc_count);
```

### 4. **コンパイラがシンプル**
- 動的配列の特殊処理が不要
- トレイト境界チェックだけで済む
- インライン化により最適化が容易

### 5. **Rustライクな設計**
```rust
// Rust の例（参考）
use std::vec::Vec;
use std::alloc::{Allocator, Global};

let v: Vec<i32, MyAllocator> = Vec::new_in(my_allocator);
```

Cb言語でも同様の設計：
```cb
Vector<int, MyAllocator> v = Vector.new();
```

---

## 🔄 言語機能として残すもの

### new/delete演算子（構文糖衣）

```cb
// 構文糖衣として残す
Point* p = new Point;  // グローバルアロケータを使用
delete p;

// ↓内部的には↓

Point* p = GlobalAllocator.allocate(sizeof(Point));
__call_constructor(p);
// ...
__call_destructor(p);
GlobalAllocator.deallocate(p);
```

### グローバルアロケータの設定

```cb
// デフォルトのグローバルアロケータを設定
#if BARE_METAL
    using GlobalAllocator = BumpAllocator;
#else
    using GlobalAllocator = SystemAllocator;
#endif

// ユーザーがオーバーライド可能
// cb_config.json で設定
{
    "global_allocator": "BumpAllocator"  // または "SystemAllocator"
}
```

### 型エイリアスで簡潔に

```cb
// プロジェクト全体で使う型エイリアス
using Vec<T> = Vector<T, GlobalAllocator>;
using Queue<T> = Queue<T, GlobalAllocator>;
using Stack<T> = Stack<T, GlobalAllocator>;

// 使用時は簡潔
Vec<int> arr = Vec.new();
Queue<Task> tasks = Queue.new();
```

---

## 📊 比較: RustとZigの設計

### Rust

```rust
// Vec<T>は標準ライブラリ
use std::vec::Vec;
use std::collections::HashMap;

// アロケータも差し替え可能
use std::alloc::{Allocator, Global};

let v: Vec<i32, MyAllocator> = Vec::new_in(my_allocator);
```

**特徴**:
- ✅ Vec<T>はライブラリ
- ✅ カスタムアロケータをサポート
- ✅ no_std環境（ベアメタル）に対応

### Zig

```zig
// ArrayListは標準ライブラリ
const std = @import("std");

// アロケータは必須引数
var list = std.ArrayList(i32).init(allocator);
try list.append(10);

// アロケータを選択
var gpa = std.heap.GeneralPurposeAllocator(.{}){};
var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
```

**特徴**:
- ✅ すべてのデータ構造がアロケータを必要とする
- ✅ アロケータは明示的に渡す
- ✅ freestanding環境に完全対応

### Cb言語の方針（Zigに近い）

```cb
import std.collections.vector;
import std.allocator.system;

SystemAllocator alloc = SystemAllocator.new();
Vector<int> v = Vector.new(&alloc);
v.push(10);
```

---

## 🚀 実装方針の変更

### Phase 1aの変更

**旧計画（言語機能）**:
```
1. new/deleteをコンパイラに実装
2. int[]構文をコンパイラに実装
3. .push()などをコンパイラに実装
```

**新計画（ライブラリ）**:
```
1. new/deleteは最小限の構文糖衣のみ
2. Vector<T>は標準ライブラリ（std.collections.vector）
3. アロケータは抽象化（std.core.allocator）
```

### ファイル構成

```
stdlib/
├── core/
│   ├── allocator.cb        # Allocator trait
│   ├── option.cb
│   └── result.cb
│
├── collections/
│   ├── vector.cb           # Vector<T>
│   ├── queue.cb
│   └── stack.cb
│
└── allocator/
    ├── system.cb           # malloc/free
    ├── bump.cb
    ├── freelist.cb
    └── slab.cb
```

---

## 🧪 テストケース

### テスト1: アロケータの切り替え

```cb
// tests/allocator/test_switch_allocator.cb

import std.collections.vector;
import std.allocator.system;
import std.allocator.bump;

void test_switch_allocator() {
    // システムアロケータ
    Vector<int, SystemAllocator> v1 = Vector.new();
    v1.push(10);
    
    // バンプアロケータ（グローバル初期化済み）
    Vector<int, BumpAllocator> v2 = Vector.new();
    v2.push(20);
    
    assert(v1[0] == 10);
    assert(v2[0] == 20);
}
```

### テスト2: カスタムアロケータ

```cb
// tests/allocator/test_custom_allocator.cb

import std.core.allocator;
import std.collections.vector;

struct CountingAllocator {
    static int alloc_count;
    static int dealloc_count;
}

impl Allocator for CountingAllocator {
    void* allocate(int size) {
        CountingAllocator.alloc_count = CountingAllocator.alloc_count + 1;
        return malloc(size);
    }
    
    void deallocate(void* ptr) {
        CountingAllocator.dealloc_count = CountingAllocator.dealloc_count + 1;
        free(ptr);
    }
}

void test_custom_allocator() {
    CountingAllocator.alloc_count = 0;
    CountingAllocator.dealloc_count = 0;
    
    {
        Vector<int, CountingAllocator> v = Vector.new();
        for (int i = 0; i < 100; i = i + 1) {
            v.push(i);  // 複数回のalloc
        }
    }  // vがスコープを抜けてデストラクタ呼ばれる
    
    // メモリリークチェック
    assert(CountingAllocator.alloc_count == CountingAllocator.dealloc_count);
}
```

---

## ✅ 結論

### 採用する設計

**トレイト + ジェネリクス設計**:
1. ✅ 動的配列は `Vector<T, A: Allocator>` として実装
2. ✅ アロケータは `trait Allocator` で抽象化
3. ✅ 型パラメータでアロケータを指定（ゼロコスト抽象化）
4. ✅ new/deleteは最小限の構文糖衣（GlobalAllocatorを使用）
5. ✅ ベアメタル環境でもOS環境でも同じAPIを使用

### メリット
- ✅ ベアメタル環境に完全対応
- ✅ ゼロコスト抽象化（静的ディスパッチ）
- ✅ ユーザーがカスタマイズ可能
- ✅ コンパイラがシンプル
- ✅ テストが容易
- ✅ Rust/Zigのベストプラクティスに従う
- ✅ 型安全性が高い

### 実装の変更点
- ❌ アロケータを「注入」する設計は不採用
- ✅ アロケータを「型パラメータ」として実装
- ✅ `trait Allocator` を定義
- ✅ `Vector<T, A: Allocator>` として実装
- ✅ 型エイリアス `using Vec<T> = Vector<T, GlobalAllocator>` で簡潔化

---

**作成者**: GitHub Copilot  
**レビュアー**: shadowlink0122  
**最終更新**: 2025年10月27日  
**ステータス**: 設計変更提案  
**影響**: Phase 1a実装方針の大幅変更
