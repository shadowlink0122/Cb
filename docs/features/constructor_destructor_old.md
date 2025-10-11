# コンストラクタ/デストラクタ機能仕様書

**作成日**: 2025年10月11日  
**対象バージョン**: v0.10.0  
**ステータス**: 📝 設計フェーズ

---

## 📋 概要

構造体に対するコンストラクタとデストラクタを実装し、オブジェクトのライフサイクル管理を提供します。

**設計原則**:
- ✅ **データと実装の分離**: 構造体 (`struct`) はデータのみ、`impl` ブロックにコンストラクタ/デストラクタを定義
- ✅ **RAII (Resource Acquisition Is Initialization)**: リソースの自動管理
- ✅ **既存機能との統合**: 既存のimpl構造を拡張

---

## 🎯 目的

1. **リソース管理の自動化**: メモリや外部リソースの確保・解放を自動化
2. **初期化の保証**: オブジェクトが必ず適切に初期化される
3. **スコープベースの管理**: スコープ終了時の自動クリーンアップ
4. **オブジェクト指向機能の基盤**: クラスライクな機能の基礎を提供

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
}
```

**構文規則**:
- コンストラクタ名は `self`
- 引数の数や型が異なる複数のコンストラクタを定義可能（オーバーロード）
- 戻り値は指定しない
- `impl` ブロック内でのみ定義可能

#### 呼び出し方法

```cb
void main() {
    // 方法1: 変数宣言と同時に呼び出し
    Point p1;              // デフォルトコンストラクタ → (0, 0)
    Point p2(10, 20);      // パラメータ付きコンストラクタ → (10, 20)
    
    // 方法2: 一時オブジェクトの生成
    Point p3 = Point();         // デフォルトコンストラクタで生成+コピー
    Point p4 = Point(30, 40);   // パラメータ付きで生成+コピー
}
```

#### 呼び出しタイミング

| 状況 | 呼び出しタイミング | 例 |
|------|-------------------|-----|
| ローカル変数宣言 | 変数宣言時 | `Point p;` |
| パラメータ付き宣言 | 変数宣言時 | `Point p(10, 20);` |
| 配列要素 | 各要素の初期化時 | `Point[3] arr;` |
| 一時オブジェクト | 式の評価時 | `func(Point(1, 2));` |
| 戻り値 | return文実行時 | `return Point(x, y);` |

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
- デストラクタ名は `~self`
- 引数は取らない
- 戻り値は指定しない
- `impl` ブロック内でのみ定義可能
- 1つの構造体に対して1つのみ定義可能

#### 呼び出しタイミング

| 状況 | 呼び出しタイミング | 順序 |
|------|-------------------|------|
| ローカル変数 | スコープ終了時 | 宣言の逆順 |
| 配列要素 | 配列破棄時 | インデックスの逆順 |
| 早期return | return実行時 | その時点までの変数を逆順 |
| 例外処理 | スコープ脱出時 | 宣言の逆順 |

#### 実行順序の例

```cb
void main() {
    Resource r1(1);
    Resource r2(2);
    {
        Resource r3(3);
        Resource r4(4);
    }  // ここで r4, r3 の順にデストラクタ呼び出し
    Resource r5(5);
}  // ここで r5, r2, r1 の順にデストラクタ呼び出し

// 出力:
// Resource acquired: 1
// Resource acquired: 2
// Resource acquired: 3
// Resource acquired: 4
// Resource released: 4
// Resource released: 3
// Resource acquired: 5
// Resource released: 5
// Resource released: 2
// Resource released: 1
```

### 3. selfキーワード

#### 仕様

`self` は `impl` ブロック内のコンストラクタ、デストラクタ、メソッドで使用可能な特殊な識別子です。

- **コンテキスト**: 現在の構造体インスタンスを参照
- **型**: 構造体型の参照（暗黙的）
- **用途**: メンバーアクセス、メンバー関数呼び出し

#### 使用例

```cb
struct Counter {
    int value;
}

impl Counter {
    self(int initial) {
        self.value = initial;  // メンバーへのアクセス
    }
    
    void increment() {
        self.value = self.value + 1;
    }
    
    void reset() {
        self.value = 0;
    }
    
    int getValue() {
        return self.value;
    }
}
```

**注意事項**:
- `self` はコンストラクタ/デストラクタ/メソッド内でのみ有効
- グローバルスコープや通常の関数では使用不可
- 既存の `impl Interface for Struct` パターンとの互換性を保つ

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
    self(const Point other) {
        self.x = other.x;
        self.y = other.y;
        println("Copy constructor called");
    }
}
```

#### 呼び出しタイミング

```cb
void main() {
    Point p1(10, 20);
    
    // コピーコンストラクタが呼び出される状況
    Point p2 = p1;           // 変数の初期化
    Point p3(p1);            // 明示的なコピー
    
    func(p1);                // 値渡しの引数
    Point p4 = returnPoint(); // 戻り値の受け取り
}

Point returnPoint() {
    Point p(5, 10);
    return p;  // コピーコンストラクタ呼び出し
}

void func(Point p) {  // コピーコンストラクタ呼び出し
    // ...
}
```

#### デフォルトコピーコンストラクタ

コピーコンストラクタが定義されていない場合、コンパイラが自動的に生成します（shallow copy）。

```cb
// ユーザー定義なし → 自動生成される
struct Simple {
    int x;
    int y;
}

impl Simple {
    self(int px, int py) {
        self.x = px;
        self.y = py;
    }
    // コピーコンストラクタ未定義 → 自動生成（メンバーごとのコピー）
}
```

### 2. ムーブコンストラクタ

一時オブジェクトからの効率的なリソース転送を実現します。

#### 定義方法

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
        // self.data = allocate(n);
    }
    
    // ムーブコンストラクタ
    self(DynamicArray&& other) {
        self.data = other.data;
        self.size = other.size;
        
        // リソースの所有権を移動（元のオブジェクトは無効化）
        other.data = nullptr;
        other.size = 0;
        
        println("Move constructor called");
    }
    
    ~self() {
        if (self.data != nullptr) {
            // メモリ解放
            // deallocate(self.data);
        }
    }
}
```

#### 呼び出しタイミング

```cb
DynamicArray createArray() {
    DynamicArray arr(100);
    return arr;  // ムーブコンストラクタ呼び出し（最適化）
}

void main() {
    DynamicArray arr1 = createArray();  // ムーブコンストラクタ
    
    DynamicArray arr2 = DynamicArray(50);  // 一時オブジェクトからムーブ
}
```

**ムーブセマンティクスの利点**:
- 不要なコピーを削減
- 大きなオブジェクトのパフォーマンス向上
- リソースの所有権管理を明確化

### 3. コンストラクタの優先順位

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
    self(const Example other) {
        self.value = other.value;
    }
    
    // 4. ムーブコンストラクタ
    self(Example&& other) {
        self.value = other.value;
        other.value = 0;
    }
}

void main() {
    Example e1;           // 1. デフォルトコンストラクタ
    Example e2(42);       // 2. パラメータ付き
    Example e3 = e2;      // 3. コピーコンストラクタ
    Example e4 = Example(10);  // 4. ムーブコンストラクタ
}
```

**選択規則**:
1. 引数の型と数が完全に一致するコンストラクタを選択
2. 一時オブジェクトの場合、ムーブコンストラクタを優先
3. 左辺値の場合、コピーコンストラクタを選択
4. 適切なコンストラクタが見つからない場合、コンパイルエラー

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
    // 配列の初期化
    Point[3] points;  // 各要素でデフォルトコンストラクタ呼び出し
    
    // 初期化リストを使った配列の初期化
    Point[3] points2 = {
        Point(1, 2),
        Point(3, 4),
        Point(5, 6)
    };
}

// 出力:
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

```cb
void main() {
    Point[5] arr = {
        Point(0, 0),
        Point(1, 1),
        Point(2, 2),
        Point(3, 3),
        Point(4, 4)
    };
}  // arr[4], arr[3], arr[2], arr[1], arr[0] の順にデストラクタ呼び出し
```

### 多次元配列

```cb
void main() {
    Point[2][3] matrix;
    // matrix[0][0], matrix[0][1], matrix[0][2],
    // matrix[1][0], matrix[1][1], matrix[1][2] の順にコンストラクタ呼び出し
    
    // 破棄は逆順:
    // matrix[1][2], matrix[1][1], matrix[1][0],
    // matrix[0][2], matrix[0][1], matrix[0][0]
}
```

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

// 出力:
// Resource acquired: 1
// Resource acquired: 2
// Resource acquired: 3
// Resource released: 3
// Resource acquired: 4
// Resource released: 4
// Resource released: 2
// Resource acquired: 5
// Resource released: 5
// Resource released: 1
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

void main() {
    test(true);
    println("---");
    test(false);
}

// 出力:
// Resource acquired: 1
// Resource acquired: 2
// Resource acquired: 3
// Resource released: 3
// Resource released: 2
// Resource released: 1
// ---
// Resource acquired: 1
// Resource acquired: 2
// Resource acquired: 4
// Resource released: 4
// Resource released: 2
// Resource released: 1
```

### ループ内のスコープ

```cb
void main() {
    for (int i = 0; i < 3; i++) {
        Resource r(i);
        println("Loop iteration: ", i);
    }  // 各イテレーション終了時にデストラクタ呼び出し
}

// 出力:
// Resource acquired: 0
// Loop iteration: 0
// Resource released: 0
// Resource acquired: 1
// Loop iteration: 1
// Resource released: 1
// Resource acquired: 2
// Loop iteration: 2
// Resource released: 2
```

---

## 🎓 実用例

### 例1: スマートポインタ風のリソース管理

```cb
struct FileHandle {
    int fd;
    string filename;
}

impl FileHandle {
    self(string name) {
        self.filename = name;
        // ファイルオープン（仮想的な実装）
        self.fd = 1;  // open(name)
        println("File opened: ", name);
    }
    
    ~self() {
        if (self.fd != 0) {
            // ファイルクローズ（仮想的な実装）
            // close(self.fd)
            println("File closed: ", self.filename);
        }
    }
    
    void write(string data) {
        println("Writing to ", self.filename, ": ", data);
    }
}

void processFile() {
    FileHandle file("data.txt");
    file.write("Hello, World!");
    // 関数終了時に自動的にファイルがクローズされる
}

void main() {
    processFile();
    println("Done");
}

// 出力:
// File opened: data.txt
// Writing to data.txt: Hello, World!
// File closed: data.txt
// Done
```

### 例2: ロギング

```cb
struct Logger {
    string context;
    int depth;
}

impl Logger {
    self(string ctx, int d) {
        self.context = ctx;
        self.depth = d;
        self.printIndent();
        println("Entering: ", ctx);
    }
    
    ~self() {
        self.printIndent();
        println("Leaving: ", self.context);
    }
    
    void printIndent() {
        for (int i = 0; i < self.depth; i++) {
            print("  ");
        }
    }
}

void innerFunction() {
    Logger log("innerFunction", 2);
    println("    Processing...");
}

void outerFunction() {
    Logger log("outerFunction", 1);
    innerFunction();
}

void main() {
    Logger log("main", 0);
    outerFunction();
}

// 出力:
// Entering: main
//   Entering: outerFunction
//     Entering: innerFunction
//       Processing...
//     Leaving: innerFunction
//   Leaving: outerFunction
// Leaving: main
```

### 例3: カウンター

```cb
struct InstanceCounter {
    int id;
}

impl InstanceCounter {
    private static int counter = 0;
    
    self() {
        counter = counter + 1;
        self.id = counter;
        println("Instance created: #", self.id);
    }
    
    ~self() {
        println("Instance destroyed: #", self.id);
        counter = counter - 1;
    }
    
    static int getCount() {
        return counter;
    }
}

void main() {
    println("Active instances: ", InstanceCounter.getCount());
    
    InstanceCounter obj1;
    println("Active instances: ", InstanceCounter.getCount());
    
    {
        InstanceCounter obj2;
        InstanceCounter obj3;
        println("Active instances: ", InstanceCounter.getCount());
    }
    
    println("Active instances: ", InstanceCounter.getCount());
}

// 出力:
// Active instances: 0
// Instance created: #1
// Active instances: 1
// Instance created: #2
// Instance created: #3
// Active instances: 3
// Instance destroyed: #3
// Instance destroyed: #2
// Active instances: 1
// Instance destroyed: #1
```

---

## 🚨 制約と注意事項

### 1. デストラクタ内での例外

デストラクタ内では例外をスローしてはいけません（将来実装時の注意事項）。

```cb
struct Bad {
    int value;
}

impl Bad {
    ~self() {
        // ❌ NG: デストラクタ内でエラーを発生させない
        // if (self.value < 0) {
        //     throw Error("Invalid value");
        // }
        
        // ✅ OK: エラーをログに記録
        if (self.value < 0) {
            println("Warning: Invalid value in destructor");
        }
    }
}
```

### 2. 循環参照

相互に参照し合う構造体は注意が必要です。

```cb
struct Node {
    Node* next;
    int value;
}

impl Node {
    self(int v) {
        self.value = v;
        self.next = nullptr;
    }
    
    ~self() {
        // 注意: 循環参照がある場合、デストラクタが無限ループする可能性
        // if (self.next != nullptr) {
        //     delete self.next;  // 危険！
        // }
    }
}
```

### 3. デフォルトメンバーとの併用

`default` メンバーを持つ構造体では、コンストラクタ内で明示的に初期化する必要があります。

```cb
struct Wrapper {
    default int value;
}

impl Wrapper {
    self() {
        self.value = 0;  // default メンバーも明示的に初期化
    }
    
    self(int v) {
        self.value = v;
    }
}
```

### 4. interface実装との関係

`impl Interface for Struct` と `impl Struct` は併用可能ですが、同じ `impl` ブロック内には記述できません。

```cb
interface Printable {
    string toString();
}

struct Point {
    int x;
    int y;
}

// ✅ OK: 別々のimplブロック
impl Point {
    self(int px, int py) {
        self.x = px;
        self.y = py;
    }
    
    ~self() {
        println("Point destroyed");
    }
}

impl Printable for Point {
    string toString() {
        return "Point";
    }
}

// ❌ NG: 同じimplブロックには記述不可
// impl Printable for Point {
//     self(int px, int py) { }  // エラー
//     string toString() { }
// }
```

---

## 🔧 実装アーキテクチャ

### AST拡張

```cpp
// src/common/ast.h

enum class ASTNodeType {
    // ... 既存のノードタイプ ...
    AST_CONSTRUCTOR_DECL,  // コンストラクタ宣言
    AST_DESTRUCTOR_DECL,   // デストラクタ宣言
    AST_COPY_CONSTRUCTOR,  // コピーコンストラクタ
    AST_MOVE_CONSTRUCTOR,  // ムーブコンストラクタ
};

// impl定義の拡張
struct ImplDefinition {
    std::string interface_name;  // 空の場合、構造体用impl
    std::string struct_name;
    
    std::vector<const ASTNode*> constructors;  // NEW: コンストラクタ
    const ASTNode* destructor;                 // NEW: デストラクタ
    std::vector<const ASTNode*> methods;
    std::vector<const ASTNode*> static_vars;
    std::vector<const ASTNode*> private_members;  // NEW: private変数
};

// コンストラクタノード
struct ConstructorNode : public ASTNode {
    std::string struct_name;
    std::vector<std::unique_ptr<ASTNode>> parameters;
    std::unique_ptr<ASTNode> body;
    bool is_copy_constructor;
    bool is_move_constructor;
};

// デストラクタノード
struct DestructorNode : public ASTNode {
    std::string struct_name;
    std::unique_ptr<ASTNode> body;
};
```

### パーサー拡張

```cpp
// src/frontend/recursive_parser/parsers/interface_parser.cpp

ASTNode* InterfaceParser::parseImplDeclaration() {
    // "impl" キーワードの処理
    
    // パターン1: impl Struct { }
    // パターン2: impl Interface for Struct { }
    
    if (/* パターン1: 構造体用impl */) {
        return parseStructImpl();
    } else {
        return parseInterfaceImpl();
    }
}

ASTNode* InterfaceParser::parseStructImpl() {
    std::string struct_name = parseIdentifier();
    
    // impl ブロックの解析
    while (!isEndOfImpl()) {
        if (isConstructor()) {
            auto ctor = parseConstructor(struct_name);
            impl_def.constructors.push_back(ctor);
        } else if (isDestructor()) {
            auto dtor = parseDestructor(struct_name);
            impl_def.destructor = dtor;
        } else if (isPrivateMember()) {
            auto priv = parsePrivateMember();
            impl_def.private_members.push_back(priv);
        } else {
            auto method = parseMethod();
            impl_def.methods.push_back(method);
        }
    }
}

bool InterfaceParser::isConstructor() {
    // "self" "(" のパターンを検出
    return current_token->type == TokenType::IDENTIFIER &&
           current_token->value == "self" &&
           peek()->type == TokenType::LPAREN;
}

bool InterfaceParser::isDestructor() {
    // "~self" "(" のパターンを検出
    return current_token->type == TokenType::TILDE &&
           peek()->type == TokenType::IDENTIFIER &&
           peek()->value == "self";
}
```

### インタプリタ拡張

```cpp
// src/backend/interpreter/core/interpreter.cpp

class Interpreter {
private:
    // スコープ管理の拡張
    struct ScopeInfo {
        std::map<std::string, Variable> variables;
        std::vector<std::string> destruction_order;  // デストラクタ呼び出し順序
    };
    
    std::vector<ScopeInfo> scope_stack_;
    
    // コンストラクタ/デストラクタ関連
    void call_constructor(Variable& instance, 
                         const ASTNode* ctor_node,
                         const std::vector<Variable>& args);
    
    void call_destructor(Variable& instance);
    
    const ASTNode* find_constructor(const std::string& struct_name,
                                   const std::vector<Variable>& args);
    
public:
    // スコープ管理
    void enter_scope();
    void exit_scope();  // デストラクタを自動呼び出し
    
    // 変数宣言の拡張
    void declare_struct_variable(const ASTNode* decl_node);
};

void Interpreter::declare_struct_variable(const ASTNode* decl_node) {
    // 変数の作成
    Variable var;
    var.type = struct_type;
    var.struct_name = decl_node->type_name;
    
    // コンストラクタの検索
    auto ctor = find_constructor(var.struct_name, decl_node->arguments);
    
    if (ctor) {
        // コンストラクタ呼び出し
        call_constructor(var, ctor, decl_node->arguments);
    }
    
    // スコープに変数追加 + 破棄順序に登録
    current_scope().variables[var_name] = var;
    current_scope().destruction_order.push_back(var_name);
}

void Interpreter::exit_scope() {
    auto& scope = current_scope();
    
    // 変数を逆順でデストラクタ呼び出し
    for (auto it = scope.destruction_order.rbegin();
         it != scope.destruction_order.rend(); ++it) {
        auto& var = scope.variables[*it];
        
        if (var.type == TypeInfo::STRUCT && var.has_destructor) {
            call_destructor(var);
        }
    }
    
    // スコープをpop
    scope_stack_.pop_back();
}

void Interpreter::call_constructor(Variable& instance,
                                   const ASTNode* ctor_node,
                                   const std::vector<Variable>& args) {
    // self を instance に設定
    Variable* old_self = current_self_;
    current_self_ = &instance;
    
    // 引数を設定
    enter_scope();
    for (size_t i = 0; i < args.size(); i++) {
        add_variable(ctor_node->parameters[i]->name, args[i]);
    }
    
    // コンストラクタ本体実行
    execute_statement(ctor_node->body.get());
    
    exit_scope();
    current_self_ = old_self;
}

void Interpreter::call_destructor(Variable& instance) {
    auto impl_def = find_impl(instance.struct_name, "");
    if (!impl_def || !impl_def->destructor) {
        return;
    }
    
    // self を instance に設定
    Variable* old_self = current_self_;
    current_self_ = &instance;
    
    // デストラクタ本体実行
    execute_statement(impl_def->destructor->body.get());
    
    current_self_ = old_self;
}
```

---

## 📊 実装ステップ

### Phase 1: 基本構造（2週間）

**目標**: コンストラクタ/デストラクタの構文解析とAST構築

- [ ] Lexer拡張: `self`, `~` の認識
- [ ] Parser拡張: `impl Struct {}` パターンの解析
- [ ] Parser拡張: コンストラクタ/デストラクタの解析
- [ ] AST拡張: `ImplDefinition` の拡張
- [ ] AST拡張: `ConstructorNode`, `DestructorNode` の追加
- [ ] 基本的な構文エラー検出

**テストケース**:
```cb
struct Point { int x; int y; }
impl Point {
    self() { self.x = 0; self.y = 0; }
    ~self() { println("Destroyed"); }
}
```

### Phase 2: コンストラクタ実装（2週間）

**目標**: コンストラクタの自動呼び出し機能

- [ ] デフォルトコンストラクタの呼び出し
- [ ] パラメータ付きコンストラクタの呼び出し
- [ ] コンストラクタのオーバーロード解決
- [ ] `self` キーワードの実装
- [ ] 配列要素の初期化

**テストケース**:
```cb
Point p1;              // デフォルトコンストラクタ
Point p2(10, 20);      // パラメータ付き
Point[3] arr;          // 配列要素の初期化
```

### Phase 3: デストラクタ実装（2週間）

**目標**: スコープ管理とデストラクタの自動呼び出し

- [ ] スコープ管理機構の拡張
- [ ] デストラクタの自動呼び出し
- [ ] 破棄順序の実装（逆順）
- [ ] 早期return時のデストラクタ呼び出し
- [ ] 配列要素の破棄

**テストケース**:
```cb
{
    Resource r1(1);
    Resource r2(2);
}  // r2, r1の順にデストラクタ
```

### Phase 4: 特殊コンストラクタ（2週間）

**目標**: コピー/ムーブコンストラクタの実装

- [ ] コピーコンストラクタの認識と呼び出し
- [ ] デフォルトコピーコンストラクタの自動生成
- [ ] ムーブコンストラクタの認識と呼び出し
- [ ] 一時オブジェクトの最適化
- [ ] `const` と `&&` の解析

**テストケース**:
```cb
Point p1(10, 20);
Point p2 = p1;              // コピーコンストラクタ
Point p3 = Point(30, 40);   // ムーブコンストラクタ
```

### Phase 5: private メンバー（1週間）

**目標**: カプセル化機能の実装

- [ ] `private` キーワードの解析
- [ ] `impl` ブロック内の private 変数
- [ ] private 変数のアクセス制御
- [ ] エラーメッセージの実装

**テストケース**:
```cb
impl Counter {
    private int count;
    self() { count = 0; }
}
```

### Phase 6: テストと最適化（1週間）

**目標**: 包括的なテストと最適化

- [ ] 統合テストの作成
- [ ] エッジケースのテスト
- [ ] パフォーマンステスト
- [ ] メモリリークチェック
- [ ] ドキュメント作成

---

## 🧪 テストケース

### 1. 基本的なコンストラクタ/デストラクタ

```cb
struct Counter {
    int value;
}

impl Counter {
    self() {
        self.value = 0;
        println("Counter constructed: ", self.value);
    }
    
    self(int v) {
        self.value = v;
        println("Counter constructed: ", self.value);
    }
    
    ~self() {
        println("Counter destructed: ", self.value);
    }
}

void main() {
    Counter c1;
    Counter c2(42);
}

// 期待される出力:
// Counter constructed: 0
// Counter constructed: 42
// Counter destructed: 42
// Counter destructed: 0
```

### 2. スコープとデストラクタの順序

```cb
struct Logger {
    string name;
}

impl Logger {
    self(string n) {
        self.name = n;
        println("Created: ", n);
    }
    
    ~self() {
        println("Destroyed: ", self.name);
    }
}

void main() {
    Logger a("First");
    {
        Logger b("Second");
        Logger c("Third");
    }
    Logger d("Fourth");
}

// 期待される出力:
// Created: First
// Created: Second
// Created: Third
// Destroyed: Third
// Destroyed: Second
// Created: Fourth
// Destroyed: Fourth
// Destroyed: First
```

### 3. 配列のコンストラクタ/デストラクタ

```cb
struct Item {
    int id;
}

impl Item {
    self() {
        self.id = 0;
        println("Item constructed: ", self.id);
    }
    
    ~self() {
        println("Item destructed: ", self.id);
    }
}

void main() {
    Item[3] items;
}

// 期待される出力:
// Item constructed: 0
// Item constructed: 0
// Item constructed: 0
// Item destructed: 0
// Item destructed: 0
// Item destructed: 0
```

### 4. コピーコンストラクタ

```cb
struct Point {
    int x;
    int y;
}

impl Point {
    self(int px, int py) {
        self.x = px;
        self.y = py;
        println("Point constructed: (", px, ", ", py, ")");
    }
    
    self(const Point other) {
        self.x = other.x;
        self.y = other.y;
        println("Point copied: (", other.x, ", ", other.y, ")");
    }
    
    ~self() {
        println("Point destructed: (", self.x, ", ", self.y, ")");
    }
}

void main() {
    Point p1(10, 20);
    Point p2 = p1;
}

// 期待される出力:
// Point constructed: (10, 20)
// Point copied: (10, 20)
// Point destructed: (10, 20)
// Point destructed: (10, 20)
```

### 5. private メンバー

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
    
    void deposit(int amount) {
        balance = balance + amount;
    }
    
    int getBalance() {
        return balance;
    }
}

void main() {
    BankAccount acc("Alice", 1000);
    acc.deposit(500);
    println("Balance: ", acc.getBalance());
    
    // ❌ コンパイルエラー: private member
    // println(acc.balance);
}

// 期待される出力:
// Balance: 1500
```

---

## ⚠️ 課題と制約

### 実装上の課題

1. **スコープ管理の複雑化**
   - 早期return、break、continueでのデストラクタ呼び出し
   - 例外処理（将来実装）との統合

2. **配列の効率的な初期化**
   - 大きな配列での初期化コスト
   - デフォルトコンストラクタの最適化

3. **コピー最適化**
   - RVO (Return Value Optimization) の実装
   - 不要なコピーの削減

4. **デバッグ情報**
   - コンストラクタ/デストラクタの呼び出しスタック
   - メモリリーク検出

### 既存機能との統合

1. **default メンバーとの併用**
   - default メンバーを持つ構造体のコンストラクタ
   - 暗黙的な変換との整合性

2. **interface 実装との共存**
   - `impl Interface for Struct` と `impl Struct` の分離
   - メソッド名の衝突回避

3. **defer文との関係**
   - defer文とデストラクタの実行順序
   - スコープ管理の一貫性

---

## 🔄 将来の拡張

### 1. 継承とポリモーフィズム

```cb
// 将来的な拡張案
struct Base {
    int value;
}

struct Derived extends Base {
    int extra;
}

impl Derived {
    self(int v, int e) : Base(v) {  // 親クラスのコンストラクタ呼び出し
        self.extra = e;
    }
}
```

### 2. 明示的なデストラクタ呼び出し

```cb
// プレースメントnewと組み合わせた使用
struct Object { int value; }

impl Object {
    ~self() { println("Destroyed"); }
}

void main() {
    Object obj;
    obj.~Object();  // 明示的なデストラクタ呼び出し
}
```

### 3. constexprコンストラクタ

```cb
// コンパイル時評価可能なコンストラクタ
struct Point {
    int x;
    int y;
}

impl Point {
    constexpr self(int px, int py) {
        self.x = px;
        self.y = py;
    }
}

// コンパイル時定数として使用可能
const Point ORIGIN = Point(0, 0);
```

---

## 📚 関連ドキュメント

- [Cb言語仕様書](../spec.md)
- [impl内static変数](../archive/features/interfaces/impl_static_design.md)
- [default メンバー機能](./default_member.md)
- [defer文機能](./defer_statement.md)

---

**作成日**: 2025年10月11日  
**最終更新**: 2025年10月11日  
**ステータス**: 📝 設計フェーズ
