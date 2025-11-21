export default function literalTypes(): string {
    return `<section class="literal-types-slide">
        <h2>型システム - リテラル型</h2>

        <div class="code-section-large">
            <h3>✨ 特定の値のみを許可する型</h3>
            <pre><code class="language-cb">// リテラル型の定義
typedef DiceValue = 1 | 2 | 3 | 4 | 5 | 6;
typedef Color = "red" | "green" | "blue";

DiceValue dice = 2;         // OK
// dice = 7;                // Error: 7 is not valid

Color color = "red";         // OK
// color = "yellow";        // Error: not a valid color

// 関数の戻り値にも使える
Color getTrafficLight(int state) {
    if (state == 0) return "red";
    if (state == 1) return "green";
    return "blue";  // エラー状態
}</code></pre>
        </div>

        <div class="feature-note">
            <p>🛡️ <strong>コンパイル時の型安全性を強化</strong></p>
        </div>
    </section>`;
}