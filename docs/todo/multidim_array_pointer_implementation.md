# 多次元配列へのポインタ実装計画

**作成日**: 2025年10月9日  
**対象機能**: 多次元配列要素へのアドレス取得とポインタ演算  
**優先度**: 🔴 高（v0.10.0 最優先機能）

---

## 📋 現状

### 動作するコード
```cb
int[2][3] matrix = [[1, 2, 3], [4, 5, 6]];
println(matrix[0][0]);  // 1 - 動作OK
println(matrix[1][2]);  // 6 - 動作OK
```

### エラーになるコード
```cb
int[2][3] matrix = [[1, 2, 3], [4, 5, 6]];
int* ptr = &matrix[0][0];  // ❌ エラー: "Multi-dimensional array address-of not yet supported"
```

---

## 🎯 実装目標

### 1. 多次元配列要素へのアドレス取得

```cb
int[2][3] matrix = [[1, 2, 3], [4, 5, 6]];

// 要素へのポインタ
int* ptr = &matrix[0][0];
println(*ptr);  // 1

ptr = &matrix[1][2];
println(*ptr);  // 6
```

### 2. ポインタ演算（フラットメモリレイアウト）

```cb
int[2][3] matrix = [[1, 2, 3], [4, 5, 6]];

// メモリレイアウト: [1, 2, 3, 4, 5, 6] (row-major order)
int* ptr = &matrix[0][0];
println(*ptr);       // 1
println(*(ptr + 1)); // 2
println(*(ptr + 3)); // 4
println(*(ptr + 5)); // 6
```

### 3. 範囲外アクセスの検出（安全性）

```cb
int[2][3] matrix = [[1, 2, 3], [4, 5, 6]];
int* ptr = &matrix[0][0];
ptr = ptr + 10;  // 範囲外
// *ptr はエラーまたは未定義動作
```

---

## 🔍 エラー箇所の特定

### エラーメッセージの検索

```bash
grep -r "Multi-dimensional array address-of not yet supported" src/
```

**予想される箇所**:
- `src/backend/interpreter/evaluators/expression_evaluator.cpp`
- `ADDRESS_OF` オペレータの処理

---

## 🛠️ 実装手順

### Step 1: エラー箇所の特定と理解（30分）

1. エラーメッセージの場所を特定
2. 現在の実装を確認
3. 必要な変更箇所を洗い出し

### Step 2: 多次元配列インデックスのフラット化（1-2時間）

**必要な処理**:
```cpp
// matrix[i][j] → フラットインデックス
// 2D配列の場合: flat_index = i * cols + j
// 3D配列の場合: flat_index = i * (rows * cols) + j * cols + k
```

**実装箇所**:
- `expression_evaluator.cpp` の `ADDRESS_OF` ケース
- 多次元インデックスを再帰的に計算

**疑似コード**:
```cpp
case ADDRESS_OF: {
    if (operand is ARRAY_ACCESS) {
        // 単一次元配列の場合（既存の処理）
        if (is_single_dimension) {
            // 既存のコード
        }
        // 多次元配列の場合（新規実装）
        else if (is_multi_dimension) {
            // 1. 配列の各次元サイズを取得
            vector<int> dimensions = get_array_dimensions(array_var);
            
            // 2. 各インデックスを評価
            vector<int> indices = evaluate_all_indices(array_access_node);
            
            // 3. フラットインデックスを計算
            int flat_index = calculate_flat_index(dimensions, indices);
            
            // 4. ベースアドレス + オフセット
            void* base_addr = get_array_base_address(array_var);
            void* element_addr = base_addr + (flat_index * element_size);
            
            // 5. ポインタ型の TypedValue を返す
            return create_pointer(element_addr, element_type);
        }
    }
}
```

### Step 3: テストケースの作成（30分）

**テストファイル**: `tests/cases/pointer/test_multidim_array_pointer_address.cb`

```cb
// Test 1: 2D配列要素へのポインタ
void test_2d_array_pointer() {
    println("=== Test 1: 2D array element pointer ===");
    int[2][3] matrix = [[1, 2, 3], [4, 5, 6]];
    
    int* ptr1 = &matrix[0][0];
    println(*ptr1);  // 1
    
    int* ptr2 = &matrix[0][1];
    println(*ptr2);  // 2
    
    int* ptr3 = &matrix[1][0];
    println(*ptr3);  // 4
    
    int* ptr4 = &matrix[1][2];
    println(*ptr4);  // 6
}

// Test 2: ポインタ演算
void test_pointer_arithmetic() {
    println("=== Test 2: Pointer arithmetic ===");
    int[2][3] matrix = [[10, 20, 30], [40, 50, 60]];
    
    int* ptr = &matrix[0][0];
    println(*ptr);       // 10
    println(*(ptr + 1)); // 20
    println(*(ptr + 2)); // 30
    println(*(ptr + 3)); // 40
    println(*(ptr + 4)); // 50
    println(*(ptr + 5)); // 60
}

// Test 3: ポインタ経由の書き込み
void test_pointer_write() {
    println("=== Test 3: Pointer write ===");
    int[2][2] arr = [[1, 2], [3, 4]];
    
    int* ptr = &arr[0][0];
    *ptr = 100;
    println(arr[0][0]);  // 100
    
    ptr = &arr[1][1];
    *ptr = 200;
    println(arr[1][1]);  // 200
}

// Test 4: 3D配列へのポインタ
void test_3d_array_pointer() {
    println("=== Test 4: 3D array pointer ===");
    int[2][2][2] cube = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]];
    
    int* ptr1 = &cube[0][0][0];
    println(*ptr1);  // 1
    
    int* ptr2 = &cube[1][1][1];
    println(*ptr2);  // 8
    
    // ポインタ演算
    int* ptr = &cube[0][0][0];
    println(*(ptr + 7));  // 8
}

void main() {
    test_2d_array_pointer();
    test_pointer_arithmetic();
    test_pointer_write();
    test_3d_array_pointer();
    println("=== All multidim array pointer tests passed ===");
}
```

**期待される出力**: `expected_output.txt`
```
=== Test 1: 2D array element pointer ===
1
2
4
6
=== Test 2: Pointer arithmetic ===
10
20
30
40
50
60
=== Test 3: Pointer write ===
100
200
=== Test 4: 3D array pointer ===
1
8
8
=== All multidim array pointer tests passed ===
```

### Step 4: 実装（2-3時間）

1. エラーチェックを削除または条件分岐に変更
2. 多次元インデックスのフラット化処理を実装
3. ポインタ作成処理を実装

### Step 5: テストと検証（1時間）

1. 新しいテストケースを実行
2. 既存のテストがすべてパスすることを確認
3. エッジケースのテスト

### Step 6: ドキュメント更新（30分）

- `docs/spec.md` に多次元配列ポインタの仕様を追加
- サンプルコード追加

---

## 🧪 テスト戦略

### 成功基準

1. ✅ 2D配列要素へのアドレス取得が成功
2. ✅ 3D配列要素へのアドレス取得が成功
3. ✅ ポインタ演算が正しく動作
4. ✅ ポインタ経由の読み書きが動作
5. ✅ 既存のすべてのテスト（2,432個）がパス

### エッジケース

- 最初の要素 `&matrix[0][0]`
- 最後の要素 `&matrix[rows-1][cols-1]`
- ポインタ演算での範囲外アクセス

---

## 📊 推定時間

| タスク | 時間 |
|--------|------|
| Step 1: 調査 | 30分 |
| Step 2: 実装 | 2-3時間 |
| Step 3: テスト作成 | 30分 |
| Step 4: 実装 | 2-3時間 |
| Step 5: 検証 | 1時間 |
| Step 6: ドキュメント | 30分 |
| **合計** | **6-8時間** |

---

## 🚀 次のステップ

実装完了後：
1. ✅ 多次元配列ポインタ実装完了
2. 次の機能: constポインタ または 構造体配列メンバー代入
3. v0.10.0に向けて継続実装

---

**開始**: 2025年10月9日  
**予定完了**: 2025年10月9日
