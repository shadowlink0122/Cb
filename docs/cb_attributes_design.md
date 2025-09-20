# Cb言語アトリビュート設計書

## はじめに

Rust言語の`#[no_std]`のような、コンパイル時指定可能なアトリビュート機能をCb言語に追加する設計です。

## アトリビュート構文設計

### 基本構文

```cb
// Rust風アトリビュート構文
#[attribute_name]
#[attribute_with_value("parameter")]
#[multiple, attributes]

// 関数レベル
#[no_libc]
int main() {
    // 標準Cライブラリを使用しない関数
    return 0;
}

// ファイル/モジュールレベル  
#[baremetal]
#[no_heap]
module my_module {
    // ベアメタル専用、ヒープ使用禁止モジュール
}
```

### 利用可能なアトリビュート

#### システムレベル
- `#[no_libc]` - 標準Cライブラリを使用しない
- `#[no_std]` - C++標準ライブラリを使用しない  
- `#[baremetal]` - ベアメタル環境向け
- `#[no_heap]` - 動的メモリ割り当て禁止
- `#[no_exceptions]` - C++例外使用禁止

#### プラットフォーム
- `#[target("baremetal")]` - ベアメタル専用
- `#[target("wasm")]` - WebAssembly専用
- `#[target("native")]` - ネイティブ専用

#### 最適化
- `#[inline]` - インライン関数として最適化
- `#[optimize("size")]` - サイズ最適化
- `#[optimize("speed")]` - 速度最適化

#### 安全性
- `#[unsafe]` - 安全性チェック無効化
- `#[bounds_check(false)]` - 境界チェック無効化

## 実装アプローチ

### Phase 1: レキサー拡張

```l
/* レキサー（lexer.l）にアトリビュート構文を追加 */

"#["                { return ATTR_START; }
"]"                 { return ATTR_END; }
"no_libc"           { return ATTR_NO_LIBC; }
"baremetal"         { return ATTR_BAREMETAL; }
"target"            { return ATTR_TARGET; }
```

### Phase 2: パーサー拡張

```y
/* パーサー（parser.y）にアトリビュート構文を追加 */

%token ATTR_START ATTR_END
%token ATTR_NO_LIBC ATTR_BAREMETAL ATTR_TARGET

attribute_list:
    attribute { $$ = create_attr_list($1); }
    | attribute_list attribute { add_attribute($1, $2); }
    ;

attribute:
    ATTR_START attribute_name ATTR_END {
        $$ = create_attribute($2);
    }
    ;

function_definition:
    attribute_list type IDENTIFIER '(' parameter_list ')' '{' statement_list '}' {
        ASTNode* func = create_function_def($3, $2, nullptr, $5, $8);
        set_attributes(func, $1);
        $$ = func;
    }
    ;
```

### Phase 3: AST拡張

```cpp
// AST（ast.h）にアトリビュート情報を追加

enum class AttributeType {
    NO_LIBC,
    NO_STD,
    BAREMETAL, 
    NO_HEAP,
    TARGET,
    INLINE,
    OPTIMIZE
};

struct Attribute {
    AttributeType type;
    std::string value;  // #[target("baremetal")] の "baremetal" 部分
    
    Attribute(AttributeType t, const std::string& v = "") : type(t), value(v) {}
};

struct ASTNode {
    // 既存のメンバー
    ASTNodeType node_type;
    // ...
    
    // 新規追加：アトリビュート情報
    std::vector<Attribute> attributes;
    
    // アトリビュート操作ヘルパー
    void add_attribute(AttributeType type, const std::string& value = "");
    bool has_attribute(AttributeType type) const;
    std::string get_attribute_value(AttributeType type) const;
};
```

### Phase 4: コンパイル時処理

```cpp
// インタープリター/コードジェネレーター拡張

class AttributeProcessor {
public:
    static void process_attributes(const ASTNode* node);
    static bool is_compatible_with_target(const ASTNode* node, TargetPlatform target);
    
private:
    static void validate_no_libc_function(const ASTNode* func);
    static void validate_baremetal_compatibility(const ASTNode* node);
};

// 使用例
void Interpreter::process_function(const ASTNode* func) {
    // アトリビュート処理
    AttributeProcessor::process_attributes(func);
    
    // no_libc属性がある場合の特別処理
    if (func->has_attribute(AttributeType::NO_LIBC)) {
        // printf()などの標準ライブラリ使用をコンパイルエラーに
        validate_no_stdlib_usage(func);
    }
    
    // baremetal属性がある場合の特別処理  
    if (func->has_attribute(AttributeType::BAREMETAL)) {
        // 動的メモリ割り当てチェックなど
        validate_baremetal_constraints(func);
    }
}
```

## 使用例

### ベアメタルアプリケーション

```cb
#[baremetal]
#[no_libc] 
#[no_heap]
int main() {
    // UART出力のみ使用、malloc/printfなどは禁止
    uart_puts("Hello, bare metal world!");
    return 0;
}

#[target("baremetal")]
#[inline]
void uart_puts(string text) {
    // ベアメタル専用実装
    for (int i = 0; i < text.length(); i++) {
        uart_write_char(text[i]);
    }
}
```

### WebAssemblyモジュール

```cb
#[target("wasm")]
#[export("fibonacci")]  
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

#[wasm_import("console", "log")]
void wasm_log(string message);

#[no_std]
int wasm_main() {
    wasm_log("Hello from WebAssembly!");
    return fibonacci(10);
}
```

### 高性能コード

```cb
#[optimize("speed")]
#[inline]
#[bounds_check(false)]
int fast_array_sum(int[] array, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += array[i];  // 境界チェックなし
    }
    return sum;
}
```

## エラーハンドリング

```cpp
// アトリビュート違反の検出とエラー報告

class AttributeValidator {
public:
    static void validate(const ASTNode* node) {
        if (node->has_attribute(AttributeType::NO_LIBC)) {
            check_no_libc_violations(node);
        }
        
        if (node->has_attribute(AttributeType::BAREMETAL)) {
            check_baremetal_violations(node);
        }
    }
    
private:
    static void check_no_libc_violations(const ASTNode* node) {
        // printf, malloc などの使用をチェック
        if (uses_prohibited_function(node, "printf")) {
            throw AttributeViolationError(
                "#[no_libc] function cannot use printf()"
            );
        }
    }
};
```

## 実装スケジュール

### Phase 1 (1-2週間): 基本アトリビュート構文
- レキサー/パーサー拡張
- AST構造拡張
- 基本的な`#[no_libc]`, `#[baremetal]`実装

### Phase 2 (1週間): バリデーション機能
- アトリビュート違反検出
- エラーメッセージ実装
- テストケース作成

### Phase 3 (1週間): 高度なアトリビュート
- `#[target]`条件コンパイル
- 最適化指示
- WebAssembly固有アトリビュート

## メリット

1. **明示的な制約**: コード上で制約が明確
2. **コンパイル時検証**: 実行前にベアメタル互換性チェック
3. **段階的適用**: 関数・モジュール単位で適用可能
4. **開発者体験**: Rustライクな親しみやすい構文

## 現在の実装との互換性

既存のprint/printf抽象化レイヤーと完全に互換性があり：

```cb
// 既存コード（変更なし）
int main() {
    print("Hello World");
    return 0;
}

// アトリビュート付きコード（新機能）
#[baremetal]
int bare_main() {
    print("Hello World");  // 自動的にUART出力
    return 0;
}
```

このアトリビュートシステムにより、Cb言語がより安全で表現力豊かなシステムプログラミング言語に進化します。
