# インタプリタリファクタリング - 実装戦略（改訂版）

**作成日**: 2025年10月7日（改訂）  
**重要な発見**: 巨大メソッドの特定

---

## 1. 巨大メソッドの実態

### 1.1 interpreter.cpp の巨大メソッド

| メソッド | 行数 | 問題の深刻度 |
|---------|------|------------|
| **`execute_statement`** | **1,215行** | 🔴 最重要 |
| **`assign_struct_literal`** | 655行 | 🔴 重要 |
| **`create_struct_variable`** | 412行 | 🟡 中 |
| **`register_global_declarations`** | 334行 | 🟡 中 |
| **`sync_struct_members_from_direct_access`** | 313行 | 🟡 中 |

### 1.2 expression_evaluator.cpp の巨大メソッド

| メソッド | 行数 | 問題の深刻度 |
|---------|------|------------|
| **`evaluate_expression`** | **3,933行** | 🔴🔴 最重要 |
| **`evaluate_typed_expression_internal`** | **1,181行** | 🔴 重要 |
| **`evaluate_ternary_typed`** | 106行 | ✅ 許容範囲 |

**総計**: たった5個のメソッドで **7,330行**（全体の26.6%）

---

## 2. 改訂された戦略

### パーサーリファクタリングからの教訓

パーサーのリファクタリングで`parseStatement` (1,452行) を64行に削減した成功例を適用：

1. **巨大switch/if-else文を分析**
2. **各caseを独立したヘルパーメソッドに抽出**
3. **ヘルパーメソッドを専門のクラスに移行**

### Phase A: execute_statement の分割（最優先）

**現状**: interpreter.cpp の `execute_statement` (1,215行)

**戦略**:
1. `execute_statement`のswitch文を分析
2. 各statement typeごとにヘルパーメソッド作成:
   - `execute_variable_declaration()`
   - `execute_assignment()`
   - `execute_if_statement()`
   - `execute_while_statement()`
   - `execute_for_statement()`
   - `execute_return_statement()`
   - `execute_function_call()`
   - 等...

3. StatementExecutorクラスに移行（既存）
   - 既に `statement_executor.cpp` (2,722行) が存在
   - `execute_statement`の実装をStatementExecutorに完全移行

**目標**: interpreter.cpp の `execute_statement` を20行以下に削減

### Phase B: evaluate_expression の分割（最優先）

**現状**: expression_evaluator.cpp の `evaluate_expression` (3,933行)

**戦略**:
1. `evaluate_expression`の巨大switch文を分析
2. 演算タイプごとにヘルパーメソッド作成:
   - `evaluate_binary_arithmetic()` - +, -, *, /, %
   - `evaluate_binary_logical()` - &&, ||
   - `evaluate_binary_comparison()` - <, >, ==, !=, etc.
   - `evaluate_array_access()`
   - `evaluate_struct_member_access()`
   - `evaluate_function_call()`
   - `evaluate_pointer_operation()` - *, &
   - 等...

3. 各ヘルパーをサブクラスに分離:
   - `ArithmeticEvaluator`
   - `ArrayAccessEvaluator`
   - `StructAccessEvaluator`
   - `PointerEvaluator`

**目標**: expression_evaluator.cpp の主メソッドを100行以下に削減

### Phase C: その他の巨大メソッド

- `assign_struct_literal` (655行)
- `create_struct_variable` (412行)
- `register_global_declarations` (334行)

これらも同様の手法で分割

---

## 3. 実装手順（Phase A: execute_statement）

### Step 1: execute_statement の構造分析

```bash
# switch文のcase一覧を確認
grep -A2 "case AST" src/backend/interpreter/core/interpreter.cpp | grep "case AST" | head -30
```

### Step 2: StatementExecutor への移行

既存の `statement_executor.cpp` (2,722行) を確認し、
`interpreter.cpp` の `execute_statement` (1,215行) を完全に移行

**before** (interpreter.cpp):
```cpp
void Interpreter::execute_statement(const ASTNode *node) {
    // 1,215行の巨大switch文
    switch (node->node_type) {
        case AST_VAR_DECL: { /* 処理 */ } break;
        case AST_ASSIGN: { /* 処理 */ } break;
        // ... 100+ cases ...
    }
}
```

**after** (interpreter.cpp):
```cpp
void Interpreter::execute_statement(const ASTNode *node) {
    statement_executor_->execute(node);  // 完全に委譲
}
```

**after** (statement_executor.cpp):
```cpp
void StatementExecutor::execute(const ASTNode* node) {
    switch (node->node_type) {
        case AST_VAR_DECL:
            execute_variable_declaration(node);
            break;
        case AST_ASSIGN:
            execute_assignment(node);
            break;
        // ... ヘルパーメソッドに委譲
    }
}

// 各ヘルパーメソッド（100-200行程度）
void StatementExecutor::execute_variable_declaration(const ASTNode* node) {
    // 変数宣言の実装（元のcaseの中身）
}

void StatementExecutor::execute_assignment(const ASTNode* node) {
    // 代入文の実装
}
// ... 他のヘルパー
```

### Step 3: テストと検証

```bash
make clean && make
make test
```

---

## 4. 実装手順（Phase B: evaluate_expression）

### Step 1: evaluate_expression の構造分析

```bash
# switch文のcase一覧を確認
grep -n "case AST" src/backend/interpreter/evaluator/expression_evaluator.cpp | head -50
```

### Step 2: ヘルパーメソッドへの分割

**before**:
```cpp
int64_t ExpressionEvaluator::evaluate_expression(const ASTNode* node) {
    // 3,933行の巨大switch文
    switch (node->node_type) {
        case AST_ADD: { /* 処理 */ } break;
        case AST_SUB: { /* 処理 */ } break;
        // ... 100+ cases ...
    }
}
```

**after**:
```cpp
int64_t ExpressionEvaluator::evaluate_expression(const ASTNode* node) {
    switch (node->node_type) {
        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV:
        case AST_MOD:
            return evaluate_arithmetic(node);
        
        case AST_EQ:
        case AST_NE:
        case AST_LT:
        case AST_LE:
        case AST_GT:
        case AST_GE:
            return evaluate_comparison(node);
        
        case AST_ARRAY_REF:
            return evaluate_array_access(node);
        
        case AST_MEMBER_ACCESS:
        case AST_ARROW_ACCESS:
            return evaluate_struct_access(node);
        
        // ... 各カテゴリに委譲
        
        default:
            throw std::runtime_error("Unknown expression type");
    }
}

// 各ヘルパーメソッド（200-500行程度）
int64_t ExpressionEvaluator::evaluate_arithmetic(const ASTNode* node) {
    // 算術演算の実装
}

int64_t ExpressionEvaluator::evaluate_comparison(const ASTNode* node) {
    // 比較演算の実装
}
// ... 他のヘルパー
```

### Step 3: サブクラスへの分離（Phase B-2）

さらに各ヘルパーが大きい場合、独立したクラスに：

```cpp
// arithmetic_evaluator.h/cpp
class ArithmeticEvaluator {
public:
    ArithmeticEvaluator(Interpreter& interpreter);
    int64_t evaluate(const ASTNode* node);
private:
    Interpreter& interpreter_;
};

// expression_evaluator.cpp
int64_t ExpressionEvaluator::evaluate_arithmetic(const ASTNode* node) {
    return arithmetic_evaluator_->evaluate(node);
}
```

---

## 5. タイムライン

| Phase | 作業内容 | 推定工数 | 優先度 |
|-------|---------|---------|--------|
| **Phase A** | `execute_statement` (1,215行) の分割 | 1日 | 🔴 最優先 |
| **Phase B** | `evaluate_expression` (3,933行) の分割 | 2-3日 | 🔴 最優先 |
| **Phase C** | その他巨大メソッドの分割 | 2日 | 🟡 高 |
| **Phase D** | ファイル全体の再編成 | 2日 | 🟡 中 |

**合計推定工数**: 7-8日

---

## 6. 成功基準

### 短期目標（Phase A, B完了時）
- ✅ `execute_statement`: 1,215行 → 50行以下
- ✅ `evaluate_expression`: 3,933行 → 200行以下
- ✅ 全テスト合格（2,380個）
- ✅ ビルド警告なし

### 長期目標（Phase A-D完了時）
- ✅ 全ファイルが1,500行以下（技術的に困難な場合を除く）
- ✅ 最大メソッドサイズ: 500行以下
- ✅ 平均メソッドサイズ: 100行以下
- ✅ コードの可読性・保守性の向上

---

## 7. 次のアクション

1. ✅ Phase A開始: `execute_statement` の分析
2. `interpreter.cpp` の628-1842行を読み取り、switch文の構造を理解
3. ヘルパーメソッドの抽出計画を立てる
4. StatementExecutorへの移行を実装

---

**作成者**: GitHub Copilot  
**ステータス**: Phase A 開始準備完了
