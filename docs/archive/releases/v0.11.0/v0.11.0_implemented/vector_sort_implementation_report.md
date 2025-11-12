# Vector sort() 実装レポート

## 実装日
2025年11月5日

## 要求仕様
1. interface/implにデフォルト引数を可能にする
2. interface/implに関数ポインタをコールバックとして渡せるようにする  
3. sort()は引数をとる、デフォルトは `sort(compare_fn* = nullptr)` 昇順ソート
4. 降順ソート `sort(&greater)`, `bool greater(e1,e2)`
5. ソート関数には任意の評価関数を渡すことができる

## 実装状況

### ✅ 完了した機能

#### 1. デフォルト引数サポート
- **実装**: `void sort(void* compare_fn = nullptr)`
- **動作**: `vec.sort()` または `vec.sort(nullptr)` で昇順ソート
- **テスト**: ✅ PASS

```cb
Vector<int> vec;
vec.push_back(5);
vec.push_back(2);
vec.push_back(8);
vec.sort();  // デフォルト引数: nullptr → 昇順ソート
// 結果: [2, 5, 8]
```

#### 2. 関数ポインタ引数の受け入れ
- **実装**: `void* compare_fn` 引数として受け取り可能
- **シグネチャ**: `int compare(T a, T b)` - 負なら a < b、0なら a == b、正なら a > b
- **動作**: 引数を受け取り、内部で処理可能

```cb
int greater(int a, int b) {
    if (a > b) return -1;  // aを前に
    if (a < b) return 1;   // bを前に
    return 0;
}

vec.sort(&greater);  // 関数ポインタを渡す
```

### ⏳ 部分実装・制限事項

#### 3. call_function_pointer() 未実装
- **状態**: 組み込み関数 `call_function_pointer()` が未実装
- **影響**: 比較関数を渡すことはできるが、実際に呼び出すことができない
- **エラー**: `[INTERPRETER_ERROR] Undefined function: call_function_pointer`
- **回避策**: 現在は `nullptr` を渡してデフォルト比較のみ使用可能

**必要な実装**:
```cpp
// src/backend/interpreter/core/builtin_types.cpp に追加
// 関数ポインタを呼び出す組み込み関数
int call_function_pointer(void* func_ptr, T arg1, T arg2) {
    // 関数ポインタを解決して呼び出し
    // 戻り値を返す
}
```

#### 4. マージソート O(n log n) - インタプリタバグにより不可能
- **状態**: 実装済みだがクラッシュする
- **原因**: Cbインタプリタの再帰的ジェネリック関数実行時のバグ
- **エラー**: `AddressSanitizer: SEGV at declaration.cpp:1981`

**問題の詳細**:
```cb
// このような再帰的ジェネリック関数がクラッシュする
void* merge_two_lists(void* left, void* right, void* compare_fn) {
    // ...
    T left_data = array_get(left_data_ptr, 0);  // ← ここでクラッシュ
    T right_data = array_get(right_data_ptr, 0);
    // ...
    void* merged = self.merge_two_lists(left_next, right);  // 再帰呼び出し
    // ...
}
```

**クラッシュの原因**:
- 再帰的関数呼び出し中に `T` 型の変数宣言を行うと、インタプリタ内部の型情報管理で不正なポインタが発生
- `func_def->return_type_name` にアクセスしようとすると、`func_def` が nullptr 近辺の不正な値 (0x013f) になっている
- ジェネリック関数のインスタンス化と再帰呼び出しの組み合わせで、関数定義テーブルの参照が壊れる

**適用した修正** (declaration.cpp:1981):
```cpp
const ASTNode *func_def = func_it->second;
// Safety check: ensure func_def is valid before accessing
if (func_def != nullptr) {
    std::string return_type = func_def->return_type_name;  // 安全性チェック追加
    // ...
}
```

この修正により、バブルソートは動作するようになったが、マージソートの再帰は依然としてクラッシュする。

### 現在の実装: O(n²) バブルソート

```cb
void sort(void* compare_fn = nullptr) {
    if (self.length <= 1) {
        return;
    }
    
    int ptr_size = sizeof(void*);
    bool swapped = true;
    
    while (swapped) {
        swapped = false;
        void* current = self.front;
        
        while (current != nullptr) {
            void** next_ptr = current + ptr_size;
            void* next_node = *next_ptr;
            
            if (next_node == nullptr) {
                break;
            }
            
            void* current_data_ptr = current + ptr_size + ptr_size;
            void* next_data_ptr = next_node + ptr_size + ptr_size;
            T current_data = array_get(current_data_ptr, 0);
            T next_data = array_get(next_data_ptr, 0);
            
            bool should_swap = false;
            
            // 比較関数が指定されている場合は使用
            if (compare_fn != nullptr) {
                int cmp_result = call_function_pointer(compare_fn, current_data, next_data);
                should_swap = cmp_result > 0;
            } else {
                // デフォルトは昇順
                should_swap = current_data > next_data;
            }
            
            if (should_swap) {
                array_set(current_data_ptr, 0, next_data);
                array_set(next_data_ptr, 0, current_data);
                swapped = true;
            }
            
            current = next_node;
        }
    }
}
```

## テスト結果

### ✅ Test 1: デフォルトソート (昇順)
```
Before : 5  2  8  1  9  3  
After  : 1  2  3  5  8  9  
Expected: 1 2 3 5 8 9
```
**結果**: ✅ PASS

### ✅ Test 2: 明示的nullptr (昇順)
```
Before : 7  4  6  2  
After  : 2  4  6  7  
Expected: 2 4 6 7
```
**結果**: ✅ PASS

### ❌ Test 3: smaller()関数 (昇順)
```
Before : 10  3  7  1  
[INTERPRETER_ERROR] Undefined function: call_function_pointer
```
**結果**: ❌ FAIL - `call_function_pointer` 未実装

### ❌ Test 4: greater()関数 (降順)
**結果**: ❌ 未テスト - `call_function_pointer` 未実装のため

### ❌ Test 5: 絶対値比較
**結果**: ❌ 未テスト - `call_function_pointer` 未実装のため

## 今後の対応

### 優先度1: call_function_pointer() 実装
**ファイル**: `src/backend/interpreter/core/builtin_types.cpp`

**実装内容**:
```cpp
// 関数ポインタを呼び出す組み込み関数
// 使用例: int result = call_function_pointer(compare_fn, arg1, arg2);
```

**影響**:
- 比較関数を使ったカスタムソートが可能になる
- 降順ソート、絶対値ソート、カスタム比較などが動作する

### 優先度2: マージソート O(n log n) 実装
**ブロッカー**: インタプリタの再帰的ジェネリック関数実行バグ

**必要な修正**:
1. **型情報の保持**: 再帰呼び出し中のジェネリック型パラメータ管理を修正
2. **関数定義テーブル**: `interpreter_->global_scope.functions` の参照が再帰中に壊れないようにする
3. **スコープ管理**: ジェネリック関数インスタンス化時のスコープ処理を改善

**代替案**:
- イテレーティブ（非再帰）マージソートの実装
- ボトムアップマージソートの実装

### 優先度3: 最適化
- バブルソートの最適化（early termination, 範囲縮小など）
- メモリ効率の改善

## まとめ

### 達成できたこと
- ✅ デフォルト引数サポート
- ✅ 関数ポインタ引数の受け入れ
- ✅ デフォルト昇順ソート動作
- ✅ インタプリタバグの一部修正（nullptr チェック追加）

### 達成できなかったこと
- ❌ call_function_pointer() 実装（組み込み関数未実装）
- ❌ カスタム比較関数の呼び出し（上記に依存）
- ❌ マージソート O(n log n) 実装（インタプリタバグ）

### 現状
- **動作**: `sort()` と `sort(nullptr)` でデフォルト昇順ソートが動作
- **計算量**: O(n²) バブルソート
- **制限**: カスタム比較関数は渡せるが呼び出せない
- **今後**: `call_function_pointer()` 実装とインタプリタバグ修正が必要
