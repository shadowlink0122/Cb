# Week 2 Day 2: Type Cast AST Extension Complete

**Date**: 2025/10/27  
**Commit**: 84ec085  
**Status**: ✅ Phase 1 Complete

## 完了した作業

### ✅ Phase 1: AST Extension

#### 1. ASTNodeType追加
```cpp
enum class ASTNodeType {
    // ... existing types ...
    AST_CAST_EXPR,  // 型キャスト (type)expr  <-- NEW
    // ... rest ...
};
```

#### 2. ASTNodeフィールド追加
```cpp
struct ASTNode {
    // ... existing fields ...
    
    // 型キャスト関連（v0.11.0 Week 2新機能）
    std::string cast_target_type;           // キャスト先の型名
    TypeInfo cast_type_info = TYPE_UNKNOWN; // パース済みの型情報
    std::unique_ptr<ASTNode> cast_expr;     // キャストする式
};
```

#### 3. 設計ドキュメント作成
- `docs/todo/week2_cast_implementation_design.md`
- 5段階の実装計画
- ユースケースの明確化
- テスト戦略の策定

#### 4. テスト基盤構築
- `tests/cases/cast/` ディレクトリ作成
- `test_cast_basic.cb` プレースホルダー

## 設計決定事項

### 構文: C-style Cast
```cb
int* typed_ptr = (int*)void_ptr;
```

**理由**:
- C/C++との互換性
- Cbの既存の構文スタイルと一致
- ユーザーの学習コストが低い

### 対象ユースケース

#### Use Case 1: Vector Data Storage
```cb
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value) {
    int* data = (int*)vec.data;  // Cast void* to int*
    data[vec.length] = value;     // Store actual value
    vec.length = vec.length + 1;
}
```

#### Use Case 2: Memory Allocator Return
```cb
void* generic_ptr = allocator.allocate(100);
int* int_array = (int*)generic_ptr;
int_array[0] = 42;
```

#### Use Case 3: Generic Data Structures
```cb
struct Node {
    void* data;
};

void node_set_int(Node& node, int value) {
    int* typed = (int*)node.data;
    *typed = value;
}
```

## 次の実装ステップ

### ⏭️ Phase 2: Parser Extension

**ファイル**: `src/frontend/recursive_parser/parsers/primary_expression_parser.cpp`

**実装内容**:
```cpp
ASTNode* RecursiveParser::parsePrimaryExpression() {
    // Check for cast: (type)expr
    if (current_token_.type == TOKEN_LPAREN) {
        size_t saved_pos = current_token_index_;
        
        eat(TOKEN_LPAREN);
        
        // Try to parse as type
        if (isType()) {
            std::string type_str = parseType();
            eat(TOKEN_RPAREN);
            
            ASTNode* expr = parsePrimaryExpression();
            
            // Create cast node
            ASTNode* cast_node = new ASTNode(ASTNodeType::AST_CAST_EXPR);
            cast_node->cast_target_type = type_str;
            cast_node->cast_expr = std::unique_ptr<ASTNode>(expr);
            
            return cast_node;
        } else {
            // Not a cast, parse as grouped expression
            current_token_index_ = saved_pos;
            current_token_ = tokens_[current_token_index_];
        }
    }
    
    // ... rest of parsing ...
}
```

**課題**:
1. `isType()` ヘルパー関数の実装
2. キャストとグループ化式の曖昧さ解消
3. ポインタ型のパース対応

### ⏭️ Phase 3: Type Checker

**ファイル**: `src/backend/interpreter/managers/types/`

**実装内容**:
- ポインタ間キャストの許可
- void*からの任意のポインタへのキャスト
- 数値型変換のサポート

### ⏭️ Phase 4: Interpreter

**ファイル**: `src/backend/interpreter/evaluator/`

**実装内容**:
- AST_CAST_EXPRの評価
- ポインタ値のコピー
- 型情報の更新

### ⏭️ Phase 5: Vector Integration

**目標**: 実際のデータ格納

## タイムライン

```
Week 2 Day 2:
├─ Phase 1: AST Extension        (✅ Complete)
├─ Phase 2: Parser (Part 1)      (⚪ Next)
└─ Phase 2: Parser (Part 2)      (⚪ Pending)

Week 2 Day 3:
├─ Phase 3: Type Checker          (⚪ Planned)
├─ Phase 4: Interpreter           (⚪ Planned)
└─ Testing                        (⚪ Planned)

Week 2 Day 4:
└─ Phase 5: Vector Integration    (⚪ Planned)
```

## 技術的詳細

### AST構造

```
AST_CAST_EXPR
├─ cast_target_type: "int*"
├─ cast_type_info: TYPE_INT_PTR
└─ cast_expr:
   └─ AST_VARIABLE "void_ptr"
```

### 型情報の伝播

```cpp
TypedValue evaluateCast(ASTNode* node) {
    TypedValue value = evaluateExpression(node->cast_expr);
    
    TypedValue result;
    result.type = node->cast_type_info;
    result.ptr_value = value.ptr_value;  // Pointer value stays same
    
    return result;
}
```

## 成功基準

Phase 1 (✅ Complete):
- [x] AST_CAST_EXPR追加
- [x] cast_target_type フィールド追加
- [x] cast_type_info フィールド追加
- [x] cast_expr フィールド追加
- [x] 設計ドキュメント作成
- [x] テスト基盤構築
- [x] ビルド成功

Phase 2 (⚪ Next):
- [ ] Parser拡張
- [ ] (type)expr パース成功
- [ ] エラーケースの処理
- [ ] 基本テスト合格

## 学んだこと

### 1. AST設計の重要性
適切なフィールド配置により、後の実装がスムーズに進む。

### 2. 段階的実装
ASTから始めることで、全体像を把握しやすい。

### 3. ドキュメント駆動開発
設計ドキュメントを先に書くことで、実装の方向性が明確になる。

## 次のセッション

**目標**: Parser拡張でキャスト式をパース可能にする

**作業内容**:
1. `isType()` ヘルパー関数実装
2. `parsePrimaryExpression()` でキャスト検出
3. 型名のパース
4. テストケース作成
5. パーサーテスト

**予想される課題**:
- 曖昧性の解消 `(x)` vs `(int*)x`
- 複雑なポインタ型 `int**`, `MyStruct*`
- エラーメッセージの改善

---

**Status**: ✅ Phase 1 Complete, Ready for Phase 2  
**Commit**: 84ec085  
**Next**: Parser extension for cast expressions
