# Queue stdlib heap-buffer-overflow修正レポート

## 問題の概要

v0.11.0のQueue stdlib実装で、テスト実行時にheap-buffer-overflowが発生していた。

### 発生していたエラー
```
AddressSanitizer: heap-buffer-overflow on address 0x000105701c9c
WRITE of size 8 at 0x000105701c9c thread T0
simple_assignment.cpp:157
```

## 根本原因の調査

### 調査プロセス

1. **初期症状**: Queue<T>のメソッド呼び出し時にクラッシュ
2. **--debugフラグで調査**: 
   - モジュール重複読み込みを発見
   - impl_definitions転送の不完全さを発見
   - nullptr代入時の型変更を発見
   - void**間接参照の誤処理を発見

### 特定された問題

#### 1. モジュール重複読み込み
**症状**: queue.cbが2回読み込まれ、impl_definitionsが重複登録される
```
[SYNC_IMPL] Total impl definitions: 2  ← 1回目
[SYNC_IMPL] Total impl definitions: 2  ← 2回目（重複）
After transfer, interpreter has 4 impl_nodes  ← 本来は2個
```

**原因**: 
- Parse時: test file parserがmodule parserを作成してqueue.cbを読み込み
- Runtime時: handle_import_statementが再度queue.cbを読み込み

#### 2. impl_definitions不完全転送
**症状**: module parserのimpl_definitionsが転送されずにparserが破棄される
**原因**: handle_import_statementでsync_impl_definitions_from_parser()が呼ばれていなかった

#### 3. nullptr代入時の型変更
**症状**: `self.front = nullptr`を実行すると、型がTYPE_POINTER→TYPE_INTに変更される
```cpp
// 修正前
self_member->value = value;
if (self_member->type != TYPE_STRING) {
    self_member->type = TYPE_INT;  // 強制的にINTに!
}
```

#### 4. void**間接参照の誤処理 (最重要)
**症状**: `*next_ptr = node_mem;` (next_ptrはvoid**型)の実行時にクラッシュ

**原因**:
```cpp
// void** next_ptr = (void**)addr;
// *next_ptr = value;  ← この時

// インタープリターは next_ptr の値(メモリアドレス)を Variable* にキャスト
Variable *var = reinterpret_cast<Variable *>(ptr_value);  // 不正!
var->value = typed_value.as_numeric();  // 8バイトしかない領域に720バイトのVariable構造体としてアクセス
```

**問題点**:
- `ptr_value`は`malloc()`で確保したメモリアドレス(例: 0x107301b94)
- このアドレスには8バイトしか確保されていない
- しかし`Variable*`にキャストして`var->value`にアクセスすると、720バイトの構造体として扱われる
- 結果: heap-buffer-overflow

## 修正内容

### 修正1: モジュール重複読み込み防止

**ファイル**: `src/frontend/main.cpp`

```cpp
// Parse時のimportをマーク
if (root && !root->statements.empty()) {
    for (const auto &stmt : root->statements) {
        if (stmt && stmt->node_type == ASTNodeType::AST_IMPORT_STMT &&
            !stmt->import_path.empty()) {
            interpreter.mark_module_loaded(stmt->import_path);
        }
    }
}
```

**ファイル**: `src/backend/interpreter/core/interpreter.cpp`

```cpp
void Interpreter::handle_import_statement(const ASTNode *node) {
    // 既に読み込み済みかチェック
    if (loaded_modules.find(module_path) != loaded_modules.end()) {
        return;  // 重複を防止
    }
    
    // ... モジュール読み込み処理 ...
    
    loaded_modules.insert(module_path);
}
```

### 修正2: impl_definitions完全転送

**ファイル**: `src/backend/interpreter/core/interpreter.cpp`

```cpp
void Interpreter::handle_import_statement(const ASTNode *node) {
    // ... パース処理 ...
    
    // impl_definitionsを転送（constructors/destructorsも含む）
    sync_impl_definitions_from_parser(&parser);
}
```

### 修正3: nullptr代入時の型保持

**ファイル**: `src/backend/interpreter/executors/statement_executor.cpp`

```cpp
// nullptr の場合、または元の型が TYPE_POINTER の場合は型を保持
bool is_nullptr = (value_node->node_type == ASTNodeType::AST_NULLPTR);

self_member->value = value;
if (self_member->type != TYPE_STRING && 
    !is_nullptr && 
    self_member->type != TYPE_POINTER) {
    self_member->type = TYPE_INT;
}
```

### 修正4: void**間接参照の正しい処理 (最重要)

**ファイル**: `src/backend/interpreter/executors/assignments/simple_assignment.cpp`

```cpp
// 間接参照への代入 (*ptr = value)
if (node->left && node->left->node_type == ASTNodeType::AST_UNARY_OP &&
    node->left->op == "DEREFERENCE") {
    
    int64_t ptr_value = interpreter.evaluate(node->left->left.get());
    
    // 左辺の変数を取得して、その型を確認
    Variable *ptr_var = nullptr;
    if (node->left->left && node->left->left->node_type == ASTNodeType::AST_VARIABLE) {
        ptr_var = interpreter.find_variable(node->left->left->name);
    }

    // void** のようなダブルポインタの場合、生メモリへの書き込みとして処理
    if (ptr_var && ptr_var->pointer_depth >= 2) {
        // 単純なメモリ書き込み (Variable* へのキャストを行わない)
        int64_t *target = reinterpret_cast<int64_t *>(ptr_value);
        *target = typed_value.as_numeric();
    } else {
        // 通常のVariableポインタへの代入
        Variable *var = reinterpret_cast<Variable *>(ptr_value);
        var->value = typed_value.as_numeric();
        var->is_assigned = true;
    }
}
```

## 修正の効果

### テスト結果

```bash
$ ./main-asan tests/cases/stdlib/collections/test_queue_single_enqueue.cb
Testing single enqueue...
✅ Single enqueue successful
Size:  1
```

✅ **成功**: ASanエラーなし、テストパス

### 動作確認済みテスト
- ✅ `test_queue_single_enqueue.cb`: 完全パス
- ✅ `test_queue_import.cb`: 基本動作OK
- ✅ `test_queue_comprehensive.cb`: 基本動作OK

## 技術的洞察

### ポインタの深さによる処理分岐

今回の修正で重要なのは、**ポインタの深さ(pointer_depth)に応じて処理を分岐**することです:

| pointer_depth | 型例 | 処理方法 |
|--------------|------|---------|
| 0 | `int` | 通常の値 |
| 1 | `int*`, `Variable*` | Variable構造体へのポインタとして扱う |
| 2 | `int**`, `void**` | 生メモリアドレスとして扱う |

### なぜVariable*キャストが危険だったか

```
メモリレイアウト:
┌────────────────────────────────┐
│ malloc(8)で確保された領域       │  ← ptr_value = 0x107301b94
│ [   8 bytes   ]                │
└────────────────────────────────┘

Variable構造体 (720 bytes):
┌────────────────────────────────┐
│ TypeInfo type           8 bytes│
│ bool is_const           1 byte │
│ ...                            │
│ int64_t value           8 bytes│  ← var->value にアクセス
│ ...                            │
│ (合計720バイト)                 │
└────────────────────────────────┘
              ↑
    heap-buffer-overflow!
    (8バイトしかないのに720バイト分アクセス)
```

## 学んだ教訓

1. **ポインタの型情報を保持することの重要性**
   - 単なる数値アドレスではなく、何へのポインタかを追跡する必要がある

2. **インタープリター実装におけるポインタ抽象化の難しさ**
   - 高水準言語のポインタを低水準メモリ操作にマッピングする際の複雑性

3. **デバッグツールの価値**
   - AddressSanitizer (ASan) による早期発見
   - `--debug` フラグによる詳細トレース

4. **段階的デバッグの重要性**
   - 複数の問題が重なっていた場合、一つずつ特定・修正することが重要

## 今後の改善案

1. **ポインタメタデータシステムの拡張**
   - pointer_depthだけでなく、指している実体の型情報を保持
   
2. **型安全性の強化**
   - キャスト前の型チェックを厳密化
   
3. **テストカバレッジの向上**
   - ダブルポインタを使用する様々なパターンのテスト追加

## 関連Issue・PR

- 関連: v0.11.0 Queue stdlib実装
- 影響範囲: Generic struct with pointer members
- 修正日: 2025年11月2日

## 参考

- ASan documentation: https://github.com/google/sanitizers
- Cb Language Specification: docs/spec.md
- Queue Implementation: stdlib/collections/queue.cb
