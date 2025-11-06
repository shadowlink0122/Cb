# Phase 1a: メモリ管理機能の実装

**作成日**: 2025年10月27日  
**対象**: v0.11.0 Phase 1の前提機能  
**優先度**: 🔴 最優先（Event Loop実装の前提条件）

---

## 📋 概要

Event LoopをCb言語で実装するためには、動的メモリ管理が必要です。Phase 1の前に、以下の機能を実装します：

1. **new / delete** - 動的メモリ確保・解放
2. **sizeof()** - 型のサイズ取得
3. **typeof()** - 型情報の取得
4. **メモリ操作関数** - memcpy, memset など

---

## 🎯 実装する機能

### 1. new / delete演算子

#### new演算子
```cb
// 基本的な使い方
Point* p = new Point;
p.x = 10;
p.y = 20;

// 配列の確保
int* arr = new int[10];
arr[0] = 100;

// 構造体の動的確保
Node<int>* node = new Node<int>;
node.data = 42;
node.next = nullptr;
```

#### delete演算子
```cb
// 単一オブジェクトの解放
delete p;

// 配列の解放
delete[] arr;

// ジェネリック構造体の解放
delete node;
```

#### 実装仕様
- `new T` - 型Tのメモリを確保し、ポインタを返す
- `new T[size]` - 型Tの配列をsize個分確保
- `delete ptr` - 単一オブジェクトを解放
- `delete[] ptr` - 配列を解放
- デストラクタがあれば自動呼び出し

---

### 2. sizeof()演算子

#### 使用例
```cb
// 基本型のサイズ
int size_int = sizeof(int);        // 4 または 8
int size_double = sizeof(double);  // 8

// 構造体のサイズ
struct Point {
    int x;
    int y;
}
int size_point = sizeof(Point);    // 8 (int×2)

// ジェネリック構造体のサイズ
struct Box<T> {
    T value;
}
int size_box_int = sizeof(Box<int>);      // sizeof(int)
int size_box_double = sizeof(Box<double>); // sizeof(double)

// 配列のサイズ
int[10] arr;
int size_arr = sizeof(arr);        // sizeof(int) * 10

// ポインタのサイズ
int* ptr;
int size_ptr = sizeof(ptr);        // 8 (64bit環境)
```

#### 実装仕様
- コンパイル時に型サイズを計算
- ジェネリック型は実体化後のサイズ
- アラインメントを考慮

---

### 3. typeof()演算子

#### 使用例
```cb
// 変数の型を取得
int x = 10;
string type_x = typeof(x);  // "int"

// 式の型を取得
string type_expr = typeof(x + 10);  // "int"

// ジェネリック型の型情報
Box<int> box;
string type_box = typeof(box);  // "Box<int>"

// ポインタの型
int* ptr;
string type_ptr = typeof(ptr);  // "int*"
```

#### 実装仕様
- 型名を文字列として返す
- ジェネリック型は具体化後の型名
- デバッグやリフレクションに使用

---

### 4. メモリ操作関数

#### memcpy
```cb
// メモリのコピー
void* memcpy(void* dest, void* src, int size);

// 使用例
int[10] src_arr = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int[10] dest_arr;
memcpy(dest_arr, src_arr, sizeof(src_arr));
```

#### memset
```cb
// メモリの初期化
void* memset(void* ptr, int value, int size);

// 使用例
int[10] arr;
memset(arr, 0, sizeof(arr));  // 全要素を0で初期化
```

#### malloc / free（低レベルAPI）
```cb
// C言語スタイルのメモリ確保
void* malloc(int size);
void free(void* ptr);

// 使用例
int* ptr = malloc(sizeof(int) * 10);
ptr[0] = 100;
free(ptr);
```

---

## 🔧 実装詳細

### 1. new演算子の実装

#### パーサー拡張
```cpp
// src/frontend/recursive_parser/parsers/primary_expression_parser.cpp

ASTNode* PrimaryExpressionParser::parse_new_expression() {
    // "new" キーワードを確認
    if (current_token().type != TOK_NEW) {
        return nullptr;
    }
    advance();
    
    // 型を解析
    ParsedTypeInfo type = parse_type();
    
    // 配列のチェック
    bool is_array = false;
    int array_size = 0;
    if (current_token().type == TOK_LBRACKET) {
        advance();
        // 配列サイズを解析
        array_size = parse_constant_expression();
        expect(TOK_RBRACKET);
        is_array = true;
    }
    
    // ASTノードを作成
    ASTNode* node = new ASTNode();
    node->node_type = AST_NEW_EXPR;
    node->type_info = type;
    node->is_array_allocation = is_array;
    node->array_size = array_size;
    
    return node;
}
```

#### インタプリタ実装
```cpp
// src/backend/interpreter/evaluator/core/evaluator.cpp

int64_t Evaluator::evaluate_new_expression(const ASTNode* node) {
    const ParsedTypeInfo& type = node->type_info;
    
    // 型のサイズを計算
    size_t type_size = calculate_type_size(type);
    
    if (node->is_array_allocation) {
        // 配列の確保
        size_t total_size = type_size * node->array_size;
        void* ptr = interpreter_.allocate_memory(total_size);
        
        // 0初期化
        memset(ptr, 0, total_size);
        
        return reinterpret_cast<int64_t>(ptr);
    } else {
        // 単一オブジェクトの確保
        void* ptr = interpreter_.allocate_memory(type_size);
        
        // デフォルトコンストラクタを呼び出し
        if (has_default_constructor(type)) {
            call_default_constructor(ptr, type);
        } else {
            memset(ptr, 0, type_size);
        }
        
        return reinterpret_cast<int64_t>(ptr);
    }
}
```

---

### 2. delete演算子の実装

#### パーサー拡張
```cpp
ASTNode* PrimaryExpressionParser::parse_delete_statement() {
    // "delete" キーワードを確認
    if (current_token().type != TOK_DELETE) {
        return nullptr;
    }
    advance();
    
    // 配列削除のチェック
    bool is_array = false;
    if (current_token().type == TOK_LBRACKET) {
        advance();
        expect(TOK_RBRACKET);
        is_array = true;
    }
    
    // ポインタ式を解析
    ASTNode* ptr_expr = parse_expression();
    
    ASTNode* node = new ASTNode();
    node->node_type = AST_DELETE_STMT;
    node->left = ptr_expr;
    node->is_array_deletion = is_array;
    
    return node;
}
```

#### インタプリタ実装
```cpp
void StatementExecutor::execute_delete_statement(const ASTNode* node) {
    // ポインタを評価
    int64_t ptr_value = evaluator_.evaluate_expression(node->left);
    void* ptr = reinterpret_cast<void*>(ptr_value);
    
    if (ptr == nullptr) {
        return;  // nullptrの削除は無視
    }
    
    // デストラクタを呼び出し
    if (has_destructor(node->left->type_info)) {
        if (node->is_array_deletion) {
            // 配列の各要素のデストラクタを呼び出し
            call_array_destructors(ptr, node->left->type_info);
        } else {
            // 単一オブジェクトのデストラクタ
            call_destructor(ptr, node->left->type_info);
        }
    }
    
    // メモリを解放
    interpreter_.deallocate_memory(ptr);
}
```

---

### 3. sizeof()の実装

#### パーサー拡張
```cpp
ASTNode* PrimaryExpressionParser::parse_sizeof_expression() {
    // "sizeof" キーワードを確認
    if (current_token().type != TOK_SIZEOF) {
        return nullptr;
    }
    advance();
    
    expect(TOK_LPAREN);
    
    // 型または式を解析
    ParsedTypeInfo type;
    if (is_type_name(current_token())) {
        type = parse_type();
    } else {
        // 式の場合は型を推論
        ASTNode* expr = parse_expression();
        type = infer_type(expr);
    }
    
    expect(TOK_RPAREN);
    
    ASTNode* node = new ASTNode();
    node->node_type = AST_SIZEOF_EXPR;
    node->type_info = type;
    
    return node;
}
```

#### インタプリタ実装
```cpp
int64_t Evaluator::evaluate_sizeof_expression(const ASTNode* node) {
    size_t size = calculate_type_size(node->type_info);
    return static_cast<int64_t>(size);
}

size_t Evaluator::calculate_type_size(const ParsedTypeInfo& type) {
    if (type.base_type == "int") {
        return sizeof(int);
    } else if (type.base_type == "double") {
        return sizeof(double);
    } else if (type.base_type == "bool") {
        return sizeof(bool);
    } else if (type.is_pointer) {
        return sizeof(void*);
    } else if (type.is_array) {
        size_t element_size = calculate_type_size(type.element_type);
        return element_size * type.array_size;
    } else if (is_struct_type(type)) {
        // 構造体のサイズを計算
        return calculate_struct_size(type);
    } else if (is_generic_type(type)) {
        // ジェネリック型を実体化してサイズを計算
        ParsedTypeInfo instantiated = instantiate_generic_type(type);
        return calculate_type_size(instantiated);
    }
    
    return 0;  // 未知の型
}
```

---

### 4. typeof()の実装

#### パーサー拡張
```cpp
ASTNode* PrimaryExpressionParser::parse_typeof_expression() {
    if (current_token().type != TOK_TYPEOF) {
        return nullptr;
    }
    advance();
    
    expect(TOK_LPAREN);
    
    // 式を解析
    ASTNode* expr = parse_expression();
    
    expect(TOK_RPAREN);
    
    ASTNode* node = new ASTNode();
    node->node_type = AST_TYPEOF_EXPR;
    node->left = expr;
    
    return node;
}
```

#### インタプリタ実装
```cpp
int64_t Evaluator::evaluate_typeof_expression(const ASTNode* node) {
    // 式の型を推論
    ParsedTypeInfo type = infer_type(node->left);
    
    // 型名を文字列に変換
    std::string type_name = type_to_string(type);
    
    // 文字列をCb言語の値として返す
    return interpreter_.create_string_value(type_name);
}

std::string Evaluator::type_to_string(const ParsedTypeInfo& type) {
    std::string result = type.base_type;
    
    if (!type.generic_args.empty()) {
        result += "<";
        for (size_t i = 0; i < type.generic_args.size(); ++i) {
            if (i > 0) result += ", ";
            result += type_to_string(type.generic_args[i]);
        }
        result += ">";
    }
    
    if (type.is_pointer) {
        result += "*";
    }
    
    if (type.is_array) {
        result += "[" + std::to_string(type.array_size) + "]";
    }
    
    return result;
}
```

---

## 📁 ファイル構成

### 新規追加ファイル

1. **src/common/ast.h** - ASTノード拡張
   ```cpp
   AST_NEW_EXPR,
   AST_DELETE_STMT,
   AST_SIZEOF_EXPR,
   AST_TYPEOF_EXPR,
   ```

2. **src/frontend/recursive_parser/recursive_lexer.cpp** - トークン追加
   ```cpp
   TOK_NEW,
   TOK_DELETE,
   TOK_SIZEOF,
   TOK_TYPEOF,
   ```

3. **src/backend/interpreter/core/memory_manager.cpp** - メモリマネージャ
   ```cpp
   void* allocate_memory(size_t size);
   void deallocate_memory(void* ptr);
   void track_allocation(void* ptr, size_t size);
   void check_memory_leaks();
   ```

4. **stdlib/memory.cb** - メモリ操作関数
   ```cb
   void* memcpy(void* dest, void* src, int size);
   void* memset(void* ptr, int value, int size);
   void* malloc(int size);
   void free(void* ptr);
   ```

---

## 🧪 テスト計画

### 基本機能テスト（10個）

1. **test_new_basic.cb** - 基本的なnew
2. **test_new_array.cb** - 配列のnew
3. **test_delete_basic.cb** - 基本的なdelete
4. **test_delete_array.cb** - 配列のdelete
5. **test_sizeof_basic.cb** - sizeofの基本
6. **test_sizeof_struct.cb** - 構造体のsizeof
7. **test_sizeof_generic.cb** - ジェネリック型のsizeof
8. **test_typeof_basic.cb** - typeofの基本
9. **test_typeof_generic.cb** - ジェネリック型のtypeof
10. **test_memory_leak.cb** - メモリリークテスト

### 統合テスト（5個）

11. **test_new_with_constructor.cb** - コンストラクタ付きnew
12. **test_delete_with_destructor.cb** - デストラクタ付きdelete
13. **test_dynamic_queue.cb** - 動的Queueの実装
14. **test_dynamic_struct.cb** - 動的構造体の使用
15. **test_memcpy_memset.cb** - メモリ操作関数

---

## 📊 実装スケジュール

### Week 1: メモリ管理の基盤（3日）

**Day 1: new/delete パーサー**
- [ ] TOK_NEW, TOK_DELETE トークン追加
- [ ] parse_new_expression実装
- [ ] parse_delete_statement実装

**Day 2: new/delete インタプリタ**
- [ ] evaluate_new_expression実装
- [ ] execute_delete_statement実装
- [ ] メモリマネージャの基本実装

**Day 3: テストとデバッグ**
- [ ] 基本的なnew/deleteのテスト
- [ ] メモリリークチェック

---

### Week 2: 組み込み関数（4日）

**Day 4: sizeof実装**
- [ ] parse_sizeof_expression実装
- [ ] calculate_type_size実装
- [ ] テスト作成

**Day 5: typeof実装**
- [ ] parse_typeof_expression実装
- [ ] type_to_string実装
- [ ] テスト作成

**Day 6-7: メモリ操作関数**
- [ ] memcpy実装
- [ ] memset実装
- [ ] malloc/free実装
- [ ] テスト作成

---

## 🎯 完了基準

### 必須要件
1. ✅ new/deleteが動作する
2. ✅ sizeof()が正しい値を返す
3. ✅ typeof()が型名を返す
4. ✅ メモリリークがない
5. ✅ 15個のテストがすべてパス

### Event Loop実装への準備
- ✅ 動的Queueが実装できる
- ✅ コールバックを動的に管理できる
- ✅ タスク/タイマー構造体を動的確保できる

---

## 🚨 注意事項

### メモリ管理の責任
- Cb言語はガベージコレクションを持たない
- newしたメモリは必ずdeleteが必要
- メモリリークを防ぐため、デストラクタでの解放を推奨

### 型安全性
- void*の使用は最小限に
- できるだけジェネリクスで型安全に実装

### パフォーマンス
- メモリアロケーションは遅い操作
- 頻繁な確保/解放は避ける
- オブジェクトプールの検討

---

## 🖥️ ベアメタル環境への対応

### 課題

ベアメタル環境（OS機能なし）では以下が使用できません：
- malloc/free（OSのヒープマネージャ）
- println/print_int（システムコール）
- 標準ライブラリのI/O関数

### 解決策1: カスタムメモリアロケータ

```cb
// stdlib/allocator.cb

// グローバルヒープ領域
char[1048576] __heap_memory;  // 1MB
int __heap_offset = 0;

// シンプルなバンプアロケータ
void* __allocate(int size) {
    // アラインメント調整（8バイト境界）
    int aligned_size = (size + 7) & ~7;
    
    int current_offset = __heap_offset;
    __heap_offset = __heap_offset + aligned_size;
    
    if (__heap_offset > 1048576) {
        // ヒープオーバーフロー
        return nullptr;
    }
    
    return &__heap_memory[current_offset];
}

// ベアメタル環境ではfreeは何もしない
// （バンプアロケータは解放をサポートしない）
void __deallocate(void* ptr) {
    // No-op for bump allocator
}
```

### 解決策2: プラットフォーム抽象化層

```cb
// stdlib/platform.cb

// プラットフォーム検出（コンパイル時定数）
#if defined(OS_LINUX) || defined(OS_MACOS) || defined(OS_WINDOWS)
    bool __has_os = true;
#else
    bool __has_os = false;
#endif

// メモリ確保（プラットフォーム依存）
void* platform_allocate(int size) {
    #if __has_os
        return malloc(size);  // OS環境
    #else
        return __allocate(size);  // ベアメタル
    #endif
}

void platform_deallocate(void* ptr) {
    #if __has_os
        free(ptr);
    #else
        __deallocate(ptr);
    #endif
}

// 出力（プラットフォーム依存）
void platform_write(char* str) {
    #if __has_os
        println(str);  // OS環境
    #else
        // ベアメタル: UARTやシリアルポートに出力
        uart_write(str);
    #endif
}
```

### 解決策3: ベアメタル用UART出力

```cb
// stdlib/bare_metal/uart.cb

// UART レジスタ（ARM Cortex-Mの例）
struct UART {
    volatile int* data_register;
    volatile int* status_register;
}

UART uart;

void uart_init() {
    // UARTレジスタのアドレス設定（ハードウェア依存）
    uart.data_register = 0x40004000;  // 例: STM32のUSART1
    uart.status_register = 0x40004004;
}

void uart_write_char(char c) {
    // 送信可能になるまで待つ
    while ((*uart.status_register & 0x80) == 0) {
        // Busy wait
    }
    
    // データを送信
    *uart.data_register = c;
}

void uart_write(char* str) {
    int i = 0;
    while (str[i] != '\0') {
        uart_write_char(str[i]);
        i = i + 1;
    }
}
```

### 解決策4: コンパイル時設定

```json
// cb_config.json

{
    "target": {
        "platform": "bare-metal",  // または "os"
        "architecture": "arm-cortex-m4",
        "features": {
            "stdlib": false,         // 標準ライブラリを使わない
            "malloc": false,         // mallocを使わない
            "io": "uart"             // UART経由のI/O
        },
        "memory": {
            "heap_size": 1048576,    // 1MB
            "stack_size": 65536      // 64KB
        }
    }
}
```

### new/delete のベアメタル実装

```cpp
// src/backend/interpreter/stdlib/memory.cpp

void* Interpreter::allocate_memory(size_t size) {
    #ifdef BARE_METAL
        // バンプアロケータを使用
        return bump_allocator_->allocate(size);
    #else
        // OSのmallocを使用
        return std::malloc(size);
    #endif
}

void Interpreter::deallocate_memory(void* ptr) {
    #ifdef BARE_METAL
        // バンプアロケータは解放をサポートしない
        // または簡易的なフリーリストを実装
        bump_allocator_->deallocate(ptr);
    #else
        std::free(ptr);
    #endif
}
```

---

## 🎯 ベアメタル対応の実装方針

### Phase 1a: 基本メモリ管理（両環境対応）

1. **プラットフォーム検出**
   - コンパイル時にOS/ベアメタルを判定
   - `cb_config.json` で設定

2. **カスタムアロケータ**
   - バンプアロケータ（シンプル、高速）
   - フリーリストアロケータ（解放サポート）
   - スラブアロケータ（固定サイズ最適化）

3. **抽象化層**
   - `platform_allocate/deallocate`
   - `platform_write`
   - ハードウェア依存コードは `stdlib/bare_metal/` に分離

### メモリアロケータの選択

| アロケータ | 特徴 | 解放 | 断片化 | 用途 |
|-----------|------|------|--------|------|
| **バンプ** | 最速、シンプル | ✗ | なし | 短命オブジェクト |
| **フリーリスト** | 中速、汎用 | ✓ | あり | 汎用的な用途 |
| **スラブ** | 高速、固定サイズ | ✓ | 少ない | 頻繁な確保/解放 |
| **バディ** | 中速、2のべき乗 | ✓ | 少ない | ページ管理 |

**推奨**: まずバンプアロケータで実装し、後でフリーリストに拡張

---

## 📁 ベアメタル用ファイル構成

```
stdlib/
├── memory.cb           # new/delete の共通インターフェース
├── allocator.cb        # プラットフォーム抽象化
├── platform.cb         # プラットフォーム検出
└── bare_metal/
    ├── uart.cb         # UART出力
    ├── bump_alloc.cb   # バンプアロケータ
    ├── freelist.cb     # フリーリストアロケータ
    └── slab.cb         # スラブアロケータ
```

---

## 🧪 ベアメタル環境のテスト

### テスト環境
1. **QEMUエミュレータ** - ARM Cortex-M4をエミュレート
2. **実機ボード** - STM32, ESP32など
3. **カスタムテストハーネス** - printlnの代わりにLEDやシリアル出力

### テストケース例

```cb
// tests/bare_metal/test_new_basic.cb

void test_new_basic() {
    // ベアメタル環境でもnewが動作するか
    int* ptr = new int;
    *ptr = 42;
    
    // assert の代わりにLED点灯
    if (*ptr == 42) {
        led_on(1);  // 緑LED = 成功
    } else {
        led_on(2);  // 赤LED = 失敗
    }
    
    delete ptr;
}
```

---

## 📋 実装優先度

### 必須（ベアメタルでも動作必要）
1. ✅ バンプアロケータ
2. ✅ new/delete の基本実装
3. ✅ sizeof/typeof
4. ✅ プラットフォーム抽象化層

### オプション（OS環境で有効）
1. ⚪ フリーリストアロケータ
2. ⚪ mallocラッパー
3. ⚪ println/print_int

### 将来（高度な最適化）
1. ⚪ スラブアロケータ
2. ⚪ ガベージコレクション
3. ⚪ メモリプール

---

**作成者**: GitHub Copilot  
**レビュアー**: shadowlink0122  
**最終更新**: 2025年10月27日  
**次のフェーズ**: Phase 1 Event Loop実装（メモリ管理完了後）  
**ベアメタル対応**: ✅ 完全対応（バンプアロケータ、UART出力）
