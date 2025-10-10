# ネスト構造体リテラル初期化の改善 - 実装レポート

**日付**: 2025年10月10日  
**バージョン**: v0.9.2  
**ステータス**: 部分的に実装

## 概要

ネストした構造体リテラル初期化における`struct_members`マップの展開を改善しました。

## 実装内容

### 1. 再帰的構造体メンバー同期

**ファイル**: `src/backend/interpreter/managers/structs/assignment.cpp`

新しいメソッド`sync_nested_struct_members_recursive`を追加：

```cpp
void StructAssignmentManager::sync_nested_struct_members_recursive(
    const std::string &base_path,
    const std::map<std::string, Variable> &members)
```

**機能**:
- 構造体メンバーを再帰的に展開
- ネストした構造体の`struct_members`マップを正しく同期
- ダイレクトアクセス変数を作成・更新

**影響範囲**:
- `assign_struct_member_struct`メソッドで使用
- 構造体メンバーへの構造体代入時に呼び出される

### 2. テストケース追加

**ファイル**: `tests/cases/c_compatibility/arrow_deref_dot.cb`

- `(*ptr->member).field` パターンのテスト
- 4つのテストケース
- 全て合格

## 改善された動作

### ✅ 動作するようになったパターン

#### 構造体代入後のアクセス
```cb
Middle m2;
m2 = m1;  // m1.val.x が正しくコピーされる
println(m2.val.x);  // ✅ 正しい値
```

#### `(*p->ptr).member` パターン
```cb
println((*head->data).x);  // ✅ 動作
println((*head->next->data).x);  // ✅ 動作
```

## 既知の制限事項

### ❌ まだ動作しないパターン

#### 1. 宣言時の構造体メンバーアクセス初期化
```cb
Middle mid = o1.val;  // struct_membersがコピーされない
println(mid.val.x);   // ❌ 0 が返される
```

**根本原因**: 変数宣言時の初期化で、`evaluate_typed`が構造体を返す際に`struct_members`が展開されていない

**回避策**: 宣言と代入を分ける
```cb
Middle mid;
mid = o1.val;  // ✅ これは動作する
println(mid.val.x);  // ✅ 正しい値
```

#### 2. 深いネスト構造体リテラル初期化
```cb
L1 lv1 = {val: {val: {val: {val: {x: 11, name: "lv1_val"}, ...}, ...}, ...}, ...};
println(lv1.val.val.val.val.x);  // ❌ 0 が返される
```

**根本原因**: 構造体リテラル初期化時に、ネストレベル2以降の`struct_members`が展開されていない

**影響**: Level 5+テストの多数のパターン（`.val.val`を含むもの）

## テスト結果

### 新規テスト
- ✅ `tests/cases/c_compatibility/arrow_deref_dot.cb`: 4/4 合格

### 既存テスト
- ✅ 全2,425テスト: 合格
- ✅ 統合テスト 30: 合格

### Level 5+テスト
- 総パターン数: 58
- 合格: 42パターン (72%)
- 失敗: 16パターン
  - 3パターン: アクセスエラー（構造体リテラル初期化問題）
  - 13パターン: `= 0`（構造体リテラル初期化問題）

## 次のステップ

### 優先度: 高

**タスク**: 変数宣言時の構造体初期化を修正

**ファイル**: `src/backend/interpreter/executors/declarations/variable_declaration.cpp`

**必要な変更**:
1. `evaluate_typed`が構造体を返す場合の処理を追加
2. 構造体メンバーアクセスからの初期化で`struct_members`をコピー
3. 再帰的な展開を実装

**推定工数**: 2-3時間

### 優先度: 中

**タスク**: 構造体リテラル初期化の深いネスト対応

**ファイル**: `src/backend/interpreter/managers/structs/assignment.cpp`  
**メソッド**: `assign_struct_literal`

**必要な変更**:
1. ネストした構造体リテラルの再帰的処理
2. 各レベルで`struct_members`を正しく展開
3. `sync_nested_struct_members_recursive`を活用

**推定工数**: 3-4時間

## 技術的詳細

### sync_nested_struct_members_recursive の動作

```
base_path: "obj.member"
members: {
  "field1": Variable{...},
  "field2": Variable{type=STRUCT, struct_members={...}}
}

処理:
1. "obj.member.field1" を作成/更新
2. "obj.member.field2" を作成/更新
3. field2が構造体なら再帰:
   - base_path: "obj.member.field2"
   - members: field2.struct_members
   - ... 繰り返し
```

### 変数宣言初期化の問題箇所

`variable_declaration.cpp` 行750-820付近:

```cpp
TypedValue typed_value = interpreter.evaluate_typed(init_node);
// ここで構造体の場合の特別な処理が必要
// しかし現在はassign_variableが呼ばれるのみ
interpreter.assign_variable(node->name, typed_value, node->type_info, false);
```

**必要な追加処理**:
```cpp
if (init_node->node_type == ASTNodeType::AST_MEMBER_ACCESS && 
    typed_value.type == TYPE_STRUCT) {
    // 構造体メンバーアクセスからの初期化
    // source_varを取得してstruct_membersをコピー
    Variable* source_var = resolve_member_access(init_node);
    if (source_var && !source_var->struct_members.empty()) {
        sync_nested_struct_members_recursive(
            node->name, source_var->struct_members);
    }
}
```

## まとめ

### 達成事項
✅ 構造体代入後のネストアクセス: 完全動作  
✅ `(*ptr->member).field` パターン: 完全動作  
✅ 再帰的メンバー同期機構: 実装完了

### 残課題
❌ 宣言時初期化: 未実装（回避策あり）  
❌ 深いネスト構造体リテラル: 未実装

### 推奨事項
1. v0.9.2リリース前に宣言時初期化を修正（優先度: 高）
2. 構造体リテラル初期化はv0.10.0に延期可能（優先度: 中）

---

**実装者**: AI Assistant  
**レビューステータス**: 要確認  
**関連Issue**: #v0.9.2-nested-struct-init
