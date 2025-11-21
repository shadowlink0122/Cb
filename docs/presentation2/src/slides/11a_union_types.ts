export default function unionTypes(): string {
    return `<section class="union-types-slide">
        <h2>型システム - Union型（合併型）</h2>

        <div class="code-section-large">
            <h3>🎯 TypeScriptライクなUnion型</h3>
            <pre><code class="language-cb">// Union型の定義
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

        <div class="feature-note">
            <p>✨ <strong>静的型付けと動的な柔軟性の両立</strong></p>
        </div>
    </section>`;
}