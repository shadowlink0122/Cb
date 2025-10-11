# コンストラクタ/デストラクタ機能仕様書 v2

**作成日**: 2025年10月11日  
**最終更新**: 2025年10月11日  
**対象バージョン**: v0.10.0  
**ステータス**: 📝 設計フェーズ

---

## 📋 概要

構造体に対するコンストラクタとデストラクタを実装し、オブジェクトのライフサイクル管理を提供します。

**設計原則**:
- ✅ **データと実装の完全分離**: 構造体 (`struct`) はデータのみ、`impl Struct {}` にコンストラクタ/デストラクタを定義
- ✅ **RAII (Resource Acquisition Is Initialization)**: リソースの自動管理
- ✅ **カプセル化**: `impl Struct {}` 内のメンバー変数・メソッドはすべて private
- ✅ **既存機能との統合**: 既存のimpl構造を拡張

---

## 🎯 目的

1. **リソース管理の自動化**: メモリや外部リソースの確保・解放を自動化
2. **初期化の保証**: オブジェクトが必ず適切に初期化される
3. **スコープベースの管理**: スコープ終了時の自動クリーンアップ
4. **カプセル化の実現**: private メンバーによる内部状態の隠蔽

---

## 📝 基本仕様

### 1. コンストラクタ

#### 定義方法

```cb
struct Point {
    int x;
    int y;
}

impl Point {
    // デフォルトコンストラクタ
    self() {
        self.x = 0;
        self.y = 0;
    }
    
    // パラメータ付きコンストラクタ
    self(int px, int py) {
        self.x = px;
        self.y = py;
    }
    
    // デストラクタ
    ~self() {
        println("Point destroyed: (", self.x, ", ", self.y, ")");
    }
}
```

**構文規則**:
- コンストラクタ名は `self`（固定）
- 引数の数や型が異なる複数のコンストラクタを定義可能（オーバーロード）
- 戻り値は指定しない
- `impl Struct {}` ブロック内でのみ定義可能

#### 呼び出し方法

```cb
void main() {
    // 方法1: 変数宣言時
    Point p1;              // デフォルトコンストラクタ → (0, 0)
    Point p2(10, 20);      // パラメータ付きコンストラクタ → (10, 20)
    
    // 方法2: 構造体名での一時オブジェクト生成
    Point p3 = Point();         // デフォルトコンストラクタで生成
    Point p4 = Point(30, 40);   // パラメータ付きで生成
}
```

**注意**: 
- `Point()` は一時オブジェクトを生成し、その後変数にコピーされます
- ムーブコンストラクタが定義されている場合は、ムーブされます

### 2. デストラクタ

#### 定義方法

```cb
struct Resource {
    int handle;
}

impl Resource {
    self(int h) {
        self.handle = h;
        println("Resource acquired: ", h);
    }
    
    ~self() {
        println("Resource released: ", self.handle);
        // リソース解放処理
    }
}
```

**構文規則**:
- デストラクタ名は `~self`（固定）
- 引数は取らない
- 戻り値は指定しない
- `impl Struct {}` ブロック内でのみ定義可能
- 1つの構造体に対して1つのみ定義可能

#### 呼び出しタイミング

| 状況 | 呼び出しタイミング | 順序 |
|------|-------------------|------|
| ローカル変数 | スコープ終了時 | 宣言の逆順 |
| 配列要素 | 配列破棄時 | インデックスの逆順 |
| 早期return | return実行時 | その時点までの変数を逆順 |

---

## 🔒 impl Struct {} の制約

### カプセル化規則

`impl Struct {}` は**コンストラクタとデストラクタ専用**のブロックです。

**許可される定義**:
- ✅ コンストラクタ（`self(...)`）
- ✅ デストラクタ（`~self()`）
- ✅ private メンバー変数
- ✅ private メソッド

**禁止される定義**:
- ❌ public メソッド（外部から `obj.method()` で呼び出せるメソッド）

### 理由

`impl Struct {}` の目的は、**オブジェクトの初期化と終了処理のカプセル化**です。通常のメソッドを定義すると、以下の問題が発生します：

1. **名前空間の汚染**: `obj.method()` で直接呼び出せるメソッドが増える
2. **設計の混乱**: コンストラクタ/デストラクタと通常のメソッドが混在
3. **既存のimplとの不整合**: `impl Interface for Struct` との役割が曖昧になる

### private メンバーとメソッド

```cb
struct BankAccount {
    string owner;  // public（外部からアクセス可能）
}

impl BankAccount {
    // private メンバー変数（外部からアクセス不可）
    private int balance;
    private int transaction_count;
    
    // コンストラクタ
    self(string name, int initial) {
        self.owner = name;
        balance = initial;           // private変数に直接アクセス可能
        transaction_count = 0;
        self.initialize();           // privateメソッド呼び出し
    }
    
    // デストラクタ
    ~self() {
        self.cleanup();              // privateメソッド呼び出し
    }
    
    // private メソッド（コンストラクタ/デストラクタからのみ呼び出し可能）
    private void initialize() {
        println("Initializing account for ", self.owner);
    }
    
    private void cleanup() {
        println("Cleaning up account for ", self.owner);
    }
}

void main() {
    BankAccount acc("Alice", 1000);
    
    // ✅ OK: public メンバーへのアクセス
    println(acc.owner);
    
    // ❌ NG: private メンバーへのアクセス
    // println(acc.balance);         // コンパイルエラー
    
    // ❌ NG: private メソッドの呼び出し
    // acc.initialize();             // コンパイルエラー
}
```

**private メソッドの呼び出し方**:
- コンストラクタ内: `self.privateMethod()`
- デストラクタ内: `self.privateMethod()`
- 他のprivateメソッド内: `self.privateMethod()`

### 通常のメソッドの定義方法

通常のメソッド（外部から呼び出せるメソッド）は、別の `impl` ブロックまたは `impl Interface for Struct` で定義します。

```cb
struct BankAccount {
    string owner;
}

// コンストラクタ/デストラクタ専用
impl BankAccount {
    private int balance;
    
    self(string name, int initial) {
        self.owner = name;
        balance = initial;
    }
    
    ~self() {
        println("Account closed");
    }
    
    // private メソッド（内部処理用）
    private void validate_amount(int amount) {
        if (amount < 0) {
            println("Warning: Negative amount");
        }
    }
}

// 通常のメソッド（interface実装）
interface BankOperations {
    void deposit(int amount);
    void withdraw(int amount);
    int getBalance();
}

impl BankOperations for BankAccount {
    void deposit(int amount) {
        // ❌ NG: implブロックのprivate変数には直接アクセス不可
        // balance = balance + amount;
        
        // ✅ OK: getter/setterパターンを使用
        // または、balance を public にする
        println("Deposited: ", amount);
    }
    
    void withdraw(int amount) {
        println("Withdrawn: ", amount);
    }
    
    int getBalance() {
        // ❌ NG: private変数にアクセスできない
        // return balance;
        return 0;  // 仮実装
    }
}

void main() {
    BankAccount acc("Alice", 1000);
    
    // ✅ OK: interface経由でメソッド呼び出し
    acc.deposit(500);
    acc.withdraw(300);
    println("Balance: ", acc.getBalance());
}
```

**問題点**: `impl BankAccount {}` の private 変数は、`impl BankOperations for BankAccount` からアクセスできません。

**解決策**:

#### オプション1: private変数をpublicにする

```cb
struct BankAccount {
    string owner;
    int balance;  // public（外部からアクセス可能）
}

impl BankAccount {
    self(string name, int initial) {
        self.owner = name;
        self.balance = initial;
    }
}

impl BankOperations for BankAccount {
    void deposit(int amount) {
        self.balance = self.balance + amount;  // OK
    }
    
    int getBalance() {
        return self.balance;  // OK
    }
}
```

#### オプション2: interface を使わずに通常の関数として定義

```cb
struct BankAccount {
    string owner;
}

impl BankAccount {
    private int balance;
    
    self(string name, int initial) {
        self.owner = name;
        balance = initial;
    }
}

// グローバル関数として定義
void deposit(BankAccount* acc, int amount) {
    // ❌ NG: private変数にアクセスできない
    // acc.balance = acc.balance + amount;
}
```

**推奨される設計**:
1. **本当にprivateにする必要がある変数**: `impl Struct {}` に定義し、コンストラクタ/デストラクタのみで使用
2. **外部から操作する必要がある変数**: `struct` に public メンバーとして定義
3. **メソッド**: `impl Interface for Struct` または通常の関数として定義

---

## 🔧 特殊なコンストラクタ

### 1. コピーコンストラクタ

オブジェクトのコピーが必要な場合に呼び出されます。

#### 定義方法

```cb
struct Point {
    int x;
    int y;
}

impl Point {
    // 通常のコンストラクタ
    self(int px, int py) {
        self.x = px;
        self.y = py;
    }
    
    // コピーコンストラクタ
    self(const Point& other) {
        self.x = other.x;
        self.y = other.y;
        println("Copy constructor called");
    }
}
```

**シグネチャ**: `self(const StructType& other)`
- const参照を引数に取る
- コピー元が変更されないことを保証

#### 呼び出しタイミング

```cb
void main() {
    Point p1(10, 20);
    
    // コピーコンストラクタが呼び出される状況
    Point p2 = p1;           // ❌ エラー: 構造体の直接代入は未サポート
    Point p3 = Point(p1);    // ✅ OK: 明示的なコピー（参照渡し）
}
```

**注意**: 現在のCb言語では、構造体の直接代入 (`p2 = p1`) は未実装です。コピーコンストラクタは明示的に呼び出す必要があります。

#### デフォルトコピーコンストラクタ

コピーコンストラクタが定義されていない場合、自動生成は**行いません**（現状の実装では未対応）。

### 2. ムーブコンストラクタ

一時オブジェクトからの効率的なリソース転送を実現します。

#### 定義方法（ポインタを使用）

```cb
struct DynamicArray {
    int* data;
    int size;
}

impl DynamicArray {
    // 通常のコンストラクタ
    self(int n) {
        self.size = n;
        // メモリ確保（仮想的な実装）
    }
    
    // ムーブコンストラクタ（参照を引数に取る）
    self(DynamicArray& other) {
        self.data = other.data;
        self.size = other.size;
        
        // リソースの所有権を移動（元のオブジェクトを無効化）
        other.data = nullptr;
        other.size = 0;
        
        println("Move constructor called");
    }
}
```

**シグネチャ**: `self(StructType& other)`
- 非const参照を引数に取る（ムーブ元を変更するため）
- コピーとの区別: `const&` がコピー、`&` がムーブ

**`&&` は使用しない**: Cb言語では右辺値参照をサポートしないため、通常の参照で代替します。

#### コピー vs ムーブの判定

```cb
impl DynamicArray {
    // コピーコンストラクタ（const参照）
    self(const DynamicArray& other) {
        // コピー処理：新しくメモリを確保
        self.size = other.size;
        self.data = copy_data(other.data, other.size);
    }
    
    // ムーブコンストラクタ（非const参照）
    self(DynamicArray& other) {
        // ムーブ処理：所有権を移動
        self.data = other.data;
        self.size = other.size;
        other.data = nullptr;
        other.size = 0;
    }
}
```

**判定規則**: 
- `const&` → コピーコンストラクタ
- `&` → ムーブコンストラクタ
- コンパイラが引数の型に応じて適切なコンストラクタを選択

### 3. コンストラクタの選択規則

複数のコンストラクタが定義されている場合、以下の優先順位で選択されます。

```cb
struct Example {
    int value;
}

impl Example {
    // 1. デフォルトコンストラクタ
    self() {
        self.value = 0;
    }
    
    // 2. パラメータ付きコンストラクタ
    self(int v) {
        self.value = v;
    }
    
    // 3. コピーコンストラクタ
    self(const Example& other) {
        self.value = other.value;
    }
    
    // 4. ムーブコンストラクタ
    self(Example& other) {
        self.value = other.value;
        other.value = 0;  // ムーブ元を無効化
    }
}

void main() {
    Example e1;              // 1. デフォルトコンストラクタ
    Example e2(42);          // 2. パラメータ付き
    Example e3 = Example(e2);   // 3. コピーコンストラクタ（const&）
    Example e4 = Example(e2);   // 4. ムーブコンストラクタ（&）の場合
}
```

**選択規則**:
1. 引数の型と数が完全に一致するコンストラクタを選択
2. 適切なコンストラクタが見つからない場合、コンパイルエラー

---

## 🔍 配列とコンストラクタ/デストラクタ

### 配列要素の初期化

```cb
struct Point {
    int x;
    int y;
}

impl Point {
    self() {
        self.x = 0;
        self.y = 0;
        println("Point constructed: (", self.x, ", ", self.y, ")");
    }
    
    self(int px, int py) {
        self.x = px;
        self.y = py;
        println("Point constructed: (", self.x, ", ", self.y, ")");
    }
    
    ~self() {
        println("Point destructed: (", self.x, ", ", self.y, ")");
    }
}

void main() {
    // 配列の初期化（デフォルトコンストラクタ呼び出し）
    Point[3] points;
    
    // 初期化リストを使った配列の初期化
    // 注意: 現在のCb言語では構造体の初期化リストは限定的
    Point[3] points2 = {
        Point(1, 2),
        Point(3, 4),
        Point(5, 6)
    };
}

// 期待される出力:
// Point constructed: (0, 0)
// Point constructed: (0, 0)
// Point constructed: (0, 0)
// Point constructed: (1, 2)
// Point constructed: (3, 4)
// Point constructed: (5, 6)
// Point destructed: (5, 6)
// Point destructed: (3, 4)
// Point destructed: (1, 2)
// Point destructed: (0, 0)
// Point destructed: (0, 0)
// Point destructed: (0, 0)
```

### 配列要素の破棄順序

配列要素は**インデックスの逆順**でデストラクタが呼び出されます。

---

## 🔀 スコープ管理

### ローカルスコープ

```cb
void main() {
    Resource r1(1);
    
    {
        Resource r2(2);
        {
            Resource r3(3);
        }  // r3 のデストラクタ呼び出し
        
        Resource r4(4);
    }  // r4, r2 のデストラクタ呼び出し（逆順）
    
    Resource r5(5);
}  // r5, r1 のデストラクタ呼び出し（逆順）
```

### 早期return

```cb
void test(bool early_exit) {
    Resource r1(1);
    Resource r2(2);
    
    if (early_exit) {
        Resource r3(3);
        return;  // r3, r2, r1 の順にデストラクタ呼び出し
    }
    
    Resource r4(4);
}  // r4, r2, r1 の順にデストラクタ呼び出し
```

---

## 🎓 実用例

### 例1: リソース管理

```cb
struct FileHandle {
    int fd;
    string filename;
}

impl FileHandle {
    self(string name) {
        self.filename = name;
        self.fd = 1;  // open(name)
        println("File opened: ", name);
    }
    
    ~self() {
        if (self.fd != 0) {
            println("File closed: ", self.filename);
        }
    }
    
    private void internal_flush() {
        // 内部処理
    }
}

// 外部から呼び出せるメソッドは別のinterfaceで定義
interface FileOperations {
    void write(string data);
    void read();
}

impl FileOperations for FileHandle {
    void write(string data) {
        println("Writing to ", self.filename, ": ", data);
    }
    
    void read() {
        println("Reading from ", self.filename);
    }
}

void processFile() {
    FileHandle file("data.txt");
    file.write("Hello, World!");
    // 関数終了時に自動的にファイルがクローズされる
}
```

### 例2: private メンバーによるカプセル化

```cb
struct Counter {
    string name;
}

impl Counter {
    private int count;
    private int max_count;
    
    self(string n, int max) {
        self.name = n;
        count = 0;
        max_count = max;
    }
    
    ~self() {
        println("Counter '", self.name, "' destroyed with final count: ", count);
    }
    
    private bool is_at_max() {
        return count >= max_count;
    }
}

// 外部インターフェース
interface CounterOps {
    void increment();
    int getCount();
}

impl CounterOps for Counter {
    void increment() {
        // ❌ NG: private変数にアクセスできない
        // if (!self.is_at_max()) {
        //     count = count + 1;
        // }
        
        // 代替案: public変数を使用
        println("Incrementing ", self.name);
    }
    
    int getCount() {
        // ❌ NG: private変数にアクセスできない
        // return count;
        return 0;
    }
}
```

---

## 🚨 nullptr の仕様

### nullptrとは

`nullptr` は**ポインタが無効であることを示す特殊な値**です。

**現在の実装**:
- `nullptr` は内部的に `0` として表現されます
- `PointerTargetType::NULLPTR_VALUE` という特殊な型で管理されます
- nullptrを間接参照すると実行時エラーが発生します

### nullptr の動作

```cb
void main() {
    int* ptr = nullptr;
    
    // ✅ OK: nullptrチェック
    if (ptr == nullptr) {
        println("Pointer is null");
    }
    
    // ❌ NG: nullptrの間接参照
    // int value = *ptr;  // 実行時エラー: "Cannot dereference nullptr"
    
    // ✅ OK: 有効なアドレスに変更
    int x = 42;
    ptr = &x;
    
    // ✅ OK: 有効なポインタの間接参照
    println(*ptr);  // 42
    
    // ✅ OK: nullptrに戻す
    ptr = nullptr;
}
```

### nullptr と変数の無効化

**nullptrはポインタ専用**で、通常の変数を「削除」する機能ではありません。

```cb
void main() {
    int x = 42;
    
    // ❌ エラー: 通常の変数にnullptrは代入できない
    // x = nullptr;
    
    // ✅ OK: ポインタにnullptrを代入
    int* ptr = &x;
    ptr = nullptr;  // ポインタが無効になる（xは影響を受けない）
    
    println(x);  // 42（xは有効）
}
```

### ムーブコンストラクタでのnullptr使用

```cb
struct Buffer {
    int* data;
    int size;
}

impl Buffer {
    self(int n) {
        // メモリ確保（仮想的）
        self.size = n;
    }
    
    // ムーブコンストラクタ
    self(Buffer& other) {
        self.data = other.data;
        self.size = other.size;
        
        // 元のオブジェクトを無効化（ポインタをnullにする）
        other.data = nullptr;
        other.size = 0;
    }
    
    ~self() {
        if (self.data != nullptr) {
            // メモリ解放（仮想的）
            println("Buffer destroyed");
        }
    }
}
```

**注意**: `other.data = nullptr` は、ポインタメンバーを無効化するだけで、オブジェクト自体は削除されません。

---

## 📐 まとめ

### 構文規則

| 項目 | 構文 | 例 |
|------|------|-----|
| コンストラクタ | `self(引数...)` | `self(int x)` |
| デフォルトコンストラクタ | `self()` | `self()` |
| コピーコンストラクタ | `self(const StructType& other)` | `self(const Point& p)` |
| ムーブコンストラクタ | `self(StructType& other)` | `self(Point& p)` |
| デストラクタ | `~self()` | `~self()` |
| privateメンバー | `private 型 名前;` | `private int count;` |
| privateメソッド | `private 戻り値 名前(引数...)` | `private void init()` |

### 呼び出し方法

| コード | 呼び出されるコンストラクタ |
|--------|--------------------------|
| `Point p;` | デフォルトコンストラクタ |
| `Point p(10, 20);` | パラメータ付きコンストラクタ |
| `Point p = Point();` | デフォルトコンストラクタ |
| `Point p = Point(10, 20);` | パラメータ付きコンストラクタ |
| `Point p = Point(&other);` | コピー/ムーブコンストラクタ |

### impl Struct {} の制約

| 許可 | 定義 |
|------|------|
| ✅ | コンストラクタ (`self(...)`) |
| ✅ | デストラクタ (`~self()`) |
| ✅ | privateメンバー変数 |
| ✅ | privateメソッド |
| ❌ | publicメソッド（外部から呼び出せるメソッド）|

**通常のメソッド**は `impl Interface for Struct` で定義してください。

---

## 📚 関連ドキュメント

- [実装計画](../todo/v0.10.0_constructor_implementation.md)
- [Cb言語仕様書](../spec.md)
- [v0.10.0実装ロードマップ](../todo/v0.10.0_implementation_plan.md)

---

**作成日**: 2025年10月11日  
**最終更新**: 2025年10月11日  
**ステータス**: 📝 設計フェーズ（仕様確定）
