# ジェネリックコンストラクタ/デストラクタ テストスイート

## 概要

ジェネリック構造体のコンストラクタ/デストラクタのインスタンス化と実行をテストします。

## 基本構文

```cb
struct Box<T> {
    T value;
}

impl Box<T> {
    self(T v) {
        self.value = v;
    }
    
    ~self() {
        println("Destructor called");
    }
}

void main() {
    Box<int> b(42);  // コンストラクタ呼び出し
}  // デストラクタ自動呼び出し
```

## テストファイル

### 1. basic.cb
基本的なジェネリックコンストラクタ/デストラクタのテスト

**テスト内容**:
- ジェネリック構造体のコンストラクタ定義
- 引数付きコンストラクタの呼び出し
- デストラクタの自動呼び出し
- 複数の型（int, long, short）でのインスタンス化

**実行方法**:
```bash
./main tests/cases/generic_constructor/basic.cb
```

**期待される出力**:
```
=== Generic Constructor Test ===
Box<T> constructor called: value= 42
Box<T> constructor called: value= 123456789
Box<T> constructor called: value= 999
=== All Tests Passed ===
Box<T> destructor called: value= 999
Box<T> destructor called: value= 123456789
Box<T> destructor called: value= 42
```

---

### 2. sizeof_nested.cb
sizeof演算子とネストした構造体のテスト

**テスト内容**:
- sizeof()でプリミティブ型のサイズを取得
- sizeof()でネストした構造体のサイズを取得
- sizeof()でジェネリック構造体のサイズを取得

**実行方法**:
```bash
./main tests/cases/generic_constructor/sizeof_nested.cb
```

**期待される出力**:
```
=== sizeof() Test ===
sizeof(int) =  4
sizeof(long) =  8
sizeof(Point) =  8
sizeof(Rectangle) =  16
sizeof(Box<int>) =  8
sizeof(Box<long>) =  8
=== All Tests Passed ===
```

---

### 3. sizeof_in_constructor.cb
コンストラクタ内でのsizeof(T)の動作テスト

**テスト内容**:
- コンストラクタ内でsizeof(T)が正しく評価されること
- メモリアロケーション時に必要なサイズ計算が可能
- 複数の型（int, long, short）で正しく動作

**実行方法**:
```bash
./main tests/cases/generic_constructor/sizeof_in_constructor.cb
```

**期待される出力**:
```
=== sizeof(T) in Constructor Test ===

Test 1: Container<int>
Container<T> constructor:
  sizeof(T) =  4
  capacity =  10
  total_size =  40
✓ sizeof(T) correctly evaluated as 4

Test 2: Container<long>
Container<T> constructor:
  sizeof(T) =  8
  capacity =  5
  total_size =  40
✓ sizeof(T) correctly evaluated as 8

Test 3: Container<short>
Container<T> constructor:
  sizeof(T) =  2
  capacity =  20
  total_size =  40
✓ sizeof(T) correctly evaluated as 2

=== All Tests Passed ===
```

---

## 全テスト実行

```bash
for file in tests/cases/generic_constructor/*.cb; do
    echo "Running $file..."
    ./main "$file"
done
```

## 実装ステータス

- [x] ジェネリックコンストラクタのインスタンス化
- [x] ジェネリックデストラクタのインスタンス化
- [x] 複数型パラメータのサポート
- [x] sizeof演算子のネスト構造体対応
- [x] テストケース作成
- [ ] Integration test作成
