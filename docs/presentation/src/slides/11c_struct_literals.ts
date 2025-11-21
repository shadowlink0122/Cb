export default function structLiterals(): string {
    return `<section class="struct-literals-slide">
        <h2>型システム - 構造体とリテラル初期化</h2>

        <div class="code-section-large">
            <h3>📦 構造体定義と初期化</h3>
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

        <div class="feature-note">
            <p>🔧 <strong>柔軟な初期化方法をサポート</strong></p>
        </div>
    </section>`;
}