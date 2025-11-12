# Interface/Impl System Documentation

Cbプログラミング言語のInterface/Implシステムの完全実装ガイドです。

## 概要 ✅ 完全実装

Interface/Implシステムは、型に対してメソッドを定義し実装する機能で、完全に実装されています。これにより以下が可能です：

- ✅ **構造体・プリミティブ型・typedef型・配列型に対するメソッドの定義**
- ✅ **インターフェース変数による抽象化・ポリモーフィズム**
- ✅ **型安全なメソッド呼び出し・self参照対応**
- ✅ **再帰的typedefの独立した実装**
- ✅ **包括的エラー検出・プライベートメソッド対応**

## 完全実装機能 ✅

### 1. インターフェースの定義

```cb
interface Printable {
    string toString();
    int getSize();
};

interface Drawable {
    void draw();
    void clear();
};

interface Comparable {
    bool equals(int other);
    int compare(int other);
};
```

### 2. 構造体への実装

```cb
struct Point {
    int x;
    int y;
};

impl Printable for Point {
    string toString() {
        return "Point";
    }
    
    int getSize() {
        return 2;  // x, y の2つの要素
    }
};

impl Drawable for Point {
    void draw() {
        printf("Drawing point at (%d, %d)\n", self.x, self.y);
    }
    
    void clear() {
        printf("Clearing point\n");
    }
};

impl Comparable for Point {
    bool equals(int other) {
        return self.x == other || self.y == other;
    }
    
    int compare(int other) {
        return self.x - other;
    }
};
```

### 3. プリミティブ型への実装

```cb
impl Printable for int {
    string toString() {
        return "integer";
    }
    
    int getSize() {
        return 1;
    }
};

impl Printable for string {
    string toString() {
        return self;  // 自分自身を返す
    }
    
    int getSize() {
        return 10;  // 文字列の概算サイズ
    }
};

impl Drawable for bool {
    void draw() {
        if (self) {
            printf("Drawing TRUE\n");
        } else {
            printf("Drawing FALSE\n");
        }
    }
    
    void clear() {
        printf("Clearing boolean display\n");
    }
};
```

### 4. Typedef型への実装

```cb
typedef int MyInt;
typedef string MyString;
typedef int[5] IntArray;

impl Printable for MyInt {
    string toString() {
        return "MyInt value";
    }
    
    int getSize() {
        return 1;
    }
};

impl Printable for IntArray {
    string toString() {
        return "IntArray[5]";
    }
    
    int getSize() {
        return 5;
    }
};

impl Drawable for IntArray {
    void draw() {
        printf("Drawing array: [");
        for (int i = 0; i < 5; i++) {
            printf("%d", self[i]);
            if (i < 4) printf(", ");
        }
        printf("]\n");
    }
    
    void clear() {
        printf("Clearing array display\n");
    }
};
```

### 5. 配列型への実装

```cb
// 1次元配列
impl Printable for int[3] {
    string toString() {
        return "int array[3]";
    }
    
    int getSize() {
        return 3;
    }
};

// 多次元配列
impl Printable for int[2][3] {
    string toString() {
        return "2x3 matrix";
    }
    
    int getSize() {
        return 6;  // 2 * 3
    }
};
```

## 高度な機能 ✅

### インターフェース変数とポリモーフィズム

インターフェース型の変数を使用して、異なる型を統一的に扱えます：

```cb
// make_counter / produce_dynamic などの補助関数はテストケース内で定義されています。
int main() {
    // 異なる型の変数作成
    MyInt mi = 42;
    MyString ms = "test";
    IntArray arr = [1, 2, 3, 4, 5];
    Point point = {x: 10, y: 20};
    
    // 同じインターフェース変数に代入可能
    Printable p1 = mi;      // MyInt -> Printable
    Printable p2 = ms;      // MyString -> Printable  
    Printable p3 = arr;     // IntArray -> Printable
    Printable p4 = point;   // Point -> Printable
    
    // 統一的なメソッド呼び出し
    printf("MyInt: %s\n", p1.toString());        // "MyInt value"
    printf("MyString: %s\n", p2.toString());     // "MyString value"
    printf("IntArray: %s\n", p3.toString());     // "IntArray[5]"
    printf("Point: %s\n", p4.toString());        // "Point"
    
    // サイズ情報の取得
    printf("Sizes: %d, %d, %d, %d\n", 
           p1.getSize(), p2.getSize(), p3.getSize(), p4.getSize());
    
    return 0;
}
```

### 再帰的Typedef独立性

再帰的なtypedefの場合、各typedef型は独立してインターフェースを実装できます：

```cb
typedef int INT;
typedef INT INT2;
typedef INT2 INT3;

// INT3にのみPrintableを実装
impl Printable for INT3 {
    string toString() {
        return "INT3 implementation";
    }
    
    int getSize() {
        return 333;
    }
};

int main() {
    int original = 100;      // Printableなし
    INT int1 = 200;          // Printableなし  
    INT2 int2 = 300;         // Printableなし
    INT3 int3 = 400;         // Printableあり
    
    // これは成功する
    Printable p3 = int3;
    printf("INT3: %s\n", p3.toString());  // "INT3 implementation"
    
    // これらは失敗する（実行時エラー）
    // Printable p_orig = original; // Error: No impl found for interface 'Printable' with type 'int'
    // Printable p1 = int1;        // Error: No impl found for interface 'Printable' with type 'INT'
    // Printable p2 = int2;        // Error: No impl found for interface 'Printable' with type 'INT2'
    
    return 0;
}
```

### メソッドチェーンと型推論

Interface/Impl システムでは、実行時の受け取り方（receiver）を解決することで、型を跨いだ長いメソッドチェーンも安全に評価できます。最新の `type_inference_chain` シナリオでは次のような 14 ステップのチェーンをカバーしています。

```cb
int main() {
    // 1. プリミティブと構造体を混在させた Calculable チェーン
    Counter base = make_counter(10);
    println("%d", base.add(5).multiply(2).add(3));     // -> 33

    // 2. 関数戻り値や inline 生成した Counter でも同様にチェーン可能
    println("%d", make_counter(4).multiply(3).add(2));  // -> 14
    println("%d", get_primitive_value().multiply(3).add(1)); // -> 37

    // 3. 配列・関数・Typedef Union を跨ぐケースも同じ要領で評価
    println("%d", counter_array[1].add(-3).multiply(2)); // -> 10
    println("%d", produce_dynamic(true).add(3).multiply(4)); // -> 20
    println("%d", compute_union_counter_chain()); // -> 32
    println("%d", compute_union_int_chain());     // -> 22

    return 0;
}
```

代表的な到達点は `tests/cases/interface/type_inference_chain.cb` および `tests/integration/interface/test_type_inference_chain.hpp` で検証しており、プリミティブ → 構造体 → Union のような受け渡しでも、中間結果はすべて `Calculable` インターフェース経由で正しく推論されます。

> ℹ️ 現時点ではネストした構造体メンバーへの直接代入（`obj.member.submember = value`）は未サポートのため、テスト内ではチェーン入力を設定するための軽量な `TeamStats` 構造体を利用しています。

### 複数インターフェース実装

一つの型に複数のインターフェースを実装できます：

```cb
struct Circle {
    int x, y;
    int radius;
};

impl Printable for Circle {
    string toString() {
        return "Circle";
    }
    
    int getSize() {
        return self.radius * self.radius * 3;  // 簡易面積計算
    }
};

impl Drawable for Circle {
    void draw() {
        printf("Drawing circle at (%d, %d) with radius %d\n", 
               self.x, self.y, self.radius);
    }
    
    void clear() {
        printf("Clearing circle\n");
    }
};

int main() {
    Circle c = {x: 10, y: 20, radius: 5};
    
    // 同じオブジェクトを異なるインターフェースとして使用
    Printable p = c;
    Drawable d = c;
    
    printf("Circle info: %s, area: %d\n", p.toString(), p.getSize());
    d.draw();  // "Drawing circle at (10, 20) with radius 5"
    d.clear(); // "Clearing circle"
    
    return 0;
}
```

### selfによるプライベートメンバーアクセス ✅

implメソッド内では、`self`を通じて構造体のプライベートメンバーを安全に参照・更新できます。プライバシー制約はランタイムで保護されつつ、実装側からの内部操作が可能です。

```cb
struct SecretBox {
    private int secret;
    int visible;
};

interface Inspector {
    void bump(int value);
    int reveal();
};

impl Inspector for SecretBox {
    void bump(int value) {
        self.secret = self.secret + value;
        self.visible = self.visible + value;
    }
    
    int reveal() {
        return self.secret;
    }
};

int main() {
    SecretBox box = { secret: 11, visible: 4 };
    box.bump(5);
    println("secret = %d, visible = %d", box.reveal(), box.visible);
    return 0;
}
```

`self.secret` の読み書きは同一構造体のimpl内に限定されるため、外部コードからの不正アクセスは引き続き防止されます。

## エラー処理とデバッグ ✅

### 実装エラーの検出

システムは以下のエラーを適切に検出します：

1. **未定義インターフェース**
   ```cb
   impl UndefinedInterface for int { ... } // Error: Interface 'UndefinedInterface' is not defined
   ```

2. **実装なしでのメソッド呼び出し**
   ```cb
   int x = 5;
   Printable p = x; // Error: No impl found for interface 'Printable' with type 'int'
   ```

3. **メソッド署名の不一致**
   ```cb
   interface Test {
       int getValue();
   };
   
   impl Test for int {
       string getValue() { ... } // Error: Return type mismatch
   };
   ```

4. **重複実装**
   ```cb
   impl Printable for int { ... }
   impl Printable for int { ... } // Error: Duplicate implementation
   ```

5. **インクリメントメソッド実装なし**
   ```cb
   interface Counter {
       void increment();
   };
   
   impl Counter for int {
       // increment メソッドが未実装 → エラー
   };
   ```

### デバッグ機能 ✅

デバッグモード（`--debug`フラグ）で以下の詳細情報が出力されます：

```cb
// デバッグ出力例
[INTERFACE] Assigning struct to interface variable: Circle -> Printable
[METHOD] Method call started: toString() on Circle
[METHOD] Interface method call: Printable.toString()
[METHOD] Found implementation: Circle : Printable
[METHOD] Executing method: toString() with self reference
[METHOD] Method execution completed: returned "Circle"
```

**実行例**:
```bash
./main --debug interface_example.cb
```

### プライベートメソッド ✅

impl内でプライベートメソッドも実装可能です：

```cb
interface Calculator {
    int calculate(int a, int b);
};

impl Calculator for int {
    // プライベートメソッド
    private int validate_input(int value) {
        if (value < 0) {
            printf("Warning: negative value detected\n");
            return 0;
        }
        return value;
    }
    
    // パブリックメソッド
    int calculate(int a, int b) {
        int valid_a = self.validate_input(a);
        int valid_b = self.validate_input(b);
        return valid_a + valid_b;
    }
};
```

## サポートされる型 ✅

### プリミティブ型
- `int`, `long`, `short`, `tiny` - 整数型
- `string` - 文字列型
- `bool` - ブール型  
- `char` - 文字型

### 複合型
- **構造体型** (`struct`) - 完全対応・self参照可能
- **配列型** (1次元・多次元) - 配列要素アクセス対応
- **Typedef型** (プリミティブ・配列・構造体) - 独立impl実装
- **Union型** - Union型変数へのimpl実装

### 実用例

```cb
// Union型との組み合わせ
typedef StatusUnion = 200 | 404 | 500;

impl Printable for StatusUnion {
    string toString() {
        return "HTTP Status";
    }
    
    int getSize() {
        return 1;
    }
};

// Enum型との組み合わせ
enum Color {
    RED = 1,
    GREEN = 2,
    BLUE = 3
}

impl Printable for Color {
    string toString() {
        return "Color enum";
    }
    
    int getSize() {
        return 1;
    }
};

int main() {
    StatusUnion status = 200;
    Color color = RED;
    
    Printable p1 = status;
    Printable p2 = color;
    
    printf("Status: %s\n", p1.toString());
    printf("Color: %s\n", p2.toString());
    
    return 0;
}
```

## パフォーマンスと最適化 ✅

- **メソッド呼び出し**: 実行時にimpl定義を高速検索
- **型名マッチング**: ハッシュマップによる厳密で高速なマッチング
- **インターフェース変数**: 代入時の実装存在チェックで実行時エラーを防止
- **メモリ効率**: 型情報の効率的な管理とキャッシュ

## 制限事項

### 現在サポートされていない機能
- **インターフェース継承** (`interface A extends B`): 将来実装予定
- **ジェネリック型パラメータ**: 将来実装予定  
- **デフォルト実装**: 現在は全メソッドの実装が必須

### 回避策
```cb
// インターフェース継承の代替案
interface Printable {
    string toString();
}

interface ExtendedPrintable {
    string toString();      // 重複定義で代替
    string toDetailString(); // 追加メソッド
}
```

## まとめ ✅

Interface/Implシステムにより、Cbは以下を実現しています：

- ✅ **型安全なポリモーフィズム**: 実行時型チェックと安全なメソッド呼び出し
- ✅ **柔軟な型システム**: プリミティブ型から複合型まで統一的なインターフェース
- ✅ **構造化プログラミング**: オブジェクト指向的なコード設計の実現
- ✅ **高いパフォーマンス**: 効率的な実行時型解決とメソッド呼び出し
- ✅ **包括的エラー処理**: 開発時とランタイムでの詳細なエラー検出

これにより、Cbは実用的で型安全なシステムプログラミング言語として機能します。
