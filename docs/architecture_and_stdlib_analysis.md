# Cbアーキテクチャと標準ライブラリ設計分析

## はじめに

本文書は、Cb言語のベアメタル対応実装において、**標準ライブラリ化とモジュールシステムの必要性**について詳細に分析し、最適な実装順序を提案します。

## 現在のアーキテクチャ分析

### 1. print/println実装の現状

#### 現在の実装構造
```
OutputManager::print_value() → printf() → libc（標準Cライブラリ）
OutputManager::print_value_with_newline() → printf() + putchar('\n') → libc
```

#### 問題点の詳細
1. **完全なlibc依存**
   - `printf()`, `fprintf()`, `putchar()`を直接使用
   - `<cstdio>`, `<cinttypes>`への依存
   - ベアメタル環境では利用不可能

2. **ハードコーディングされた出力先**
   - 常にstdoutに固定
   - 出力先の変更が不可能（ファイル、ネットワーク、シリアルポートなど）

3. **プラットフォーム固有の出力処理**
   - ベアメタル: シリアルUART、VGAバッファ、LCDディスプレイ
   - WebAssembly: JavaScript console.log
   - ネイティブ: stdout/stderr

### 2. メモリ管理の現状

#### 現在の実装構造
```cpp
// 動的メモリ割り当て
std::vector<Variable> variables;
std::string str_values;
std::map<std::string, Function> functions;
```

#### 問題点の詳細
1. **標準C++ライブラリ依存**
   - `std::vector`, `std::string`, `std::map`を多用
   - ベアメタル環境では利用不可能

2. **動的メモリ割り当て**
   - `new/delete`, `malloc/free`への依存
   - ベアメタル環境では手動メモリ管理が必要

### 3. エラーハンドリングの現状

#### 現在の実装構造
```cpp
// C++例外使用
throw std::runtime_error("Error message");
throw ReturnException(value);
```

#### 問題点の詳細
1. **C++例外機構依存**
   - 標準ライブラリのrtti（Run-Time Type Information）が必要
   - ベアメタル環境では例外処理が利用できない場合が多い

## 標準ライブラリ化の必要性分析

### 1. 現在のprintが標準ライブラリ的機能となる理由

#### 機能の汎用性
```cb
// 現在サポートしている機能
print(42);                    // 数値出力
print("Hello");              // 文字列出力  
print(array[0]);             // 配列要素出力
print(some_function());      // 関数戻り値出力

// 将来的に必要となる機能
printf("Number: %d", value); // フォーマット出力
println(multiple, args);     // 複数引数出力
```

#### プラットフォーム抽象化の必要性
```cb
// 理想的な抽象化レベル
import std.io;

// 自動的にプラットフォームに応じた実装が選択される
io.print("Hello");           // ベアメタル: UART出力
                            // WASM: console.log
                            // ネイティブ: printf
```

### 2. モジュールシステムの必要性分析

#### 現在のモノリシック構造の問題
1. **機能の分離不可能**
   - すべての機能が単一バイナリに含まれる
   - ベアメタル環境でのメモリ使用量最適化ができない

2. **プラットフォーム固有コードの混在**
   - 異なるターゲット用のコードが同じファイルに存在
   - 条件コンパイルが複雑になる

#### モジュールシステムのメリット
1. **選択的機能インポート**
   ```cb
   // 必要な機能のみをインポート（メモリ節約）
   import std.io.serial;     // シリアル出力のみ
   import std.memory.static; // 静的メモリ管理のみ
   ```

2. **プラットフォーム固有実装の分離**
   ```cb
   // プラットフォーム毎の実装を選択
   import std.io.uart   when target("baremetal");
   import std.io.console when target("wasm");
   import std.io.stdio  when target("native");
   ```

## 実装順序の検討

### 選択肢1: ベアメタル対応優先

#### メリット
- 現在の機能をベアメタル環境で即座に利用可能
- 低レベル要件が明確になる
- パフォーマンス最適化が早期に可能

#### デメリット
- 将来的な大規模リファクタリングが必要
- モジュラー設計の恩恵を受けられない
- 複数プラットフォーム対応が困難

#### 実装アプローチ
```cpp
// 条件コンパイル方式
#ifdef CB_TARGET_BAREMETAL
    void cb_print_impl(const char* str) {
        uart_write_string(str);
    }
#else
    void cb_print_impl(const char* str) {
        printf("%s", str);
    }
#endif
```

### 選択肢2: モジュールシステム優先

#### メリット
- 将来的な拡張性が高い
- プラットフォーム固有実装の綺麗な分離
- 標準ライブラリの段階的構築が可能

#### デメリット
- 実装期間が長い
- ベアメタル対応が遅れる
- 設計の複雑さが増加

#### 実装アプローチ
```cb
// モジュール定義
module std.io {
    export interface PrintInterface {
        void print(string text);
        void println(string text);
    }
    
    export implementation for baremetal: UARTPrinter;
    export implementation for wasm: ConsolePrinter;
    export implementation for native: StdioPrinter;
}
```

### 選択肢3: ハイブリッドアプローチ（推奨）

#### 段階的実装戦略
1. **Phase 5.1: 出力抽象化レイヤー実装** (2-3週間)
   - OutputInterface の定義
   - 条件コンパイル対応
   - ベアメタル向けUART実装

2. **Phase 5.2: 基本モジュールシステム実装** (4-6週間)
   - import/export構文の実装
   - モジュール解決システム
   - std.ioモジュールの実装

3. **Phase 5.3: 標準ライブラリ拡張** (継続的)
   - std.memory, std.math等の追加
   - プラットフォーム固有実装の追加

## 具体的な設計提案

### 1. 出力抽象化レイヤー

#### インターフェース設計
```cpp
// src/common/io_interface.h
class IOInterface {
public:
    virtual void write_char(char c) = 0;
    virtual void write_string(const char* str) = 0;
    virtual void write_formatted(const char* format, ...) = 0;
    virtual ~IOInterface() = default;
};

// プラットフォーム固有実装
class BareMetalUARTOutput : public IOInterface {
public:
    void write_char(char c) override;
    void write_string(const char* str) override;
    void write_formatted(const char* format, ...) override;
};

class NativeStdioOutput : public IOInterface {
public:
    void write_char(char c) override { putchar(c); }
    void write_string(const char* str) override { printf("%s", str); }
    void write_formatted(const char* format, ...) override;
};
```

#### 工場パターンによる実装選択
```cpp
// src/common/io_factory.h
class IOFactory {
public:
    static std::unique_ptr<IOInterface> create_output();
private:
#ifdef CB_TARGET_BAREMETAL
    static std::unique_ptr<IOInterface> create_baremetal_output();
#elif CB_TARGET_WASM
    static std::unique_ptr<IOInterface> create_wasm_output();
#else
    static std::unique_ptr<IOInterface> create_native_output();
#endif
};
```

### 2. モジュールシステム設計

#### 構文設計
```cb
// Cbソースコード内でのモジュール使用
import std.io;
import std.memory.static;

int main() {
    io.println("Hello from module!");
    return 0;
}
```

#### モジュール定義ファイル
```cb
// stdlib/std/io.cb
module std.io {
    export fn print(text: string);
    export fn println(text: string);
    export fn printf(format: string, ...args);
}
```

#### 実装ファイル（プラットフォーム固有）
```cb
// stdlib/std/io/baremetal.cb
implementation std.io for target("baremetal") {
    fn print(text: string) {
        uart_write_string(text.c_str());
    }
    
    fn println(text: string) {
        print(text);
        uart_write_char('\n');
    }
}
```

### 3. メモリ管理抽象化

#### 静的メモリプール（ベアメタル用）
```cpp
// src/backend/memory/static_allocator.h
class StaticAllocator {
private:
    static constexpr size_t POOL_SIZE = 64 * 1024; // 64KB
    alignas(8) static uint8_t memory_pool_[POOL_SIZE];
    static size_t next_offset_;
    
public:
    template<typename T>
    static T* allocate(size_t count = 1);
    
    static void deallocate(void* ptr); // No-op for static allocator
    static size_t remaining_bytes();
};
```

#### 文字列管理の最適化
```cpp
// src/common/static_string.h
template<size_t MaxSize>
class static_string {
private:
    char data_[MaxSize + 1];
    size_t length_;
    
public:
    static_string(const char* str);
    const char* c_str() const { return data_; }
    size_t length() const { return length_; }
    // std::stringと同等のインターフェース
};
```

## コスト・効果分析

### 実装コスト見積もり

| アプローチ | 開発期間 | 複雑度 | 保守性 | 将来性 |
|------------|----------|--------|--------|--------|
| ベアメタル優先 | 2-3週間 | 低 | 中 | 低 |
| モジュール優先 | 8-12週間 | 高 | 高 | 高 |
| ハイブリッド | 6-9週間 | 中 | 高 | 高 |

### 技術的負債分析

#### ベアメタル優先の技術的負債
- 条件コンパイルの複雑化
- プラットフォーム固有コードの散在
- 将来のリファクタリング負荷

#### モジュール優先の初期投資
- パーサー拡張（import/export構文）
- モジュール解決システム
- 依存関係管理

## 推奨実装戦略

### Phase 5: Hybrid Module System Implementation

#### 5.1 出力抽象化 (優先度: 最高)
```
目標: 現在のprint機能をベアメタル環境で動作させる
期間: 2-3週間
成果物:
- IOInterface基底クラス
- BareMetalUARTOutput実装
- OutputManager の抽象化レイヤー対応
```

#### 5.2 基本モジュールシステム (優先度: 高)
```
目標: import/export機能の基本実装
期間: 4-6週間  
成果物:
- レキサー/パーサーのimport/export対応
- モジュール解決システム
- std.ioモジュール実装
```

#### 5.3 標準ライブラリ拡張 (優先度: 中)
```
目標: 包括的な標準ライブラリ
期間: 継続的開発
成果物:
- std.memory (静的/動的メモリ管理)
- std.math (数学関数)
- std.string (文字列操作)
```

## まとめ

### 最終推奨事項

**ハイブリッドアプローチを強く推奨します。**

**理由:**

1. **即座のベアメタル対応**: 出力抽象化により、短期間でベアメタル環境での動作が可能

2. **将来への投資**: モジュールシステムにより、長期的な拡張性と保守性を確保

3. **段階的実装**: リスクを分散し、各段階で動作するシステムを維持

4. **技術的負債の最小化**: 最初から適切なアーキテクチャで設計

### 次のステップ

1. **Phase 5.1の開始**: 出力抽象化レイヤーの実装
2. **プロトタイプ作成**: ベアメタル向けUART出力の実装とテスト
3. **設計レビュー**: IOInterface設計の詳細検討
4. **実装開始**: OutputManagerのリファクタリング

この戦略により、**短期的なベアメタル対応と長期的な言語発展**の両方を実現できます。
