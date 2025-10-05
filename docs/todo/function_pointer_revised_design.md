# 関数ポインタ実装設計書（改訂版 v3）

## 1. 概要

関数ポインタを**通常のポインタ変数の拡張**として実装する。`typedef`は使用せず、既存のポインタ機能を活用して実装する。

**重要な設計方針**:
- 関数ポインタは**ポインタ変数の一種**
- `型* 変数 = &関数` で関数のアドレスを代入
- `(*変数)(引数...)` または `変数(引数...)` で関数を実行（2つの形式）
- 戻り値の型が同じであれば異なる関数を代入可能
- `void*` は void型関数のアドレスを格納（汎用ポインタではない）

## 2. 構文設計

### 2.1. 基本的な関数ポインタ

```c++
// 関数定義
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

void main() {
    // 関数ポインタ変数の宣言と初期化
    int* funcPtr = &add;              // &演算子で関数のアドレスを取得
    
    // 関数ポインタを通して関数を呼び出し（2つの形式）
    int result1 = (*funcPtr)(5, 3);   // 形式1: 明示的なデリファレンス
    int result2 = funcPtr(5, 3);      // 形式2: 暗黙的な呼び出し（推奨）
    
    println(string(result1));          // 8
    println(string(result2));          // 8
    
    // 別の関数を代入
    funcPtr = &multiply;
    result1 = (*funcPtr)(5, 3);       // 15
    result2 = funcPtr(5, 3);          // 15
    println(string(result1));
    println(string(result2));
}
```

**注意**: `*funcPtr(引数...)`という構文は避けること。これは複合代入演算子`*=`と混同される可能性がある。

### 2.2. void型関数のポインタ

```c++
void printHello() {
    println("Hello");
}

void printWorld() {
    println("World");
}

void main() {
    void* voidFuncPtr = &printHello;
    (*voidFuncPtr)();   // 形式1: Hello
    voidFuncPtr();      // 形式2: Hello (推奨)
    
    voidFuncPtr = &printWorld;
    (*voidFuncPtr)();   // 形式1: World
    voidFuncPtr();      // 形式2: World (推奨)
}
```

### 2.3. 型の互換性

戻り値の型が同じであれば、パラメータの型や数が異なっていても代入可能。

```c++
int func1(int a) { return a * 2; }
int func2(int a, int b) { return a + b; }
int func3() { return 42; }

void main() {
    int* ptr = &func1;
    int r1 = (*ptr)(5);        // 形式1: 10
    int r2 = ptr(5);           // 形式2: 10
    
    ptr = &func2;
    int r3 = (*ptr)(3, 7);     // 形式1: 10
    int r4 = ptr(3, 7);        // 形式2: 10
    
    ptr = &func3;
    int r5 = (*ptr)();         // 形式1: 42
    int r6 = ptr();            // 形式2: 42
}
```

### 2.4. 関数ポインタの配列

```c++
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }

void main() {
    int* operations[3];
    operations[0] = &add;
    operations[1] = &sub;
    operations[2] = &mul;
    
    int x = 10, y = 5;
    // 形式1: 明示的なデリファレンス
    println("Add: " + string((*operations[0])(x, y)));  // 15
    println("Sub: " + string((*operations[1])(x, y)));  // 5
    
    // 形式2: 暗黙的な呼び出し（推奨）
    println("Mul: " + string(operations[2](x, y)));  // 50
}
```

### 2.5. 構造体メンバーとしての関数ポインタ

```c++
struct Calculator {
    int* operation;
}

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

void main() {
    Calculator calc;
    calc.operation = &add;
    
    // 形式1: 明示的なデリファレンス
    println(string((*calc.operation)(10, 5)));  // 15
    
    // 形式2: 暗黙的な呼び出し（推奨）
    println(string(calc.operation(10, 5)));     // 15
    
    calc.operation = &sub;
    println(string(calc.operation(10, 5)));     // 5
}
```

### 2.6. 関数ポインタを関数のパラメータとして渡す

```c++
int apply(int a, int b, int* op) {
    // 形式1: 明示的なデリファレンス
    return (*op)(a, b);
    
    // または形式2: 暗黙的な呼び出し
    // return op(a, b);
}

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

void main() {
    int result1 = apply(5, 3, &add);       // 8
    int result2 = apply(5, 3, &multiply);  // 15
    
    println(string(result1));
    println(string(result2));
}
```

### 2.7. 呼び出し構文の比較と注意点

```c++
int getValue() { return 10; }

void main() {
    int* funcPtr = &getValue;
    
    // ❌ 避けるべき構文（複合代入演算子と混同される）
    // int n = *funcPtr();  // これは *= と見間違える可能性がある
    
    // ✅ 推奨される構文
    int result1 = (*funcPtr)();   // 形式1: 明示的なデリファレンス
    int result2 = funcPtr();      // 形式2: 暗黙的な呼び出し（最も簡潔）
    
    println(string(result1));  // 10
    println(string(result2));  // 10
}
```

**パーサーの判定ルール**:
- `(*識別子)(` → 関数ポインタ呼び出し（形式1）
- `識別子(` → 通常の関数呼び出しまたは関数ポインタ呼び出し（形式2）
  - 識別子が関数名 → 通常の関数呼び出し
  - 識別子がポインタ変数 → 関数ポインタ呼び出し

## 3. 実装方針

### 3.1. 既存のポインタ機能の拡張

既存のポインタ実装（`&`演算子、`*`演算子、ポインタ型）を拡張して関数ポインタをサポートする。

**必要な拡張箇所**:

1. **`&`演算子の拡張**:
   - 現在: 変数のアドレスを取得
   - 追加: 関数のアドレスを取得

2. **ポインタ変数の拡張**:
   - 現在: 変数のアドレスを格納
   - 追加: 関数のアドレスを格納

3. **`*`演算子の拡張**:
   - 現在: ポインタのデリファレンス（変数アクセス）
   - 追加: 関数ポインタ経由の関数呼び出し

4. **型チェックの拡張**:
   - 戻り値の型が一致するかチェック

### 3.2. ASTの拡張（最小限）

既存のポインタ関連のASTノードを活用し、最小限の追加のみ行う。

```c++
// ASTNode に追加
struct ASTNode {
    // 既存のポインタ関連メンバー
    bool is_pointer;
    int pointer_depth;
    TypeInfo pointer_base_type;
    std::string pointer_base_type_name;
    
    // 関数ポインタ用に追加
    bool is_function_address;        // &関数 の場合にtrue
    std::string function_address_name; // 関数名
};
```

### 3.3. パーサーの拡張

#### 3.3.1. `&`演算子の拡張

```c++
ASTNode* RecursiveParser::parseUnary() {
    // 既存: &変数
    if (check(TokenType::TOK_BIT_AND)) {
        advance();
        
        if (check(TokenType::TOK_IDENTIFIER)) {
            std::string name = current_token_.value;
            
            // 関数名かどうかチェック
            if (isFunctionName(name)) {
                // 関数のアドレス取得
                ASTNode* node = new ASTNode(ASTNodeType::AST_ADDRESS_OF);
                node->is_function_address = true;
                node->function_address_name = name;
                node->type_info = TYPE_POINTER;
                // 関数の戻り値型を取得
                node->pointer_base_type = getFunctionReturnType(name);
                advance();
                return node;
            }
            
            // 既存の変数アドレス取得処理
            // ...
        }
    }
    // ...
}
```

#### 3.3.2. 関数ポインタ呼び出しの解析

**形式1: `(*ptr)(args)` の解析**

```c++
ASTNode* RecursiveParser::parsePostfix() {
    ASTNode* node = parsePrimary();
    
    while (true) {
        // `(*ptr)(args)` のパターンをチェック
        if (check(TokenType::TOK_LPAREN) && 
            node->node_type == ASTNodeType::AST_UNARY_OP && 
            node->op == "DEREFERENCE") {
            
            // 関数ポインタ呼び出しに変換
            ASTNode* funcPtrCall = new ASTNode(ASTNodeType::AST_FUNC_PTR_CALL);
                funcPtrCall->left = std::move(node->left);  // ポインタ変数
                
                // 引数の解析
                advance(); // '(' を消費
                while (!check(TokenType::TOK_RPAREN)) {
                    funcPtrCall->arguments.push_back(
                        std::unique_ptr<ASTNode>(parseExpression()));
                    if (check(TokenType::TOK_COMMA)) {
                        advance();
                    } else {
                        break;
                    }
                }
                consume(TokenType::TOK_RPAREN, "Expected ')'");
                
                return funcPtrCall;
            }
            
            // 既存の通常の関数呼び出し処理
            // ...
        }
        // ...
    }
}
```

**形式2: `ptr(args)` の解析**

形式2は、識別子が関数ポインタ変数である場合、通常の関数呼び出し構文と同じ形式になる。インタプリタ側で識別子の型をチェックし、ポインタ変数であれば関数ポインタ呼び出しとして扱う。

```c++
// parsePrimary() での関数呼び出し解析
if (check(TokenType::TOK_IDENTIFIER)) {
    Token token = advance();
    
    if (check(TokenType::TOK_LPAREN)) {
        // 関数呼び出し構文: identifier(args)
        // この時点では通常の関数呼び出しか関数ポインタ呼び出しか判別できない
        // インタプリタで識別子の型を確認して判断する
        
        ASTNode* call_node = new ASTNode(ASTNodeType::AST_FUNC_CALL);
        call_node->name = token.value;
        
        // 引数リストの解析
        // ...
        
        return call_node;
    }
}
```

### 3.4. インタプリタの拡張

#### 3.4.1. 関数アドレスの保存

```c++
// Evaluator クラスに追加
class Evaluator {
    // 関数ポインタ値を格納する構造体
    struct FunctionPointerValue {
        std::string function_name;    // 関数名
        const ASTNode* function_ast;  // 関数定義のASTノード
        TypeInfo return_type;         // 戻り値の型
    };
    
    // ポインタ変数が関数アドレスを持つ場合のマップ
    std::map<std::string, FunctionPointerValue> function_pointer_values_;
};
```

#### 3.4.2. `&関数` の評価

```c++
int64_t Evaluator::evaluate(const ASTNode* node) {
    // ...
    case ASTNodeType::AST_ADDRESS_OF:
        if (node->is_function_address) {
            // 関数のアドレスを取得
            std::string func_name = node->function_address_name;
            const ASTNode* func_ast = findFunctionAST(func_name);
            
            // 関数ポインタ値を作成（実際のアドレスは使わず、識別子として扱う）
            FunctionPointerValue fp_value;
            fp_value.function_name = func_name;
            fp_value.function_ast = func_ast;
            fp_value.return_type = func_ast->type_info;
            
            // ポインタ変数に保存（擬似アドレスとして関数名のハッシュ値を返す）
            int64_t pseudo_address = std::hash<std::string>{}(func_name);
            
            // マップに登録
            // (代入時にポインタ変数名と紐付ける)
            
            return pseudo_address;
        }
        // 既存の変数アドレス取得
        // ...
    // ...
}
```

#### 3.4.3. 関数ポインタ呼び出しの実行

**形式1: `(*ptr)(args)` の実行**

```c++
int64_t Evaluator::evaluate(const ASTNode* node) {
    // ...
    case ASTNodeType::AST_FUNC_PTR_CALL:
        {
            // ポインタ変数名を取得
            std::string ptr_var_name = node->left->name;
            
            // 関数ポインタ値を取得
            if (function_pointer_values_.find(ptr_var_name) != 
                function_pointer_values_.end()) {
                
                FunctionPointerValue& fp_value = function_pointer_values_[ptr_var_name];
                const ASTNode* actual_func = fp_value.function_ast;
                
                // 引数を評価
                std::vector<int64_t> arg_values;
                for (const auto& arg : node->arguments) {
                    arg_values.push_back(evaluate(arg.get()));
                }
                
                // 実際の関数を呼び出し
                return callFunction(actual_func, arg_values);
            }
            
            error("Invalid function pointer call");
        }
    // ...
}
```

**形式2: `ptr(args)` の実行**

```c++
int64_t Evaluator::evaluate(const ASTNode* node) {
    // ...
    case ASTNodeType::AST_FUNC_CALL:
        {
            std::string func_name = node->name;
            
            // まず、関数ポインタ変数かチェック
            if (function_pointer_values_.find(func_name) != 
                function_pointer_values_.end()) {
                
                // 関数ポインタ呼び出し
                FunctionPointerValue& fp_value = function_pointer_values_[func_name];
                const ASTNode* actual_func = fp_value.function_ast;
                
                // 引数を評価
                std::vector<int64_t> arg_values;
                for (const auto& arg : node->arguments) {
                    arg_values.push_back(evaluate(arg.get()));
                }
                
                // 実際の関数を呼び出し
                return callFunction(actual_func, arg_values);
            }
            
            // 通常の関数呼び出し
            // ...
        }
    // ...
}
```

#### 3.4.4. ポインタ代入時の関数ポインタ処理

```c++
// ポインタ変数への代入時
void Evaluator::assignPointerVariable(const std::string& var_name, 
                                      int64_t address_value,
                                      const ASTNode* rhs) {
    // rhsが関数アドレス取得の場合
    if (rhs->node_type == ASTNodeType::AST_ADDRESS_OF && 
        rhs->is_function_address) {
        
        std::string func_name = rhs->function_address_name;
        const ASTNode* func_ast = findFunctionAST(func_name);
        
        FunctionPointerValue fp_value;
        fp_value.function_name = func_name;
        fp_value.function_ast = func_ast;
        fp_value.return_type = func_ast->type_info;
        
        // ポインタ変数名と関数ポインタ値を紐付け
        function_pointer_values_[var_name] = fp_value;
    }
    
    // 通常のポインタ代入処理
    pointer_values_[var_name] = address_value;
}
```

## 4. 実装手順

### Phase 1: AST拡張 ✅ 部分的に完了
- [x] 既存のポインタ関連ノードの確認
- [ ] `is_function_address` フラグを追加
- [ ] `AST_FUNC_PTR_CALL` ノードタイプを追加

### Phase 2: パーサー拡張 🚧 進行中
- [ ] `&演算子` で関数名を認識
- [ ] `*ptr(args)` 構文を関数ポインタ呼び出しとして解析
- [ ] 乗算との区別ロジック実装

### Phase 3: インタプリタ拡張 🔜 未着手
- [ ] `FunctionPointerValue` 構造体を追加
- [ ] `&関数` の評価実装
- [ ] ポインタ変数への関数アドレス代入
- [ ] `*funcPtr(args)` の実行実装

### Phase 4: 型チェック 🔜 未着手
- [ ] 戻り値型の一致チェック
- [ ] `void*` への void型関数代入チェック

### Phase 5: テスト作成 🔜 未着手
- [ ] 基本的な関数ポインタテスト
- [ ] 配列テスト
- [ ] 構造体メンバーテスト
- [ ] パラメータ渡しテスト

## 5. 重要な注意点

1. **typedefは使用しない**: 通常のポインタ変数として扱う
2. **戻り値型のみで互換性判定**: パラメータの型や数は無視
3. **`void*`は特殊**: void型関数のアドレスのみ格納可能
4. **乗算との区別**: パーサーで文脈から判断
5. **既存のポインタ機能を活用**: 新規実装を最小限にする

## 6. テストケース例

```c++
// tests/cases/function_pointer/test_basic_function_pointer.cb
int add(int a, int b) {
    return a + b;
}

void main() {
    int* funcPtr = &add;
    int result = *funcPtr(5, 3);
    println(string(result));  // 期待値: 8
}
```

```c++
// tests/cases/function_pointer/test_void_function_pointer.cb
void sayHello() {
    println("Hello");
}

void main() {
    void* voidPtr = &sayHello;
    *voidPtr();  // 期待値: Hello
}
```

```c++
// tests/cases/function_pointer/test_function_pointer_array.cb
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

void main() {
    int* ops[2];
    ops[0] = &add;
    ops[1] = &sub;
    
    println(string(*ops[0](10, 5)));  // 期待値: 15
    println(string(*ops[1](10, 5)));  // 期待値: 5
}
```
