# Week 1 Day 5: テストケース計画

## テスト一覧

Week 1で作成・実行するテストケース:

### ✅ 完了済み (Day 1-3)

1. **test_simple.cb** - シンプルなジェネリック構造体
2. **test_basic_bounds.cb** - 基本的なインターフェース境界
3. **test_multiple_bounds.cb** - 複数の境界
4. **test_mixed_bounds.cb** - 混合型パラメータ
5. **test_type_check_valid.cb** - 有効な型チェック
6. **test_type_check_invalid.cb** - 無効な型チェック

### 🔵 Day 5 追加テスト

#### Test 7: Forward declaration with bounds
```cb
// test_forward_decl_bounds.cb
interface Allocator {
    void* allocate(int size);
}

// 前方宣言with bounds
struct Vector<T, A: Allocator>;

struct SystemAllocator {};
impl Allocator for SystemAllocator { /* ... */ }

// 完全定義
struct Vector<T, A: Allocator> {
    int capacity;
};

void main() {
    Vector<int, SystemAllocator> vec;
    println("Forward declaration with bounds: OK");
}
```

#### Test 8: Nested generic with bounds
```cb
// test_nested_bounds.cb
interface Allocator {
    void* allocate(int size);
}

struct SystemAllocator {};
impl Allocator for SystemAllocator { /* ... */ }

struct Box<T, A: Allocator> {
    int value;
};

struct Container<T, A: Allocator> {
    Box<T, A> inner;  // ネストされたジェネリック
};

void main() {
    Container<int, SystemAllocator> c;
    c.inner.value = 42;
    println("Nested generic with bounds: value=%d", c.inner.value);
}
```

#### Test 9: Multiple type arguments same interface
```cb
// test_same_interface_multiple_params.cb
interface Allocator {
    void* allocate(int size);
}

struct Allocator1 {};
struct Allocator2 {};

impl Allocator for Allocator1 { /* ... */ }
impl Allocator for Allocator2 { /* ... */ }

// 両方のパラメータが同じインターフェースを実装
struct BiAllocator<A1: Allocator, A2: Allocator> {
    int size;
};

void main() {
    BiAllocator<Allocator1, Allocator2> ba;
    ba.size = 100;
    println("Same interface for multiple params: OK");
}
```

#### Test 10: Error - Missing impl
```cb
// test_error_missing_impl.cb
interface Allocator {
    void* allocate(int size);
}

struct BadAllocator {};
// implが定義されていない

struct Vector<T, A: Allocator> {
    int capacity;
};

void main() {
    Vector<int, BadAllocator> vec;  // エラー: BadAllocatorはAllocatorを実装していない
}
```
**Expected**: エラーメッセージを出力

#### Test 11: Error - Wrong interface
```cb
// test_error_wrong_interface.cb
interface Allocator {
    void* allocate(int size);
}

interface Iterator {
    bool has_next();
}

struct MyIterator {};
impl Iterator for MyIterator { /* ... */ }

struct Vector<T, A: Allocator> {
    int capacity;
};

void main() {
    Vector<int, MyIterator> vec;  // エラー: MyIteratorはAllocatorではなくIteratorを実装
}
```
**Expected**: エラーメッセージを出力

#### Test 12: Error - Undefined interface
```cb
// test_error_undefined_interface.cb
struct SystemAllocator {};

// Allocatorインターフェースが未定義
struct Vector<T, A: Allocator> {
    int capacity;
};

void main() {
    Vector<int, SystemAllocator> vec;
}
```
**Expected**: パースエラーまたは型チェックエラー

#### Test 13: Complex type arguments
```cb
// test_complex_type_args.cb
interface Allocator {
    void* allocate(int size);
}

struct SystemAllocator {};
impl Allocator for SystemAllocator { /* ... */ }

struct Box<T, A: Allocator> {
    int value;
};

// 複雑な型引数: Box<int, SystemAllocator>
struct Container<T, A: Allocator> {
    Box<int, A> data;  // Aを内部ジェネリックに伝播
};

void main() {
    Container<int, SystemAllocator> c;
    c.data.value = 123;
    println("Complex type arguments: value=%d", c.data.value);
}
```

## テスト実行計画

### Day 5 Morning (3時間)
- Test 7-9の実装・実行
- 既存テストの回帰テスト

### Day 5 Afternoon (3時間)
- Test 10-13の実装・実行
- エラーケースの検証
- Week 1サマリー作成

## 成功基準

### 必須 (MUST)
- ✅ Test 1-6: すべて成功
- ⚪ Test 7-9: すべて成功
- ⚪ Test 10-12: 適切なエラーメッセージを出力
- ⚪ Test 13: 成功

### 推奨 (SHOULD)
- パフォーマンス: 各テスト100ms以内
- エラーメッセージ: ユーザーフレンドリーな出力
- ドキュメント: すべてのテストケースに説明コメント

### オプション (NICE TO HAVE)
- デバッグモードでの詳細出力
- テスト自動化スクリプト
- カバレッジレポート

## エラーメッセージの期待値

### Type Check Error
```
Error: Type 'BadAllocator' does not implement interface 'Allocator' 
required by type parameter 'A' in 'Vector_int_BadAllocator<T, A: Allocator>'
```

### Missing Interface Error
```
Error: Interface 'Allocator' not found
```

### Parse Error
```
Error: Expected interface name after ':' in type parameter bound
```

## Week 1 完了チェックリスト

### 実装
- [x] Day 1: AST拡張
- [x] Day 2: パーサー拡張
- [x] Day 3: 型チェック
- [x] Day 4: 設計文書作成
- [ ] Day 5: テストケース作成・実行

### ドキュメント
- [x] week1_interface_bounds_plan.md
- [x] day4_type_parameter_method_resolution.md
- [ ] Week 1 サマリー文書
- [ ] リリースノート更新

### テスト
- [x] 基本テスト (Test 1-6)
- [ ] 追加テスト (Test 7-13)
- [ ] 回帰テスト
- [ ] エラーケーステスト

### コードレビュー
- [ ] AST変更のレビュー
- [ ] パーサー変更のレビュー
- [ ] 型チェック実装のレビュー
- [ ] テストカバレッジ確認

---

**作成日**: 2025/10/27  
**Week 1 目標**: インターフェース境界の基礎実装  
**Next**: Week 2 - Vector<T, A>実装
