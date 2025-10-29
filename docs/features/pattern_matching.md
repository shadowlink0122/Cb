# Pattern Matching (match文) 設計書

**バージョン**: v0.11.0 Phase 1a  
**ステータス**: **完了**（実装済み）  
**優先度**: 高（async/await実装の前提条件）

---

## 概要

Cb言語にRust風のパターンマッチング（`match`文）を実装しました。Result<T, E>やOption<T>などのEnum型を効率的に処理できます。

---

## 🎯 目標

### ✅ Phase 1a: 基本的なEnum match（完了）
- ✅ Enum variant のマッチング
- ✅ 関連値の抽出（destructuring）
- ✅ 基本的な制御フロー
- ✅ ワイルドカード（`_`）バインディング
- ✅ 関数返り値のEnum処理
- ✅ 変数、関数呼び出し、Enum構築式のmatch式サポート

### 将来の拡張（Phase 2）
- リテラル値のマッチング
- ガード条件（`if`）
- ネストしたEnum型のパターン

---

## 📝 構文仕様

### 基本構文

```cb
match (expression) {
    Pattern1 => statement,
    Pattern2 => { block },
    Pattern3 => statement,
}
```

### Enumパターンマッチング

```cb
enum Option<T> {
    Some(T),
    None
};

int main() {
    Option<int> opt = Option<int>::Some(42);
    
    match (opt) {
        Some(value) => {
            println("Value: ", value);
        },
        None => {
            println("No value");
        }
    }
    
    return 0;
}
```

### Resultパターンマッチング

```cb
enum Result<T, E> {
    Ok(T),
    Err(E)
};

Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    return Result<int, string>::Ok(a / b);
}

int main() {
    Result<int, string> result = divide(10, 2);
    
    match (result) {
        Ok(value) => {
            println("Success: ", value);
        },
        Err(error) => {
            println("Error: ", error);
        }
    }
    
    return 0;
}
```

### ワイルドカードバインディング

```cb
enum Status {
    Ready(int),
    Running(int),
    Stopped(int),
    Done,
    Failed
};

int main() {
    Status s1 = Status::Ready(100);
    
    match (s1) {
        Ready(value) => println("Ready with value: ", value),
        _ => println("Other status"),
    }
    
    return 0;
}
```

**注意**: Cbでは前置返り値型のみをサポートしています。関数定義は次の形式を使用します：

```cb
int my_function(int x) {
    // ...
}
```

後置返り値型（`->`構文）はサポートされていません。

---

## 🏗️ BNF定義

```bnf
<match_statement> ::= 'match' '(' <expression> ')' '{' <match_arm_list> '}'

<match_arm_list> ::= <match_arm> { ',' <match_arm> } [',']

<match_arm> ::= <pattern> '=>' ( <statement> | <block> )

<pattern> ::= <enum_pattern>
            | <wildcard_pattern>
            | <literal_pattern>       // 将来の拡張

<enum_pattern> ::= <identifier> [ '(' <pattern_binding_list> ')' ]

<pattern_binding_list> ::= <pattern_binding> { ',' <pattern_binding> }

<pattern_binding> ::= <identifier> | '_'

<wildcard_pattern> ::= '_'

<literal_pattern> ::= <integer_literal>
                    | <string_literal>
                    | <boolean_literal>
```

### 例

**基本的なパターンマッチング:**
```cb
match (opt) {
    Some(value) => println("Value: ", value),
    None => println("No value"),
}
```

**ワイルドカードバインディング:**
```cb
match (status) {
    Ready(value) => println("Ready: ", value),
    Running(_) => println("Running (value discarded)"),
    _ => println("Other status"),
}
```

**関数返り値のマッチング:**
```cb
match (divide(10, 2)) {
    Ok(result) => println("Result: ", result),
    Err(code) => println("Error code: ", code),
}
```

---

## 🔧 実装要件

### 1. AST拡張

#### ast.h

```cpp
// Match文のノードタイプ
enum ASTNodeType {
    // ... existing types ...
    AST_MATCH,           // match文全体
    AST_MATCH_ARM,       // match文のアーム（1つの分岐）
    AST_PATTERN,         // パターン
};

// パターンの種類
enum PatternType {
    PATTERN_ENUM_VARIANT,    // Enum variant: Some(value)
    PATTERN_WILDCARD,        // ワイルドカード: _
    PATTERN_LITERAL,         // リテラル: 42, "string"
};

// Match Arm（分岐）
struct MatchArm {
    PatternType pattern_type;
    std::string variant_name;             // "Some", "Ok", "Err"
    std::vector<std::string> bindings;    // 束縛する変数名 ["value"]
    std::unique_ptr<ASTNode> body;        // arm の本体
};

// Match文のASTノード
struct ASTNode {
    ASTNodeType type;
    
    // match文用フィールド
    std::unique_ptr<ASTNode> match_expression;  // match対象の式
    std::vector<MatchArm> match_arms;           // 各分岐
    
    // ... existing fields ...
};
```

### 2. パーサー実装

#### statement_parser.cpp

```cpp
// match文のパース
ASTNode *StatementParser::parseMatchStatement() {
    // 'match' キーワードの消費
    parser_->consume(TokenType::TOK_MATCH, "Expected 'match'");
    
    // '(' と match対象の式をパース
    parser_->consume(TokenType::TOK_LPAREN, "Expected '(' after 'match'");
    auto match_expr = parser_->parseExpression();
    parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after match expression");
    
    // '{' を消費
    parser_->consume(TokenType::TOK_LBRACE, "Expected '{' to start match arms");
    
    // match armsをパース
    std::vector<MatchArm> arms;
    
    while (!parser_->check(TokenType::TOK_RBRACE) && !parser_->isAtEnd()) {
        MatchArm arm = parseMatchArm();
        arms.push_back(std::move(arm));
        
        // オプショナルなカンマ
        if (parser_->check(TokenType::TOK_COMMA)) {
            parser_->advance();
        }
    }
    
    // '}' を消費
    parser_->consume(TokenType::TOK_RBRACE, "Expected '}' after match arms");
    
    // ASTノードを構築
    auto match_node = std::make_unique<ASTNode>(AST_MATCH);
    match_node->match_expression = std::move(match_expr);
    match_node->match_arms = std::move(arms);
    
    return match_node.release();
}

MatchArm StatementParser::parseMatchArm() {
    MatchArm arm;
    
    // パターンをパース: Some(value) or None
    std::string variant_name = parser_->consume(TokenType::TOK_IDENTIFIER, 
                                                 "Expected pattern");
    arm.variant_name = variant_name;
    arm.pattern_type = PATTERN_ENUM_VARIANT;
    
    // 関連値の束縛をパース: (value)
    if (parser_->match(TokenType::TOK_LPAREN)) {
        do {
            std::string binding = parser_->consume(TokenType::TOK_IDENTIFIER,
                                                    "Expected binding variable");
            arm.bindings.push_back(binding);
        } while (parser_->match(TokenType::TOK_COMMA));
        
        parser_->consume(TokenType::TOK_RPAREN, "Expected ')' after bindings");
    }
    
    // '=>' を消費
    parser_->consume(TokenType::TOK_FAT_ARROW, "Expected '=>' after pattern");
    
    // 本体をパース（statement or block）
    if (parser_->check(TokenType::TOK_LBRACE)) {
        arm.body = parser_->parseBlock();
    } else {
        arm.body = parser_->parseStatement();
    }
    
    return arm;
}
```

### 3. トークン追加

#### token.h

```cpp
enum class TokenType {
    // ... existing tokens ...
    TOK_MATCH,       // "match"
    TOK_FAT_ARROW,   // "=>"
    // ... existing tokens ...
};
```

#### lexer.cpp

```cpp
// キーワード登録
keywords_["match"] = TokenType::TOK_MATCH;

// => の認識
if (current == '=' && peek() == '>') {
    advance();
    advance();
    return Token(TokenType::TOK_FAT_ARROW, "=>", line, column);
}
```

### 4. インタプリタ実装

#### interpreter.cpp

```cpp
int64_t Interpreter::eval_match(const ASTNode *node) {
    // match対象の式を評価
    int64_t match_value = eval_expression(node->match_expression.get());
    
    // Enum値の取得（EnumValue構造体を想定）
    EnumValue enum_val = getEnumValue(match_value);
    
    // 各armを評価
    for (const auto &arm : node->match_arms) {
        // パターンマッチング
        if (arm.variant_name == enum_val.variant) {
            // 関連値を束縛
            for (size_t i = 0; i < arm.bindings.size(); ++i) {
                std::string binding_name = arm.bindings[i];
                int64_t binding_value = enum_val.associated_values[i];
                
                // 新しいスコープに変数を追加
                setVariable(binding_name, binding_value);
            }
            
            // armの本体を実行
            return eval_statement(arm.body.get());
        }
    }
    
    // どのパターンにもマッチしない場合はエラー
    throw std::runtime_error("Non-exhaustive match: no pattern matched");
}
```

---

## 🧪 テスト計画

### Test 1: Basic Option Match (test_match_option_basic.cb)

```cb
enum Option<T> {
    Some(T),
    None
};

int main() {
    Option<int> some_val = Option<int>::Some(42);
    
    match (some_val) {
        Some(value) => {
            println("Value: ", value);
            assert(value == 42);
        },
        None => {
            println("No value");
            assert(false);  // Should not reach here
        }
    }
    
    println("✓ Test 1 passed");
    return 0;
}
```

### Test 2: Basic Result Match (test_match_result_basic.cb)

```cb
enum Result<T, E> {
    Ok(T),
    Err(E)
};

int main() {
    Result<int, string> ok_val = Result<int, string>::Ok(100);
    
    match (ok_val) {
        Ok(value) => {
            println("Success: ", value);
            assert(value == 100);
        },
        Err(error) => {
            println("Error: ", error);
            assert(false);  // Should not reach here
        }
    }
    
    println("✓ Test 2 passed");
    return 0;
}
```

### Test 3: Match with Error (test_match_error.cb)

```cb
enum Result<T, E> {
    Ok(T),
    Err(E)
};

Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Result<int, string>::Err("Division by zero");
    }
    return Result<int, string>::Ok(a / b);
}

int main() {
    Result<int, string> err_result = divide(10, 0);
    
    match (err_result) {
        Ok(value) => {
            println("Success: ", value);
            assert(false);  // Should not reach here
        },
        Err(error) => {
            println("Error: ", error);
            assert(error == "Division by zero");
        }
    }
    
    println("✓ Test 3 passed");
    return 0;
}
```

### Test 4: Multiple Matches (test_match_multiple.cb)

```cb
enum Option<T> {
    Some(T),
    None
};

int process_option(Option<int> opt) {
    match (opt) {
        Some(value) => {
            return value * 2;
        },
        None => {
            return -1;
        }
    }
}

int main() {
    Option<int> some_val = Option<int>::Some(21);
    int result1 = process_option(some_val);
    assert(result1 == 42);
    
    Option<int> none_val = Option<int>::None;
    int result2 = process_option(none_val);
    assert(result2 == -1);
    
    println("✓ Test 4 passed");
    return 0;
}
```

---

## 📋 実装ステータス

### ✅ Phase 1a: 完了（実装済み）
1. ✅ AST拡張（AST_MATCH_STMT, AST_MATCH_ARM, MatchArm構造体）
2. ✅ トークン追加（TOK_MATCH, TOK_FAT_ARROW, TOK_UNDERSCORE）
3. ✅ レキサー拡張（"match", "=>", "_"の認識）
4. ✅ parseMatchStatement()実装
5. ✅ parseMatchArm()実装
6. ✅ execute_match_statement()実装
7. ✅ Enum値の取得と処理
8. ✅ パターンマッチングロジック
9. ✅ 変数束縛（関連値の取り出し）
10. ✅ ワイルドカード（`_`）バインディング
11. ✅ 関数返り値のEnum処理
12. ✅ match式の拡張（変数、関数呼び出し、Enum構築式）
13. ✅ 全13テスト成功

### Phase 2: 将来の拡張
- リテラルパターンマッチング
- ガード条件（`if`）
- ネストしたEnum型のパターン

---

## 🎯 成功基準（達成済み）

- ✅ 基本的なmatch文がパースできる
- ✅ Option<T>のSome/Noneがマッチできる
- ✅ Result<T, E>のOk/Errがマッチできる
- ✅ 関連値が正しく抽出される
- ✅ 変数束縛が機能する
- ✅ ワイルドカード（`_`）が機能する
- ✅ 関数返り値のEnum処理が動作する
- ✅ 全テストが成功する（9/9）
- ✅ ドキュメントが完備される

---

## 🔄 依存関係

**前提条件（完了済み）:**
- ✅ ジェネリックEnum実装（Option<T>, Result<T, E>）
- ✅ Enum関連値のサポート

**後続機能:**
- async/await（match文を使ったエラーハンドリング）
- ?オペレーター（match文の簡略記法）

---

## 📚 参考

### Rust match文
```rust
match result {
    Ok(value) => println!("Success: {}", value),
    Err(error) => println!("Error: {}", error),
}
```

### Swift switch文
```swift
switch result {
case .success(let value):
    print("Success: \(value)")
case .failure(let error):
    print("Error: \(error)")
}
```

---

## 🚀 将来の拡張（Phase 2以降）

### ワイルドカードパターン

```cb
match (value) {
    Some(x) => println("Value: ", x),
    _ => println("Default case"),
}
```

### ガード条件

```cb
match (opt) {
    Some(value) if value > 10 => println("Large value"),
    Some(value) => println("Small value"),
    None => println("No value"),
}
```

### リテラルパターン

```cb
match (status_code) {
    200 => println("OK"),
    404 => println("Not Found"),
    500 => println("Server Error"),
    _ => println("Unknown"),
}
```

### ネストしたパターン

```cb
enum Option<T> {
    Some(T),
    None
};

match (outer) {
    Some(Option<int>::Some(value)) => println("Nested Some: ", value),
    Some(Option<int>::None) => println("Inner None"),
    None => println("Outer None"),
}
```
