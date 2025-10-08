# インタプリタリファクタリング - 実用的アプローチ

**作成日**: 2025年10月7日（最終改訂）  
**方針**: 完全なリファクタリングではなく、**最大の問題点を解決する実用的なアプローチ**

---

## 現実的な評価

### 問題の本質

1. **巨大メソッド**:
   - `evaluate_expression` (3,933行) - expression_evaluator.cpp
   - `execute_statement` (1,215行) - interpreter.cpp
   - `assign_struct_literal` (655行) - interpreter.cpp
   - `create_struct_variable` (412行) - interpreter.cpp

2. **リファクタリングのリスク**:
   - 全テスト(2,380個)が通ることを保証しながら進める必要がある
   - interpreter.cppとexpression_evaluator.cppは相互に密結合
   - 大規模な変更は予期しないバグを生む可能性が高い

### パーサーリファクタリングとの違い

パーサーは成功した（parseStatement 1,452行→64行）が、以下の理由で容易だった：
- パーサーは**ステートレス**（変数状態を持たない）
- 各ヘルパーメソッドは**独立している**
- テストが**文法解析のみ**に焦点を当てている

インタプリタは異なる：
- **ステートフル**（変数、スコープ、型情報など多数の状態を持つ）
- メソッド間の**相互依存が強い**
- テストが**実行結果**に焦点を当てている

---

## 実用的な改善案

### Phase 1: ドキュメント化と可読性の向上（最優先）

**目標**: コードを変更せずに、理解しやすくする

#### 1.1 巨大メソッドへのコメント追加

**対象**: `evaluate_expression` (3,933行)

**実装**:
```cpp
int64_t ExpressionEvaluator::evaluate_expression(const ASTNode* node) {
    // ==========================================
    // このメソッドは3,933行の巨大switch文です
    // 
    // 主なセクション:
    // - Line 100-500: 算術演算 (+, -, *, /, %)
    // - Line 500-1000: 比較演算 (<, >, ==, !=等)
    // - Line 1000-1500: 配列アクセス
    // - Line 1500-2500: 構造体アクセス
    // - Line 2500-3000: 関数呼び出し
    // - Line 3000-3500: ポインタ操作
    // - Line 3500-3933: その他
    // 
    // TODO: このメソッドは将来的に分割すべき
    // ==========================================
    
    switch (node->node_type) {
        // ========== 算術演算 ==========
        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        // ...
        
        // ========== 配列アクセス ==========
        case AST_ARRAY_REF:
        // ...
    }
}
```

**工数**: 半日  
**リスク**: なし  
**効果**: 中（将来のメンテナンスが容易に）

#### 1.2 セクション分割マーカーの追加

各巨大メソッドに`#pragma mark`や大きなコメントブロックでセクションを明示

**工数**: 半日  
**リスク**: なし  
**効果**: 中

### Phase 2: 軽微なヘルパーメソッド抽出（低リスク）

**目標**: 明らかに独立している処理のみを抽出

#### 2.1 ユーティリティ関数の抽出

**対象**: 以下のような独立した処理
- 型変換ロジック
- 文字列処理
- エラーメッセージ生成

**例**:
```cpp
// before (interpreter.cpp内)
void Interpreter::some_method() {
    // ... 型変換処理（30行）
}

// after
std::string TypeConversionUtils::convert_type(...) {
    // 型変換処理
}
```

**工数**: 1日  
**リスク**: 低  
**効果**: 中

### Phase 3: 既存のManagerクラスの活用（中リスク）

**目標**: 既に存在するManagerクラスにより多くの責務を移譲

#### 3.1 StatementExecutorの拡充

**現状**: StatementExecutorは一部のstatementのみ処理  
**改善**: より多くのstatementをStatementExecutorに移譲

**実装**:
```cpp
// interpreter.cpp
void Interpreter::execute_statement(const ASTNode *node) {
    // 既にStatementExecutorが処理できるものは委譲
    if (statement_executor_->can_handle(node->node_type)) {
        statement_executor_->execute(node);
        return;
    }
    
    // まだ移行できていないものは従来通り処理
    switch (node->node_type) {
        case AST_IF_STMT:
            // 複雑な処理...
            break;
        // ...
    }
}
```

**工数**: 2日  
**リスク**: 中  
**効果**: 高

#### 3.2 ExpressionEvaluatorの部分的分割

**対象**: 最も独立している演算（算術演算、比較演算）のみ

**実装**:
```cpp
// expression_evaluator.cpp
int64_t ExpressionEvaluator::evaluate_expression(const ASTNode* node) {
    switch (node->node_type) {
        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV:
        case AST_MOD:
            return evaluate_arithmetic_operation(node);  // 新しいヘルパー
        
        // 残りは従来通り
        case AST_ARRAY_REF:
            // 複雑な処理...
            break;
    }
}

// 新しいヘルパーメソッド（同じファイル内）
int64_t ExpressionEvaluator::evaluate_arithmetic_operation(const ASTNode* node) {
    // 算術演算のみ（200-300行程度）
}
```

**工数**: 2日  
**リスク**: 中  
**効果**: 中

### Phase 4: テストカバレッジの向上（保険）

**目標**: リファクタリング時のバグ検出能力を高める

#### 4.1 単体テストの追加

**対象**: 
- ExpressionEvaluatorの各演算タイプ
- StatementExecutorの各statement

**工数**: 2日  
**リスク**: なし  
**効果**: 高（将来のリファクタリングが安全に）

---

## 推奨される実行順序

### Tier 1: 即座に実行可能（リスクなし）

1. ✅ Phase 1.1: 巨大メソッドへのコメント追加（半日）
2. ✅ Phase 1.2: セクション分割マーカーの追加（半日）

**合計**: 1日  
**効果**: コードの理解が容易に、将来の改善の基礎

### Tier 2: 短期的改善（低〜中リスク）

3. ✅ Phase 2.1: ユーティリティ関数の抽出（1日）
4. ✅ Phase 3.2: ExpressionEvaluatorの部分的分割（2日）

**合計**: 3日  
**効果**: ファイルサイズの小幅削減、可読性向上

### Tier 3: 中長期的改善（中リスク、要テスト）

5. ✅ Phase 3.1: StatementExecutorの拡充（2日）
6. ✅ Phase 4.1: テストカバレッジの向上（2日）

**合計**: 4日  
**効果**: より大規模なリファクタリングの準備

---

## 現実的な成功基準

### 短期目標（Tier 1完了時）
- ✅ 全巨大メソッドに明確なセクションコメント
- ✅ 各セクションの行範囲が明示
- ✅ TODOコメントで将来の改善点を記録
- ✅ 全テスト合格（2,380個）

### 中期目標（Tier 2完了時）
- ✅ `evaluate_expression`: 3,933行 → 3,500行（約400行削減）
- ✅ 明確なヘルパーメソッド: 5-10個追加
- ✅ 全テスト合格

### 長期目標（Tier 3完了時）
- ✅ `execute_statement`: 1,215行 → 800行（約400行削減）
- ✅ StatementExecutorが全statementの50%以上を処理
- ✅ 単体テスト: +50個追加
- ✅ 全テスト合格

---

## なぜこのアプローチが現実的か

1. **段階的な改善**: 一度に全てを変更しない
2. **低リスク優先**: リスクの低い改善から始める
3. **テストの重視**: 各段階で全テスト通過を確認
4. **ドキュメント重視**: コードを変更せずに理解を深める
5. **実用的な目標**: 完璧を目指さず、十分に良い状態を目指す

---

## 次のアクション

### 即座に開始できる作業（Tier 1）

1. `expression_evaluator.cpp` にセクションコメントを追加
2. `interpreter.cpp` の `execute_statement` にセクションコメントを追加
3. 他の巨大メソッドにも同様のコメントを追加

**推定時間**: 1日  
**承認**: 即座に開始可能（コード変更なし）

---

**作成者**: GitHub Copilot  
**ステータス**: Tier 1 実行準備完了  
**推奨**: まずTier 1を完了してから、Tier 2/3を検討
