# 文字列補間機能 - 実装例

## 実際の使用例

### 基本的な使い方

```cb
fn main() {
    // 1. 変数の埋め込み
    string name = "Alice";
    int age = 25;
    println("My name is {name} and I'm {age} years old.");
    // 出力: My name is Alice and I'm 25 years old.
    
    // 2. 計算結果の埋め込み
    int x = 10;
    int y = 20;
    println("{x} + {y} = {x + y}");
    // 出力: 10 + 20 = 30
    
    // 3. フォーマット指定
    double price = 1234.5;
    println("Price: ${price:.2}");
    // 出力: Price: $1234.50
    
    // 4. 16進数表示
    int color = 0xFF5733;
    println("Color: #{color:06X}");
    // 出力: Color: #FF5733
}
```

### 実用例1: ログシステム

```cb
enum LogLevel {
    INFO,
    WARNING,
    ERROR
}

fn log(LogLevel level, string file, int line, string message) {
    string level_str = level == LogLevel.ERROR ? "ERROR" : 
                       level == LogLevel.WARNING ? "WARN" : "INFO";
    
    println("[{level_str:>5}] {file}:{line} - {message}");
}

fn main() {
    log(LogLevel.ERROR, "main.cb", 42, "Memory allocation failed");
    // 出力: [ERROR] main.cb:42 - Memory allocation failed
    
    log(LogLevel.INFO, "network.cb", 100, "Connection established");
    // 出力: [ INFO] network.cb:100 - Connection established
}
```

### 実用例2: テーブル表示

```cb
struct Student {
    string name;
    int id;
    double gpa;
}

fn print_student_table(Student[] students) {
    // ヘッダー
    println("+{"":-^12}+{"":-^8}+{"":-^8}+");
    println("| {"Name":^10} | {"ID":^6} | {"GPA":^6} |");
    println("+{"":-^12}+{"":-^8}+{"":-^8}+");
    
    // データ行
    int i = 0;
    while i < students.length {
        Student s = students[i];
        println("| {s.name:<10} | {s.id:>6} | {s.gpa:>6.2} |");
        i = i + 1;
    }
    
    println("+{"":-^12}+{"":-^8}+{"":-^8}+");
}

fn main() {
    Student[] students = [
        Student{"Alice", 1001, 3.85},
        Student{"Bob", 1002, 3.42},
        Student{"Charlie", 1003, 3.91}
    ];
    
    print_student_table(students);
    
    // 出力:
    // +------------+--------+--------+
    // |    Name    |   ID   |  GPA   |
    // +------------+--------+--------+
    // | Alice      |   1001 |   3.85 |
    // | Bob        |   1002 |   3.42 |
    // | Charlie    |   1003 |   3.91 |
    // +------------+--------+--------+
}
```

### 実用例3: プログレスバー

```cb
fn show_progress(int current, int total) {
    double percent = (current as double) / (total as double) * 100.0;
    int bar_width = 40;
    int filled = (percent as int) * bar_width / 100;
    
    print("\rProgress: [");
    
    int i = 0;
    while i < bar_width {
        if i < filled {
            print("=");
        } else if i == filled {
            print(">");
        } else {
            print(" ");
        }
        i = i + 1;
    }
    
    println("] {current:>4}/{total:<4} ({percent:5.1}%)");
}

fn main() {
    int total = 100;
    int i = 0;
    
    while i <= total {
        show_progress(i, total);
        // シミュレーション: 実際には何か処理を行う
        i = i + 10;
    }
    
    // 出力（最終行）:
    // Progress: [========================================] 100/100  (100.0%)
}
```

### 実用例4: JSON風の出力

```cb
struct Point {
    double x;
    double y;
}

struct Circle {
    Point center;
    double radius;
}

fn print_circle_json(Circle c) {
    println("{{");
    println("  \"type\": \"circle\",");
    println("  \"center\": {{ \"x\": {c.center.x:.2}, \"y\": {c.center.y:.2} }},");
    println("  \"radius\": {c.radius:.2}");
    println("}}");
}

fn main() {
    Circle circle = Circle{Point{10.5, 20.3}, 5.7};
    print_circle_json(circle);
    
    // 出力:
    // {
    //   "type": "circle",
    //   "center": { "x": 10.50, "y": 20.30 },
    //   "radius": 5.70
    // }
}
```

### 実用例5: デバッグ出力

```cb
fn debug_print<T>(string var_name, T value) {
    println("[DEBUG] {var_name} = {value:?}");
}

fn main() {
    int x = 42;
    double y = 3.14159;
    string z = "Hello";
    bool flag = true;
    
    debug_print("x", x);
    debug_print("y", y);
    debug_print("z", z);
    debug_print("flag", flag);
    
    // 出力:
    // [DEBUG] x = 42
    // [DEBUG] y = 3.14159
    // [DEBUG] z = "Hello"
    // [DEBUG] flag = true
    
    // 複雑な式のデバッグ
    int a = 10;
    int b = 20;
    println("[DEBUG] Expression: {a} + {b} = {a + b}");
    // 出力: [DEBUG] Expression: 10 + 20 = 30
}
```

### 実用例6: 多言語対応（将来拡張）

```cb
struct Message {
    string template;
}

fn format_message(Message msg, string name, int count) {
    // 将来的には名前付き引数をサポート
    // return format(msg.template, name=name, count=count);
    
    // 現時点では手動で置換
    string result = msg.template;
    // ... 置換ロジック
    return result;
}

fn main() {
    Message welcome = Message{"Welcome, {name}! You have {count} new messages."};
    
    string formatted = format_message(welcome, "Alice", 5);
    println("{formatted}");
    // 出力: Welcome, Alice! You have 5 new messages.
}
```

---

## Rust/C++風との比較

### Rustスタイル（位置引数）
```rust
println!("Hello, {}! Age: {}", name, age);
println!("Sum: {}", a + b);  // 式は不可、変数のみ
```

### Cb言語スタイル（直接埋め込み）
```cb
println("Hello, {name}! Age: {age}");
println("Sum: {a + b}");  // 式も直接記述可能
```

### 利点
- 🎯 **可読性**: 変数名が直接見えるので、何が埋め込まれるか一目瞭然
- 🔄 **再利用**: `{x} * {x} = {x * x}` のように同じ変数を何度でも使える
- 📝 **簡潔**: 位置を気にせず書ける
- 🧮 **式の評価**: 変数だけでなく、式も直接記述できる

---

## まとめ

Python/C#スタイルの`{式}`構文により、Cb言語の文字列補間は:
- ✅ 直感的で読みやすい
- ✅ 強力なフォーマット機能
- ✅ 式の直接埋め込み
- ✅ エスケープシーケンスのサポート
- ✅ 実用的なユースケースに対応

次のステップ: 実装フェーズへ！
