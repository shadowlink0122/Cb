# Phase 5実装ガイド: メソッド実装の移行

## 📋 概要

このドキュメントは、RecursiveParserから各パーサークラスへの実装移行（Phase 5）の詳細な手順を提供します。

**目標**:
- recursive_parser.cppを5606行から3000行以下に削減（-46%）
- 各パーサーファイルを1000行以下に保つ
- 全2380テストの100%合格を維持
- パフォーマンスの維持または改善（804ms以下）

---

## 🎯 移行の優先順位

### 移行順序（推奨）

1. **ExpressionParser** - 最優先
   - 理由: 最も多くのメソッド（19個）、独立性が高い
   - 期待削減: 約800-1000行

2. **StatementParser** - 優先度: 高
   - 理由: ExpressionParserに依存、制御構造が明確
   - 期待削減: 約500-700行

3. **DeclarationParser** - 優先度: 中
   - 理由: TypeParserとStructParserに依存
   - 期待削減: 約600-800行

4. **TypeParser** - 優先度: 中
   - 理由: 他のパーサーから参照される
   - 期待削減: 約300-400行

5. **StructParser** - 優先度: 低
   - 理由: TypeParserに依存、最も複雑
   - 期待削減: 約500-600行

---

## 🔧 移行の基本手順

### ステップ1: メソッドの選定

1. recursive_parser.cppから移行対象メソッドを特定
2. メソッドの依存関係を確認
3. 必要なヘルパーメソッドをリストアップ

### ステップ2: 実装のコピー

```cpp
// 元（recursive_parser.cpp）
ASTNode* RecursiveParser::parseExpression() {
    ASTNode* left = parseAssignment();
    
    while (check(TOKEN_COMMA)) {
        advance();
        ASTNode* right = parseAssignment();
        // ...
    }
    
    return left;
}
```

### ステップ3: 内部状態アクセスの変換

```cpp
// 移行先（expression_parser.cpp）
ASTNode* ExpressionParser::parseExpression() {
    ASTNode* left = parseAssignment();
    
    while (parser_->check(TOKEN_COMMA)) {     // check() → parser_->check()
        parser_->advance();                    // advance() → parser_->advance()
        ASTNode* right = parseAssignment();
        // ...
    }
    
    return left;
}
```

#### 変換ルール

| 元のコード | 変換後のコード | 説明 |
|-----------|---------------|------|
| `current_token_` | `parser_->current_token_` | 現在のトークン |
| `advance()` | `parser_->advance()` | トークン進行 |
| `check(type)` | `parser_->check(type)` | トークンチェック |
| `match(type)` | `parser_->match(type)` | トークンマッチ |
| `consume(...)` | `parser_->consume(...)` | トークン消費 |
| `error(msg)` | `parser_->error(msg)` | エラー報告 |
| `peek()` | `parser_->peek()` | 次のトークン |
| `isAtEnd()` | `parser_->isAtEnd()` | 終端チェック |
| `setLocation(...)` | `parser_->setLocation(...)` | 位置情報設定 |

### ステップ4: RecursiveParserの更新

```cpp
// recursive_parser.cpp
ASTNode* RecursiveParser::parseExpression() {
    return expression_parser_->parseExpression();  // 委譲に変更
}
```

既存の実装は削除し、委譲呼び出しのみに変更します。

### ステップ5: ビルドとテスト

```bash
# ビルド
make clean
make -j4

# テスト実行
make integration-test
```

**成功基準**:
- コンパイルエラーなし
- 全2380テスト合格（100%）
- パフォーマンス804ms以下

### ステップ6: コミット

```bash
git add -A
git commit -m "Phase 5-1: ExpressionParserの実装移行

- parseExpression, parseAssignment等の実装を移行
- recursive_parser.cppから約XXX行削減
- 全2380テスト合格維持"
```

---

## 📝 ExpressionParser移行の詳細

### 移行対象メソッド（19個）

#### Level 1: 基本メソッド（最優先）
1. `parseExpression()` - エントリーポイント
2. `parseAssignment()` - 代入演算子
3. `parsePrimary()` - プライマリ式

#### Level 2: 二項演算子（優先度順）
4. `parseTernary()` - 三項演算子
5. `parseLogicalOr()` - 論理OR
6. `parseLogicalAnd()` - 論理AND
7. `parseBitwiseOr()` - ビットOR
8. `parseBitwiseXor()` - ビットXOR
9. `parseBitwiseAnd()` - ビットAND
10. `parseComparison()` - 比較演算子
11. `parseShift()` - シフト演算子
12. `parseAdditive()` - 加減算
13. `parseMultiplicative()` - 乗除算

#### Level 3: 単項・後置演算子
14. `parseUnary()` - 単項演算子
15. `parsePostfix()` - 後置演算子

#### Level 4: メンバーアクセス
16. `parseMemberAccess()` - ドット演算子
17. `parseArrowAccess()` - アロー演算子

#### Level 5: リテラル
18. `parseStructLiteral()` - 構造体リテラル
19. `parseArrayLiteral()` - 配列リテラル

### 移行例: parseExpression()

#### 元の実装（recursive_parser.cpp）
```cpp
ASTNode* RecursiveParser::parseExpression() {
    ASTNode* left = parseAssignment();
    
    while (check(TOKEN_COMMA)) {
        advance();
        ASTNode* right = parseAssignment();
        
        ASTNode* comma_expr = new ASTNode();
        comma_expr->type = NODE_COMMA_EXPRESSION;
        comma_expr->children.push_back(left);
        comma_expr->children.push_back(right);
        setLocation(comma_expr, current_token_);
        
        left = comma_expr;
    }
    
    return left;
}
```

#### 移行後の実装（expression_parser.cpp）
```cpp
ASTNode* ExpressionParser::parseExpression() {
    ASTNode* left = parseAssignment();
    
    while (parser_->check(TOKEN_COMMA)) {
        parser_->advance();
        ASTNode* right = parseAssignment();
        
        ASTNode* comma_expr = new ASTNode();
        comma_expr->type = NODE_COMMA_EXPRESSION;
        comma_expr->children.push_back(left);
        comma_expr->children.push_back(right);
        parser_->setLocation(comma_expr, parser_->current_token_);
        
        left = comma_expr;
    }
    
    return left;
}
```

#### RecursiveParserの更新
```cpp
// recursive_parser.cpp
ASTNode* RecursiveParser::parseExpression() {
    return expression_parser_->parseExpression();
}
```

### 必要なヘルパーメソッドの移行

ExpressionParserで使用されるヘルパーメソッドも移行が必要な場合があります：

```cpp
// expression_parser.h
class ExpressionParser {
private:
    RecursiveParser* parser_;
    
    // ヘルパーメソッド（必要に応じて）
    bool isComparisonOperator(TokenType type);
    bool isAdditiveOperator(TokenType type);
    bool isMultiplicativeOperator(TokenType type);
};
```

---

## 📝 StatementParser移行の詳細

### 移行対象メソッド（11個）

#### Level 1: 基本メソッド
1. `parseStatement()` - エントリーポイント
2. `parseCompoundStatement()` - ブロック文

#### Level 2: 制御構造
3. `parseIfStatement()` - if文
4. `parseForStatement()` - for文
5. `parseWhileStatement()` - while文

#### Level 3: ジャンプ文
6. `parseReturnStatement()` - return文
7. `parseBreakStatement()` - break文
8. `parseContinueStatement()` - continue文

#### Level 4: その他
9. `parseAssertStatement()` - assert文
10. `parsePrintlnStatement()` - println文
11. `parsePrintStatement()` - print文

### 移行例: parseIfStatement()

#### 元の実装（recursive_parser.cpp）
```cpp
ASTNode* RecursiveParser::parseIfStatement() {
    consume(TOKEN_IF, "Expected 'if'");
    consume(TOKEN_LPAREN, "Expected '(' after 'if'");
    
    ASTNode* condition = parseExpression();
    
    consume(TOKEN_RPAREN, "Expected ')' after condition");
    
    ASTNode* then_branch = parseStatement();
    ASTNode* else_branch = nullptr;
    
    if (match(TOKEN_ELSE)) {
        else_branch = parseStatement();
    }
    
    ASTNode* if_stmt = new ASTNode();
    if_stmt->type = NODE_IF_STATEMENT;
    if_stmt->children.push_back(condition);
    if_stmt->children.push_back(then_branch);
    if (else_branch) {
        if_stmt->children.push_back(else_branch);
    }
    
    return if_stmt;
}
```

#### 移行後の実装（statement_parser.cpp）
```cpp
ASTNode* StatementParser::parseIfStatement() {
    parser_->consume(TOKEN_IF, "Expected 'if'");
    parser_->consume(TOKEN_LPAREN, "Expected '(' after 'if'");
    
    ASTNode* condition = parser_->expression_parser_->parseExpression();
    
    parser_->consume(TOKEN_RPAREN, "Expected ')' after condition");
    
    ASTNode* then_branch = parseStatement();
    ASTNode* else_branch = nullptr;
    
    if (parser_->match(TOKEN_ELSE)) {
        else_branch = parseStatement();
    }
    
    ASTNode* if_stmt = new ASTNode();
    if_stmt->type = NODE_IF_STATEMENT;
    if_stmt->children.push_back(condition);
    if_stmt->children.push_back(then_branch);
    if (else_branch) {
        if_stmt->children.push_back(else_branch);
    }
    
    return if_stmt;
}
```

**注意点**:
- `parseExpression()`は`parser_->expression_parser_->parseExpression()`に変更
- 他のパーサーへの参照も同様にアクセス

---

## 🎯 チェックリスト

### 移行前の確認

- [ ] git statusが clean（変更がコミット済み）
- [ ] 全テストが合格（2380/2380）
- [ ] パフォーマンス基準を確認（804ms以下）

### 移行中の確認

- [ ] メソッドの実装をコピー
- [ ] 内部状態アクセスを変換（`xxx` → `parser_->xxx`）
- [ ] 他のパーサーへのアクセスを変換（`parseXXX()` → `parser_->xxx_parser_->parseXXX()`）
- [ ] RecursiveParserの実装を委譲に変更
- [ ] コンパイルエラーの解消

### 移行後の確認

- [ ] ビルド成功（警告なし）
- [ ] 全テスト合格（2380/2380）
- [ ] パフォーマンス維持（804ms以下）
- [ ] コミット実行

### Phase 5完了時の確認

- [ ] recursive_parser.cppが3000行以下
- [ ] 各パーサーファイルが1000行以下
- [ ] 全テスト合格（2380/2380）
- [ ] パフォーマンス800ms以下
- [ ] ドキュメント更新（refactoring_progress.md）
- [ ] リリースノート作成（v0.10.0.md）

---

## 🚨 よくある問題と解決策

### 問題1: コンパイルエラー「メソッドが見つからない」

**原因**: 他のパーサーのメソッドを直接呼んでいる

**解決策**:
```cpp
// ❌ 誤り
ASTNode* result = parseExpression();

// ✅ 正しい
ASTNode* result = parser_->expression_parser_->parseExpression();
```

### 問題2: セグメンテーションフォルト

**原因**: パーサーインスタンスが初期化されていない

**解決策**: RecursiveParserのコンストラクタを確認
```cpp
RecursiveParser::RecursiveParser(...) 
    : expression_parser_(std::make_unique<ExpressionParser>(this)),
      statement_parser_(std::make_unique<StatementParser>(this)),
      // ...
```

### 問題3: テスト失敗

**原因**: 内部状態の変換ミス

**解決策**:
1. 変換ルールを再確認
2. デバッガで実行時の値を確認
3. 元の実装と比較

### 問題4: パフォーマンス低下

**原因**: 不要な間接呼び出しが増加

**解決策**:
1. プロファイラで遅い箇所を特定
2. 頻繁に呼ばれるメソッドをインライン化
3. 最適化オプションを確認（-O2, -O3）

---

## 📚 参考ドキュメント

- `docs/refactoring_progress.md` - 進捗状況
- `docs/architecture.md` - アーキテクチャ図
- `release_notes/v0.9.1.md` - v0.9.1リリースノート

---

## 🎯 成功の定義

Phase 5が成功したと判断する基準:

1. **コード量**: recursive_parser.cppが3000行以下
2. **ファイルサイズ**: 各パーサーファイルが1000行以下
3. **テスト**: 全2380テスト合格（100%）
4. **パフォーマンス**: 800ms以下
5. **保守性**: 各ファイルの責任が明確
6. **可読性**: コードの理解が容易

---

**作成日**: 2025年1月  
**バージョン**: v0.9.1  
**対象**: Phase 5実装
