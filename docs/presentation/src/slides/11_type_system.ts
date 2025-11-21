export default function typeSystem(): string {
    return `<section class="type-system-slide">
        <h2>型システム - TypeScript風の高度な型</h2>

        <div class="type-features">
            <div class="feature-block">
                <h3>🎯 Union型（合併型）</h3>
                <pre><code class="language-cb">// TypeScriptライクなUnion型
typedef MixedType = int | string | bool;

MixedType value = 42;      // OK: int
value = "hello";            // OK: string
value = true;               // OK: bool

// 型チェックとパターンマッチング
switch (value) {
case int n:
    println("Number: ", n);
    break;
case string s:
    println("String: ", s);
    break;
case bool b:
    println("Boolean: ", b);
    break;
}</code></pre>
            </div>

            <div class="feature-block">
                <h3>✨ リテラル型</h3>
                <pre><code class="language-cb">// 特定の値のみを許可する型
typedef DiceValue = 1 | 2 | 3;
typedef Color = "red" | "green" | "blue";

DiceValue dice = 2;         // OK
// dice = 4;                // Error: 4 is not valid

Color color = "red";         // OK
// color = "yellow";        // Error: not a valid color

// 関数の戻り値にも使える
Color getTrafficLight(int state) {
    if (state == 0) return "red";
    if (state == 1) return "green";
    return "blue";  // エラー状態
}</code></pre>
            </div>

            <div class="feature-block">
                <h3>📦 構造体とリテラル初期化</h3>
                <pre><code class="language-cb">// 構造体定義
struct Person {
    string name;
    int age;
    int height;
};

// 構造体リテラル（名前付き初期化）
Person p1 = {name: "Alice", age: 25, height: 165};

// 位置ベース初期化
Person p2 = {"Bob", 30, 180};

// 構造体のUnion型
typedef PersonData = Person | string | int;

// 配列のUnion型
typedef ArrayUnion = int[3] | bool[2];
typedef NumberArrays = int[3] | int[5];</code></pre>
            </div>
        </div>
    </section>`;
}