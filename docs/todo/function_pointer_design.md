# 関数ポインタ実装設計書（改訂版）

## 1. 概要

関数ポインタ機能をCb言語に追加する。**関数ポインタはポインタ変数の一種**として実装し、関数のアドレスを格納し、そのポインタを通して関数を呼び出すことができる。

## 2. 構文設計（改訂）

### 2.1. 基本構文

**関数ポインタはポインタ変数**として扱う。`typedef`は使用しない。

```c++
// 関数定義
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

// 関数ポインタ変数の宣言と代入
int* op = &add;           // &演算子で関数のアドレスを取得
int result = *op(5, 3);   // *演算子で関数を呼び出し（デリファレンス + 呼び出し）

// 別の関数を代入
op = &multiply;
result = *op(5, 3);       // 15
```

### 2.2. void型関数のポインタ

void型関数のアドレスは`void*`型に格納できる。

```c++
void printMessage() {
    println("Hello");
}

void* voidFunc = &printMessage;
*voidFunc();  // Hello
```

**注意**: `void*`は汎用ポインタではなく、**void型関数のアドレスを格納するポインタ**として扱う。

### 2.3. 型の互換性

**戻り値の型が同じ**であれば、異なる関数のアドレスを同じポインタ変数に代入できる。

```c++
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int x, int y) { return x * y; }

int* op = &add;       // OK
op = &sub;            // OK（戻り値の型がint）
op = &mul;            // OK（戻り値の型がint）

int result1 = *op(10, 5);
```

### 2.4. 関数ポインタの配列

関数ポインタも配列に格納できる。

```c++
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

int* ops[2];
ops[0] = &add;
ops[1] = &sub;

int result1 = *ops[0](10, 5); // 15
int result2 = *ops[1](10, 5); // 5
```

### 2.5. 構造体メンバーとしての関数ポインタ

構造体のメンバーとして関数ポインタを持つことができる。

```c++
struct Calculator {
    int* operation;
}

int add(int a, int b) { return a + b; }

Calculator calc;
calc.operation = &add;
int result = *calc.operation(5, 3); // 8
```

### 2.6. 関数ポインタをパラメータとして渡す

関数ポインタを関数のパラメータとして渡すことができる。

```c++
int apply(int a, int b, int* operation) {
    return *operation(a, b);
}

int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }

int result1 = apply(5, 3, &add);      // 8
int result2 = apply(5, 3, &multiply); // 15
```

### 2.7. 乗算との区別

**重要**: `*func()`は関数ポインタの呼び出しであり、通常の乗算`val * func()`とは異なる。

```c++
int getValue() { return 10; }

int val = 5;
int result1 = val * getValue();  // 乗算: 5 * 10 = 50

int* funcPtr = &getValue;
int result2 = *funcPtr();        // 関数ポインタ呼び出し: 10
```

パーサーは文脈から判断する必要がある:
- `*識別子(` → 関数ポインタ呼び出し
- `式 * 識別子(` → 乗算

```c++
typedef int (*BinaryOp)(int, int);

int add(int a, int b) { return a + b; }
int subtract(int a, int b) { return a - b; }

BinaryOp getOperation(bool useAdd) {
    if (useAdd) {
        return add;
    } else {
        return subtract;
    }
}

BinaryOp op = getOperation(true);
int result = op(10, 5); // 15
```

## 3. AST拡張

### 3.1. 関数ポインタ型情報構造体

```c++
// ast.h に追加
struct FunctionPointerTypeInfo {
    TypeInfo return_type;                    // 戻り値の型
    std::string return_type_name;            // 戻り値型名（カスタム型対応）
    std::vector<TypeInfo> param_types;       // パラメータ型のリスト
    std::vector<std::string> param_type_names; // パラメータ型名（カスタム型対応）
    std::vector<std::string> param_names;    // パラメータ名（オプション）
    
    FunctionPointerTypeInfo() : return_type(TYPE_UNKNOWN) {}
    
    // 型情報文字列を生成（例: "int (*)(int, int)"）
    std::string to_string() const;
    
    // 型の互換性をチェック
    bool is_compatible_with(const FunctionPointerTypeInfo& other) const;
};
```

### 3.2. TypeInfo拡張

```c++
// ast.h の TypeInfo enum に追加
enum TypeInfo {
    // ... 既存の型 ...
    TYPE_FUNCTION_POINTER = 18, // 関数ポインタ型
};
```

### 3.3. ASTNode拡張

```c++
// ast.h の ASTNode クラスに追加
class ASTNode {
    // ... 既存のメンバー ...
    
    // 関数ポインタ関連
    FunctionPointerTypeInfo function_pointer_type; // 関数ポインタ型情報
    bool is_function_pointer = false;              // 関数ポインタかどうか
};
```

### 3.4. AST ノードタイプ追加

```c++
// ast.h の ASTNodeType enum に追加
enum class ASTNodeType {
    // ... 既存のノードタイプ ...
    AST_FUNCTION_POINTER_TYPEDEF, // 関数ポインタtypedef宣言
};
```

## 4. パーサー拡張

### 4.1. parseTypedefDeclaration の拡張

```c++
ASTNode* RecursiveParser::parseTypedefDeclaration() {
    consume(TokenType::TOK_TYPEDEF, "Expected 'typedef'");
    
    // 既存のチェック（struct, enum, union）
    // ...
    
    // 関数ポインタtypedef構文のチェック
    if (isFunctionPointerTypedef()) {
        return parseFunctionPointerTypedefDeclaration();
    }
    
    // 既存のtypedef処理
    // ...
}
```

### 4.2. isFunctionPointerTypedef の実装

```c++
bool RecursiveParser::isFunctionPointerTypedef() {
    // 先読みして構文パターンを確認
    // typedef <return_type> (*<name>)(...)
    
    size_t lookahead_pos = 0;
    
    // <return_type> をスキップ
    if (!isType(peek(lookahead_pos))) {
        return false;
    }
    lookahead_pos++;
    
    // '(' をチェック
    if (peek(lookahead_pos).type != TokenType::TOK_LPAREN) {
        return false;
    }
    lookahead_pos++;
    
    // '*' をチェック
    if (peek(lookahead_pos).type != TokenType::TOK_STAR) {
        return false;
    }
    
    return true; // 関数ポインタtypedef構文
}
```

### 4.3. parseFunctionPointerTypedefDeclaration の実装

```c++
ASTNode* RecursiveParser::parseFunctionPointerTypedefDeclaration() {
    // typedef <return_type> (*<name>)(<param_types>);
    
    // 戻り値型の解析
    TypeInfo return_type = parseType();
    std::string return_type_name = current_token_.value;
    
    // '(' の消費
    consume(TokenType::TOK_LPAREN, "Expected '(' in function pointer typedef");
    
    // '*' の消費
    consume(TokenType::TOK_STAR, "Expected '*' in function pointer typedef");
    
    // 関数ポインタ型名の取得
    std::string typedef_name = current_token_.value;
    consume(TokenType::TOK_IDENTIFIER, "Expected identifier in function pointer typedef");
    
    // ')' の消費
    consume(TokenType::TOK_RPAREN, "Expected ')' after function pointer name");
    
    // '(' の消費（パラメータリスト開始）
    consume(TokenType::TOK_LPAREN, "Expected '(' for parameter list");
    
    // パラメータリストの解析
    std::vector<TypeInfo> param_types;
    std::vector<std::string> param_type_names;
    std::vector<std::string> param_names;
    
    if (!check(TokenType::TOK_RPAREN)) {
        do {
            TypeInfo param_type = parseType();
            std::string param_type_name = current_token_.value;
            param_types.push_back(param_type);
            param_type_names.push_back(param_type_name);
            
            // パラメータ名は省略可能
            if (check(TokenType::TOK_IDENTIFIER)) {
                param_names.push_back(current_token_.value);
                advance();
            } else {
                param_names.push_back(""); // 匿名パラメータ
            }
            
            if (check(TokenType::TOK_COMMA)) {
                advance();
            } else {
                break;
            }
        } while (true);
    }
    
    // ')' の消費（パラメータリスト終了）
    consume(TokenType::TOK_RPAREN, "Expected ')' after parameter list");
    
    // ';' の消費
    consume(TokenType::TOK_SEMICOLON, "Expected ';' after function pointer typedef");
    
    // ASTノードの作成
    ASTNode* node = new ASTNode(ASTNodeType::AST_FUNCTION_POINTER_TYPEDEF);
    node->name = typedef_name;
    node->is_function_pointer = true;
    node->function_pointer_type.return_type = return_type;
    node->function_pointer_type.return_type_name = return_type_name;
    node->function_pointer_type.param_types = param_types;
    node->function_pointer_type.param_type_names = param_type_names;
    node->function_pointer_type.param_names = param_names;
    
    // typedef マップに登録
    function_pointer_typedefs_[typedef_name] = node->function_pointer_type;
    
    setLocation(node, current_token_);
    return node;
}
```

## 5. 型システム拡張

### 5.1. 関数ポインタ型チェック

```c++
// recursive_parser.h に追加
class RecursiveParser {
    // ... 既存のメンバー ...
    
    std::map<std::string, FunctionPointerTypeInfo> function_pointer_typedefs_;
    
    bool isFunctionPointerType(const std::string& type_name) {
        return function_pointer_typedefs_.find(type_name) != function_pointer_typedefs_.end();
    }
    
    FunctionPointerTypeInfo getFunctionPointerTypeInfo(const std::string& type_name) {
        return function_pointer_typedefs_[type_name];
    }
};
```

### 5.2. 関数から関数ポインタへの暗黙的変換

変数代入時に、右辺が関数名の場合、関数ポインタ型への暗黙的変換を行う。

```c++
// 変数宣言の解析時
// BinaryOp op = add; のような構文をサポート
```

## 6. インタプリタ拡張

### 6.1. 関数ポインタ値の表現

```c++
// 関数ポインタの値を保持する構造体
struct FunctionPointerValue {
    std::string function_name;  // 関数名
    const ASTNode* function_ast; // 関数定義のASTノード
    
    FunctionPointerValue() : function_ast(nullptr) {}
    FunctionPointerValue(const std::string& name, const ASTNode* ast)
        : function_name(name), function_ast(ast) {}
};
```

### 6.2. 変数値マップの拡張

```c++
// Evaluator クラスに追加
class Evaluator {
    // ... 既存のメンバー ...
    
    std::map<std::string, FunctionPointerValue> function_pointer_values_;
};
```

### 6.3. 関数ポインタ呼び出しの実装

```c++
// 関数呼び出し時に、関数名が関数ポインタ変数の場合の処理
int64_t Evaluator::evaluateFunctionCall(const ASTNode* node) {
    std::string func_name = node->name;
    
    // 関数ポインタ変数かチェック
    if (function_pointer_values_.find(func_name) != function_pointer_values_.end()) {
        FunctionPointerValue fp_value = function_pointer_values_[func_name];
        const ASTNode* actual_function = fp_value.function_ast;
        
        // 実際の関数を呼び出し
        return callFunction(actual_function, node->arguments);
    }
    
    // 通常の関数呼び出し
    // ...
}
```

## 7. テスト計画

### 7.1. 基本テスト

- `tests/cases/function_pointer/test_basic_function_pointer.cb`
- `tests/cases/function_pointer/test_function_pointer_call.cb`
- `tests/cases/function_pointer/test_function_pointer_assignment.cb`

### 7.2. 配列テスト

- `tests/cases/function_pointer/test_function_pointer_array.cb`

### 7.3. 構造体テスト

- `tests/cases/function_pointer/test_function_pointer_struct.cb`

### 7.4. パラメータテスト

- `tests/cases/function_pointer/test_function_pointer_parameter.cb`

### 7.5. 戻り値テスト

- `tests/cases/function_pointer/test_function_pointer_return.cb`

### 7.6. コールバックテスト

- `tests/cases/function_pointer/test_callback_example.cb`

## 8. 実装フェーズ

### Phase 1: AST拡張（1日） ✅ 完了
- [x] FunctionPointerTypeInfo 構造体を追加
- [x] TypeInfo に TYPE_FUNCTION_POINTER を追加
- [x] ASTNode に関数ポインタ関連メンバーを追加
- [x] AST_FUNCTION_POINTER_TYPEDEF ノードタイプを追加
- [x] FunctionPointerTypeInfo::to_string() 実装
- [x] type_utils.cpp に TYPE_FUNCTION_POINTER ケース追加

**実装ファイル**:
- `src/common/ast.h`: FunctionPointerTypeInfo構造体、TYPE_FUNCTION_POINTER、AST_FUNCTION_POINTER_TYPEDEF追加
- `src/common/type_utils.cpp`: to_string()実装、TYPE_FUNCTION_POINTERケース追加

### Phase 2: パーサー拡張（2-3日） ✅ 完了
- [x] isFunctionPointerTypedef 実装
- [x] parseFunctionPointerTypedefDeclaration 実装
- [x] parseTypedefDeclaration に関数ポインタ処理を追加
- [x] 関数ポインタ型マップの管理
- [x] 基本テスト: typedef宣言のパースが成功

**実装ファイル**:
- `src/frontend/recursive_parser/recursive_parser.h`: メソッド宣言、function_pointer_typedefs_マップ追加
- `src/frontend/recursive_parser/recursive_parser.cpp`: isFunctionPointerTypedef()、parseFunctionPointerTypedefDeclaration()実装

**テスト結果**:
```bash
$ ./main tests/cases/function_pointer/test_basic_typedef.cb
Function pointer typedef test
BinaryOp type declared successfully
```

### Phase 3: 型システム拡張（2日） 🚧 進行中
- [ ] 関数ポインタ型チェック実装
- [ ] 関数から関数ポインタへの暗黙的変換
- [ ] 型互換性チェック実装
- [ ] 変数宣言時の関数ポインタ型認識

**次に実装する必要があるもの**:
1. `parseVariableDeclaration()` で関数ポインタ型を認識
2. `getTypeInfoFromString()` で関数ポインタtypedef名を処理
3. インタプリタで関数ポインタ型の変数を保持

### Phase 4: インタプリタ拡張（3-4日） 🔜 未着手
- [ ] FunctionPointerValue 構造体を追加
- [ ] 関数ポインタ値の保持
- [ ] 関数ポインタ呼び出しの実装
- [ ] 関数ポインタ代入の実装

**実装詳細**:
1. **関数ポインタ変数の宣言**:
   ```c++
   // Cb コード
   typedef int (*BinaryOp)(int, int);
   BinaryOp op;  // ← この部分の実装が必要
   ```
   
2. **関数ポインタへの関数代入**:
   ```c++
   // Cb コード
   op = add;  // ← 関数名から関数ポインタへの暗黙変換
   ```
   
3. **関数ポインタ経由の関数呼び出し**:
   ```c++
   // Cb コード
   int result = op(5, 3);  // ← 関数ポインタを使った呼び出し
   ```

### Phase 5: テスト作成（2-3日）
- [ ] 基本テストの作成と実行
- [ ] 配列テストの作成と実行
- [ ] 構造体テストの作成と実行
- [ ] パラメータ/戻り値テストの作成と実行
- [ ] コールバック例の作成と実行

### Phase 6: 統合テスト（1-2日）
- [ ] 既存のテストが全て通過することを確認
- [ ] エッジケースのテスト
- [ ] エラーハンドリングのテスト

### Phase 7: ドキュメント作成（1日）
- [ ] spec.md に関数ポインタ仕様を追加
- [ ] README.md を更新
- [ ] サンプルコードの作成

**合計推定時間**: 12-16日

## 9. 制約事項と将来の拡張

### 現在の制約
- 関数ポインタは typedef を使用した型エイリアスのみサポート
- inline関数ポインタ宣言（`int (*fp)(int, int);`）は未サポート
- 関数ポインタの関数ポインタ（高階関数ポインタ）は未サポート
- varargs（可変長引数）を持つ関数ポインタは未サポート

### 将来の拡張候補
- inline関数ポインタ宣言のサポート
- 高階関数ポインタのサポート
- ラムダ式/クロージャのサポート
- 関数ポインタのnullチェック
- 関数ポインタのポインタ（`int (**fpp)(int, int);`）

## 10. まとめ

関数ポインタ機能の追加により、Cbは以下のような高度なプログラミング技法をサポートできるようになる:

1. **コールバック関数**: イベント駆動プログラミング
2. **戦略パターン**: アルゴリズムの動的切り替え
3. **関数テーブル**: 関数のディスパッチ
4. **プラグインシステム**: 動的な機能拡張

この機能は、v0.10.0の最優先実装項目として位置づけられる。
