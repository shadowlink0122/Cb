# Phase 4: execute_statement 完全分割計画

## 目的
ユーザー指示: 「execute_statementは全て分割した方が良いです」

execute_statement (1,428行) を小さなハンドラーに完全分割し、
最終的に ~50-100行のディスパッチャーのみに縮小する。

## 現状分析

### execute_statement の構造 (716-2144行)
```
├── デバッグ情報表示 (~60行)
├── switch (node->node_type) {
│   ├── AST_STMT_LIST (~30行) - 文リスト実行
│   ├── AST_COMPOUND_STMT (~12行) - 複合文実行
│   ├── AST_VAR_DECL (~30行) - 変数宣言 ✅委譲済み(VariableManager)
│   ├── AST_ASSIGN (~15行) - 代入文 ✅委譲済み(StatementExecutor)
│   ├── AST_MULTIPLE_VAR_DECL (~5行) - 複数変数宣言 ✅委譲済み(StatementExecutor)
│   ├── AST_ARRAY_DECL (~5行) - 配列宣言 ✅委譲済み(StatementExecutor)
│   ├── AST_STRUCT_DECL/AST_STRUCT_TYPEDEF_DECL (~30行) - 構造体定義
│   ├── AST_INTERFACE_DECL (~45行) - インターフェース定義
│   ├── AST_IMPL_DECL (~8行) - impl宣言(スキップ)
│   ├── AST_PRINT_STMT (~15行) - print文 ✅委譲済み(OutputManager)
│   ├── AST_PRINTLN_STMT (~14行) - println文 ✅委譲済み(OutputManager)
│   ├── AST_PRINTLN_EMPTY (~3行) - 改行のみ ✅委譲済み(OutputManager)
│   ├── AST_PRINTF_STMT (~3行) - printf文 ✅委譲済み(OutputManager)
│   ├── AST_PRINTLNF_STMT (~3行) - printlnf文 ✅委譲済み(OutputManager)
│   ├── AST_IF_STMT (~3行) - if文 ✅委譲済み(ControlFlowExecutor)
│   ├── AST_WHILE_STMT (~3行) - while文 ✅委譲済み(ControlFlowExecutor)
│   ├── AST_FOR_STMT (~3行) - for文 ✅委譲済み(ControlFlowExecutor)
│   ├── AST_ASSERT_STMT (~27行) - アサーション検証
│   ├── AST_RETURN_STMT (~1,065行) ⚠️ 巨大セクション!
│   ├── AST_BREAK_STMT (~10行) - break文
│   ├── AST_CONTINUE_STMT (~14行) - continue文
│   ├── AST_FUNC_DECL (~5行) - 関数宣言登録
│   └── default (~7行) - 式文として評価
```

## 委譲状況

### ✅ 既に委譲済み
1. **VariableManager**: VAR_DECL (変数宣言)
2. **StatementExecutor**: ASSIGN, MULTIPLE_VAR_DECL, ARRAY_DECL
3. **OutputManager**: PRINT_STMT, PRINTLN_STMT, PRINTLN_EMPTY, PRINTF_STMT, PRINTLNF_STMT
4. **ControlFlowExecutor**: IF_STMT, WHILE_STMT, FOR_STMT
5. **ReturnHandler**: 構造作成済み (未統合)

### ⏳ 分割が必要
1. **AST_STMT_LIST** (~30行) → **StatementListExecutor**
2. **AST_COMPOUND_STMT** (~12行) → **StatementListExecutor** (同じ処理)
3. **AST_STRUCT_DECL/AST_STRUCT_TYPEDEF_DECL** (~30行) → **StructDeclarationHandler**
4. **AST_INTERFACE_DECL** (~45行) → **InterfaceDeclarationHandler**
5. **AST_IMPL_DECL** (~8行) → **ImplDeclarationHandler** (スキップ処理)
6. **AST_ASSERT_STMT** (~27行) → **AssertionHandler**
7. **AST_RETURN_STMT** (~1,065行) → **ReturnHandler** (既に作成済み、統合が必要)
8. **AST_BREAK_STMT** (~10行) → **BreakContinueHandler**
9. **AST_CONTINUE_STMT** (~14行) → **BreakContinueHandler**
10. **AST_FUNC_DECL** (~5行) → **FunctionDeclarationHandler**
11. **default** (~7行) → **ExpressionStatementHandler**

## 実装順序

### Phase 4.3: StatementListExecutor
- **対象**: AST_STMT_LIST, AST_COMPOUND_STMT
- **行数**: ~42行
- **配置**: `executors/statement_list_executor.{h,cpp}`
- **メソッド**:
  - `execute_statement_list(const ASTNode *node)`
  - `execute_compound_statement(const ASTNode *node)`

### Phase 4.4: StructDeclarationHandler
- **対象**: AST_STRUCT_DECL, AST_STRUCT_TYPEDEF_DECL
- **行数**: ~30行
- **配置**: `handlers/struct_declaration_handler.{h,cpp}`
- **メソッド**:
  - `handle_struct_declaration(const ASTNode *node)`

### Phase 4.5: InterfaceDeclarationHandler
- **対象**: AST_INTERFACE_DECL
- **行数**: ~45行
- **配置**: `handlers/interface_declaration_handler.{h,cpp}`
- **メソッド**:
  - `handle_interface_declaration(const ASTNode *node)`

### Phase 4.6: ImplDeclarationHandler
- **対象**: AST_IMPL_DECL
- **行数**: ~8行
- **配置**: `handlers/impl_declaration_handler.{h,cpp}`
- **メソッド**:
  - `handle_impl_declaration(const ASTNode *node)` (スキップ処理のみ)

### Phase 4.7: AssertionHandler
- **対象**: AST_ASSERT_STMT
- **行数**: ~27行
- **配置**: `handlers/assertion_handler.{h,cpp}`
- **メソッド**:
  - `handle_assertion(const ASTNode *node)`

### Phase 4.8: BreakContinueHandler
- **対象**: AST_BREAK_STMT, AST_CONTINUE_STMT
- **行数**: ~24行
- **配置**: `handlers/break_continue_handler.{h,cpp}`
- **メソッド**:
  - `handle_break(const ASTNode *node)`
  - `handle_continue(const ASTNode *node)`

### Phase 4.9: FunctionDeclarationHandler
- **対象**: AST_FUNC_DECL
- **行数**: ~5行
- **配置**: `handlers/function_declaration_handler.{h,cpp}`
- **メソッド**:
  - `handle_function_declaration(const ASTNode *node)`

### Phase 4.10: ExpressionStatementHandler
- **対象**: default (式文)
- **行数**: ~7行
- **配置**: `handlers/expression_statement_handler.{h,cpp}`
- **メソッド**:
  - `handle_expression_statement(const ASTNode *node)`

### Phase 4.11: ReturnHandler Integration
- **対象**: AST_RETURN_STMT (~1,065行)
- **配置**: `handlers/return_handler.{h,cpp}` (既存)
- **作業**: 既存のReturnHandlerを execute_statement に統合

## 最終的な execute_statement

```cpp
void Interpreter::execute_statement(const ASTNode *node) {
    if (!node) return;
    
    // 異常値チェック
    int node_type_int = static_cast<int>(node->node_type);
    if (node_type_int < 0 || node_type_int > 100) {
        if (debug_mode) {
            std::cerr << "[CRITICAL_CORE] Abnormal node_type detected: " 
                      << node_type_int << std::endl;
        }
        return;
    }
    
    debug_msg(DebugMsgId::INTERPRETER_EXEC_STMT, node_type_int);
    
    // ディスパッチのみ (~50行)
    switch (node->node_type) {
    case ASTNodeType::AST_STMT_LIST:
    case ASTNodeType::AST_COMPOUND_STMT:
        statement_list_executor_->execute(node);
        break;
        
    case ASTNodeType::AST_VAR_DECL:
        variable_manager_->process_var_decl_or_assign(node);
        break;
        
    case ASTNodeType::AST_ASSIGN:
    case ASTNodeType::AST_MULTIPLE_VAR_DECL:
    case ASTNodeType::AST_ARRAY_DECL:
        statement_executor_->execute(node);
        break;
        
    case ASTNodeType::AST_STRUCT_DECL:
    case ASTNodeType::AST_STRUCT_TYPEDEF_DECL:
        struct_declaration_handler_->handle(node);
        break;
        
    case ASTNodeType::AST_INTERFACE_DECL:
        interface_declaration_handler_->handle(node);
        break;
        
    case ASTNodeType::AST_IMPL_DECL:
        impl_declaration_handler_->handle(node);
        break;
        
    case ASTNodeType::AST_PRINT_STMT:
    case ASTNodeType::AST_PRINTLN_STMT:
    case ASTNodeType::AST_PRINTLN_EMPTY:
    case ASTNodeType::AST_PRINTF_STMT:
    case ASTNodeType::AST_PRINTLNF_STMT:
        output_manager_->handle(node);
        break;
        
    case ASTNodeType::AST_IF_STMT:
    case ASTNodeType::AST_WHILE_STMT:
    case ASTNodeType::AST_FOR_STMT:
        control_flow_executor_->execute(node);
        break;
        
    case ASTNodeType::AST_ASSERT_STMT:
        assertion_handler_->handle(node);
        break;
        
    case ASTNodeType::AST_RETURN_STMT:
        return_handler_->handle(node);
        break;
        
    case ASTNodeType::AST_BREAK_STMT:
    case ASTNodeType::AST_CONTINUE_STMT:
        break_continue_handler_->handle(node);
        break;
        
    case ASTNodeType::AST_FUNC_DECL:
        function_declaration_handler_->handle(node);
        break;
        
    default:
        expression_statement_handler_->handle(node);
        break;
    }
}
```

## 期待される削減量

| Phase | 抽出対象 | 削減行数 | 累積削減 |
|-------|---------|---------|---------|
| 4.3 | StatementListExecutor | ~42行 | ~42行 |
| 4.4 | StructDeclarationHandler | ~30行 | ~72行 |
| 4.5 | InterfaceDeclarationHandler | ~45行 | ~117行 |
| 4.6 | ImplDeclarationHandler | ~8行 | ~125行 |
| 4.7 | AssertionHandler | ~27行 | ~152行 |
| 4.8 | BreakContinueHandler | ~24行 | ~176行 |
| 4.9 | FunctionDeclarationHandler | ~5行 | ~181行 |
| 4.10 | ExpressionStatementHandler | ~7行 | ~188行 |
| 4.11 | ReturnHandler統合 | ~1,065行 | ~1,253行 |

**最終的な execute_statement**: ~100-150行 (ディスパッチのみ)
**interpreter.cpp 削減**: ~1,253行 → **4,605行** (6,868行から 2,263行削減、33.0%)

## 成功基準
1. ✅ 全2,410テストがパス
2. ✅ execute_statement が 150行以下
3. ✅ 各ハンドラーが単一責任原則に従う
4. ✅ ビルド警告なし
5. ✅ デバッグメッセージの一貫性維持

## 次のアクション
Phase 4.3 の StatementListExecutor から開始します。
