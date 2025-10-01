# ポインタシステム実装計画

## 概要 🚧 実装着手フェーズ

現在のCb言語は構造体・Union・Interface機能が完全実装されており、実用レベルに達しています。ポインタシステムは仕様が固まったため、以下の要件で実装を進めます。

### ポインタ仕様（確定）
- ポインタ型表記は `T*`。多重ポインタは `T**` のように `*` を重ねる。
- リテラルとして `nullptr` を導入し、`TYPE_NULLPTR` として扱う。
- ポインタ変数・メンバへのアクセスは `->` 演算子で行う。値として保持している構造体にのみ `.` を使用する。
    - 例: `Node* p;` のとき `p->value` がメンバアクセス。必要に応じて明示的に `*p` で値を取得し、`.` を使う。
- 任意ポインタの操作として、値の参照・代入（`*ptr`）、アドレスの取得（`&var`）、生ポインタ値の直接代入（`ptr = 0x1000` など）をサポートする。
- アドレスリテラルは `0x` 形式の16進数（および既存の10進数）を受け付け、将来的なOSレベルの開発を想定した直接番地操作を可能にする。
- 多重ポインタ（`T**` など）についても、値・アドレスの取得と変更を再帰的に許容する。
- 参照型 `T&` を導入し、変数・構造体メンバ・関数戻り値での参照宣言を許可する。`type& val;` で参照を宣言し、ムーブと同様に元の値と同一ストレージを共有する。
- 参照は左辺値・右辺値の双方に束縛可能な統一シンタックスとし、構造体・配列・プリミティブなどすべての型に適用できるようにする。
- 任意の型に対して `&value` が有効となるよう型システムと評価器を拡張し、取得したアドレスを対応するポインタ型へ安全に代入できるようにする。
- ポインタ変数に対して `val = val + 1;` のようなポインタ算術を許可し、型サイズに応じたアドレスオフセットを内部で計算する。ただし異なる型間での加減算や比較は安全性の観点から制限する。
- アドレスを再束縛する際は `&val = &other;` のようにアンパサンド付きの左辺を用いる特別構文を導入し、値操作とアドレス操作を明示的に分離する。
- 汎用ポインタ `void*` はメモリ安全確保の観点から当面禁止とし、型安全な仕組み（例えば型消去ハンドル）が整ってから再検討する。
- アドレス間の距離計算として `&val1 - &val2` をサポートし、同一型（または互換型）に属する変数同士の差分を**バイト単位の純粋なアドレス差分**として返す。要素数換算は利用者が `sizeof(T)` で調整する前提とする。異なる型や不正な比較にはコンパイルエラーを返す。
- 参照変数は代入によって再束縛可能とし、`int& a; a = val;` のような操作で常に最新の対象を指す。一方、ポインタは `&p = &val;` の構文でアドレスを明示的に差し替える。
- ポインタおよび参照はいずれも関数の引数・戻り値として利用可能であり、シグネチャ情報をIR生成に転用しやすいよう型情報を保持する。
- メンバがポインタでない限り、構造体同士での自己再帰・相互再帰はコンパイルエラーとする。
- 動的メモリは `new Type` と `delete ptr;` の文で管理する。関数形の `malloc`, `free` は提供しない。
- `sizeof(Type)` を導入し、静的サイズを算出できるようにする（実装フェーズ2で対応）。
- `new` の戻り値は対象型へのポインタ。変数への暗黙コピー禁止で、必ずポインタ変数へ代入する必要がある。
- `delete` は `nullptr` に対して安全。解放後は自動的に `nullptr` を代入する。
- Interface/Impl を実装した構造体をメンバに持つ構造体では、`s.member.func()` が動作するよう型推論を保証する。

#### ポインタ/参照操作サマリ
- `p = value;` はポインタ変数 `p` に数値アドレス（または別ポインタのアドレス値）を格納する。実体とのエイリアスは張られない。
- `&p = &var;` でポインタ `p` を変数 `var` のアドレスへ明示的に束縛する。以後 `*p` による読み書きは `var` を介して行われる。
- `p++;` / `p = p + offset;` はポインタが保持するアドレス数値を直接更新する（バイト単位）。束縛先の値を更新したい場合は `*p += 1` のように明示的にデリファレンスする。
- `(*p)++` / `(*p)--` のような式で、ポインタが参照する値を直接インクリメント／デクリメントできる。書き心地向上のため `*p++` / `*p--` を `(*p)++` / `(*p)--` の糖衣構文として解釈し、ポインタ自体のアドレスは変化させない。
- `int& ref; ref = expr;` で参照を再束縛すると、`ref` は常に `expr` の左辺値と同じストレージを共有する。再束縛はコピーではなくエイリアスの切り替えとして扱う。
- すべての実体は `&variable` でアドレス取得可能。取得結果は対応するポインタ型へ代入でき、差分計算 `&a - &b` もサポートされる。

## 現在の機能状況 ✅

### 完全実装済み機能
- ✅ **構造体システム**: 基本構造体・配列メンバー・構造体配列・リテラル初期化
- ✅ **Union型システム**: TypeScript風Union型・型安全性・エラーハンドリング
- ✅ **Interface/Implシステム**: ポリモーフィズム・型抽象化・メソッド定義
- ✅ **多次元配列**: typedef配列・配列戻り値・境界チェック
- ✅ **型システム**: プリミティブ型・複合型・型推論・型変換

### 現在の制限事項
- 🚧 **ネストした構造体**: `obj.member.submember` 未サポート
- 🚧 **構造体関数引数**: 値渡し未実装
- 🚧 **動的メモリ管理**: new/delete 文は実装中

## ポインタ実装の目標

### 実装後に可能になる機能

#### 1. ネストした構造体アクセス
```cb
struct Address {
    string street;
    string city;
    int zipcode;
};

struct Person {
    string name;
    Address* address;  // ポインタ参照
    int age;
};

int main() {
    Person* person = new Person;
    person->name = "Alice";

    person->address = new Address {street: "123 Main St", city: "Tech City", zipcode: 12345};

    // ポインタは `->` でアクセスし、値として保持している場合のみ `.` を使用する
    person->address->street = "456 New Ave";
    printf("Address: %s\n", person->address->street);

    delete person->address;
    delete person;
    return 0;
}
```

#### 2. 動的メモリ管理
```cb
Person* create_person(string name, int age) {
    Person* p = new Person;
    p->name = name;
    p->age = age;
    p->address = nullptr;
    return p;
}

void destroy_person(Person* p) {
    if (p->address != nullptr) {
        delete p->address;
    }
    delete p;
}

int main() {
    Person* alice = create_person("Alice", 25);
    alice->address = new Address;
    alice->address->street = "789 Oak St";
    
    printf("Person: %s, %d\n", alice->name, alice->age);
    printf("Address: %s\n", alice->address->street);
    
    destroy_person(alice);
    return 0;
}
```

#### 3. リンクリスト・木構造
```cb
struct Node {
    int data;
    Node* next;     // 自己参照ポインタ
    Node* left;     // 二分木用
    Node* right;
};

Node* create_node(int value) {
    Node* node = new Node;
    node.data = value;
    node.next = nullptr;
    node.left = nullptr;
    node.right = nullptr;
    return node;
}

void append_node(Node** head, int value) {
    Node* new_node = create_node(value);
    if (*head == nullptr) {
        *head = new_node;
    } else {
        Node* current = *head;
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = new_node;
    }
}
```

## 現在の回避策 ✅

ネストした構造体が必要な場合の現在利用可能な方法：

### 1. フラット構造体設計
```cb
// 推奨: フラット構造体
struct Person {
    string name;
    int age;
    string address_street;   // フラット化
    string address_city;
    int address_zipcode;
};

int main() {
    Person person;
    person.name = "Alice";
    person.age = 25;
    person.address_street = "123 Main St";  // ✅ 直接アクセス可能
    person.address_city = "Tech City";
    person.address_zipcode = 12345;
    
    printf("Person: %s, %d\n", person.name, person.age);
    printf("Address: %s, %s, %d\n", 
           person.address_street, person.address_city, person.address_zipcode);
    
    return 0;
}
```

### 2. 分離した構造体管理
```cb
struct Person {
    string name;
    int age;
    int address_id;  // 関連ID
};

struct Address {
    int id;
    string street;
    string city;
    int zipcode;
};

int main() {
    Person person = {name: "Alice", age: 25, address_id: 1};
    Address address = {id: 1, street: "123 Main St", city: "Tech City", zipcode: 12345};
    
    // 関連による管理
    if (person.address_id == address.id) {
        printf("Person: %s lives at %s\n", person.name, address.street);
    }
    
    return 0;
}
```

### 3. 配列による関連データ管理
```cb
struct Person {
    string name;
    int age;
};

struct Address {
    string street;
    string city;
    int zipcode;
};

int main() {
    Person[2] people = [
        {name: "Alice", age: 25},
        {name: "Bob", age: 30}
    ];
    
    Address[2] addresses = [
        {street: "123 Main St", city: "Tech City", zipcode: 12345},
        {street: "456 Oak Ave", city: "Dev Town", zipcode: 67890}
    ];
    
    // インデックスによる関連
    for (int i = 0; i < 2; i++) {
        printf("Person: %s lives at %s\n", 
               people[i].name, addresses[i].street);
    }
    
    return 0;
}
```

## 実装計画 🚧

### Phase 1: 基本ポインタ機能
1. **型システム拡張**
   - `TYPE_POINTER` 型の実装（`ast.h`に定義済み）
   - ポインタ変数の管理システム

2. **パーサー拡張**  
    - `*` と `&`、`new`、`delete`、`nullptr` のトークン化・構文解析
    - ポインタ宣言構文: `TYPE* variable;` / `TYPE** variable;`
    - ポインタアクセス構文: `ptr.member`（暗黙デリファレンス）

3. **インタープリター拡張**
    - ポインタ変数の作成・初期化・管理
    - アドレス演算子 (`&`) と間接参照 (`*`) の評価
    - ポインタ対象構造体メンバへの暗黙デリファレンスアクセス
    - すべての変数が論理的なアドレスハンドルを持ち、`&variable` 呼び出し時に即座に取得できるよう変数/配列マネージャを拡張（実体は既存のストレージ管理構造体のアドレスやIDを再利用）。
    - アドレス情報は必要時に生成・キャッシュし、ポインタ変数のみが恒常的に格納する設計とすることでメモリ使用量を抑制。

### Phase 2: 動的メモリ管理
1. **メモリ管理システム**
    - `new` 文と `delete` 文の実装
    - メモリプール管理
    - ガベージコレクションは導入しない（手動メモリ管理を前提に最適化）

2. **安全性機能**
   - ヌルポインタチェック
   - ダングリングポインタ検出
   - メモリリーク検出

### Phase 3: 高度な機能
1. **ネストした構造体アクセス**
   - `obj.member.submember` 構文対応
   - 深いネスト構造のサポート

2. **構造体関数引数・戻り値**
   - 構造体の値渡し・参照渡し
   - 構造体戻り値の効率的実装

## コンパイラ化を見据えた方針
- インタープリタ段階でも AST/IR を抽象化し、ポインタ・参照情報をそのまま中間表現へ落とし込めるデータ構造を採用する。
- メモリ管理は `new/delete` ベースの手動管理を基本とし、将来ネイティブコードを生成する際にも同じモデルを共有できるようにする。
- 実行時のガード（境界チェックなど）はオプション化し、最終的にはコンパイラで最適化・省略できるよう設計する。

## 技術的課題

### メモリ管理
- **課題**: ヒープメモリの効率的管理
- **解決策**: メモリプールとスマートポインタの実装

### パフォーマンス
- **課題**: ポインタアクセスのオーバーヘッド
- **解決策**: コンパイル時最適化とキャッシュ効率の向上

### 安全性
- **課題**: メモリ安全性の確保
- **解決策**: 境界チェックと型安全なポインタ操作

## まとめ

現在のCb言語は構造体・Union・Interface機能が完全実装されており、多くの実用的なプログラムを作成可能です。ポインタシステムは将来的な拡張として計画されており、実装により以下が可能になります：

- ✅ **現在利用可能**: フラット構造体・分離構造体・配列関連による高度なデータ構造
- 🚧 **将来実装**: ネストした構造体・動的メモリ管理・リンクリスト・木構造

ポインタ実装までの間は、現在の回避策により十分実用的なプログラムを作成できます。
