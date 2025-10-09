# Cb言語パーサーアーキテクチャ（v0.9.1）

## 📐 全体アーキテクチャ

```
┌────────────────────────────────────────────────────────────────┐
│                        RecursiveParser                          │
│                     (メインパーサークラス)                        │
│                                                                 │
│  役割: パーサー全体の調整、トークン管理、状態管理                  │
│  サイズ: 5606行（Phase 5完了後は約3000行に削減予定）              │
│                                                                 │
│  主要メンバー:                                                   │
│  - RecursiveLexer lexer_;          // 字句解析                  │
│  - Token current_token_;            // 現在のトークン             │
│  - typedef_map_;                    // typedef管理              │
│  - struct_definitions_;             // 構造体定義               │
│  - enum_definitions_;               // enum定義                │
│  - interface_definitions_;          // interface定義            │
│  - unique_ptr<T> parsers_[5];       // 分離パーサーインスタンス    │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │         friend 宣言により内部状態へのアクセスを許可           │  │
│  └─────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────┘
                              │
                              │ 委譲
                              ▼
        ┌──────────────────────────────────────────┐
        │                                          │
        ▼                                          ▼
┌────────────────┐                       ┌─────────────────┐
│ ExpressionParser│                       │ StatementParser │
│  (式解析)       │                       │  (文解析)        │
├────────────────┤                       ├─────────────────┤
│ 19メソッド      │                       │ 11メソッド       │
├────────────────┤                       ├─────────────────┤
│ • parseExpression       ┌───────────────────────────┐    │
│ • parseAssignment       │   DeclarationParser       │    │
│ • parseTernary          │    (宣言解析)              │    │
│ • parseLogicalOr        ├───────────────────────────┤    │
│ • parseLogicalAnd       │ 6メソッド                  │    │
│ • parseBitwiseOr        ├───────────────────────────┤    │
│ • parseBitwiseXor       │ • parseVariableDecl       │    │
│ • parseBitwiseAnd       │ • parseTypedefVariableDecl│    │
│ • parseComparison       │ • parseFunctionDecl       │    │
│ • parseShift            │ • parseFuncDeclAfterName  │    │
│ • parseAdditive         │ • parseTypedefDecl        │    │
│ • parseMultiplicative   │ • parseFuncPointerTypedef │    │
│ • parseUnary            └───────────────────────────┘    │
│ • parsePostfix                    │                      │
│ • parsePrimary                    │                      │
│ • parseMemberAccess               ▼                      │
│ • parseArrowAccess      ┌───────────────────────────┐    │
│ • parseStructLiteral    │     TypeParser            │    │
│ • parseArrayLiteral     │      (型解析)              │    │
└────────────────┘        ├───────────────────────────┤    │
                          │ 7メソッド                  │    │
                          ├───────────────────────────┤    │
                          │ • parseType               │    │
                          │ • resolveParsedTypeInfo   │    │
                          │ • resolveArrayType        │    │
                          │ • getPointerLevel         │    │
                          │ • isValidType             │    │
                          │ • isStructType            │    │
                          │ • isEnumType              │    │
                          └───────────────────────────┘    │
                                      │                    │
                                      ▼                    │
                          ┌───────────────────────────┐    │
                          │     StructParser          │    │
                          │    (構造体解析)            │    │
                          ├───────────────────────────┤    │
                          │ 10メソッド                 │    │
                          ├───────────────────────────┤    │
│ • parseStatement        │ • parseStructDecl         │    │
│ • parseCompoundStatement│ • parseStructTypedefDecl  │    │
│ • parseIfStatement      │ • parseForwardDecl        │    │
│ • parseForStatement     │ • parseUnionDecl          │    │
│ • parseWhileStatement   │ • parseUnionTypedefDecl   │    │
│ • parseReturn           │ • parseEnumDecl           │    │
│ • parseBreak            │ • parseEnumTypedefDecl    │    │
│ • parseContinue         │ • parseStructMembers      │    │
│ • parseAssert           │ • parseUnionMembers       │    │
│ • parsePrintln          │ • detectCircularRef       │    │
│ • parsePrint            └───────────────────────────┘    │
└─────────────────┘                                        │
                                                           │
                                                           │
                                                           ▼
```

## 🔄 Phase別の進化

### Phase 0: Baseline (v0.9.0)
```
┌────────────────────────────────┐
│      RecursiveParser           │
│         5589行                  │
│                                │
│  全てのパーサー機能が           │
│  単一ファイルに実装             │
└────────────────────────────────┘
```

### Phase 1: パーサー分離の基盤構築 ✅
```
┌────────────────────────────────┐
│      RecursiveParser           │
│         5589行                  │
│                                │
│  + unique_ptr<T> parsers_[5]   │
│  + friend 宣言                  │
└────────────────────────────────┘
         │
         ├─→ ExpressionParser (ヘッダーのみ)
         ├─→ StatementParser (ヘッダーのみ)
         ├─→ DeclarationParser (ヘッダーのみ)
         ├─→ TypeParser (ヘッダーのみ)
         └─→ StructParser (ヘッダーのみ)
```

### Phase 2: 委譲パターンの実装 ✅
```
┌────────────────────────────────┐
│      RecursiveParser           │
│         5606行                  │
│                                │
│  実装は全てここに残る            │
└────────────────────────────────┘
         ▲ 委譲呼び出し
         │
         ├─→ ExpressionParser (委譲のみ)
         ├─→ StatementParser (委譲のみ)
         ├─→ DeclarationParser (委譲のみ)
         ├─→ TypeParser (委譲のみ)
         └─→ StructParser (委譲のみ)
```

### Phase 3: ドキュメント化とパフォーマンス改善 ✅
```
┌────────────────────────────────┐
│      RecursiveParser           │
│         5606行                  │
│                                │
│  実装は全てここに残る            │
└────────────────────────────────┘
         ▲ 委譲呼び出し
         │
         ├─→ ExpressionParser (委譲 + 300行コメント)
         ├─→ StatementParser (委譲 + 150行コメント)
         ├─→ DeclarationParser (委譲 + 120行コメント)
         ├─→ TypeParser (委譲 + 200行コメント)
         └─→ StructParser (委譲 + 180行コメント)

【パフォーマンス】: 804ms（Baselineより3.1%高速化）
```

### Phase 5: メソッド実装の移行（予定）
```
┌────────────────────────────────┐
│      RecursiveParser           │
│         ~3000行                 │
│                                │
│  トークン管理、状態管理のみ      │
└────────────────────────────────┘
         ▲ 内部状態アクセス
         │
         ├─→ ExpressionParser (~1000行実装)
         ├─→ StatementParser (~700行実装)
         ├─→ DeclarationParser (~800行実装)
         ├─→ TypeParser (~400行実装)
         └─→ StructParser (~600行実装)

【合計】: ~6500行（-80行、構造化により重複削減）
```

## 📊 メソッド分布（Phase 3完了時点）

### ExpressionParser（19メソッド）
- **役割**: 式の解析、演算子の優先順位処理
- **演算子優先順位**: 14レベル
  1. 代入演算子（Level 1）
  2. 三項演算子（Level 2）
  3. 論理OR（Level 3）
  4. 論理AND（Level 4）
  5. ビットOR（Level 5）
  6. ビットXOR（Level 6）
  7. ビットAND（Level 7）
  8. 比較演算子（Level 8）
  9. シフト演算子（Level 9）
  10. 加減算（Level 10）
  11. 乗除算（Level 11）
  12. 単項演算子（Level 12）
  13. 後置演算子（Level 13）
  14. プライマリ式（Level 14）

### StatementParser（11メソッド）
- **役割**: 文の解析、制御構造
- **カテゴリ**:
  - 制御構文: if, for, while（3メソッド）
  - ジャンプ文: return, break, continue（3メソッド）
  - 出力: println, print（2メソッド）
  - その他: statement, compoundStatement, assert（3メソッド）

### DeclarationParser（6メソッド）
- **役割**: 宣言の解析
- **カテゴリ**:
  - 変数宣言: variable, typedefVariable（2メソッド）
  - 関数宣言: function, functionAfterName（2メソッド）
  - Typedef: typedef, functionPointerTypedef（2メソッド）

### TypeParser（7メソッド）
- **役割**: 型の解析と検証
- **カテゴリ**:
  - 解析: parseType（1メソッド）
  - 解決: resolveParsedTypeInfo, resolveArrayType（2メソッド）
  - 検証: getPointerLevel, isValidType, isStructType, isEnumType（4メソッド）

### StructParser（10メソッド）
- **役割**: 構造体とenum、unionの解析
- **カテゴリ**:
  - 構造体: structDecl, structTypedefDecl, forwardDecl（3メソッド）
  - Union: unionDecl, unionTypedefDecl（2メソッド）
  - Enum: enumDecl, enumTypedefDecl（2メソッド）
  - メンバー: structMembers, unionMembers（2メソッド）
  - 検証: detectCircularReference（1メソッド）

## 🔧 技術的な設計パターン

### 1. friend宣言による内部状態アクセス
```cpp
class RecursiveParser {
    friend class ExpressionParser;
    friend class StatementParser;
    friend class DeclarationParser;
    friend class TypeParser;
    friend class StructParser;
    
private:
    Token current_token_;         // 分離パーサーからアクセス可能
    RecursiveLexer lexer_;
    // ... 他のprivateメンバー
};
```

**利点**:
- 段階的リファクタリングが可能
- 既存実装を保持しながら構造改善
- Phase 5で実装移行時に徐々にfriend宣言を削除可能

### 2. unique_ptrによる自動メモリ管理
```cpp
class RecursiveParser {
private:
    std::unique_ptr<ExpressionParser> expression_parser_;
    std::unique_ptr<StatementParser> statement_parser_;
    // ...
};
```

**利点**:
- 自動的なメモリ解放
- 所有権の明確化
- コピー禁止による安全性

### 3. 委譲パターン（Phase 2-3）
```cpp
// ExpressionParser
ASTNode* ExpressionParser::parseExpression() {
    return parser_->parseExpression();  // RecursiveParserに委譲
}
```

**利点**:
- 既存実装を完全保持
- 段階的な移行が可能
- テストの100%合格を保証

### 4. 実装パターン（Phase 5予定）
```cpp
// ExpressionParser（Phase 5以降）
ASTNode* ExpressionParser::parseExpression() {
    // RecursiveParserから移行された実装
    ASTNode* left = parseAssignment();
    
    while (parser_->check(TOKEN_COMMA)) {
        parser_->advance();
        // 内部状態アクセスは parser_-> 経由
        ASTNode* right = parseAssignment();
        // ...
    }
    
    return left;
}
```

**変換ルール**:
- `current_token_` → `parser_->current_token_`
- `advance()` → `parser_->advance()`
- `check()` → `parser_->check()`
- `match()` → `parser_->match()`
- `error()` → `parser_->error()`

## 📈 パフォーマンスの推移

```
900ms ┤
      │
850ms ┤         ●Phase2 (863ms)
      │        /
800ms ┤       /    ●Phase3 (804ms) ← 22%改善！
      │●Baseline (830ms)    \
750ms ┤      \               \
      │       \               ●Phase1 (833ms)
700ms ┤        \______________
      │
      └─────────────────────────────────→
       Phase0  Phase1  Phase2  Phase3
```

**改善要因**:
1. Phase 1-2: 構造化によるわずかな増加（+30ms）
2. Phase 3: ドキュメント化によるコード整理（-59ms、22%改善）
3. 最終結果: Baselineより3.1%高速化

## 🎯 Phase 5の目標

### コード量の削減目標

| ファイル | 現在 | Phase 5後 | 削減率 |
|---------|------|-----------|--------|
| recursive_parser.cpp | 5606行 | ~3000行 | -46% |
| expression_parser.cpp | 398行 | ~1000行 | +151% |
| statement_parser.cpp | 211行 | ~700行 | +232% |
| declaration_parser.cpp | 162行 | ~800行 | +394% |
| type_parser.cpp | 252行 | ~400行 | +59% |
| struct_parser.cpp | 234行 | ~600行 | +156% |
| **合計** | **6863行** | **~6500行** | **-5%** |

**期待効果**:
- recursive_parser.cppが約2600行削減
- 各パーサーファイルが1000行以下
- 重複コードの削減により全体で約350行削減

### アーキテクチャの最終形

```
┌──────────────────────────────────────────────────────┐
│            RecursiveParser (3000行)                   │
│            - トークン管理                              │
│            - 状態管理（typedef, struct等）             │
│            - ヘルパーメソッド                          │
└──────────────────────────────────────────────────────┘
                      ▲ 内部状態アクセス
                      │
    ┌─────────────────┼─────────────────┐
    │                 │                 │
    ▼                 ▼                 ▼
┌─────────┐     ┌─────────┐     ┌─────────┐
│ Expr    │     │ Stmt    │     │ Decl    │
│ Parser  │     │ Parser  │     │ Parser  │
│ 1000行  │     │ 700行   │     │ 800行   │
└─────────┘     └─────────┘     └─────────┘
                      │
              ┌───────┴───────┐
              ▼               ▼
         ┌─────────┐     ┌─────────┐
         │ Type    │     │ Struct  │
         │ Parser  │     │ Parser  │
         │ 400行   │     │ 600行   │
         └─────────┘     └─────────┘
```

**設計原則**:
1. **単一責任の原則**: 各パーサーは明確な責任を持つ
2. **1000行ルール**: 各ファイルは1000行以下を維持
3. **テスト駆動**: 各移行後に2380テスト全合格を確認
4. **パフォーマンス維持**: 804ms以下を維持

---

**作成日**: 2025年1月  
**バージョン**: v0.9.1  
**ステータス**: Phase 3完了、Phase 4進行中
