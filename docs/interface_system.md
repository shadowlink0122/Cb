# Interface/Impl System Documentation

Cbプログラミング言語のInterface/Implシステムの完全なガイドです。

## 概要

Interface/Implシステムは、型に対してメソッドを定義し実装する機能です。これにより以下が可能になります：

- 構造体、プリミティブ型、typedef型に対するメソッドの定義
- インターフェース変数による抽象化
- 型安全なポリモーフィズム
- 再帰的typedefの独立した実装

## 基本的な使用法

### 1. インターフェースの定義

```cb
interface Printable {
    string toString();
    int getSize();
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
        return 2;
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
        return self;
    }
    
    int getSize() {
        return 10; // 実装例
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
```

## 高度な機能

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
    println(p3.toString()); // "INT3 implementation"
    
    // これらは失敗する（コンパイル時またはランタイムエラー）
    // Printable p_orig = original; // Error: No impl found for interface 'Printable' with type 'int'
    // Printable p1 = int1;        // Error: No impl found for interface 'Printable' with type 'INT'
    // Printable p2 = int2;        // Error: No impl found for interface 'Printable' with type 'INT2'
    
    return 0;
}
```

### インターフェース変数

インターフェース型の変数を使用して、異なる型を統一的に扱えます：

```cb
int main() {
    // 異なる型でも同じインターフェース変数に代入可能
    MyInt mi = 42;
    MyString ms = "test";
    IntArray arr = [1, 2, 3, 4, 5];
    
    Printable p1 = mi;
    Printable p2 = ms;
    Printable p3 = arr;
    
    // 統一的なメソッド呼び出し
    println("MyInt: %s", p1.toString());      // "MyInt value"
    println("MyString: %s", p2.toString());   // "MyString value"
    println("IntArray: %s", p3.toString());   // "IntArray[5]"
    
    return 0;
}
```

## サポートされる型

### プリミティブ型
- `int`, `long`, `short`, `tiny` - 整数型
- `string` - 文字列型
- `bool` - ブール型
- `char` - 文字型

### 複合型
- 構造体型 (`struct`)
- 配列型 (1次元・多次元)
- Typedef型 (プリミティブ・配列・構造体)
- Union型

### 配列型の例

```cb
impl Printable for int[] {
    string toString() {
        return "int array";
    }
    
    int getSize() {
        return 10; // 配列要素数を取得する実装
    }
};

// 多次元配列
impl Printable for int[2][3] {
    string toString() {
        return "2x3 matrix";
    }
    
    int getSize() {
        return 6; // 2 * 3
    }
};
```

## エラー処理

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

## パフォーマンスの考慮

- メソッド呼び出しは実行時にimpl定義を検索します
- 型名による厳密なマッチングが行われます
- インターフェース変数の代入時に実装存在チェックが行われます

## デバッグ

デバッグモード（`--debug`フラグ）で以下の情報が出力されます：

- インターフェース変数の代入: `[INTERFACE] Assigning struct to interface variable`
- メソッド呼び出し: `[METHOD] Method call started`, `[METHOD] Interface method call`
- 実装解決: `[METHOD] Executing method`

## 制限事項

### 現在サポートされていない機能
- プライベートメソッド（実装予定）
- ジェネリック型パラメータ
- 継承関係のあるインターフェース

### 型制限
- 関数ポインタに対するimpl実装は未サポート
- 一部の複合型（union内の動的型など）

## まとめ

Interface/Implシステムにより、Cbは強力な型システムと柔軟なメソッド定義機能を提供します。これにより、構造化プログラミングとオブジェクト指向的なコード設計が可能になります。
