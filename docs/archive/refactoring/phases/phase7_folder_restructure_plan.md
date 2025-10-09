# Phase 7: src全体のフォルダ・ファイル名再編成計画

## 現状分析

### 問題点
1. **evaluatorディレクトリ**: 31ファイル、8,733行 - フラットすぎる
   - expression_* という命名が多く、階層化が必要
   - function_call関連が分散している
   
2. **executorsディレクトリ**: statement_executor.cpp が3,392行で巨大
   
3. **handlersディレクトリ**: 小さいファイルが多数、整理の余地あり

4. **ファイル命名の冗長性**:
   - `expression_function_call_impl.cpp` → 長すぎる
   - `expression_*` というプレフィックスが多すぎる

## 再編成計画

### A. evaluatorディレクトリの再構成

#### 現状 (フラット)
```
evaluator/
├── expression_address_ops.cpp/h
├── expression_array_access.cpp/h
├── expression_assignment.cpp/h
├── expression_binary_unary_typed.cpp/h
├── expression_dispatcher.cpp/h
├── expression_evaluator.cpp/h (985行)
├── expression_function_call_impl.cpp (2,128行) ★
├── expression_function_call.cpp/h
├── expression_helpers.cpp/h
├── expression_incdec.cpp/h
├── expression_literal_eval.cpp/h
├── expression_member_access_impl.cpp/h
├── expression_member_helpers.cpp/h
├── expression_receiver_resolution.cpp/h
├── expression_special_access.cpp/h
├── expression_ternary.cpp/h
└── function_call_evaluator.h
```

#### 提案 (階層化)
```
evaluator/
├── core/
│   ├── evaluator.cpp/h (expression_evaluator → evaluator)
│   ├── dispatcher.cpp/h (expression_dispatcher → dispatcher)
│   └── helpers.cpp/h (expression_helpers → helpers)
│
├── operators/
│   ├── binary_unary.cpp/h (expression_binary_unary_typed)
│   ├── assignment.cpp/h (expression_assignment)
│   ├── incdec.cpp/h (expression_incdec)
│   └── ternary.cpp/h (expression_ternary)
│
├── access/
│   ├── array.cpp/h (expression_array_access)
│   ├── member.cpp/h (expression_member_access_impl)
│   ├── member_helpers.cpp/h
│   ├── special.cpp/h (expression_special_access)
│   ├── address_ops.cpp/h (expression_address_ops)
│   └── receiver_resolution.cpp/h
│
├── functions/
│   ├── call.cpp/h (expression_function_call)
│   ├── call_impl.cpp/h (expression_function_call_impl) - 要分割
│   └── call_evaluator.h (function_call_evaluator)
│
└── literals/
    └── eval.cpp/h (expression_literal_eval)
```

### B. executorsディレクトリの再構成

#### 現状
```
executors/
├── statement_executor.cpp (3,392行) ★★★
├── statement_executor.h
├── control_flow_executor.cpp
├── control_flow_executor.h
├── statement_list_executor.cpp
└── statement_list_executor.h
```

#### 提案
```
executors/
├── core/
│   ├── executor.cpp/h (statement_executor → executor)
│   └── statement_list.cpp/h
│
├── control_flow/
│   ├── control_flow.cpp/h
│   ├── loops.cpp/h (statement_executorから抽出)
│   └── conditionals.cpp/h (statement_executorから抽出)
│
├── declarations/
│   ├── variables.cpp/h (statement_executorから抽出)
│   └── functions.cpp/h (statement_executorから抽出)
│
└── assignments/
    ├── assignments.cpp/h (statement_executorから抽出)
    └── member_assignments.cpp/h (statement_executorから抽出)
```

### C. handlersディレクトリの再構成

#### 現状 (小さいファイルが分散)
```
handlers/
├── assertion_handler.cpp/h
├── break_continue_handler.cpp/h
├── expression_statement_handler.cpp/h
├── function_declaration_handler.cpp/h
├── impl_declaration_handler.cpp/h
├── interface_declaration_handler.cpp/h
├── return_handler.cpp/h (637行)
└── struct_declaration_handler.cpp/h
```

#### 提案 (統合)
```
handlers/
├── declarations/
│   ├── function.cpp/h (function_declaration_handler)
│   ├── struct.cpp/h (struct_declaration_handler)
│   ├── interface.cpp/h (interface_declaration_handler)
│   └── impl.cpp/h (impl_declaration_handler)
│
├── control/
│   ├── return.cpp/h (return_handler)
│   ├── break_continue.cpp/h (break_continue_handler)
│   └── assertion.cpp/h (assertion_handler)
│
└── statements/
    └── expression.cpp/h (expression_statement_handler)
```

## ファイル命名規則

### 削除するプレフィックス
- `expression_` → 不要（evaluatorディレクトリ内で明確）
- `statement_` → 不要（executorsディレクトリ内で明確）
- `*_handler` → 不要（handlersディレクトリ内で明確）

### 新しい命名規則
- **短く、明確に**: `evaluator/operators/binary_unary.cpp`
- **ディレクトリで分類**: フォルダ名で役割を明示
- **重複排除**: 親ディレクトリ名と重複しない

## 実施順序

### Step 1: evaluator再構成
1. サブディレクトリ作成: core/, operators/, access/, functions/, literals/
2. ファイル移動とリネーム
3. includeパス更新
4. ビルド・テスト

### Step 2: executors再構成 + statement_executor分割
1. statement_executor.cpp分析・分割計画
2. サブディレクトリ作成
3. ファイル分割・移動
4. includeパス更新
5. ビルド・テスト

### Step 3: handlers再構成
1. サブディレクトリ作成
2. ファイル移動とリネーム
3. includeパス更新
4. ビルド・テスト

### Step 4: 他の大きなファイル対応
1. expression_function_call_impl.cpp (2,128行) 分割
2. arrays/manager.cpp (2,107行) 分割検討

## 期待される効果
- ✅ ファイル名の簡潔化
- ✅ 論理的な階層構造
- ✅ 巨大ファイルの分割
- ✅ 関連ファイルのグループ化
- ✅ 可読性・保守性の向上
