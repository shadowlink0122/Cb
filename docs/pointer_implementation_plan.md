# ポインタシステム実装計画

## 概要 🚧 将来実装予定

現在のCb言語は構造体・Union・Interface機能が完全実装されており、実用レベルに達しています。ポインタシステムは次のフェーズでの実装予定です。

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
- 🚧 **動的メモリ管理**: malloc/free相当機能未実装

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
    Person person;
    Address addr = {"123 Main St", "Tech City", 12345};
    
    person.name = "Alice";
    person.address = &addr;  // アドレス取得
    
    // ネストしたアクセス
    person.address->street = "456 New Ave";  // ポインタ経由アクセス
    printf("Address: %s\n", person.address->street);
    
    return 0;
}
```

#### 2. 動的メモリ管理
```cb
// 動的構造体作成
Person* create_person(string name, int age) {
    Person* p = malloc(sizeof(Person));
    p->name = name;
    p->age = age;
    p->address = NULL;
    return p;
}

// メモリ解放
void destroy_person(Person* p) {
    if (p->address != NULL) {
        free(p->address);
    }
    free(p);
}

int main() {
    Person* alice = create_person("Alice", 25);
    alice->address = malloc(sizeof(Address));
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

// リンクリスト操作
Node* create_node(int value) {
    Node* node = malloc(sizeof(Node));
    node->data = value;
    node->next = NULL;
    node->left = NULL;
    node->right = NULL;
    return node;
}

void append_node(Node** head, int value) {
    Node* new_node = create_node(value);
    if (*head == NULL) {
        *head = new_node;
    } else {
        Node* current = *head;
        while (current->next != NULL) {
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
   - `*` と `&` 演算子のトークン化・構文解析
   - ポインタ宣言構文: `TYPE* variable;`
   - ポインタアクセス構文: `ptr->member`

3. **インタープリター拡張**
   - ポインタ変数の作成・初期化・管理
   - アドレス演算子 (`&`) の実装
   - 間接参照演算子 (`*`) の実装

### Phase 2: 動的メモリ管理
1. **メモリ管理システム**
   - `malloc()`, `free()` 相当の実装
   - メモリプール管理
   - ガベージコレクション（オプション）

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
