# Cb言語 よくある間違いと解決方法

Cb言語を学習する際によく遭遇する間違いと、その解決方法をまとめました。

## 目次

1. [構文エラー](#1-構文エラー)
2. [型エラー](#2-型エラー)
3. [実行時エラー](#3-実行時エラー)
4. [ポインタ関連](#4-ポインタ関連)
5. [配列関連](#5-配列関連)
6. [関数関連](#6-関数関連)
7. [構造体関連](#7-構造体関連)
8. [パフォーマンス問題](#8-パフォーマンス問題)

---

## 1. 構文エラー

### セミコロン忘れ

**❌ 間違い**
```cb
int x = 10
int y = 20
```

**エラーメッセージ**
```
error: Expected ';' after statement
```

**✅ 正しい**
```cb
int x = 10;
int y = 20;
```

**解説**: Cbでは全ての文の末尾にセミコロンが必要です。

---

### 構造体定義後のセミコロン忘れ

**❌ 間違い**
```cb
struct Point {
    int x;
    int y;
}

int main() {
    return 0;
}
```

**エラーメッセージ**
```
error: Expected ';' after struct definition
```

**✅ 正しい**
```cb
struct Point {
    int x;
    int y;
};

int main() {
    return 0;
}
```

**解説**: 構造体定義の後には必ずセミコロンが必要です。

---

### 中括弧の不一致

**❌ 間違い**
```cb
int main() {
    if (true) {
        println("test");
    // 閉じ括弧忘れ
    return 0;
}
```

**エラーメッセージ**
```
error: Expected '}' to match '{'
```

**✅ 正しい**
```cb
int main() {
    if (true) {
        println("test");
    }
    return 0;
}
```

**ヒント**: インデントを正しく使うと、括弧の対応が見やすくなります。

---

## 2. 型エラー

### 型の不一致

**❌ 間違い**
```cb
int main() {
    int x = 42;
    string s = x;  // intをstringに代入できない
    return 0;
}
```

**エラーメッセージ**
```
error: Cannot assign 'int' to 'string'
```

**✅ 正しい**
```cb
int main() {
    int x = 42;
    string s = "42";  // 文字列リテラルを使用
    return 0;
}
```

---

### unsigned型への負値代入

**❌ 間違い（警告が出る）**
```cb
int main() {
    unsigned int x = -10;  // 負値は0にクランプされる
    println(x);  // 0
    return 0;
}
```

**警告メッセージ**
```
warning: Negative value clamped to 0 for unsigned type
```

**✅ 正しい**
```cb
int main() {
    unsigned int x = 10;  // 正の値を使用
    println(x);  // 10
    return 0;
}
```

---

### 配列の型不一致

**❌ 間違い**
```cb
int main() {
    int[5] arr1 = [1, 2, 3, 4, 5];
    int[10] arr2 = arr1;  // サイズが異なる
    return 0;
}
```

**エラーメッセージ**
```
error: Array size mismatch: expected 10, got 5
```

**✅ 正しい**
```cb
int main() {
    int[5] arr1 = [1, 2, 3, 4, 5];
    int[5] arr2 = arr1;  // 同じサイズ
    return 0;
}
```

---

### 文字列のnull終端文字忘れ

すべての文字列は`\0`(null終端文字)で終わる必要があります。

**❌ 間違い: null終端文字を忘れる**
```cb
void copy_string(string dest, string src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i = i + 1;
    }
    // dest[i] = '\0'; を忘れている!
}

int main() {
    string original = "Hello";
    string copy;
    copy_string(copy, original);
    println(copy);  // 不定動作: null終端文字がない
    return 0;
}
```

**✅ 正しい: null終端文字を追加**
```cb
void copy_string(string dest, string src) {
    int i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i = i + 1;
    }
    dest[i] = '\0';  // null終端文字を追加
}

int main() {
    string original = "Hello";
    string copy;
    copy_string(copy, original);
    println(copy);  // Hello
    return 0;
}
```

**重要**: 
- 文字列をコピーする際は必ずnull終端文字も含める
- 文字列の長さを計算する際はnull終端文字まで読む
- 文字列の比較も両方がnull終端文字に達したか確認する

---

### 文字列操作で配列境界を越える

**❌ 危険: バッファオーバーフロー**
```cb
void bad_string_copy(string dest, string src) {
    int i = 0;
    // destのサイズを考慮していない
    while (src[i] != '\0') {
        dest[i] = src[i];  // destが小さすぎると境界外アクセス
        i = i + 1;
    }
    dest[i] = '\0';
}

int main() {
    string small;  // サイズが不足している可能性
    string large = "This is a very long string";
    bad_string_copy(small, large);  // 危険!
    return 0;
}
```

**✅ 正しい: サイズを確認**
```cb
void safe_string_copy(string dest, string src, int max_size) {
    int i = 0;
    // max_size - 1 まで (null終端文字用に1つ残す)
    while (src[i] != '\0' && i < max_size - 1) {
        dest[i] = src[i];
        i = i + 1;
    }
    dest[i] = '\0';
}

int main() {
    string small;
    string large = "Hello";
    safe_string_copy(small, large, 100);  // 最大サイズを指定
    println(small);  // Hello
    return 0;
}
```

---

## 3. 実行時エラー

### 配列の境界外アクセス

**❌ 実行時エラー**
```cb
int main() {
    int[5] arr = [1, 2, 3, 4, 5];
    int x = arr[10];  // 範囲外
    return 0;
}
```

**エラーメッセージ**
```
error: Array index out of bounds: index 10, size 5
```

**✅ 正しい**
```cb
int main() {
    int[5] arr = [1, 2, 3, 4, 5];
    int x = arr[4];  // 有効な範囲（0-4）
    return 0;
}
```

**ヒント**: 配列のインデックスは0から始まり、サイズ-1までです。

---

### ゼロ除算

**❌ 実行時エラー**
```cb
int main() {
    int x = 100;
    int y = 0;
    int z = x / y;  // ゼロ除算
    return 0;
}
```

**エラーメッセージ**
```
error: Division by zero
```

**✅ 正しい**
```cb
int main() {
    int x = 100;
    int y = 0;
    
    if (y != 0) {
        int z = x / y;
        println(z);
    } else {
        println("Error: Cannot divide by zero");
    }
    
    return 0;
}
```

---

### nullポインタ参照

**❌ 実行時エラー**
```cb
int main() {
    int* ptr = nullptr;
    int x = *ptr;  // nullポインタ参照
    return 0;
}
```

**エラーメッセージ**
```
error: Null pointer dereference
```

**✅ 正しい**
```cb
int main() {
    int* ptr = nullptr;
    
    if (ptr != nullptr) {
        int x = *ptr;
        println(x);
    } else {
        println("Error: Pointer is null");
    }
    
    return 0;
}
```

---

## 4. ポインタ関連

### 未初期化ポインタの使用

**❌ 危険**
```cb
int main() {
    int* ptr;  // 未初期化
    *ptr = 42;  // 不定なアドレスへのアクセス
    return 0;
}
```

**✅ 正しい**
```cb
int main() {
    int value = 0;
    int* ptr = &value;  // 初期化
    *ptr = 42;
    println(*ptr);  // 42
    return 0;
}
```

---

### ローカル変数へのポインタを返す

**❌ 危険**
```cb
int* get_pointer() {
    int local = 42;
    return &local;  // ローカル変数のアドレスを返す（危険）
}

int main() {
    int* ptr = get_pointer();
    int x = *ptr;  // 不正なメモリアクセス
    return 0;
}
```

**✅ 正しい（代替案1: 値を返す）**
```cb
int get_value() {
    int local = 42;
    return local;  // 値をコピーして返す
}

int main() {
    int x = get_value();
    println(x);  // 42
    return 0;
}
```

**✅ 正しい（代替案2: 参照渡し）**
```cb
void set_value(int& ref) {
    ref = 42;
}

int main() {
    int x = 0;
    set_value(x);
    println(x);  // 42
    return 0;
}
```

---

### const違反

**❌ エラー**
```cb
int main() {
    int value = 42;
    const int* ptr = &value;
    *ptr = 100;  // constポインタ経由の変更は禁止
    return 0;
}
```

**エラーメッセージ**
```
error: Cannot modify through const pointer
```

**✅ 正しい**
```cb
int main() {
    int value = 42;
    const int* ptr = &value;
    int x = *ptr;  // 読み取りはOK
    println(x);    // 42
    
    value = 100;   // 元の変数の変更はOK
    return 0;
}
```

---

### const変数へのポインタの制約

**❌ エラー: const変数への非constポインタ**
```cb
int main() {
    const int value = 42;
    int* ptr = &value;  // エラー: const変数への非constポインタは禁止
    *ptr = 100;         // これを許すとconst制約が破られる
    return 0;
}
```

**エラーメッセージ**
```
error: Cannot take non-const pointer to const variable
```

**理由**: const変数の値は外部から変更されてはいけません。非constポインタを許すと、そのポインタ経由でconst変数の値を変更できてしまい、const制約が無意味になります。

**✅ 正しい: constポインタを使用**
```cb
int main() {
    const int value = 42;
    const int* ptr = &value;  // OK: constポインタなら許可
    int x = *ptr;             // 読み取りはOK
    println(x);               // 42
    // *ptr = 100;            // エラー: constポインタ経由の変更は禁止
    return 0;
}
```

---

### *constポインタ(アドレス変更不可)

`*const`は「ポインタ自体が定数」を意味し、ポインタが指すアドレスを変更できません。

**❌ エラー: *constポインタのアドレス変更**
```cb
int main() {
    int a = 10;
    int b = 20;
    int *const ptr = &a;  // アドレス変更不可
    *ptr = 15;            // OK: 値の変更は可能
    ptr = &b;             // エラー: アドレスの変更は禁止
    return 0;
}
```

**エラーメッセージ**
```
error: Cannot reassign const pointer
```

**✅ 正しい**
```cb
int main() {
    int a = 10;
    int b = 20;
    int *const ptr = &a;  // アドレス変更不可
    *ptr = 15;            // OK: 値の変更は可能
    println(*ptr);        // 15
    
    // 新しいポインタが必要な場合は別の変数を使う
    int* ptr2 = &b;
    println(*ptr2);       // 20
    return 0;
}
```

**const修飾子の組み合わせ**
```cb
int main() {
    int value = 42;
    const int value2 = 100;
    
    // パターン1: 値が定数
    const int* ptr1 = &value;
    // ptr1 = &value2;  // OK: アドレス変更可能
    // *ptr1 = 50;      // エラー: 値の変更は不可
    
    // パターン2: アドレスが定数
    int *const ptr2 = &value;
    // ptr2 = &value;   // エラー: アドレス変更不可
    // *ptr2 = 50;      // OK: 値の変更は可能
    
    // パターン3: 値もアドレスも定数
    const int *const ptr3 = &value2;
    // ptr3 = &value;   // エラー: アドレス変更不可
    // *ptr3 = 50;      // エラー: 値の変更も不可
    
    return 0;
}
```

---

## 5. 配列関連

### 配列サイズの不一致

**❌ 間違い**
```cb
int main() {
    int[5] arr = [1, 2, 3];  // 要素数が足りない
    return 0;
}
```

**エラーメッセージ**
```
error: Array initializer size mismatch
```

**✅ 正しい**
```cb
int main() {
    int[5] arr = [1, 2, 3, 4, 5];  // 正確に5個
    return 0;
}
```

---

### 多次元配列の初期化ミス

**❌ 間違い**
```cb
int main() {
    int[2][3] matrix = [1, 2, 3, 4, 5, 6];  // フラットな配列
    return 0;
}
```

**✅ 正しい**
```cb
int main() {
    int[2][3] matrix = [
        [1, 2, 3],
        [4, 5, 6]
    ];
    return 0;
}
```

---

## 6. 関数関連

### return文の忘れ

**❌ 間違い**
```cb
int get_value() {
    int x = 42;
    // returnを忘れた
}

int main() {
    int value = get_value();  // 戻り値が不定
    return 0;
}
```

**エラーメッセージ**
```
error: Missing return statement in non-void function
```

**✅ 正しい**
```cb
int get_value() {
    int x = 42;
    return x;
}

int main() {
    int value = get_value();
    println(value);  // 42
    return 0;
}
```

---

### 引数の型不一致

**❌ 間違い**
```cb
void print_int(int x) {
    println(x);
}

int main() {
    string s = "hello";
    print_int(s);  // stringをintに渡せない
    return 0;
}
```

**エラーメッセージ**
```
error: Type mismatch in function call: expected 'int', got 'string'
```

**✅ 正しい**
```cb
void print_int(int x) {
    println(x);
}

int main() {
    int i = 42;
    print_int(i);
    return 0;
}
```

---

## 7. 構造体関連

### メンバーの初期化忘れ

**❌ 問題のあるコード**
```cb
struct Point {
    int x;
    int y;
};

int main() {
    Point p;
    println(p.x);  // 未初期化の値
    return 0;
}
```

**✅ 正しい**
```cb
struct Point {
    int x;
    int y;
};

int main() {
    Point p;
    p.x = 0;  // 明示的に初期化
    p.y = 0;
    println(p.x);  // 0
    return 0;
}
```

---

### 構造体のコピー

**⚠️ 注意が必要**
```cb
struct Data {
    int[100] values;
};

int main() {
    Data d1;
    Data d2 = d1;  // 大きな構造体のコピー（遅い）
    return 0;
}
```

**✅ 推奨（参照渡し）**
```cb
struct Data {
    int[100] values;
};

void process_data(Data& d) {
    // 参照渡しでコピーを避ける
    d.values[0] = 42;
}

int main() {
    Data d;
    process_data(d);
    return 0;
}
```

---

## 8. パフォーマンス問題

### 不要なコピー

**❌ 非効率**
```cb
int sum_array(int[1000] arr) {  // 配列全体をコピー
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += arr[i];
    }
    return sum;
}
```

**✅ 効率的**
```cb
int sum_array(int[1000]& arr) {  // 参照渡し（コピーなし）
    int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += arr[i];
    }
    return sum;
}
```

---

### 再帰の深さ

**❌ スタックオーバーフローの危険**
```cb
int bad_recursion(int n) {
    return bad_recursion(n - 1);  // 終了条件がない
}
```

**✅ 正しい**
```cb
int factorial(int n) {
    if (n <= 1) {
        return 1;  // 終了条件
    }
    return n * factorial(n - 1);
}
```

---

## デバッグのコツ

### 1. エラーメッセージを読む

エラーメッセージには以下の情報が含まれます：
- **ファイル名と行番号**: エラーの場所
- **エラーの種類**: 何が問題なのか
- **コード抜粋**: エラー箇所の前後

例：
```
test.cb:3:13: error: Undefined variable 'unknown_var'
   2 |     int x = 10;
   3 |     int y = unknown_var;
     |             ^
   4 |     return 0;
```

### 2. デバッグモードを使う

```bash
# 英語デバッグ
./main --debug program.cb

# 日本語デバッグ
./main --debug-ja program.cb
```

### 3. 小さくテストする

- 大きなプログラムを小さな部分に分割
- 各部分を個別にテスト
- 動作を確認してから統合

### 4. println()でデバッグ

```cb
int main() {
    int x = 10;
    println("Debug: x =", x);  // 変数の値を確認
    
    x = x * 2;
    println("Debug: after multiplication, x =", x);
    
    return 0;
}
```

---

## まとめ

### よくある間違いトップ5

1. **セミコロン忘れ** → 文末には必ずセミコロン
2. **配列の境界外アクセス** → インデックスの範囲を確認
3. **型の不一致** → 変数の型を意識する
4. **return文の忘れ** → 戻り値のある関数は必ずreturn
5. **未初期化変数** → 変数は使用前に初期化

### トラブルシューティングの流れ

1. エラーメッセージを読む
2. 該当行を確認する
3. 似た動作する例と比較する
4. デバッグモードで実行する
5. 小さな例で再現する

### 参考資料

- [基本構文ガイド](basic_syntax_guide.md)
- [サンプルコード集](sample_code_collection.md)
- [言語仕様書](../spec.md)

エラーは学習の機会です。諦めずにトライしましょう! 💪
