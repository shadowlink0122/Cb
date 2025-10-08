# 構造体の前方宣言（Forward Declaration）

実装日: 2025年10月6日  
バージョン: v0.10.0

## 概要

構造体の前方宣言機能を実装しました。これにより、相互参照する構造体（A→B、B→A）を定義できるようになりました。

## 構文

### 前方宣言

```cb
struct StructName;  // 前方宣言（定義は後で行う）
```

### 相互参照の例

```cb
// 前方宣言
struct NodeB;

// 構造体A: Bへのポインタを持つ
struct NodeA {
    int value;
    NodeB* ref_b;  // ✅ OK: ポインタ型
};

// 構造体B: Aへのポインタを持つ
struct NodeB {
    int data;
    NodeA* ref_a;  // ✅ OK: Aは既に定義済み
};
```

## 使用例

### 基本的な相互参照

```cb
struct B;  // 前方宣言

struct A {
    int value;
    B* ref_to_b;
};

struct B {
    int data;
    A* ref_to_a;
};

void main() {
    A my_a;
    B my_b;
    
    my_a.value = 10;
    my_b.data = 20;
    
    my_a.ref_to_b = &my_b;
    my_b.ref_to_a = &my_a;
    
    // ポインタ経由でアクセス
    int b_data = (*my_a.ref_to_b).data;  // 20
    int a_value = (*my_b.ref_to_a).value;  // 10
}
```

### 複雑な相互参照

```cb
struct GraphNode;
struct TreeNode;

struct GraphEdge {
    int weight;
    GraphNode* target;
};

struct GraphNode {
    int id;
    TreeNode* tree_ref;
};

struct TreeNode {
    int value;
    TreeNode* left;
    TreeNode* right;
    GraphNode* graph_ref;
};
```

## 制限事項

### ❌ 循環参照（値型メンバー）

構造体が値型メンバーで循環参照を形成する場合はエラーになります。

```cb
struct B;  // 前方宣言

struct A {
    int value;
    B member_b;  // 値型メンバー
};

struct B {
    int data;
    A member_a;  // ❌ エラー: 循環参照（A → B → A）
};
```

**エラーメッセージ**:
```
Error: Circular reference detected in struct value members: B -> A -> B. 
       Use pointers to break the cycle.
```

### ✅ ポインタで循環参照を解決

```cb
struct B;  // 前方宣言

struct A {
    int value;
    B* ptr_b;  // ✅ OK: ポインタ型で循環を断つ
};

struct B {
    int data;
    A* ptr_a;  // ✅ OK: ポインタ型
};
```

### ✅ 値型メンバーとして使用可能（循環がない場合）

前方宣言された構造体でも、循環参照がなければ値型メンバーとして使用できます。

```cb
struct B;  // 前方宣言

struct A {
    int value;
};

struct B {
    int data;
    A member_a;  // ✅ OK: Aは完全定義済みで循環なし
};
```

## 実装の詳細

### 前方宣言の登録

- `struct Name;` の構文を認識
- `StructDefinition::is_forward_declaration` フラグを設定
- `struct_definitions_` に登録

### 完全な定義での上書き

```cpp
// 前方宣言
struct_def.is_forward_declaration = true;
struct_definitions_[struct_name] = struct_def;

// 完全な定義
struct_def.is_forward_declaration = false;
struct_definitions_[struct_name] = struct_def;  // 上書き
```

### 型チェック

構造体メンバーの型解析時に前方宣言チェックを実施：

```cpp
if (member_type_info == TYPE_STRUCT && !var_parsed.is_pointer) {
    if (member_struct_def.is_forward_declaration) {
        error("Cannot use forward-declared struct...");
    }
}
```

## 利点

### 1. 循環参照の解決

```cb
// 前: 循環参照不可
struct A {
    B member;  // ❌ Bが未定義
};
struct B {
    A member;  // Aは定義済みだがサイズ未確定
};

// 後: ポインタで循環参照可能
struct B;  // 前方宣言
struct A {
    B* ptr;  // ✅ OK
};
struct B {
    A* ptr;  // ✅ OK
};
```

### 2. 複雑なデータ構造の実装

- グラフ構造（相互参照ノード）
- 二分木とグラフの相互リンク
- イベントシステム（送信者↔受信者）

### 3. コードの柔軟性向上

構造体の定義順序に依存しないコードが書けるようになります。

## テストケース

### test_forward_declaration_ok.cb

```cb
struct NodeB;

struct NodeA {
    int value;
    NodeB* ref_b;
};

struct NodeB {
    int data;
    NodeA* ref_a;
};
```

**期待結果**: コンパイル成功、実行成功

### test_forward_declaration_error.cb

```cb
struct B;

struct A {
    int value;
    B member_b;  // 値型メンバー
};

struct B {
    int data;
    A member_a;  // ❌ 循環参照
};
```

**期待結果**: コンパイルエラー（循環参照検出）

### test_forward_value_member_ok.cb

```cb
struct B;

struct A {
    int value;
};

struct B {
    int data;
    A member_a;  // ✅ OK: 循環なし
};
```

**期待結果**: コンパイル成功、実行成功

## まとめ

| 機能 | 状態 | 説明 |
|------|------|------|
| ✅ 前方宣言の構文 | 実装済み | `struct Name;` |
| ✅ ポインタ型での使用 | サポート | `Name* ptr;` |
| ✅ 値型での使用 | サポート | 循環参照がなければ可能 |
| ✅ 相互参照 | サポート | A↔B の循環参照（ポインタ） |
| ✅ 循環参照検出 | 実装済み | 値型での循環はエラー検出 |

---

**実装者**: GitHub Copilot  
**レビュー状態**: テスト完了・動作確認済み  
**実装日**: 2025年10月6日
