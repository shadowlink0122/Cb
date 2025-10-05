# 次のステップ: プレインクリメント/デクリメントの完全実装

## 📋 現状の問題

### Test 2: Struct member pre-increment
```cb
Counter c2;
c2.value = 5;
++c2.value;  // ❌ エラー: "Invalid prefix operation"
```

**エラー詳細**:
```
Direct array assignment error
Error: Invalid prefix operation
```

## 🔍 根本原因

### 1. parseUnary() の制限
`parseUnary()` は `++expr` の形式をパースできるが、`expr` が `obj.member` の場合に正しく処理できない。

**現在の実装** (`recursive_parser.cpp:2150`):
```cpp
ASTNode* RecursiveParser::parseUnary() {
    if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
        Token op = advance();
        ASTNode* operand = parseUnary();  // ← ここで再帰的にparseUnary()を呼ぶ
        
        ASTNode* unary = new ASTNode(ASTNodeType::AST_UNARY_OP);
        unary->op = op.value;
        unary->left = std::unique_ptr<ASTNode>(operand);
        
        return unary;
    }
    
    return parsePostfix();
}
```

**問題点**:
- `++obj.member` をパースすると、`operand` に `obj.member` (AST_MEMBER_ACCESS) が入る
- しかし、`AST_UNARY_OP` として処理されるため、インクリメント/デクリメント専用の処理が行われない
- `AST_PRE_INCDEC` ノードが生成されない

### 2. ステートメントレベルでの処理不足
`parseStatement()` では、プレインクリメント/デクリメントが式の一部としてのみ扱われる。

## 🎯 解決策

### アプローチ1: parseUnary() を修正 (推奨)

**修正内容**:
```cpp
ASTNode* RecursiveParser::parseUnary() {
    // Prefix operators: !, -, ++, --, ~, &, *
    if (check(TokenType::TOK_NOT) || check(TokenType::TOK_MINUS) || 
        check(TokenType::TOK_BIT_NOT) || check(TokenType::TOK_BIT_AND) ||
        check(TokenType::TOK_MUL)) {
        Token op = advance();
        ASTNode* operand = parseUnary();
        
        ASTNode* unary = new ASTNode(ASTNodeType::AST_UNARY_OP);
        if (op.type == TokenType::TOK_BIT_AND) {
            unary->op = "ADDRESS_OF";
        } else if (op.type == TokenType::TOK_MUL) {
            unary->op = "DEREFERENCE";
        } else {
            unary->op = op.value;
        }
        unary->left = std::unique_ptr<ASTNode>(operand);
        
        return unary;
    }
    
    // ++ と -- は別処理: AST_PRE_INCDEC を生成
    if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
        Token op = advance();
        ASTNode* operand = parsePostfix();  // ← parsePostfix()を直接呼ぶ
        
        // AST_PRE_INCDEC ノードを生成
        ASTNode* incdec = new ASTNode(ASTNodeType::AST_PRE_INCDEC);
        incdec->op = op.value;
        incdec->left = std::unique_ptr<ASTNode>(operand);
        
        return incdec;
    }
    
    return parsePostfix();
}
```

**利点**:
- 最小限の変更で実装可能
- `++var` も `++obj.member` も同じロジックで処理できる
- 既存のコードとの互換性が高い

### アプローチ2: parseStatement() で処理 (より複雑)

**修正内容**:
```cpp
ASTNode* RecursiveParser::parseStatement() {
    // ... (既存のチェック)
    
    // プレインクリメント/デクリメント
    if (check(TokenType::TOK_INCR) || check(TokenType::TOK_DECR)) {
        TokenType op_type = current_token_.type;
        advance(); // consume '++' or '--'
        
        if (!check(TokenType::TOK_IDENTIFIER)) {
            error("Expected identifier after '++' or '--'");
            return nullptr;
        }
        
        std::string name = advance().value;
        
        // メンバーアクセスのチェック
        if (check(TokenType::TOK_DOT)) {
            advance(); // consume '.'
            
            if (!check(TokenType::TOK_IDENTIFIER)) {
                error("Expected member name after '.'");
                return nullptr;
            }
            
            std::string member_name = advance().value;
            consume(TokenType::TOK_SEMICOLON, "Expected ';' after increment/decrement");
            
            // AST_PRE_INCDEC + AST_MEMBER_ACCESS を作成
            ASTNode* member_access = new ASTNode(ASTNodeType::AST_MEMBER_ACCESS);
            member_access->name = member_name;
            ASTNode* obj_var = new ASTNode(ASTNodeType::AST_VARIABLE);
            obj_var->name = name;
            member_access->left = std::unique_ptr<ASTNode>(obj_var);
            
            ASTNode* incdec = new ASTNode(ASTNodeType::AST_PRE_INCDEC);
            incdec->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
            incdec->left = std::unique_ptr<ASTNode>(member_access);
            
            return incdec;
        } else {
            // 通常の変数
            consume(TokenType::TOK_SEMICOLON, "Expected ';' after increment/decrement");
            
            ASTNode* var_node = new ASTNode(ASTNodeType::AST_VARIABLE);
            var_node->name = name;
            
            ASTNode* incdec = new ASTNode(ASTNodeType::AST_PRE_INCDEC);
            incdec->op = (op_type == TokenType::TOK_INCR) ? "++" : "--";
            incdec->left = std::unique_ptr<ASTNode>(var_node);
            
            return incdec;
        }
    }
    
    // ... (残りの処理)
}
```

**欠点**:
- コードの重複が多い
- 式としてのインクリメント/デクリメントとステートメントとしてのインクリメント/デクリメントで別の処理が必要

## 📝 実装手順 (アプローチ1を採用)

### ステップ1: parseUnary() を修正
ファイル: `src/frontend/recursive_parser/recursive_parser.cpp`

**修正箇所**: 約2150行目の `parseUnary()` 関数

**変更内容**:
- `TOK_INCR` と `TOK_DECR` を他の単項演算子から分離
- `AST_PRE_INCDEC` ノードを生成するように変更
- `parsePostfix()` を直接呼び出して、メンバーアクセスを含む式を取得

### ステップ2: expression_evaluator.cpp の確認
ファイル: `src/backend/interpreter/evaluator/expression_evaluator.cpp`

**既に実装済みの部分**:
- 約683行目の `AST_PRE_INCDEC` / `AST_POST_INCDEC` の処理
- `AST_VARIABLE` と `AST_MEMBER_ACCESS` の両方に対応済み

**確認事項**:
- `AST_PRE_INCDEC` + `AST_MEMBER_ACCESS` の組み合わせが正しく動作するか

### ステップ3: テストの実行
```bash
cd /Users/shadowlink/Documents/git/Cb
make clean && make
./main tests/cases/struct_member_incdec/test_struct_member_incdec.cb
```

**期待される出力**:
```
Test 1: Struct member post-increment
Before: value = 10, count = 20
After post-increment: value = 11, count = 21

Test 2: Struct member pre-increment
Before: value = 5, count = 15
After pre-increment: value = 6, count = 16

Test 3: Struct member post-decrement
Before: value = 30, count = 40
After post-decrement: value = 29, count = 39

Test 4: Struct member pre-decrement
Before: value = 50, count = 60
After pre-decrement: value = 49, count = 59

Test 5: Float struct member increment
Before: x = 1.500000, y = 2.500000
After increment: x = 2.500000, y = 3.500000

Test 6: Float struct member decrement
Before: x = 10.500000, y = 20.500000
After decrement: x = 9.500000, y = 19.500000
```

## 🎯 次の次のステップ

1. **浮動小数点ポインタのバグ修正**
   - `expression_evaluator.cpp` の `DEREFERENCE` 演算子を拡張
   - `TypedValue` を使った型判定と処理

2. **配列要素のインクリメント/デクリメント**
   - `arr[i]++` のサポート
   - `++arr[i]` のサポート

3. **impl 内での構造体メンバー操作**
   - `self.member++` のサポート

4. **より高度な機能**
   - 構造体ポインタメンバー
   - アロー演算子

## 📊 予想される影響範囲

**修正が必要なファイル**:
- `src/frontend/recursive_parser/recursive_parser.cpp` (parseUnary 関数)

**影響を受ける可能性のあるファイル**:
- なし (expression_evaluator.cpp は既に対応済み)

**テストが必要なケース**:
- `++var;` (通常の変数)
- `++obj.member;` (構造体メンバー)
- `++arr[i];` (配列要素) - 将来対応
- 式の中での使用: `int x = ++y;` (既に動作しているはず)

---

**準備完了**: 次のコマンドで実装を開始できます。
