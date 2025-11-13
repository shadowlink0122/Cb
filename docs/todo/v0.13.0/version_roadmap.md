# Cb言語 バージョンロードマップ

**作成日**: 2025-11-13
**ステータス**: 方針確定

## FFI (Foreign Function Interface) の対応言語

### 対象：C ABI互換の任意の言語

`foreign`は**C ABI（Application Binary Interface）を公開している任意の言語**を対象とします。

#### 主要対応言語

| 言語 | 対応 | ファイル形式 | 備考 |
|------|------|-------------|------|
| **C** | ✅ 完全対応 | .so/.dylib/.dll | 主要ターゲット |
| **C++** | ✅ 完全対応 | .so/.dylib/.dll | `extern "C"`が必要 |
| **Rust** | ✅ 対応 | .so/.dylib/.dll | `#[no_mangle]`と`extern "C"`が必要 |
| **Zig** | ✅ 対応 | .so/.dylib/.dll | C ABIをネイティブサポート |
| **Go** | ✅ 対応 | .so/.dylib/.dll | cgoで`export`が必要 |
| **Nim** | ✅ 対応 | .so/.dylib/.dll | `{.exportc.}`が必要 |
| **D** | ✅ 対応 | .so/.dylib/.dll | `extern(C)`が必要 |
| **Assembly** | ✅ 対応 | .so/.dylib/.dll | C ABIに準拠すれば可能 |
| Python | ⚠️ 限定的 | .so/.dylib/.dll | ctypes/cffiで作成したライブラリ |
| Java | ⚠️ 限定的 | .so/.dylib/.dll | JNIで作成したライブラリ |

**原則**: C ABIを公開していれば、どの言語でも対応可能

#### 対応ファイル形式

##### 共有ライブラリ（動的リンク）

| プラットフォーム | 拡張子 | 例 |
|-----------------|--------|-----|
| Linux | `.so` | libmath.so |
| macOS | `.dylib` | libmath.dylib |
| Windows | `.dll` | math.dll |

##### 静的ライブラリ（将来対応予定）

| プラットフォーム | 拡張子 | 例 |
|-----------------|--------|-----|
| Linux/macOS | `.a` | libmath.a |
| Windows | `.lib` | math.lib |

### 使用例

#### 例1: C言語

**C側**（math.c）:
```c
// C言語で実装
int add(int a, int b) {
    return a + b;
}
```

コンパイル:
```bash
gcc -shared -fPIC math.c -o libmath.so
```

**Cb側**（foreign/math.cbf）:
```cb
foreign module math {
    int add(int a, int b);
}
```

**使用**:
```cb
use foreign.math {
    int add(int a, int b);
}

void main() {
    int result = math.add(10, 20);
    println(result);  // 30
}
```

#### 例2: C++言語

**C++側**（math.cpp）:
```cpp
// C++で実装（extern "C"が必要）
extern "C" {
    int add(int a, int b) {
        return a + b;
    }

    double sqrt(double x) {
        return std::sqrt(x);
    }
}
```

コンパイル:
```bash
g++ -shared -fPIC math.cpp -o libmath.so
```

**Cb側**は同じ:
```cb
use foreign.math {
    int add(int a, int b);
    double sqrt(double x);
}
```

#### 例3: Rust言語

**Rust側**（lib.rs）:
```rust
// Rustで実装
#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 {
    a + b
}

#[no_mangle]
pub extern "C" fn factorial(n: i32) -> i64 {
    (1..=n as i64).product()
}
```

**Cargo.toml**:
```toml
[lib]
crate-type = ["cdylib"]
```

コンパイル:
```bash
cargo build --release
# 生成: target/release/libmath.so
```

**Cb側**（foreign/math.cbf）:
```cb
foreign module math {
    int add(int a, int b);
    long factorial(int n);
}
```

**使用**:
```cb
use foreign.math {
    int add(int a, int b);
    long factorial(int n);
}

void main() {
    int sum = math.add(10, 20);
    long fact = math.factorial(10);
    println(sum, fact);  // 30 3628800
}
```

#### 例4: Zig言語

**Zig側**（math.zig）:
```zig
// Zigで実装
export fn add(a: i32, b: i32) i32 {
    return a + b;
}

export fn multiply(a: i32, b: i32) i32 {
    return a * b;
}
```

コンパイル:
```bash
zig build-lib math.zig -dynamic
# 生成: libmath.so
```

**Cb側**は同じ形式:
```cb
use foreign.math {
    int add(int a, int b);
    int multiply(int a, int b);
}
```

#### 例5: Go言語

**Go側**（math.go）:
```go
package main

import "C"

//export add
func add(a, b C.int) C.int {
    return a + b
}

//export factorial
func factorial(n C.int) C.longlong {
    result := C.longlong(1)
    for i := C.int(2); i <= n; i++ {
        result *= C.longlong(i)
    }
    return result
}

func main() {}
```

コンパイル:
```bash
go build -buildmode=c-shared -o libmath.so math.go
```

**Cb側**:
```cb
use foreign.math {
    int add(int a, int b);
    long factorial(int n);
}
```

#### 例6: アセンブリ言語

**Assembly側**（math.asm）:
```nasm
; x86-64 assembly (NASM)
global add

section .text

add:
    mov rax, rdi    ; 第1引数をraxに
    add rax, rsi    ; 第2引数を加算
    ret
```

コンパイル:
```bash
nasm -f elf64 math.asm -o math.o
ld -shared math.o -o libmath.so
```

**Cb側**:
```cb
use foreign.math {
    int add(int a, int b);
}

void main() {
    int result = math.add(100, 200);
    println(result);  // 300
}
```

### 対応言語の選定基準

#### ✅ 対応可能な言語の条件

1. **C ABIを公開できる**
   - `extern "C"` や同等の機能がある
   - 共有ライブラリ（.so/.dylib/.dll）を生成できる

2. **関数シグネチャが明確**
   - 引数と戻り値の型が定義できる
   - Cbの型システムとマッピング可能

3. **安定したABI**
   - バージョン間でABIが変わらない
   - プラットフォーム間で一貫性がある

#### ❌ 対応困難な言語

- **JavaScript/Node.js**: C ABIを直接サポートしない（N-APIは複雑）
- **Python**: Pythonインタプリタが必要（単独の.soは作れない）
- **Ruby**: 同上
- **Java**: JVMが必要（JNIは複雑）

### 言語別の注意点

#### C++
```cpp
// ✅ 正しい（extern "C"を使用）
extern "C" {
    int add(int a, int b) { return a + b; }
}

// ❌ 間違い（C++の名前マングリング）
int add(int a, int b) { return a + b; }
// → シンボル名が_Z3addii等に変わってしまう
```

#### Rust
```rust
// ✅ 正しい
#[no_mangle]
pub extern "C" fn add(a: i32, b: i32) -> i32 { a + b }

// ❌ 間違い（no_mangleがない）
pub extern "C" fn add(a: i32, b: i32) -> i32 { a + b }
```

### まとめ

**`foreign`の対象**:
- 主要: **C/C++** （最も広く使われる）
- サポート: **Rust, Zig, Go, Nim, D, Assembly**
- 条件: **C ABIを公開できる任意の言語**

**ファイル形式**:
- `.so` (Linux)
- `.dylib` (macOS)
- `.dll` (Windows)
- `.cbf` (Cb Foreign定義ファイル・型情報)

**原則**: 「C ABIで呼び出せるなら、どの言語でもOK」

## バージョン戦略

### v0.x.x - インタプリタ版

**アーキテクチャ**:
- 再帰下降パーサー + ASTインタプリタ
- C++で実装された実行エンジン
- ソースコードを直接解釈・実行

**目的**:
- 言語仕様の確立
- 機能の実験と検証
- ユーザーフィードバックの収集

**制限事項**:
- インラインアセンブリは不可（技術的に困難）
- パフォーマンスは中程度
- 実行ファイルは生成しない

---

### v1.0.0 - ネイティブコンパイラ版

**アーキテクチャ**:
- ソースコード → 機械語（実行ファイル）
- GCC/Clang のようなネイティブコンパイラ
- C++またはLLVM IRを中間表現として使用（検討中）

**新機能**:
- ✅ インラインアセンブリ (`asm("")`) の実装可能
- ✅ 最適化されたバイナリ生成
- ✅ 実行ファイルの生成
- ✅ 大幅なパフォーマンス向上

**実装方針**:
- JITコンパイラは実装しない（複雑すぎる）
- インタプリタからネイティブコンパイラへ直接移行
- v0.x.xで確立された言語仕様を元に実装

---

## 各バージョンの詳細

### v0.13.0 (現在開発中)

**主要機能**:
- FFI (Foreign Function Interface)
  - `use foreign.modulename { ... }`
  - 外部C/C++ライブラリとの連携
- プリプロセッサ
  - `#define`, `#ifdef`, `#ifndef`, `#elseif`, `#else`, `#endif`
  - 条件付きコンパイル
- C風マクロ
  - 定数定義
  - 関数マクロ

**実装期間**: 8週間

---

### v0.14.0 - v0.20.0（予定）

**目標**: インタプリタ版の完成

実装予定の機能:
- inline関数（マクロの代替）
- constexpr（コンパイル時定数）
- 標準ライブラリの拡充
- エラーメッセージの改善
- デバッガーの実装
- パッケージマネージャー（検討中）

---

### v1.0.0（将来）

**目標**: ネイティブコンパイラの完成

**開発アプローチ（検討中）**:

#### アプローチA: C++トランスパイラ

```
Cb → C++ → 機械語
```

**メリット**:
- ✅ 実装が比較的簡単
- ✅ C++の最適化を活用
- ✅ 既存のツールチェーン（GCC/Clang）を利用

**デメリット**:
- ❌ C++への依存
- ❌ コンパイル時間が長い

#### アプローチB: LLVM IRバックエンド

```
Cb → LLVM IR → 機械語
```

**メリット**:
- ✅ 強力な最適化
- ✅ 複数のアーキテクチャに対応
- ✅ モダンなコンパイラ技術

**デメリット**:
- ❌ LLVM依存（大きな依存関係）
- ❌ 実装が複雑

#### アプローチC: 独自バックエンド

```
Cb → 独自IR → 機械語
```

**メリット**:
- ✅ 完全な制御
- ✅ 依存関係なし
- ✅ Cbに最適化されたIR

**デメリット**:
- ❌ 実装が非常に困難
- ❌ 開発期間が長い
- ❌ 最適化が限定的

**推奨**: アプローチA（C++トランスパイラ）→ 将来的にアプローチB（LLVM）へ移行

---

## インラインアセンブリの実装時期

### v0.x.x（インタプリタ）
**❌ 実装しない**

理由:
- インタプリタで機械語を実行するのは技術的に困難
- LLVM JIT等の巨大な依存関係が必要
- FFIで代替可能

### v1.0.0（ネイティブコンパイラ）
**✅ 実装可能**

実装方法（GCC/Clang方式）:
```cb
void main() {
    int x = 42;
    int result;

    asm("mov %1, %%eax\n"
        "add $10, %%eax\n"
        "mov %%eax, %0\n"
        : "=r"(result)
        : "r"(x)
        : "eax");

    println(result);  // 52
}
```

**実装パターン**:
- C++トランスパイラの場合: そのまま`__asm__`に変換
- LLVM IRの場合: InlineAsm命令を使用
- 独自バックエンドの場合: 機械語を直接生成

---

## マイルストーン

### Phase 1: インタプリタの完成（v0.13.0 - v0.20.0）
- [ ] FFI実装（v0.13.0）
- [ ] プリプロセッサ実装（v0.13.0）
- [ ] inline/constexpr実装（v0.14.0）
- [ ] 標準ライブラリ拡充（v0.15.0 - v0.18.0）
- [ ] ツール整備（v0.19.0 - v0.20.0）

**期間**: 1-2年（予定）

### Phase 2: コンパイラの設計（v0.21.0）
- [ ] コンパイラアーキテクチャの決定
- [ ] 中間表現（IR）の設計
- [ ] バックエンドの選定（C++/LLVM/独自）
- [ ] プロトタイプ実装

**期間**: 3-6ヶ月

### Phase 3: コンパイラの実装（v0.22.0 - v0.99.0）
- [ ] フロントエンド（パーサー）の移植
- [ ] 中間表現の生成
- [ ] バックエンド（コード生成）の実装
- [ ] 最適化パスの実装
- [ ] テストとデバッグ

**期間**: 1-2年

### Phase 4: v1.0.0リリース
- [ ] すべての機能の動作確認
- [ ] ドキュメントの整備
- [ ] パフォーマンステスト
- [ ] 安定性の確認

---

## なぜJITコンパイラを実装しないのか？

### JITコンパイラの欠点

1. **複雑性が非常に高い**
   - LLVM等の巨大なライブラリが必要
   - 実装とメンテナンスが困難

2. **ビルドサイズの増大**
   - LLVM: 数百MB
   - ビルド時間: 数時間

3. **限定的なメリット**
   - 実行時コンパイルのオーバーヘッド
   - Cbの用途（システムプログラミング）には不要

4. **デプロイの複雑化**
   - ランタイムに巨大なライブラリが必要
   - 配布サイズが大きくなる

### ネイティブコンパイラの利点

1. **シンプル**
   - 従来のコンパイラと同じアーキテクチャ
   - 理解しやすく、メンテナンスしやすい

2. **高性能**
   - 最適化されたバイナリを生成
   - 実行時のオーバーヘッドなし

3. **デプロイが容易**
   - 単一の実行ファイル
   - 依存関係なし（静的リンクの場合）

4. **既存のエコシステム**
   - GCC/Clangのツールチェーンを活用
   - デバッガー（GDB）との統合

---

## まとめ

| バージョン | アーキテクチャ | インラインasm | 実行形態 | ステータス |
|----------|--------------|--------------|---------|----------|
| v0.x.x | インタプリタ | ❌ 不可 | ソースコード実行 | 開発中 |
| v1.0.0 | ネイティブコンパイラ | ✅ 可能 | 実行ファイル生成 | 将来 |

**現在の焦点**: v0.13.0（FFI + プリプロセッサ）の完成

**長期目標**: v1.0.0（ネイティブコンパイラ）の実装
