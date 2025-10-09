# ポインタ・構造体・selfメンバアクセスの制限事項

## ✅ サポートされている機能

### ポインタ操作
- ✅ アロー演算子 (`ptr->field`, `ptr->method()`)
- ✅ デリファレンス構文 (`(*ptr).field`, `(*ptr).method()`)
- ✅ ネストされたポインタ操作 (`(*(*ptr).inner).value`)
- ✅ ポインタtoポインタ (`int**`, `struct**`)
- ✅ アドレス取得 (`&variable` - 全変数型)
- ✅ unsigned型ポインタ (`unsigned int*`, `unsigned long*`)
- ✅ constパラメータ (`int func(const int x)`)

### selfメンバアクセス
- ✅ 基本的なメンバアクセス (`self.field`)
- ✅ ポインタメンバアクセス (`self.ptr->field`, `self.data->method()`)
- ✅ 配列メンバアクセス (`self.values[0]`)
- ✅ ネストされた構造体 (`self.outer.inner.value`)
- ✅ ダブルポインタ (`(*(*self.ptr_ptr)).data`)
- ✅ メソッド内での`self`変更 (`self.count = self.count + 1`)

### 構造体とInterface
- ✅ プリミティブ型へのinterface実装 (`impl IOperation for int`)
- ✅ 構造体へのinterface実装 (`impl IOperation for Point`)
- ✅ ポインタ型への自動interface適用 (`Point*`が自動的に`IOperation`を実装)
- ✅ Interface変数の代入 (`IOperation op = 5`)

## ⚠️ 制限事項・未サポート機能

### 1. 構造体内メソッド定義
**状態**: ❌ 未サポート

構造体内部でメソッドを直接定義することはできません。

```cb
// ❌ エラー: Expected ';' after struct member
struct Point {
    int x;
    int y;
    
    int sum() {  // 構造体内でのメソッド定義は不可
        return self.x + self.y;
    }
};
```

**回避策**: Interface経由でメソッドを定義
```cb
// ✅ 正しい方法
struct Point {
    int x;
    int y;
};

interface IPoint {
    int sum();
};

impl IPoint for Point {
    int sum() {
        return self.x + self.y;
    }
};
```

### 2. Interface型ポインタのメソッド呼び出し
**状態**: ⚠️ 部分的にサポート（結果が不正確）

Interface型のポインタに対してアロー演算子でメソッド呼び出しはできますが、結果が正しくありません。

```cb
// ⚠️ コンパイルは通るが、結果が0になる
int x = 5;
IOperation op = x;
IOperation* op_ptr = &op;
println("Result: %d", op_ptr->apply(10));  // 期待: 15, 実際: 0
```

**回避策**: デリファレンスしてからメソッド呼び出し
```cb
// ✅ 正しい方法
println("Result: %d", (*op_ptr).apply(10));  // 正しく 15 が出力される
```

### 3. メソッド戻り値に対するアロー演算子
**状態**: ❌ 未サポート

メソッドが返すポインタに対して、直接アロー演算子を使用できません。

```cb
struct Container {
    Point* data;
    
    Point* getData() {
        return self.data;
    }
};

// ❌ エラーまたは不正確な結果
int x = container.getData()->x;

// ❌ エラーまたは不正確な結果
int sum = container.getData()->sum();
```

**回避策**: 中間変数を使用
```cb
// ✅ 正しい方法
Point* ptr = container.getData();
int x = ptr->x;
int sum = ptr->sum();
```

### 4. const構造体メンバの変更チェック
**状態**: ✅ 実装済み（2025-10-05）

const修飾されたselfメンバへの代入は正しくエラーになります。

```cb
struct Counter {
    const int limit;  // constメンバ
    int count;
};

impl ICounter for Counter {
    void reset() {
        self.limit = 100;  // ✅ エラー: Cannot assign to const self member: limit
    }
};
```

通常の構造体メンバへの代入も同様にconstチェックが実行されます。

### 5. 旧式配列宣言（構造体メンバ）
**状態**: ❌ 明示的にサポート対象外

構造体メンバでの旧式配列宣言（C言語スタイル）はサポートされていません。

```cb
struct Matrix {
    // ❌ エラー: Old-style array declaration not supported
    int data[2][2];
};
```

**回避策**: 新式配列宣言を使用
```cb
// ✅ 正しい方法
struct Matrix {
    int[2][2] data;
};
```

### 6. 多次元関数戻り値配列の代入
**状態**: ❌ 未サポート

多次元配列を返す関数の結果をメンバに代入することは制限されています。

```cb
// ❌ エラー: Multi-dimensional function return member array assignment not supported
struct Matrix {
    int[2][2] data;
};

int[2][2] createMatrix() {
    return [[1, 2], [3, 4]];
}

// ❌ エラー
Matrix m;
m.data = createMatrix();
```

**回避策**: 直接初期化またはループで代入
```cb
// ✅ 初期化時に指定
Matrix m = { data: [[1, 2], [3, 4]] };

// ✅ または個別に代入
Matrix m;
m.data[0][0] = 1;
m.data[0][1] = 2;
// ...
```

## 📋 テスト状況

### 実装済み・テスト済み
- ✅ 包括的ポインタ操作テスト (20テスト)
- ✅ 包括的アドレス取得テスト (15テスト)
- ✅ selfポインタメンバアクセステスト
- ✅ ネストされたデリファレンステスト
- ✅ unsigned型ポインタテスト

### コメントアウト（未実装）
- ❌ `test_arrow_in_impl()` - implメソッド戻り値のアロー演算子
- ❌ `test_interface_pointer_arrow()` - Interface型ポインタのアロー演算子

## 🔄 今後の改善予定

1. **Interface型ポインタのメソッド呼び出し**
   - アロー演算子で正しく動作するように修正
   - デリファレンスが不要になるように改善

2. **メソッド戻り値に対するアロー演算子**
   - チェーン呼び出しをサポート
   - 中間変数なしで直接アクセス可能に

3. **const検証の強化**
   - constメンバへの代入を実行時にチェック
   - コンパイル時警告の追加

4. **構造体内メソッド定義**
   - シンタックスシュガーとしてサポート検討
   - 内部的にはinterface経由で実装

## 📝 関連ドキュメント

- [Interface実装ガイド](interface_system.md)
- [ポインタ実装プラン](pointer_implementation_plan.md)
- [構造体実装状況](struct_implementation_status.md)
