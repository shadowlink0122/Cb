# 自己再帰構造体チェック機能実装レポート

実装日: 2025年10月5日

## 📋 概要

構造体が自己再帰メンバー（自分自身の型のメンバー）を持つ場合、そのメンバーはポインタ型でなければならないという制約をパーサーレベルでチェックする機能を実装しました。

## 🎯 実装目的

自己再帰構造体は理論的に値メンバーとして定義することは不可能です（無限サイズになるため）。しかし、ポインタ経由であれば完全に動作します。この実装により：

1. **値メンバーとしての自己再帰をコンパイル時にエラー検出**
2. **明確なエラーメッセージで開発者をガイド**
3. **ポインタ経由の正しい実装を促進**

## 🔧 実装内容

### 修正ファイル

**`src/frontend/recursive_parser/recursive_parser.cpp`**

2箇所に自己再帰チェックロジックを追加：

#### 1. 通常のstruct定義（`parseStructDeclaration()`）

```cpp
// 自己再帰構造体チェック: 自分自身の型のメンバーはポインタでなければならない
std::string member_base_type = var_parsed.base_type;
if (member_base_type.empty()) {
    member_base_type = var_parsed.full_type;
}
// "struct " プレフィックスを除去
if (member_base_type.rfind("struct ", 0) == 0) {
    member_base_type = member_base_type.substr(7);
}

if (member_base_type == struct_name && !var_parsed.is_pointer) {
    error("Self-recursive struct member '" + member_name + 
          "' must be a pointer type. Use '" + struct_name + "* " + 
          member_name + ";' instead of '" + struct_name + " " + 
          member_name + ";'");
    return nullptr;
}
```

#### 2. typedef struct定義（`parseStructTypedefDeclaration()`）

```cpp
// 自己再帰構造体チェック (typedef structの場合はタグ名でチェック)
if (!tag_name.empty()) {
    std::string member_base_type = var_parsed.base_type;
    if (member_base_type.empty()) {
        member_base_type = var_parsed.full_type;
    }
    // "struct " プレフィックスを除去
    if (member_base_type.rfind("struct ", 0) == 0) {
        member_base_type = member_base_type.substr(7);
    }
    
    if (member_base_type == tag_name && !var_parsed.is_pointer) {
        error("Self-recursive struct member '" + member_name + 
              "' must be a pointer type. Use '" + tag_name + "* " + 
              member_name + ";' instead of '" + tag_name + " " + 
              member_name + ";'");
        return nullptr;
    }
}
```

### チェックロジックの詳細

1. **メンバー型の抽出**: `base_type`または`full_type`から型名を取得
2. **プレフィックス除去**: `"struct "` プレフィックスを除去して型名を正規化
3. **自己参照チェック**: メンバー型が構造体自身の名前と一致するか確認
4. **ポインタチェック**: ポインタ型（`is_pointer`）でない場合はエラー
5. **明確なエラーメッセージ**: 正しい書き方を提示

## 📝 テストケース

### 作成したテストファイル

#### 1. `tests/cases/struct/self_recursive_ok.cb` ✅

**正しい自己再帰構造体の使用例**

```cb
struct Node {
    int value;
    Node* next;  // ✅ OK: ポインタなら自己参照可能
};

int main() {
    Node n1, n2, n3;
    n1.value = 1; n1.next = 0;
    n2.value = 2; n2.next = &n1;
    n3.value = 3; n3.next = &n2;
    
    // リスト走査
    Node* current = &n3;
    int sum = 0;
    while (current != 0) {
        println("Node value: ", current->value);
        sum = sum + current->value;
        current = current->next;
    }
    
    println("Sum: ", sum);
    assert(sum == 6);
    return 0;
}
```

**実行結果**:
```
Node value:  3
Node value:  2
Node value:  1
Sum:  6
```

#### 2. `tests/cases/struct/self_recursive_error.cb` ❌

**エラーケース: 値メンバーとして自己参照**

```cb
struct Node {
    int value;
    Node child;  // ❌ エラー: 値メンバーとして自己参照は不可
}
```

**エラーメッセージ**:
```
Location: tests/cases/struct/self_recursive_error.cb:5:15
Error: Self-recursive struct member 'child' must be a pointer type. 
       Use 'Node* child;' instead of 'Node child;'
```

#### 3. `tests/cases/struct/typedef_self_recursive_ok.cb` ✅

**typedef structでの正しい二分木実装**

```cb
typedef struct TreeNode {
    int data;
    TreeNode* left;   // ✅ OK: ポインタなら自己参照可能
    TreeNode* right;  // ✅ OK
} TreeNode;

int main() {
    TreeNode root, left_child, right_child;
    root.data = 10;
    left_child.data = 5;
    right_child.data = 15;
    
    root.left = &left_child;
    root.right = &right_child;
    
    println("Root: ", root.data);
    println("Left: ", root.left->data);
    println("Right: ", root.right->data);
    
    assert(root.data == 10);
    assert(root.left->data == 5);
    assert(root.right->data == 15);
    
    println("Binary tree test passed");
    return 0;
}
```

**実行結果**:
```
Root:  10
Left:  5
Right:  15
Binary tree test passed
```

#### 4. `tests/cases/struct/typedef_self_recursive_error.cb` ❌

**typedef structでのエラーケース**

```cb
typedef struct TreeNode {
    int data;
    TreeNode left;   // ❌ エラー: 値メンバーとして自己参照は不可
    TreeNode right;  // ❌ エラー
} TreeNode;
```

**エラーメッセージ**:
```
Location: tests/cases/struct/typedef_self_recursive_error.cb:5:18
Error: Self-recursive struct member 'left' must be a pointer type. 
       Use 'TreeNode* left;' instead of 'TreeNode left;'
```

### 統合テストへの追加

**`tests/integration/struct/struct_tests.hpp`**

4つのテスト関数を追加：

```cpp
inline void test_self_recursive_ok()
inline void test_self_recursive_error()
inline void test_typedef_self_recursive_ok()
inline void test_typedef_self_recursive_error()
```

テストスイートに組み込み：
```cpp
test_self_recursive_ok(); // 自己再帰構造体（OK）
test_self_recursive_error(); // 自己再帰構造体（エラー）
test_typedef_self_recursive_ok(); // typedef自己再帰（OK）
test_typedef_self_recursive_error(); // typedef自己再帰（エラー）
```

## ✅ テスト結果

### 実行結果

```
[integration] Running test_self_recursive_ok...
[integration] Running test_self_recursive_error...
[integration] Running test_typedef_self_recursive_ok...
[integration] Running test_typedef_self_recursive_error...

✅ PASS: Struct Tests (149 tests)

Total Integration Tests: 2229 (+16 from 2213)
Unit Tests: 50
All tests passed!
```

### テストカバレッジ

| カテゴリ | テストケース | 結果 |
|---------|------------|------|
| 通常struct自己再帰（OK） | 単方向リスト実装 | ✅ PASS |
| 通常struct自己再帰（Error） | 値メンバーエラー検出 | ✅ PASS |
| typedef struct自己再帰（OK） | 二分木実装 | ✅ PASS |
| typedef struct自己再帰（Error） | 値メンバーエラー検出 | ✅ PASS |

すべてのテストが成功しました。

## 🎯 実装の効果

### 1. 実行時エラーから**コンパイル時エラー**へ

**Before（実装前）**:
- パーサーを通過してしまう
- 実行時に不正なメモリアクセス
- デバッグが困難

**After（実装後）**:
- パース時に即座にエラー検出
- 明確なエラーメッセージ
- 正しい修正方法を提示

### 2. 開発者体験の向上

**エラーメッセージの質**:
```
Error: Self-recursive struct member 'child' must be a pointer type. 
       Use 'Node* child;' instead of 'Node child;'
```

- ✅ どこが問題か明確
- ✅ なぜ問題か説明
- ✅ どう修正すべきか提示

### 3. データ構造実装の安全性向上

以下のような一般的なデータ構造を安全に実装可能：

- **単方向リスト**: `Node* next`
- **双方向リスト**: `Node* prev; Node* next;`
- **二分木**: `TreeNode* left; TreeNode* right;`
- **N分木**: `Node* children[N];` または `Node** children;`
- **グラフ**: `Node* neighbors[MAX_NEIGHBORS];`

## 📊 実装統計

| 項目 | 値 |
|------|-----|
| 修正ファイル数 | 1 |
| 追加コード行数 | 約40行 |
| 新規テストファイル数 | 4 |
| 追加統合テスト関数数 | 4 |
| 総テスト数増加 | +16 (2213 → 2229) |
| テスト成功率 | 100% (2229/2229) |

## 🔍 技術的詳細

### 型名の正規化

構造体型は複数の形式で表現されるため、正規化が必要：

```cpp
// Case 1: "Node" (型名のみ)
// Case 2: "struct Node" (struct付き)
// Case 3: base_typeとfull_typeの使い分け

// 正規化処理
std::string member_base_type = var_parsed.base_type;
if (member_base_type.empty()) {
    member_base_type = var_parsed.full_type;
}
if (member_base_type.rfind("struct ", 0) == 0) {
    member_base_type = member_base_type.substr(7);
}
```

### typedef structの特殊ケース

typedef structは2つの名前を持つ：
1. **タグ名** (struct TreeNode の TreeNode)
2. **エイリアス名** (typedef ... TreeNode の TreeNode)

実装では**タグ名**でチェック：

```cpp
if (!tag_name.empty()) {
    // タグ名でチェック
    if (member_base_type == tag_name && !var_parsed.is_pointer) {
        // エラー
    }
}
```

### ポインタ判定

`ParsedTypeInfo::is_pointer` フラグを使用：

```cpp
if (member_base_type == struct_name && !var_parsed.is_pointer) {
    // エラー: ポインタでない自己再帰
}
```

## 🚀 今後の拡張可能性

### 1. 相互再帰構造体のチェック

現在は自己再帰のみチェック。将来的には相互再帰も検出可能：

```cb
struct A {
    B b;  // ❌ 将来的にエラー検出
};

struct B {
    A a;  // ❌ 将来的にエラー検出
};
```

### 2. より詳細なエラーメッセージ

```
Error: Circular struct dependency detected:
  struct A contains B (line 5)
  struct B contains A (line 10)
Suggestion: Use pointers to break the cycle
```

### 3. スマートポインタ対応

将来的にスマートポインタが実装された場合：

```cb
struct Node {
    int value;
    unique_ptr<Node> next;  // ✅ OK: スマートポインタも許可
};
```

## 📚 関連ドキュメント

- **実装ドキュメント**: `docs/UNIMPLEMENTED_FEATURES.md`
  - セクション 1.2: 自己再帰構造体の説明を更新済み
  - ポインタ経由で動作することを明記

- **仕様書**: `docs/spec.md`
  - Phase 3: ポインタシステムの実装状況

## 🎉 まとめ

### 達成項目

✅ 自己再帰構造体の値メンバーをコンパイル時にエラー検出  
✅ 明確なエラーメッセージと修正方法の提示  
✅ 通常structとtypedef structの両方に対応  
✅ 包括的なテストケース作成（OK/Error各2件）  
✅ 統合テスト追加（+16テスト）  
✅ すべてのテスト成功（2229/2229）  

### 実装の意義

この実装により、Cb言語は以下を実現：

1. **型安全性の向上**: 理論的に不可能な構造を事前検出
2. **開発者体験の向上**: 明確なエラーメッセージ
3. **データ構造実装の促進**: リスト・木・グラフ等を安全に実装可能
4. **言語仕様の完成度向上**: 自己再帰構造体の扱いを明確化

---

**実装者**: GitHub Copilot  
**レビュー状態**: テスト完了・動作確認済み  
**実装日**: 2025年10月5日
