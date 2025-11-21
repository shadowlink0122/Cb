export default function controlFlow(): string {
    return `<section class="control-flow-slide">
        <h2>基本構文 - 制御構造</h2>

        <div class="code-section-large">
            <h3>🔄 条件分岐とループ</h3>
            <pre><code class="language-cb">// if-else文
if (x > 0) {
    println("Positive");
} else if (x < 0) {
    println("Negative");
} else {
    println("Zero");
}

// forループ（break/continue対応）
for (int i = 0; i < 100; i++) {
    if (i % 2 == 0) continue;  // 偶数をスキップ
    if (i > 10) break;          // 10を超えたら終了
    println(i);  // 1, 3, 5, 7, 9
}

// whileループ
int count = 0;
while (count < 5) {
    println("Count: ", count);
    count++;
}</code></pre>
        </div>

        <div class="feature-note">
            <p>📝 <strong>C言語ライクな親しみやすい構文</strong></p>
        </div>
    </section>`;
}