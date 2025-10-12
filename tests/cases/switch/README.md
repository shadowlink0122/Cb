# Switch Statement Tests

switch文機能の包括的なテストケース集です。

## テストファイル一覧

### 基本機能
- **test_switch_basic.cb**: 基本的なswitch文のテスト
  - 単一値のマッチング
  - else節（default相当）

### OR演算子
- **test_switch_or.cb**: OR演算子（||）を使った複数値マッチング
  - `case (1 || 2 || 3)` のような条件

### 範囲演算子
- **test_switch_range.cb**: 範囲演算子（...）を使った範囲マッチング
  - `case (90...100)` のような閉区間での範囲指定
  - 範囲の境界値テスト

### 混合機能
- **test_switch_mixed.cb**: OR演算子と範囲演算子の組み合わせ
  - 単一値、OR条件、範囲条件が混在するswitch文

### 制御フロー
- **test_switch_return.cb**: switch文内でのreturn文
  - 関数内でのswitch文による分岐とreturn

### 複合条件（新規）
- **test_switch_complex.cb**: ||と...の複雑な組み合わせ
  - `case (1 || 2 || 3...5)` のような複合条件
  - `case (10...15 || 20)` のような範囲とOR値の混在

### 型システムとの統合（新規）
- **test_switch_typedef.cb**: typedef型を使ったswitch文
  - `typedef int Age;` などのカスタム型での条件判定
  - 型エイリアスの完全なサポート確認

- **test_switch_struct.cb**: 構造体メンバを使ったswitch文
  - `switch (student.score)` のようなメンバアクセス
  - 構造体を引数として受け取る関数内でのswitch

- **test_switch_enum.cb**: enum型を使ったswitch文
  - enum値での条件判定
  - enum値の範囲チェック

- **test_switch_array.cb**: 配列要素を使ったswitch文
  - 1次元配列要素での条件判定
  - 多次元配列要素での条件判定
  - ループ内での配列要素の動的判定

- **test_switch_nested.cb**: ネストしたswitch文
  - switch文の中にswitch文を配置
  - 複雑な条件分岐の処理

## switch文の仕様

### 基本構文
```cb
switch (expr) {
    case (value1) {
        // statements
    } case (value2) {
        // statements
    } else {
        // default case
    }
}
```

### 主な特徴
1. **OR演算子**: `case (2 || 3)` で複数の値をマッチング
2. **範囲演算子**: `case (10...20)` で範囲内の値をマッチング（閉区間）
3. **elseブロック**: `default`の代わりに`else`を使用
4. **自動break**: 各caseは自動的に終了（fallthrough無し）
5. **ブロック必須**: 各caseは`{}`ブロックで囲む必須

### 範囲演算子の詳細
- 閉区間：`10...20` は 10 以上 20 以下
- int型での数値範囲をサポート

### 自動break
各case節は自動的にbreakされるため、C言語のようなfallthrough（次のcaseへの継続実行）は発生しません。

## 期待される出力

### test_switch_basic.cb
```
=== Basic Switch Test ===
Two
Not one or two
=== Test completed ===
```

### test_switch_or.cb
```
=== Switch OR Test ===
One, Two or Three
Four or Five
Other
=== Test completed ===
```

### test_switch_range.cb
```
=== Switch Range Test ===
Grade A
Grade B
Grade B
Grade F
=== Test completed ===
```

### test_switch_mixed.cb
```
=== Switch Mixed Test ===
One
Two or Three
Ten to Twenty
Other
=== Test completed ===
```

### test_switch_return.cb
```
=== Switch Return Test ===
A
B
C
D
F
=== Test completed ===
```

### test_switch_complex.cb
```
=== Switch Complex Conditions Test ===
1, 2, or 3 to 5
10 to 15, or 20
10 to 15, or 20
Other
=== Test completed ===
```

### test_switch_typedef.cb
```
=== Switch Typedef Test ===
Child
Teenager
Adult
Senior
A
B
C
D
F
=== Test completed ===
```

### test_switch_struct.cb
```
=== Switch Struct Member Test ===
Alice score evaluation:
Excellent
Bob score evaluation:
Average
Charlie score evaluation:
Fail
Alice age check:
University
Bob age check:
University
Charlie age check:
Graduate
Direct member access:
Alice is university age
=== Test completed ===
```

### test_switch_enum.cb
```
=== Switch Enum Test ===
Red
Green
Yellow or Black
Waiting
In progress
Done
Error
Range check:
Primary color
=== Test completed ===
```

### test_switch_array.cb
```
=== Switch Array Element Test ===
Score at index:
0
Grade A
Score at index:
1
Grade B
Score at index:
2
Grade C
Score at index:
3
Grade D
Score at index:
4
Grade F
Multidimensional array test:
Small
Small
Small
Medium
Medium
Large
=== Test completed ===
```

### test_switch_nested.cb
```
=== Switch Nested Test ===
Category: Academic
Level: Good
Category: Sports
Level: Professional
=== Test completed ===
```

## 統合テスト

全テストケースは `tests/integration/switch/test_switch.hpp` に統合されており、
`make integration-test` コマンドで自動実行されます。

**統合テスト結果**: ✅ **11テスト全て成功**（86アサーション）
