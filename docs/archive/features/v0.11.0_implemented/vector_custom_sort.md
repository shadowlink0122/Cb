# Vector<T> カスタムソート機能

## 概要

`Vector<T>`は`sort_with()`メソッドを通じて、カスタム比較関数による柔軟なソートをサポートします。

**v0.11.0更新**: デフォルト引数をサポート。`sort_with()`を引数なしで呼び出すとデフォルトソートを実行します。

## API

```cb
void sort_with(void* compare_fn = nullptr);
```

**パラメータ**:
- `compare_fn`: 比較関数へのポインタ（デフォルト: `nullptr`）
  - 省略または`nullptr`の場合、デフォルト昇順ソートを実行
  - カスタム関数を指定すると、その比較関数を使用

### 比較関数のシグネチャ

```cb
int compare_fn(T a, T b);
```

**戻り値**:
- 負の値: `a < b` (a が b より前に来るべき)
- `0`: `a == b` (順序は変わらない)
- 正の値: `a > b` (a が b より後に来るべき)

## 使用例

### 例1: 降順ソート（整数）

```cb
int compare_desc(int a, int b) {
    if (a > b) {
        return -1;  // a を前に
    }
    if (a < b) {
        return 1;   // b を前に
    }
    return 0;
}

int main() {
    Vector<int> vec;
    vec.push_back(5);
    vec.push_back(2);
    vec.push_back(8);
    vec.push_back(1);
    
    // カスタム比較関数で降順ソート
    vec.sort_with(&compare_desc);
    
    // 結果: [8, 5, 2, 1]
    return 0;
}
```

### 例2: 絶対値でソート

```cb
int compare_by_abs(int a, int b) {
    int abs_a = a;
    int abs_b = b;
    if (a < 0) { abs_a = -a; }
    if (b < 0) { abs_b = -b; }
    
    if (abs_a < abs_b) {
        return -1;
    }
    if (abs_a > abs_b) {
        return 1;
    }
    return 0;
}

int main() {
    Vector<int> vec;
    vec.push_back(-5);
    vec.push_back(2);
    vec.push_back(-8);
    vec.push_back(1);
    
    vec.sort_with(&compare_by_abs);
    
    // 結果: [1, 2, -5, -8]
    return 0;
}
```

### 例3: 構造体のメンバーでソート

```cb
struct Student {
    string name;
    int score;
};

int compare_by_score(Student a, Student b) {
    if (a.score < b.score) {
        return -1;
    }
    if (a.score > b.score) {
        return 1;
    }
    return 0;
}

int main() {
    Vector<Student> students;
    
    Student s1;
    s1.name = "Alice";
    s1.score = 85;
    students.push_back(s1);
    
    Student s2;
    s2.name = "Bob";
    s2.score = 92;
    students.push_back(s2);
    
    Student s3;
    s3.name = "Carol";
    s3.score = 78;
    students.push_back(s3);
    
    // スコアの昇順でソート
    students.sort_with(&compare_by_score);
    
    // 結果: [Carol(78), Alice(85), Bob(92)]
    return 0;
}
```

### 例4: 複数キーでソート

```cb
struct Person {
    int age;
    string name;
};

int compare_age_then_name(Person a, Person b) {
    // まず年齢で比較
    if (a.age < b.age) {
        return -1;
    }
    if (a.age > b.age) {
        return 1;
    }
    
    // 年齢が同じなら名前で比較
    // NOTE: 文字列比較は strcmp() 相当の実装が必要
    return 0;  // 簡略化
}

int main() {
    Vector<Person> people;
    // ... データ追加
    
    people.sort_with(&compare_age_then_name);
    
    return 0;
}
```

## 実装の特徴

### アルゴリズム

- **マージソート (O(n log n))**
  - 安定ソート（同じ値の要素の順序を保持）
  - 最悪ケースでもO(n log n)を保証
  - リンクリスト実装に最適

### 安定性の例

```cb
struct Item {
    int priority;
    string id;
};

int compare_by_priority(Item a, Item b) {
    return a.priority - b.priority;
}

// 入力: [{priority:1, id:"A"}, {priority:2, id:"B"}, {priority:1, id:"C"}]
// ソート後: [{priority:1, id:"A"}, {priority:1, id:"C"}, {priority:2, id:"B"}]
//           ↑ A と C の順序が保持される（安定ソート）
```

## デフォルトソートとの比較

| メソッド | 比較方法 | 順序 | 適用例 |
|---------|---------|------|--------|
| `sort()` | `<=` 演算子 | 昇順 | プリミティブ型 |
| `sort_with()` | カスタム関数 | 任意 | 複雑な比較ロジック |

## v0.11.0 新機能

### デフォルト引数サポート

`sort_with()`メソッドはデフォルト引数をサポートし、以下の呼び出し方が可能です：

```cb
Vector<int> vec;
vec.push_back(5);
vec.push_back(2);
vec.push_back(8);

// 方法1: デフォルトソート（引数省略）
vec.sort_with();

// 方法2: デフォルトソート（明示的にnullptr）
vec.sort_with(nullptr);

// 方法3: カスタム比較関数
void* cmp = &compare_desc;
vec.sort_with(cmp);
```

**利点**:
- `nullptr`を明示的に渡す必要がない
- APIが統一され、より直感的
- デフォルト動作とカスタム動作を同じメソッドで実現

### 関数ポインタ呼び出しサポート

v0.11.0で`call_function_pointer()`組み込み関数が実装され、カスタム比較関数が完全に動作するようになりました：

```cb
// インタプリタ側での実装
if (compare_fn == nullptr) {
    should_take_left = (left_data <= right_data);  // デフォルト比較
} else {
    int cmp_result = call_function_pointer(compare_fn, left_data, right_data);
    should_take_left = (cmp_result <= 0);  // カスタム比較
}
```

この機能により、完全にカスタマイズ可能なソートが実現しました。

## パフォーマンス

- **時間計算量**: O(n log n)
- **空間計算量**: O(log n) (再帰スタック)
- **安定性**: ✅ 安定ソート

## 参考

- [Vector<T> 基本仕様](../spec.md#vector)
- [マージソート実装詳細](vector_queue_generic_complete.md)
- [関数ポインタ実装計画](../../todo/function_pointer_implementation.md)
