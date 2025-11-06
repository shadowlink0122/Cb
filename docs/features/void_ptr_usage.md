# Cb言語におけるvoid*（汎用ポインタ）の使用方法

**Date**: 2025/10/27  
**Feature**: Generic Pointer Type

## 概要

Cbでは`void*`（汎用ポインタ）が実装されており、型に依存しないポインタ操作が可能です。

## 基本的な使用方法

### 1. 変数宣言

```cb
void* ptr = nullptr;
```

### 2. 関数の戻り値

```cb
void* allocate_memory(int size) {
    println("Allocating %d bytes", size);
    return nullptr;  // 実際の実装ではメモリアドレスを返す
}

void main() {
    void* ptr = allocate_memory(100);
}
```

### 3. 関数の引数

```cb
void deallocate_memory(void* ptr) {
    println("Deallocating memory");
}

void main() {
    void* ptr = nullptr;
    deallocate_memory(ptr);
}
```

### 4. Interface定義での使用

```cb
interface Allocator {
    void* allocate(int size);      // メモリ割り当て
    void deallocate(void* ptr);    // メモリ解放
}

struct SystemAllocator {
    int allocation_count;
};

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        println("[SystemAllocator] Allocate %d bytes", size);
        return nullptr;  // プレースホルダー
    }
    
    void deallocate(void* ptr) {
        println("[SystemAllocator] Deallocate");
    }
}
```

### 5. ラムダ式の格納

```cb
void* print_hello = void func() {
    println("Hello from lambda!");
};

print_hello();  // "Hello from lambda!"
```

## 実用例: メモリアロケータ

### SystemAllocator (汎用アロケータ)

```cb
struct SystemAllocator {
    int allocation_count;
    int deallocation_count;
};

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        println("[SystemAllocator] Allocate %d bytes", size);
        self.allocation_count = self.allocation_count + 1;
        // 実際の実装ではmallocなどを呼び出す
        return nullptr;
    }
    
    void deallocate(void* ptr) {
        println("[SystemAllocator] Deallocating pointer");
        self.deallocation_count = self.deallocation_count + 1;
        // 実際の実装ではfreeなどを呼び出す
    }
    
    int get_allocation_count() {
        return self.allocation_count;
    }
}

void test_allocator() {
    SystemAllocator alloc;
    
    // メモリ割り当て
    void* ptr1 = alloc.allocate(100);
    void* ptr2 = alloc.allocate(200);
    
    // メモリ解放
    alloc.deallocate(ptr1);
    alloc.deallocate(ptr2);
}
```

### BumpAllocator (線形アロケータ)

```cb
struct BumpAllocator {
    int buffer_size;
    int current_offset;
    int allocation_count;
};

impl Allocator for BumpAllocator {
    void* allocate(int size) {
        println("[BumpAllocator] Allocating %d bytes (offset=%d)", 
                size, self.current_offset);
        // 実際の実装ではバッファ内のポインタを進める
        // self.current_offset = self.current_offset + size;
        return nullptr;
    }
    
    void deallocate(void* ptr) {
        // BumpAllocatorは個別の解放を行わない
        println("[BumpAllocator] Deallocation ignored");
    }
}

void bump_allocator_reset(BumpAllocator& alloc) {
    alloc.current_offset = 0;
    println("[BumpAllocator] Reset (all memory reclaimed)");
}
```

## 現在の制限事項

### 1. 型キャスト

現時点では明示的な型キャストの構文は未実装:

```cb
// 将来的な構文 (現在は未サポート):
int* int_ptr = (int*)void_ptr;
int* int_ptr = void_ptr as int*;
```

### 2. void*の逆参照

void*は型情報を持たないため、直接逆参照はできません:

```cb
void* ptr = allocate(sizeof(int));
// *ptr = 42;  // エラー: void*は逆参照できない

// 型付きポインタにキャストしてから操作する必要がある (将来的な機能)
```

## 使用パターン

### パターン0: implメソッド内でのメンバーアクセス

**重要**: implメソッド内では`self.メンバー名`でstructのメンバーにアクセスします。

```cb
struct SystemAllocator {
    int allocation_count;
    int deallocation_count;
};

impl Allocator for SystemAllocator {
    void* allocate(int size) {
        // self.を使ってメンバーにアクセス
        self.allocation_count = self.allocation_count + 1;
        println("Total allocations: %d", self.allocation_count);
        return nullptr;
    }
    
    void deallocate(void* ptr) {
        // selfを使ってメンバーを更新
        self.deallocation_count = self.deallocation_count + 1;
    }
    
    int get_allocation_count() {
        // selfを使ってメンバーを返す
        return self.allocation_count;
    }
}
```

**エラー例**:
```cb
impl Allocator for SystemAllocator {
    void* allocate(int size) {
        // エラー: selfなしではメンバーにアクセスできない
        allocation_count = allocation_count + 1;  // ❌ Undefined variable
        
        // 正しい方法: selfを使う
        self.allocation_count = self.allocation_count + 1;  // ✅ OK
    }
}
```

### パターン1: アロケータインターフェース

```cb
interface Allocator {
    void* allocate(int size);
    void deallocate(void* ptr);
}

// 異なる実装を提供
impl Allocator for SystemAllocator { ... }
impl Allocator for BumpAllocator { ... }
impl Allocator for PoolAllocator { ... }
```

**利点**:
- インターフェースで統一的な抽象化
- 実装の切り替えが容易
- ゼロコスト抽象化 (コンパイル時解決)

### パターン2: ジェネリック型との組み合わせ

```cb
struct Vector<T, A: Allocator> {
    int capacity;
    int length;
    // void* data;  // 将来的にT型のデータを格納
};

void vector_init<T, A>(Vector<T, A>& vec, int capacity) {
    // A.allocate()を呼び出してメモリ確保
    // void* ptr = A.allocate(capacity * sizeof(T));
}
```

### パターン3: 関数ポインタの格納

```cb
void* callback = void func() {
    println("Callback invoked");
};

void register_callback(void* cb) {
    // コールバックを登録
}
```

## ベストプラクティス

### 1. nullptr初期化

```cb
void* ptr = nullptr;  // 常にnullptrで初期化
```

### 2. インターフェースでの型安全性

```cb
interface Allocator {
    void* allocate(int size);      // 型安全な抽象化
    void deallocate(void* ptr);
}
```

### 3. ドキュメント化

```cb
// @returns: void* - Pointer to allocated memory or nullptr on failure
void* allocate(int size);
```

## テストケース

### 基本テスト

```cb
void test_void_ptr() {
    // Test 1: nullptr代入
    void* ptr1 = nullptr;
    
    // Test 2: 関数からの取得
    void* ptr2 = allocate_memory(100);
    
    // Test 3: 関数への渡し
    deallocate_memory(ptr2);
    
    println("void* basic test: OK");
}
```

### アロケータテスト

```cb
void test_allocators() {
    // SystemAllocatorテスト
    SystemAllocator sys_alloc;
    void* ptr1 = sys_alloc.allocate(100);
    sys_alloc.deallocate(ptr1);
    
    // BumpAllocatorテスト
    BumpAllocator bump_alloc;
    bump_allocator_init(bump_alloc, 1024);
    void* ptr2 = bump_alloc.allocate(100);
    void* ptr3 = bump_alloc.allocate(200);
    bump_allocator_reset(bump_alloc);
    
    println("Allocator test: OK");
}
```

## 今後の拡張予定

### 1. 型キャスト構文

```cb
// C言語スタイル
int* int_ptr = (int*)void_ptr;

// Rustスタイル
int* int_ptr = void_ptr as int*;
```

### 2. sizeof演算子との連携

```cb
void* ptr = allocator.allocate(sizeof(MyStruct));
```

### 3. アライメント考慮

```cb
void* allocate_aligned(int size, int alignment);
```

## まとめ

Cbの`void*`は以下の用途で使用されます:

1. **メモリアロケータ**: 型に依存しないメモリ割り当て
2. **インターフェース抽象化**: 汎用的なポインタ操作
3. **ラムダ式の格納**: 関数ポインタの型消去
4. **ジェネリックプログラミング**: 型パラメータと組み合わせた抽象化

現在はプレースホルダー的な使用が主ですが、将来的には実際のメモリ操作にも使用される予定です。

## 関連ファイル

- `tests/cases/void_ptr_test.cb` - 基本テスト
- `tests/cases/interface_bounds/test_*.cb` - インターフェースでの使用例
- `stdlib/allocators/system_allocator.cb` - SystemAllocator実装
- `stdlib/allocators/bump_allocator.cb` - BumpAllocator実装
- `stdlib/collections/vector.cb` - Vector<T, A>での使用例
