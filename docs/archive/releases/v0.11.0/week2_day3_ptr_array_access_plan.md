# Week 2 Day 3: Pointer Array Access Implementation Plan

**目標**: ポインタ経由での配列アクセス `ptr[index]` を実装  
**ブランチ**: feature/trait-allocator  
**期間**: Day 3  

---

## 🎯 目的

Day 2で型キャストが実装できたので、次はポインタ経由での配列アクセスを実装します:

```cb
Vector<int, SystemAllocator> vec;
int* data = (int*)vec.data;

// これを実装したい！
data[0] = 42;
data[1] = 100;
int value = data[2];
```

---

## 📋 実装タスク

### Phase 1: AST拡張
**タスク**: ポインタ配列アクセス用のASTノード拡張

**既存のノード**:
- `AST_ARRAY_REF`: 通常の配列アクセス `arr[i]`

**必要な拡張**:
- ポインタ変数からの配列アクセスを`AST_ARRAY_REF`で処理
- または新しい`AST_PTR_ARRAY_ACCESS`を追加

**設計方針**:
```cpp
// Option 1: 既存のAST_ARRAY_REFを拡張
struct ASTNode {
    // 既存: name (配列名)
    // 既存: index (インデックス式)
    bool is_pointer_access;  // ポインタアクセスかどうか
};

// Option 2: 新しいノードタイプ
enum class ASTNodeType {
    AST_PTR_ARRAY_ACCESS,  // ptr[index]
};
```

### Phase 2: パーサー実装
**タスク**: `identifier[index]` 構文でポインタか配列かを判定

**現在のパース処理**:
```cpp
// src/frontend/recursive_parser/parsers/expression_parser.cpp
// postfix expressionで [index] を処理
```

**必要な変更**:
1. 識別子がポインタ型かチェック
2. ポインタなら`is_pointer_access = true`をセット
3. 配列なら従来通り

**判定ロジック**:
```cpp
if (parser_->check(TokenType::TOK_LBRACKET)) {
    // 識別子の型情報を取得
    TypeInfo type = getVariableType(node->name);
    
    if (type.pointer_level > 0) {
        node->is_pointer_access = true;
    }
    
    // インデックス式をパース
    node->index = parseExpression();
}
```

### Phase 3: インタープリタ実装
**タスク**: ポインタ配列アクセスの評価

**必要な機能**:
1. **読み取り**: `value = ptr[i]`
   ```cpp
   int64_t ptr_value = /* ポインタ変数の値 */;
   int64_t offset = index * sizeof(T);
   int64_t address = ptr_value + offset;
   // アドレスからデータ読み取り（モック）
   ```

2. **書き込み**: `ptr[i] = value`
   ```cpp
   int64_t ptr_value = /* ポインタ変数の値 */;
   int64_t offset = index * sizeof(T);
   int64_t address = ptr_value + offset;
   // アドレスにデータ書き込み（モック）
   ```

**実装ファイル**:
- `src/backend/interpreter/evaluator/access/array.cpp`
- 既存の`ArrayAccessHelpers`を拡張

### Phase 4: メモリモック
**タスク**: 簡易的なメモリシミュレーション

**目的**: Day 4のmalloc実装までの仮実装

**設計**:
```cpp
// グローバルメモリマップ（仮）
std::unordered_map<int64_t, int64_t> simulated_memory;

// 書き込み
simulated_memory[address] = value;

// 読み取り
int64_t value = simulated_memory[address];
```

**範囲**:
- アドレス計算の正確性検証
- 基本的な読み書きテスト
- エラーハンドリング（範囲外アクセス）

### Phase 5: Vector統合
**タスク**: Vector実装で実際に配列アクセス

**現在のコード**:
```cb
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value) {
    int* data = (int*)vec.data;
    // data[vec.length] = value;  // ← これを動作させる
    vec.length = vec.length + 1;
}
```

**実装後**:
```cb
void vector_push_int_system(Vector<int, SystemAllocator>& vec, int value) {
    int* data = (int*)vec.data;
    data[vec.length] = value;  // ✅ 動作
    vec.length = vec.length + 1;
}

int vector_pop_int_system(Vector<int, SystemAllocator>& vec) {
    vec.length = vec.length - 1;
    int* data = (int*)vec.data;
    return data[vec.length];  // ✅ 動作
}
```

---

## 🧪 テスト計画

### Test 1: 基本的なポインタ配列アクセス
```cb
void test_ptr_array_basic() {
    int arr[5] = {10, 20, 30, 40, 50};
    int* ptr = arr;  // 配列からポインタへ
    
    // 読み取り
    int v0 = ptr[0];  // 10
    int v1 = ptr[1];  // 20
    int v2 = ptr[2];  // 30
    
    // 書き込み
    ptr[0] = 100;
    ptr[1] = 200;
    
    println("ptr[0] = %d", ptr[0]);  // 100
    println("ptr[1] = %d", ptr[1]);  // 200
}
```

### Test 2: Vector統合テスト
```cb
void test_vector_with_ptr_access() {
    Vector<int, SystemAllocator> vec;
    vector_init_int_system(vec, 10);
    
    // Push with pointer access
    vector_push_int_system(vec, 42);
    vector_push_int_system(vec, 100);
    
    // Direct access
    int* data = (int*)vec.data;
    println("data[0] = %d", data[0]);  // 42
    println("data[1] = %d", data[1]);  // 100
    
    // Pop with pointer access
    int val = vector_pop_int_system(vec);
    println("popped = %d", val);  // 100
}
```

### Test 3: エラーケース
```cb
void test_ptr_array_errors() {
    int arr[5];
    int* ptr = arr;
    
    // 範囲外アクセス（将来的にエラー）
    // ptr[100] = 42;  // ← Out of bounds
    
    // nullptr アクセス
    int* null_ptr = nullptr;
    // int x = null_ptr[0];  // ← Null pointer dereference
}
```

---

## 📊 実装スケジュール

| Phase | タスク | 見積もり時間 |
|-------|--------|-------------|
| Phase 1 | AST拡張 | 20分 |
| Phase 2 | パーサー実装 | 40分 |
| Phase 3 | インタープリタ | 50分 |
| Phase 4 | メモリモック | 30分 |
| Phase 5 | Vector統合 | 20分 |
| **合計** | | **2.5時間** |

---

## 🔍 技術的課題

### Challenge 1: 配列 vs ポインタの判定

**問題**:
```cb
int arr[10];
int* ptr = arr;

arr[0] = 42;  // 配列アクセス
ptr[0] = 42;  // ポインタアクセス
```

**解決策**:
- 変数テーブルから型情報を取得
- `pointer_level > 0` ならポインタアクセス
- `dimensions.size() > 0` なら配列アクセス

### Challenge 2: アドレス計算

**問題**: ポインタ演算の正確性

**解決策**:
```cpp
// ptr[i] のアドレス計算
int64_t ptr_value = /* ポインタの値 */;
int element_size = get_type_size(base_type);
int64_t address = ptr_value + (index * element_size);
```

**型サイズ**:
- `int`: 4 or 8 bytes
- `char`: 1 byte
- `void*`: 8 bytes (64-bit)
- `MyStruct`: sizeof(struct)

### Challenge 3: メモリシミュレーション

**問題**: 実際のメモリがない

**解決策**:
```cpp
// 仮想メモリマップ
std::unordered_map<int64_t, std::vector<uint8_t>> memory_blocks;

// 読み書き
void write_memory(int64_t addr, int64_t value, size_t size);
int64_t read_memory(int64_t addr, size_t size);
```

---

## 🚀 実装後の機能

### 可能になること

1. **Vectorの実データ格納**
   ```cb
   int* data = (int*)vec.data;
   data[0] = 42;  // ✅ 実際に書き込める
   ```

2. **ポインタ経由の配列操作**
   ```cb
   int arr[10];
   int* p = arr;
   for (int i = 0; i < 10; i = i + 1) {
       p[i] = i * 10;
   }
   ```

3. **文字列操作の基礎**
   ```cb
   char* str = "Hello";
   char first = str[0];  // 'H'
   ```

---

## 📈 Week 2 進捗更新

| Day | タスク | 進捗 |
|-----|--------|------|
| Day 1 | Allocator + Vector構造 | ✅ 100% |
| Day 2 | 型キャスト | ✅ 100% |
| Day 3 | 配列ポインタアクセス | 🔵 開始 |
| Day 4 | malloc/free統合 | ⚪ 0% |
| Day 5 | 完全なVector | ⚪ 0% |

**現在の進捗**: 40% → 60% (Day 3完了後)

---

## 🎯 成功基準

✅ Phase 1: `AST_ARRAY_REF`拡張または新ノード追加  
✅ Phase 2: `ptr[index]`構文のパース成功  
✅ Phase 3: ポインタ配列アクセスの評価実装  
✅ Phase 4: メモリモックで読み書き動作  
✅ Phase 5: Vector統合完了  
✅ **全テストパス**  

---

**Status**: 🔵 Ready to Start  
**Previous**: Week 2 Day 2 (100% Complete)  
**Current**: Week 2 Day 3 - Pointer Array Access  
**Next**: Week 2 Day 4 - malloc/free Integration
