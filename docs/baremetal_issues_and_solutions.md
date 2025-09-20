# Cb言語 ベアメタル対応 - 問題点と解決策

## 🚨 発見された重大な問題点

### Critical Issues (即座解決必要)

#### 1. 標準ライブラリ依存
**問題**: `printf()`, `std::cout` による libc/C++ standard library 依存
```cpp
// ❌ 現在の実装 - ベアメタル不可
printf("%" PRId64, value);
std::cout << value << std::endl;
```

**影響**: ベアメタル環境では動作不可、OS カーネルでクラッシュ

#### 2. 動的メモリアロケーション
**問題**: `std::vector`, `std::string`, `std::unique_ptr` による heap 依存
```cpp
// ❌ 現在の実装 - malloc/free 依存
std::vector<std::unique_ptr<ASTNode>> statements;
std::string str_value;
```

**影響**: ベアメタル環境ではメモリアロケーターが存在しない

#### 3. C++ 例外処理
**問題**: `throw`/`catch` による C++ runtime 依存
```cpp
// ❌ 現在の実装 - 例外ランタイム必要
throw std::runtime_error("Error");
```

**影響**: ベアメタル環境では例外処理機構が無い

#### 4. ファイルI/O システムコール
**問題**: `FILE*`, `std::ofstream` による POSIX/OS 依存
```cpp
// ❌ 現在の実装 - OS 依存
FILE *yyin;
std::ofstream ofs(filename);
```

**影響**: ベアメタル環境ではファイルシステムが存在しない

## 🔧 解決策アーキテクチャ

### 1. 条件付きコンパイル設計

```cpp
// config.h - ビルド時設定
#ifndef CB_CONFIG_H
#define CB_CONFIG_H

// ターゲット環境の定義
#define CB_TARGET_HOSTED    1  // 通常OS環境
#define CB_TARGET_FREESTANDING 2  // ベアメタル環境

#ifndef CB_TARGET
#define CB_TARGET CB_TARGET_HOSTED
#endif

// 機能の有効/無効化
#if CB_TARGET == CB_TARGET_FREESTANDING
    #define CB_NO_STDLIB     1  // 標準ライブラリ無効
    #define CB_NO_EXCEPTIONS 1  // 例外処理無効
    #define CB_NO_FILEIO     1  // ファイルI/O無効
    #define CB_NO_HEAP       1  // ヒープメモリ無効
#endif

#endif // CB_CONFIG_H
```

### 2. 出力システム抽象化

```cpp
// output_interface.h - 出力抽象化
#ifndef OUTPUT_INTERFACE_H
#define OUTPUT_INTERFACE_H

#include "config.h"

// 出力インターフェース
struct OutputInterface {
    void (*write_char)(char c);
    void (*write_string)(const char* str);
    void (*write_int)(int64_t value);
    void (*write_newline)(void);
};

// ホスト環境実装
#if CB_TARGET == CB_TARGET_HOSTED
void hosted_write_char(char c);
void hosted_write_string(const char* str);
void hosted_write_int(int64_t value);
void hosted_write_newline(void);

extern const OutputInterface hosted_output;
#endif

// ベアメタル環境実装  
#if CB_TARGET == CB_TARGET_FREESTANDING
void baremetal_write_char(char c);
void baremetal_write_string(const char* str);
void baremetal_write_int(int64_t value);
void baremetal_write_newline(void);

extern const OutputInterface baremetal_output;
#endif

// 現在の出力インターフェース
extern const OutputInterface* current_output;

#endif // OUTPUT_INTERFACE_H
```

### 3. メモリ管理抽象化

```cpp
// memory_interface.h - メモリ管理抽象化
#ifndef MEMORY_INTERFACE_H
#define MEMORY_INTERFACE_H

#include "config.h"
#include <cstddef>

#if CB_TARGET == CB_TARGET_FREESTANDING

// ベアメタル用の固定サイズコンテナ
template<typename T, size_t N>
class fixed_vector {
private:
    T data_[N];
    size_t size_;
    
public:
    fixed_vector() : size_(0) {}
    
    void push_back(const T& value) {
        if (size_ < N) {
            data_[size_++] = value;
        }
        // エラー処理は Result<T, Error> で実装
    }
    
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    size_t size() const { return size_; }
};

// 固定サイズ文字列
class fixed_string {
private:
    static constexpr size_t MAX_SIZE = 256;
    char data_[MAX_SIZE];
    size_t length_;
    
public:
    fixed_string() : length_(0) { data_[0] = '\0'; }
    fixed_string(const char* str);
    
    const char* c_str() const { return data_; }
    size_t length() const { return length_; }
    // ... その他メソッド
};

// エイリアス定義
using cb_vector = fixed_vector;
using cb_string = fixed_string;

#else

// ホスト環境では標準ライブラリを使用
#include <vector>
#include <string>

template<typename T, size_t N = 1000>  // デフォルト最大サイズ
using cb_vector = std::vector<T>;
using cb_string = std::string;

#endif

#endif // MEMORY_INTERFACE_H
```

### 4. エラーハンドリング抽象化

```cpp
// error_handling.h - エラー処理抽象化
#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include "config.h"

// エラーコード定義
enum class CbError {
    OK = 0,
    NULL_POINTER,
    OUT_OF_BOUNDS,
    TYPE_MISMATCH,
    DIVISION_BY_ZERO,
    STACK_OVERFLOW,
    MEMORY_EXHAUSTED
};

// Result<T, E> パターン (Rust風)
template<typename T>
class Result {
private:
    bool is_ok_;
    union {
        T value_;
        CbError error_;
    };
    
public:
    Result(const T& value) : is_ok_(true), value_(value) {}
    Result(CbError error) : is_ok_(false), error_(error) {}
    
    bool is_ok() const { return is_ok_; }
    bool is_error() const { return !is_ok_; }
    
    T& unwrap() { 
        if (!is_ok_) {
            // ベアメタル環境では panic!()
            handle_error(error_);
        }
        return value_; 
    }
    
    CbError error() const { return error_; }
};

// エラーハンドラー
#if CB_TARGET == CB_TARGET_FREESTANDING
[[noreturn]] void handle_error(CbError error);
#else
void handle_error(CbError error); // 例外として投げる
#endif

// エラーマクロ
#define CB_TRY(expr) \
    do { \
        auto result = (expr); \
        if (result.is_error()) { \
            return Result<decltype(result.unwrap())>(result.error()); \
        } \
    } while(0)

#endif // ERROR_HANDLING_H
```

### 5. ベアメタル出力実装例

```cpp
// baremetal_output.cpp - ベアメタル出力実装
#include "output_interface.h"
#include "config.h"

#if CB_TARGET == CB_TARGET_FREESTANDING

// UART または VGA 出力 (x86_64 例)
static void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// シリアルポート初期化 (COM1)
static void init_serial() {
    outb(0x3F8 + 1, 0x00);    // 割り込み無効
    outb(0x3F8 + 3, 0x80);    // DLAB設定
    outb(0x3F8 + 0, 0x03);    // ボーレート設定 (lo byte)
    outb(0x3F8 + 1, 0x00);    // ボーレート設定 (hi byte)
    outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7);    // FIFO有効
    outb(0x3F8 + 4, 0x0B);    // IRQ有効
}

static bool is_transmit_empty() {
    return inb(0x3F8 + 5) & 0x20;
}

void baremetal_write_char(char c) {
    // シリアルポートが初期化済みかチェック
    static bool initialized = false;
    if (!initialized) {
        init_serial();
        initialized = true;
    }
    
    // 送信バッファが空になるまで待機
    while (!is_transmit_empty()) {
        // スピンウェイト
    }
    
    outb(0x3F8, c);
}

void baremetal_write_string(const char* str) {
    if (!str) return;
    
    while (*str) {
        baremetal_write_char(*str);
        str++;
    }
}

void baremetal_write_int(int64_t value) {
    if (value == 0) {
        baremetal_write_char('0');
        return;
    }
    
    char buffer[32];
    int pos = 0;
    bool negative = false;
    
    if (value < 0) {
        negative = true;
        value = -value;
    }
    
    // 数値を文字列に変換
    while (value > 0) {
        buffer[pos++] = '0' + (value % 10);
        value /= 10;
    }
    
    if (negative) {
        baremetal_write_char('-');
    }
    
    // 逆順で出力
    for (int i = pos - 1; i >= 0; i--) {
        baremetal_write_char(buffer[i]);
    }
}

void baremetal_write_newline() {
    baremetal_write_char('\r');
    baremetal_write_char('\n');
}

// ベアメタル出力インターフェース
const OutputInterface baremetal_output = {
    .write_char = baremetal_write_char,
    .write_string = baremetal_write_string,
    .write_int = baremetal_write_int,
    .write_newline = baremetal_write_newline
};

#endif // CB_TARGET_FREESTANDING
```

## 🎯 実装マイルストーン

### Phase 8.1: 抽象化レイヤー実装 (1週間)
- [ ] 出力インターフェース設計・実装
- [ ] メモリ管理抽象化
- [ ] エラーハンドリング改修  
- [ ] 条件付きコンパイル設定

### Phase 8.2: ベアメタル実装 (2週間)
- [ ] ベアメタル出力実装 (シリアル/VGA)
- [ ] 固定サイズコンテナ実装
- [ ] Result<T> エラー処理実装
- [ ] ベアメタルランタイム整備

### Phase 8.3: テスト・検証 (1週間)
- [ ] ホスト環境での回帰テスト
- [ ] ベアメタル環境でのテスト
- [ ] クロスプラットフォーム検証
- [ ] パフォーマンス測定

## 🚀 ビルドシステム拡張

```makefile
# Makefile 拡張案
# ホスト環境 (デフォルト)
hosted: CFLAGS += -DCB_TARGET=CB_TARGET_HOSTED
hosted: $(MAIN_TARGET)

# ベアメタル環境
baremetal: CFLAGS += -DCB_TARGET=CB_TARGET_FREESTANDING -nostdlib -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti
baremetal: LDFLAGS += -nostdlib -lgcc
baremetal: $(MAIN_TARGET)

# OS Kernel ターゲット (特別設定)
kernel: CFLAGS += -DCB_TARGET=CB_TARGET_FREESTANDING -nostdlib -ffreestanding -mno-red-zone -mcmodel=kernel
kernel: LDFLAGS += -nostdlib -T linker.ld
kernel: $(MAIN_TARGET)
```

---

**この改革により、Cb言語は真のシステムプログラミング言語として、ベアメタルからWebまで統一的に動作可能になります。**
