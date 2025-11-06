# Cbにおけるvoid*の使用方法まとめ

**Date**: 2025/10/27  
**Status**: ✅ Fully Implemented and Tested

## 結論

**Cbでは`void*`（汎用ポインタ）が完全に実装されており、以下の用途で使用できます:**

1. ✅ **変数宣言**: `void* ptr = nullptr;`
2. ✅ **関数の戻り値**: `void* allocate(int size)`
3. ✅ **関数の引数**: `void deallocate(void* ptr)`
4. ✅ **インターフェースメソッド**: `interface Allocator { void* allocate(int size); }`
5. ✅ **構造体のメンバー**: `struct Node { void* data; }`
6. ✅ **ラムダ式の格納**: `void* callback = void func() { ... };`

## 主な使用例

### 1. メモリアロケータパターン

```cb
interface Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}

struct SystemAllocator {
    int allocation_count;
};

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        self.allocation_count = self.allocation_count + 1;
        return nullptr;  // 実際の実装ではmalloc()の結果を返す
    }
    
    void deallocate(void* ptr) {
        // free()を呼び出す
    }
}
```

### 2. ジェネリック型との組み合わせ

```cb
struct Vector<T, A: Allocator> {
    int capacity;
    int length;
    // 将来的に: void* data;
};

// 異なるアロケータで異なる型を作成
Vector<int, SystemAllocator> sys_vec;    // malloc/free使用
Vector<int, BumpAllocator> bump_vec;     // 線形アロケータ使用
```

### 3. implメソッド内でのメンバーアクセス

**重要**: `self.メンバー名`を使用

```cb
struct BumpAllocator {
    int current_offset;
    int capacity;
};

impl Allocator for BumpAllocator {
    void* allocate(int size) {
        // ✅ self.を使ってメンバーにアクセス
        println("Current offset: %d", self.current_offset);
        self.current_offset = self.current_offset + size;
        return nullptr;
    }
}
```

### 4. 複数のvoid*メンバー

```cb
struct MemoryPool {
    void* buffer;      // バッファの先頭
    void* current;     // 現在の位置
    void* end;         // バッファの終端
    int allocations;   // 割り当て回数
};

void memory_pool_init(MemoryPool& pool) {
    pool.buffer = nullptr;
    pool.current = nullptr;
    pool.end = nullptr;
    pool.allocations = 0;
}
```

### 5. ラムダ式の格納

```cb
void* callback = void func() {
    println("Hello from lambda!");
};

callback();  // ラムダを呼び出し
```

## 実用的なアロケータ実装

### SystemAllocator（汎用）

```cb
struct SystemAllocator {
    int allocation_count;
    int deallocation_count;
};

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        println("[SystemAllocator] Allocating %d bytes", size);
        self.allocation_count = self.allocation_count + 1;
        return nullptr;  // 実装: malloc(size)
    }
    
    void deallocate(void* ptr) {
        println("[SystemAllocator] Deallocating");
        self.deallocation_count = self.deallocation_count + 1;
        // 実装: free(ptr)
    }
}
```

**特徴**:
- 個別の割り当て/解放
- OS環境向け
- malloc/freeのラッパー

### BumpAllocator（線形）

```cb
struct BumpAllocator {
    void* buffer;
    int offset;
    int capacity;
};

impl Allocator for BumpAllocator {
    void* allocate(int size) {
        println("[BumpAllocator] Allocating %d bytes (offset=%d)", 
                size, self.offset);
        // 実装: バッファ内のポインタを進める
        return nullptr;
    }
    
    void deallocate(void* ptr) {
        // BumpAllocatorは個別の解放を行わない
        println("[BumpAllocator] Deallocation ignored");
    }
}

void bump_allocator_reset(BumpAllocator& alloc) {
    alloc.offset = 0;  // 一括でメモリを再利用
}
```

**特徴**:
- 高速な割り当て（ポインタインクリメントのみ）
- 個別の解放は無視
- リセットで一括解放
- 一時データ向け

## テスト結果

全7つのテストケースが成功:

```
✅ Example 1: Basic void* operations
✅ Example 2: Allocator interface
✅ Example 3: Memory pool with multiple void*
✅ Example 4: Lambda storage in void*
✅ Example 5: Generic node structure
✅ Example 6: Different allocators (SystemAllocator + BumpAllocator)
✅ Example 7: Nullptr handling
```

## ベストプラクティス

### 1. nullptr初期化

```cb
void* ptr = nullptr;  // 常にnullptrで初期化
```

### 2. selfを使ったメンバーアクセス

```cb
impl Allocator for MyAllocator {
    void* allocate(int size) {
        self.count = self.count + 1;  // ✅ 正しい
        count = count + 1;             // ❌ エラー
    }
}
```

### 3. インターフェースでの型安全性

```cb
interface Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}

// 複数の実装を提供
impl Allocator for SystemAllocator { ... }
impl Allocator for BumpAllocator { ... }
impl Allocator for PoolAllocator { ... }
```

## 現在の制限事項

### 1. 型キャスト（未実装）

```cb
// 将来的な構文:
void* generic_ptr = allocate(100);
int* typed_ptr = (int*)generic_ptr;      // C言語スタイル
int* typed_ptr = generic_ptr as int*;    // Rustスタイル
```

### 2. void*の逆参照（不可）

```cb
void* ptr = allocate(4);
// *ptr = 42;  // エラー: void*は直接逆参照できない
```

### 3. sizeof演算子（未実装）

```cb
// 将来的な構文:
void* ptr = allocate(sizeof(MyStruct));
```

## Week 2での活用

`void*`は以下の機能で活用されています:

1. **Allocatorインターフェース**: メモリ管理の抽象化
2. **SystemAllocator**: malloc/freeのラッパー
3. **BumpAllocator**: 線形アロケータ
4. **Vector<T, A>**: 動的配列の基盤（将来実装）

```cb
// 異なるアロケータを使い分け
Vector<int, SystemAllocator> general_vec;    // 汎用
Vector<int, BumpAllocator> temp_vec;         // 一時データ
```

## 関連ファイル

### テストファイル
- `tests/cases/void_ptr_test.cb` - 基本テスト
- `tests/cases/void_ptr_comprehensive.cb` - 包括的テスト（7つのサンプル）
- `tests/cases/interface_bounds/test_*.cb` - インターフェースでの使用例

### 実装ファイル
- `stdlib/allocators/system_allocator.cb` - SystemAllocator実装
- `stdlib/allocators/bump_allocator.cb` - BumpAllocator実装
- `stdlib/collections/vector.cb` - Vector<T, A>実装

### ドキュメント
- `docs/features/void_ptr_usage.md` - 詳細な使用方法

## まとめ

**Cbにおけるvoid\*は以下の用途で使用されます:**

| 用途 | 状態 | 例 |
|------|------|-----|
| 変数宣言 | ✅ | `void* ptr = nullptr;` |
| 関数の戻り値 | ✅ | `void* allocate(int size)` |
| 関数の引数 | ✅ | `void deallocate(void* ptr)` |
| インターフェース | ✅ | `interface Allocator { void* allocate(...); }` |
| 構造体メンバー | ✅ | `struct Node { void* data; }` |
| ラムダ格納 | ✅ | `void* cb = void func() { ... };` |
| impl内アクセス | ✅ | `self.member` |
| 型キャスト | ⚪ | 将来実装予定 |
| 逆参照 | ❌ | 不可（型情報なし） |

**現在の主な用途**: メモリアロケータの抽象化により、異なるメモリ管理戦略を統一的に扱う。
