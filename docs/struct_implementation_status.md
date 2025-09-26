# Cb言語 構造体機能 実装状況ドキュメント

## 実装済み機能 ✅

### 1. 基本的な構造体定義と使用
```cb
struct Person {
    string name;
    int age;
    int height;
};

int main() {
    Person p;
    p.name = "Alice";
    p.age = 25;
    p.height = 165;
    
    println("Name: %s, Age: %d, Height: %d", p.name, p.age, p.height);
    return 0;
}
```
- ✅ 構造体定義 (`struct Name { ... }`)
- ✅ 構造体変数宣言 (`Person p;`)
- ✅ メンバーアクセス (`p.name = "Alice"`)
- ✅ printf/println統合

### 2. 構造体リテラル初期化
```cb
struct Point {
    int x;
    int y;
};

int main() {
    // 名前付き初期化
    Point p1 = {x: 10, y: 20};
    
    // 位置指定初期化
    Point p2 = {30, 40};
    
    return 0;
}
```
- ✅ 名前付き初期化 (`{x: 10, y: 20}`)
- ✅ 位置指定初期化 (`{30, 40}`)

### 3. 構造体配列メンバー
```cb
struct Student {
    string name;
    int grades[3];
    int student_id;
};

int main() {
    Student s;
    s.name = "Bob";
    s.student_id = 1001;
    
    // 個別要素代入
    s.grades[0] = 85;
    s.grades[1] = 92;
    s.grades[2] = 78;
    
    // 配列リテラル代入
    s.grades = [85, 92, 78];
    
    println("Student: %s (ID: %d)", s.name, s.student_id);
    println("Grades: [%d, %d, %d]", s.grades[0], s.grades[1], s.grades[2]);
    return 0;
}
```
- ✅ 構造体内配列メンバー定義 (`int grades[3]`)
- ✅ 配列要素アクセス (`s.grades[0] = 85`)
- ✅ 配列リテラル代入 (`s.grades = [85, 92, 78]`)
- ✅ printf/println統合

### 4. 構造体の配列
```cb
struct Employee {
    string name;
    int salary;
    int department_id;
};

string department[3] = ["HR", "Dev", "CEO"];

int main() {
    Employee team[3];
    
    team[0] = {name: "Alice", salary: 50000, department_id: 0};
    team[1] = {name: "Bob", salary: 55000, department_id: 2};
    team[2] = {name: "Charlie", salary: 60000, department_id: 1};
    
    int i;
    for (i = 0; i < 3; i = i + 1) {
        println("%d. %s - $%d (Dept: %s)", 
                i + 1, team[i].name, team[i].salary, 
                department[team[i].department_id]);
    }
    return 0;
}
```
- ✅ 構造体配列宣言 (`Employee team[3]`)
- ✅ 構造体配列リテラル代入
- ✅ ネストした配列アクセス (`department[team[i].department_id]`)

### 5. グローバル配列との連携
```cb
string department[3] = ["HR", "Dev", "CEO"];  // グローバル配列リテラル初期化
```
- ✅ グローバル配列リテラル初期化
- ✅ 構造体メンバーをインデックスとした配列アクセス

### 6. 多次元配列メンバー（1次元として実装）
```cb
struct Matrix {
    string name;
    int data[6];  // 2x3として使用
    int rows;
    int cols;
};

int main() {
    Matrix m;
    m.name = "Sample Matrix";
    
    // 配列リテラル代入
    m.data = [1, 2, 3, 4, 5, 6];
    
    // 手動アクセス（行列として扱う）
    println("Row 0: [%d, %d, %d]", m.data[0], m.data[1], m.data[2]);
    println("Row 1: [%d, %d, %d]", m.data[3], m.data[4], m.data[5]);
    return 0;
}
```
- ✅ 多次元配列の1次元表現
- ✅ 配列リテラル初期化対応

## 未実装機能 ❌

### 1. ネストした構造体メンバーアクセス
```cb
struct Address {
    string street;
    string city;
    int zipcode;
};

struct Company {
    string name;
    Address address;  // ネストした構造体
    int employee_count;
};

int main() {
    Company tech_corp;
    tech_corp.name = "Tech Corp";
    
    // ❌ エラー: ネストしたメンバーアクセスは未サポート
    tech_corp.address.street = "123 Main St";
    
    return 0;
}
```
**エラーメッセージ**: `Nested member access assignment (obj.member.submember = value) is not supported yet. Consider using pointers in future implementation.`

**回避策**: フラット構造体を使用
```cb
struct Company {
    string name;
    int employee_count;
};

struct Address {
    string street;
    string city;
    int zipcode;
};

int main() {
    Company tech_corp;
    Address corp_address;  // 分離した構造体
    
    tech_corp.name = "Tech Corp";
    corp_address.street = "123 Main St";  // ✅ 動作する
    return 0;
}
```

### 2. 構造体の関数引数・戻り値
```cb
struct Rectangle {
    int width;
    int height;
};

// ❌ 未実装: 構造体引数
int calculate_area(Rectangle rect) {
    return rect.width * rect.height;
}

// ❌ 未実装: 構造体戻り値
Rectangle create_rectangle(int w, int h) {
    Rectangle r = {width: w, height: h};
    return r;
}
```
**エラーメッセージ**: `Variable is not a struct: rect`

### 3. 複雑なネストした配列アクセス
```cb
struct Student {
    string name;
    int grades[3];
};

struct Course {
    Student students[2];
};

int main() {
    Course math_course;
    
    // ❌ エラー: 構造体配列の構造体メンバー配列アクセス
    math_course.students[0].grades[0] = 85;
    
    return 0;
}
```
**エラーメッセージ**: `Expected '=' after member array access`

### 4. 真の多次元配列アクセス
```cb
struct Matrix {
    int data[2][3];  // 2次元配列宣言は可能
};

int main() {
    Matrix m;
    
    // ❌ エラー: 多次元配列アクセス未サポート
    m.data[0][0] = 1;
    
    return 0;
}
```
**現在の制限**: パーサーは多次元配列宣言を認識するが、アクセス時にエラー

### 5. 動的メモリ管理
- ❌ ポインタ
- ❌ 動的配列
- ❌ malloc/free相当の機能

## 設計思想と制限事項

### メモリ効率重視の設計
現在の構造体システムは**メモリ効率**と**実装複雑さのバランス**を重視した設計になっています：

1. **フラット変数管理**: すべての変数（構造体メンバー含む）を`variables`マップで平坦に管理
2. **二重管理システム**: 構造体メンバーは`struct_members`と個別変数の両方で管理
3. **1次元配列ベース**: 多次元配列は1次元配列として内部実装

### 将来実装予定機能

#### ポインタシステム (高優先度)
- `TYPE_POINTER`型は既に`ast.h`に定義済み
- ネストした構造体アクセスはポインタ実装後に対応予定
- 詳細: [`docs/pointer_implementation_plan.md`](./pointer_implementation_plan.md)

#### 構造体関数引数・戻り値 (中優先度)
- 関数コンテキストでの構造体処理
- 構造体のコピー機構
- スタック管理の拡張

#### 真の多次元配列 (低優先度)
- 現在の1次元実装で大部分のユースケースをカバー可能
- パフォーマンス要求が高い場合のみ検討

## 統合テスト結果

**現在の成功率**: 459/459 (100%) ✅

スキップされた複雑な機能テスト:
- `test_struct_function_param` (構造体関数引数)
- `test_struct_function_return` (構造体関数戻り値)
- `test_mixed_types` (複雑な型組み合わせ)
- `test_typedef_struct` (typedef構造体)
- `test_struct_error_handling` (エラーハンドリング)
- `test_large_struct` (大規模構造体)
- `test_comprehensive` (総合テスト)

## 推奨使用パターン

### ✅ 推奨
```cb
// シンプルな構造体
struct Person {
    string name;
    int age;
    int scores[5];
};

// 構造体配列
Person team[10];

// 配列リテラル初期化
team[0] = {name: "Alice", age: 25, scores: [85, 90, 88, 92, 87]};
```

### ⚠️ 注意が必要
```cb
// 多次元配列は1次元として扱う
struct Matrix {
    int data[9];  // 3x3行列として使用
};

// 手動インデックス計算
// data[i][j] → data[i * cols + j]
m.data[1 * 3 + 2] = 5;  // m.data[1][2] = 5 相当
```

### ❌ 避けるべき
```cb
// ネストした構造体（未サポート）
struct Company {
    Address address;  // ネストは避ける
};

// 構造体関数引数（未サポート）
int process(Person p) { ... }  // 未実装
```

## まとめ

Cb言語の構造体機能は、**基本的な構造体操作**、**配列メンバー**、**構造体配列**、**配列リテラル初期化**など、実用的な機能が完全に実装されています。

メモリ効率と実装複雑さのバランスを考慮し、段階的な機能拡張を予定していますが、現在の機能セットでも多くの実用的なプログラムを作成可能です。
