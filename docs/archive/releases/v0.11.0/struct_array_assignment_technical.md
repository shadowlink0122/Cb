# 構造体配列への代入バグ - 技術詳細

**日付**: 2025年10月27日  
**発見**: Week 3 Day 1 TaskQueue実装中  
**重要度**: 🔴 Critical

---

## 問題の核心

### 症状

```cb
struct Task {
    int task_id;
    int priority;
};

void main() {
    Task[10] tasks;
    Task t = {42, 5};
    
    tasks[0] = t;  // 代入操作
    
    println(tasks[0].task_id);  // 期待値: 42, 実際: 0
}
```

**結果**: 代入が無視され、配列要素はゼロ初期化されたまま

---

## インタプリタの処理フロー

### 現在の実装（問題あり）

#### 1. 配列要素への代入検出

ファイル: `src/backend/interpreter/executors/assignments/simple_assignment.cpp`

```cpp
// Line 315-370付近
} else if (ret.is_struct) {
    // 構造体変数または構造体戻り値を配列要素に代入
    std::string element_name =
        interpreter.extract_array_element_name(node->left.get());
    
    // ❌ 問題: 新しい変数を作成してしまう
    interpreter.current_scope().variables[element_name] = ret.struct_value;
    
    Variable &assigned_var =
        interpreter.current_scope().variables[element_name];
    
    // 個別メンバー変数も更新
    for (const auto &member : assigned_var.struct_members) {
        std::string member_path = element_name + "." + member.first;
        Variable *member_var = interpreter.find_variable(member_path);
        if (member_var) {
            member_var->value = member.second.value;
            // ...
        }
    }
    
    return;
}
```

**問題点**:
1. `extract_array_element_name()`は`"tasks[0]"`を返すが、これは配列名ではない
2. `interpreter.current_scope().variables["tasks[0]"]`は**新しい変数を作成**
3. 元の`tasks`配列の内部データ構造は**更新されない**

---

## 配列の内部構造

### Variable構造体（推定）

```cpp
struct Variable {
    TypeInfo type;
    int64_t value;
    std::string str_value;
    bool is_array;
    std::vector<int64_t> array_values;  // プリミティブ型配列
    
    // 構造体配列の場合
    std::vector<Variable> struct_array;  // 構造体要素の配列
    
    std::map<std::string, Variable> struct_members;
    std::string struct_type_name;
    // ...
};
```

### 構造体配列の初期化時

```cb
Task[10] tasks;
```

この時、インタプリタは：
- `tasks`という`Variable`を作成
- `tasks.is_array = true`
- `tasks.struct_array.resize(10)`（各要素はゼロ初期化）

### 配列要素への代入時（現状）

```cb
tasks[0] = t;
```

現在の処理：
1. ✅ `extract_array_element_name()`で`"tasks[0]"`を取得
2. ❌ `variables["tasks[0]"]`に新しい変数を作成
3. ❌ `tasks.struct_array[0]`は更新されない

---

## 正しい実装

### 必要な処理フロー

```cpp
// 配列要素への構造体代入を検出
if (ret.is_struct && is_array_element_assignment(node->left.get())) {
    // 1. 配列名とインデックスを抽出
    std::string array_name = extract_array_name(node->left.get());  // "tasks"
    int64_t index = evaluate_array_index(node->left.get());         // 0
    
    // 2. 配列変数を取得
    Variable *array_var = interpreter.find_variable(array_name);
    if (!array_var || !array_var->is_array) {
        throw std::runtime_error("Not an array: " + array_name);
    }
    
    // 3. 範囲チェック
    if (index < 0 || index >= array_var->struct_array.size()) {
        throw std::runtime_error("Array index out of bounds");
    }
    
    // 4. 配列要素を更新
    array_var->struct_array[index] = ret.struct_value;
    
    // 5. メンバー変数パスも更新
    update_member_variables(array_name, index, ret.struct_value);
    
    return;
}
```

---

## 実装すべき関数

### 1. `extract_array_name()`

```cpp
std::string Interpreter::extract_array_name(ASTNode *array_ref) {
    // AST_ARRAY_REF -> left が配列名
    if (array_ref->node_type == ASTNodeType::AST_ARRAY_REF && 
        array_ref->left) {
        if (array_ref->left->node_type == ASTNodeType::AST_VARIABLE ||
            array_ref->left->node_type == ASTNodeType::AST_IDENTIFIER) {
            return array_ref->left->name;
        }
    }
    throw std::runtime_error("Cannot extract array name");
}
```

### 2. `evaluate_array_index()`

```cpp
int64_t Interpreter::evaluate_array_index(ASTNode *array_ref) {
    // AST_ARRAY_REF -> right がインデックス式
    if (array_ref->node_type == ASTNodeType::AST_ARRAY_REF && 
        array_ref->right) {
        return evaluate(array_ref->right.get());
    }
    throw std::runtime_error("Cannot evaluate array index");
}
```

### 3. `assign_struct_to_array_element()`

```cpp
void Interpreter::assign_struct_to_array_element(
    const std::string &array_name,
    int64_t index,
    const Variable &struct_value) {
    
    Variable *array_var = find_variable(array_name);
    if (!array_var) {
        throw std::runtime_error("Array not found: " + array_name);
    }
    
    if (!array_var->is_array) {
        throw std::runtime_error("Not an array: " + array_name);
    }
    
    // 構造体配列のサイズチェック
    if (array_var->struct_array.empty()) {
        throw std::runtime_error("Struct array not initialized");
    }
    
    if (index < 0 || index >= static_cast<int64_t>(array_var->struct_array.size())) {
        throw std::runtime_error("Array index out of bounds");
    }
    
    // 配列要素を更新
    array_var->struct_array[index] = struct_value;
    
    // メンバー変数パスも更新
    std::string element_path = array_name + "[" + std::to_string(index) + "]";
    for (const auto &member : struct_value.struct_members) {
        std::string member_path = element_path + "." + member.first;
        Variable *member_var = find_variable(member_path);
        if (member_var) {
            *member_var = member.second;
        }
    }
}
```

---

## 修正箇所

### ファイル: `simple_assignment.cpp`

#### Before (Line 315-370)

```cpp
} else if (ret.is_struct) {
    std::string element_name =
        interpreter.extract_array_element_name(node->left.get());
    
    interpreter.current_scope().variables[element_name] = ret.struct_value;
    // ...
}
```

#### After

```cpp
} else if (ret.is_struct) {
    // 配列要素への構造体代入
    std::string array_name = interpreter.extract_array_name(node->left.get());
    int64_t index = interpreter.evaluate_array_index(node->left.get());
    
    interpreter.assign_struct_to_array_element(
        array_name, index, ret.struct_value);
    
    return;
}
```

---

## 同様の問題が存在する箇所

### 1. 構造体リテラルの代入 (Line 220-230)

```cpp
// 配列要素への構造体リテラル代入 (team[0] = {})
std::string element_name =
    interpreter.extract_array_element_name(node->left.get());
interpreter.assign_struct_literal(element_name, node->right.get());
```

**問題**: `assign_struct_literal()`も同じ問題を抱えている可能性

### 2. 関数戻り値の代入 (Line 240-280)

```cpp
if (ret.is_struct) {
    std::string element_name =
        interpreter.extract_array_element_name(node->left.get());
    interpreter.current_scope().variables[element_name] = ret.struct_value;
    // ...
}
```

**問題**: 同じパターン

---

## テストケース

### Test 1: 基本的な代入

```cb
struct Task {
    int task_id;
    int priority;
};

void main() {
    Task[5] tasks;
    Task t = {42, 5};
    
    tasks[0] = t;
    
    assert(tasks[0].task_id == 42);
    assert(tasks[0].priority == 5);
}
```

### Test 2: リテラル代入

```cb
void main() {
    Task[5] tasks;
    
    tasks[0] = {42, 5};
    
    assert(tasks[0].task_id == 42);
}
```

### Test 3: ループ内での代入

```cb
void main() {
    Task[10] tasks;
    
    for (int i = 0; i < 10; i++) {
        Task t = {i, i * 2};
        tasks[i] = t;
    }
    
    assert(tasks[5].task_id == 5);
    assert(tasks[5].priority == 10);
}
```

### Test 4: 配列要素間のコピー

```cb
void main() {
    Task[5] tasks;
    
    tasks[0] = {1, 10};
    tasks[1] = tasks[0];
    
    assert(tasks[1].task_id == 1);
    assert(tasks[1].priority == 10);
}
```

---

## 影響範囲

### 現在動作しないコード

1. 構造体配列への直接代入
2. 構造体配列のソート
3. 構造体配列の初期化
4. Week 3 TaskQueueの理想的な実装

### 回避策の問題

- 並列配列パターン（コード量2倍）
- フィールドごとの代入（冗長）
- 構造体の意味的一貫性の喪失

---

## 優先度

🔴 **Critical** - 基本的な言語機能として必須

---

## 実装計画

### Phase 1: 基本機能実装（1-2日）

1. `extract_array_name()`実装
2. `evaluate_array_index()`実装
3. `assign_struct_to_array_element()`実装
4. `simple_assignment.cpp`の修正

### Phase 2: テスト（半日）

1. 基本テストケース追加
2. エッジケース確認
3. 既存テストの確認

### Phase 3: TaskQueue移行（半日）

1. `task_queue_ideal.cb`を正式版に
2. 並列配列版を削除
3. ドキュメント更新

---

## まとめ

**根本原因**: 配列要素への構造体代入時、新しい変数を作成してしまい、元の配列を更新していない

**解決策**: 配列名とインデックスを正しく抽出し、`struct_array[index]`を直接更新する

**次のステップ**: インタプリタのC++コードを修正し、Phase 1への移行を可能にする
