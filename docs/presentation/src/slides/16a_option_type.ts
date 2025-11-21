export default function optionType(): string {
    return `<section class="option-type-slide">
        <h2>Option型 - Null安全性</h2>

        <div class="code-section-large">
            <h3>📦 組み込みOption型</h3>
            <pre><code class="language-cb">// Option型でnullを安全に扱う
Option&lt;int&gt; findValue(string key) {
    if (map.contains(key)) {
        return Option&lt;int&gt;::Some(map[key]);
    }
    return Option&lt;int&gt;::None;
}

// パターンマッチングで処理
Option&lt;int&gt; result = findValue("age");
match (result) {
    Some(value) => {
        println("Found: ", value);
    }
    None => {
        println("Not found");
    }
}</code></pre>
        </div>

        <div class="feature-note">
            <p>🛡️ <strong>Null参照エラーを型レベルで防止</strong></p>
        </div>
    </section>`;
}